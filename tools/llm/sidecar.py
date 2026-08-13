from __future__ import annotations

"""Phase 1 LLM sidecar：为 LPC mudlib（llmd.c）提供「自然语言 → MUD 指令」解析。

与 Phase 0 网关（gateway.py）的关系：Phase 1 把解析层下沉到驱动内 daemon +
本地 sidecar。本文件就是 sidecar——纯 Python（纯标准库），作为 llmd.c 的本地
翻译器：LPC 侧采集 grounding（房间/出口/可见之物/背包/境界）→ 经 TCP 127.0.0.1
发来自然语言 → sidecar 组装 prompt、调 LLM（或 mock）、safety 过滤 →
返回单行 JSON 指令序列 → LPC 侧异步注入。

协议（每请求一连接，新行分隔 JSON，请求/响应各一行）：
  请求: {"player": "...", "language": "...", "grounding": {"room_short": ...,
        "room_long": ..., "exits": [...], "objects": [...], "inventory": [...],
        "realm": "..."}}
  响应: {"commands": [...], "confirm": [...], "blocked": [...],
        "reason": "...", "error": null}
    commands —— 可直接注入的安全指令（白名单内，safety 已校验）
    confirm  —— 高危指令（仅 DANGEROUS 动词 + 字符集合法），交给 LPC 二次确认
    blocked  —— 被拦截的条目（DENIED 管理命令 / 非白名单 / 超限 / 格式非法）
    error    —— 非空表示本次解析失败（LLM 调用失败/超时/请求非法），commands 必空

密钥红线（延续 #69）：LLM API 地址/密钥/模型名只从玩家本地配置读取
（环境变量或 ~/.config/fanren-mud/llm.json），代码库/提交/ticket 零密钥。

用法：
    python -m tools.llm.sidecar --port 37777          # 真实 LLM 模式
    python -m tools.llm.sidecar --port 37777 --mock   # 无 key 演示/自测
    python -m tools.llm.sidecar --help
"""

import argparse
import json
import queue
import socket
import sys
import threading

from tools.llm.llm_client import LLMConfig, LLMError, chat_completion
from tools.llm.safety import check_command, check_confirm_command, SafetyError
from tools.llm.mock_llm import parse_mock

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 37777          # 与 llmd.c 中 LLM_SIDECAR_PORT 保持一致
MAX_COMMANDS = 8              # 单次最多指令条数（对齐 #68 风险表）
REQUEST_TIMEOUT = 12          # sidecar 内 LLM 调用超时（秒），须短于 LPC 侧 watchdog 15s

# 延续 #69 SYSTEM_PROMPT，注入 grounding（LLM 翻译时必须依据的现场上下文）
SYSTEM_PROMPT = """你是一名 MUD 文字游戏的「自然语言 → 游戏指令」翻译器。玩家用自然语言描述意图，你把意图翻译成游戏可执行的指令序列。

【当前环境】（本次翻译必须依据的现场上下文）
房间：{room_short}
房间描述：{room_long}
出口：{exits}
可见之物：{objects}
玩家背包：{inventory}
玩家境界：{realm}

【指令格式】
- 每条指令形如 `动词 宾语`，全部小写，宾语用物品的英文 id（如 jian、gold、yifu）。
- 移动：`go <方向>`，方向只能是 north/south/east/west/northup/southup/eastup/westup/northdown/southdown/eastdown/northeast/northwest/southeast/southwest/up/down/enter/out。
- 查看环境：`look`；查看背包：`inventory`；查看当铺货品：`list`；典当：`pawn <物品id>`；估价：`value <物品id>`；买：`buy <物品id>`；卖：`sell <物品id>`。

【可用动词白名单】（只能使用这些动词，禁止其他任何动词）
go, look, l, inventory, i, score, hp, check, list, value, pawn, dang, buy, sell, get, put, open, close, wear, wield, unwield, remove, say, tell, reply, smile, nod, ask, watch, listen, sit, sleep, eat, drink

【禁止事项】
- 绝对禁止输出：kill/hit/fight/steal/give/drop/quit/exit/suicide 等危险动词，以及 shutdown/exec/call/update/eval 等任何管理调试命令。玩家请求这类行为时，把该意图放入 confirm 数组而不是 commands。
- 不要编造玩家不知道的路线。若当前房间出口中没有通往目标的路线，第一步输出 `look` 或 `inventory` 观察，由玩家根据结果再次发起。
- 不知道物品的确切 id 时，先输出 `inventory` 或 `look` 观察，不要凭空猜宾语。
- 优先使用【当前环境】提供的出口方向与物品 id；环境里没有的信息不要假设。

【输出契约】
只输出一个 JSON 对象（不要 markdown 代码块，不要解释文字）：
{{"commands": ["指令1", "指令2", ...], "confirm": ["高危意图对应的指令"], "reason": "一句话说明"}}
- commands：可直接安全执行的指令，最多 6 条。
- confirm：玩家要求但属于高危（战斗/财产转移/退出等）的指令，最多 3 条。
- reason：简要中文说明这次翻译的依据。"""


def filter_payload(payload: dict) -> tuple[list, list, list]:
    """安全过滤：LLM 输出 → (可注入, 待确认高危, 被拦截)。

    - commands 数组：仅白名单安全动词可注入（check_command）；
      DENIED 管理命令 / 危险动词 / 未知动词 / 超限 / 格式非法 → 拦截。
    - confirm 数组：仅 DANGEROUS 动词 + 字符集合法可交 LPC 二次确认
      （check_confirm_command，延续 #69 收紧口径）；DENIED 管理命令绝不放行。
    任何一条触线都不会进入 commands —— 拦截信息（动词 + 原因）进 blocked。
    """
    allowed: list = []
    confirm_ok: list = []
    blocked: list = []

    raw_commands = payload.get("commands") or []
    if not isinstance(raw_commands, list):
        raise SafetyError("LLM 输出缺少 commands 数组")
    if len(raw_commands) > MAX_COMMANDS:
        raise SafetyError(f"指令条数超限: {len(raw_commands)} > {MAX_COMMANDS}")

    for cmd in raw_commands:
        if not isinstance(cmd, str):
            blocked.append(f"{cmd!r} 非字符串")
            continue
        try:
            check_command(cmd)
            allowed.append(cmd)
        except SafetyError as e:
            blocked.append(f"{cmd} —— {e}")

    raw_confirm = payload.get("confirm") or []
    if not isinstance(raw_confirm, list):
        blocked.append("confirm 字段非数组，忽略")
        raw_confirm = []
    for cmd in raw_confirm:
        if not isinstance(cmd, str) or not cmd.strip():
            continue
        try:
            check_confirm_command(cmd)
            confirm_ok.append(cmd)
        except SafetyError as e:
            blocked.append(f"{cmd} —— {e}")

    return allowed, confirm_ok, blocked


class LlmSidecar:
    """TCP sidecar 服务：每请求一连接，新行分隔 JSON。"""

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT,
                 mock: bool = False, llm_cfg: LLMConfig | None = None,
                 timeout: float = REQUEST_TIMEOUT):
        self.host = host
        self.port = port
        self.mock = mock
        self.llm_cfg = llm_cfg or LLMConfig()
        self.timeout = timeout
        self._stop = threading.Event()

    # ── prompt 组装 ──────────────────────────────────────────
    def build_prompt(self, grounding: dict | None) -> str:
        g = grounding or {}
        room_long = (g.get("room_long") or "").strip().replace("\n", " ")
        return SYSTEM_PROMPT.format(
            room_short=(g.get("room_short") or "未知")[:40],
            room_long=room_long[:300] or "未知",
            exits="、".join(g.get("exits") or []) or "未知",
            objects="、".join(g.get("objects") or []) or "无",
            inventory="、".join(g.get("inventory") or []) or "空",
            realm=(g.get("realm") or "未知")[:40],
        )

    # ── LLM 调用（带 sidecar 级超时，短于 LPC watchdog）─────
    def _call_llm(self, system: str, user: str) -> tuple[str, object]:
        """调 LLM/mock，返回 (kind, value)：kind ∈ ok / err / timeout。"""
        q: queue.Queue = queue.Queue()

        def worker() -> None:
            try:
                q.put(("ok", chat_completion(self.llm_cfg, system, user)))
            except LLMError as e:
                q.put(("err", str(e)))
            except Exception as e:  # 任何意外异常都不让连接挂死
                q.put(("err", f"未知错误: {e}"))

        t = threading.Thread(target=worker, daemon=True)
        t.start()
        try:
            return q.get(timeout=self.timeout)
        except queue.Empty:
            return ("timeout", f"LLM 调用超过 {self.timeout}s 未返回")

    # ── 单请求处理 ───────────────────────────────────────────
    def handle_line(self, line: str) -> str:
        """处理一行请求，返回一行响应（绝不抛异常到连接层）。"""
        try:
            req = json.loads(line)
            if not isinstance(req, dict):
                raise ValueError("请求不是 JSON 对象")
            language = req.get("language")
            if not isinstance(language, str) or not language.strip():
                raise ValueError("缺少 language 字段")
            grounding = req.get("grounding")
            if grounding is not None and not isinstance(grounding, dict):
                raise ValueError("grounding 必须是 JSON 对象")
        except (json.JSONDecodeError, ValueError) as e:
            return self._error(f"请求格式错误: {e}")

        system = self.build_prompt(grounding)
        if self.mock:
            payload = parse_mock(language)
        else:
            if not self.llm_cfg.configured:
                return self._error("未配置 LLM API 密钥——请玩家在自己主机上配置"
                                   "（环境变量或 ~/.config/fanren-mud/llm.json），或加 --mock 演示")
            kind, val = self._call_llm(system, language)
            if kind == "err":
                return self._error(f"LLM 调用失败: {val}")
            if kind == "timeout":
                return self._error(f"{val}，已安全中止（零指令注入）")
            payload = val
            if not isinstance(payload, dict):
                return self._error("LLM 返回的不是 JSON 对象")

        try:
            allowed, confirm_ok, blocked = filter_payload(payload)
        except SafetyError as e:
            return self._error(f"解析结果不合法，已安全中止: {e}")

        reason = payload.get("reason") if isinstance(payload, dict) else ""
        if not isinstance(reason, str):
            reason = ""
        return json.dumps({
            "commands": allowed,
            "confirm": confirm_ok,
            "blocked": blocked,
            "reason": reason,
            "error": None,
        }, ensure_ascii=False)

    def _error(self, msg: str) -> str:
        return json.dumps({
            "commands": [], "confirm": [], "blocked": [], "reason": "",
            "error": msg,
        }, ensure_ascii=False)

    # ── 服务循环 ─────────────────────────────────────────────
    def _handle_conn(self, conn: socket.socket) -> None:
        try:
            conn.settimeout(30)  # 单连接最长存活 30s（防半开连接占资源）
            buf = bytearray()
            # 请求为单行 JSON（\n 定界）；读满一行即处理
            while True:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                buf += chunk
                if b"\n" in buf:
                    break
            line = bytes(buf).decode("utf-8", errors="replace").strip()
            if not line:
                return
            resp = self.handle_line(line)
            conn.sendall(resp.encode("utf-8") + b"\n")
        except OSError:
            pass
        finally:
            try:
                conn.close()
            except OSError:
                pass

    def serve_forever(self) -> None:
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        self.bound_port = srv.getsockname()[1]  # 实际绑定端口（port=0 时随机）
        srv.listen(16)
        srv.settimeout(0.5)  # 轮询 accept，使 stop() 可退出
        print(f"[sidecar] 监听 {self.host}:{self.bound_port}（LLM "
              f"{'mock' if self.mock else 'API'} 模式，超时 {self.timeout}s）")
        while not self._stop.is_set():
            try:
                conn, _ = srv.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            t = threading.Thread(target=self._handle_conn, args=(conn,), daemon=True)
            t.start()
        try:
            srv.close()
        except OSError:
            pass

    def stop(self) -> None:
        self._stop.set()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="python -m tools.llm.sidecar",
        description="Phase 1 LLM sidecar：为 LPC llmd.c 提供自然语言解析（TCP 127.0.0.1，每请求一连接）",
    )
    parser.add_argument("--host", default=DEFAULT_HOST,
                        help=f"监听地址（默认 {DEFAULT_HOST}，只应使用回环地址）")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"监听端口（默认 {DEFAULT_PORT}，与 llmd.c LLM_SIDECAR_PORT 一致）")
    parser.add_argument("--mock", action="store_true",
                        help="mock 模式：不调 LLM，用内置规则解析（演示/自测）")
    parser.add_argument("--timeout", type=float, default=REQUEST_TIMEOUT,
                        help=f"LLM 调用超时秒数（默认 {REQUEST_TIMEOUT}，须短于 LPC watchdog 15s）")
    parser.add_argument("--llm-base", help="OpenAI 兼容 API 地址（默认读环境变量 LLM_API_BASE）")
    parser.add_argument("--llm-key", help="API 密钥（默认读环境变量 LLM_API_KEY；建议用环境变量）")
    parser.add_argument("--model", help="模型名（默认读环境变量 LLM_MODEL）")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    cfg = LLMConfig(api_base=args.llm_base, api_key=args.llm_key, model=args.model)
    if not args.mock:
        print(f"[sidecar] LLM 配置：{cfg.describe()}")
        if not cfg.configured:
            print("[sidecar] 警告：未检测到 API 密钥，请求将返回错误（可加 --mock 演示）")
    sidecar = LlmSidecar(
        host=args.host, port=args.port, mock=args.mock,
        llm_cfg=cfg, timeout=args.timeout,
    )
    sidecar.serve_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
