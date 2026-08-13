from __future__ import annotations

"""Phase 0 外挂式 LLM 网关：自然语言 → MUD 指令解析并回写执行。

用法（详见同目录 README.md）：
    python -m tools.llm.gateway --host 127.0.0.1 --port 5555
    python -m tools.llm.gateway --port 6666 --mock        # 无 API key 演示/自测
    python -m tools.llm.gateway --help

职责边界（与 #68 Phase 0 一致）：
- 不做驱动/LPC 改动，纯外挂；游戏服务本身照常按 5555/6666 端口运行。
- 玩家输入以 `ai ` 开头 → 拦截并调 LLM 解析为 MUD 指令序列，
  经安全过滤后逐条回写连接执行；其余输入原样透传。
- LLM API 地址/密钥/模型由玩家在自己主机本地配置（环境变量或
  ~/.config/fanren-mud/llm.json），代码库/提交/ticket 中绝不出现密钥。
"""

import argparse
import socket
import sys
import threading
import time

from tools.llm.llm_client import LLMConfig, LLMError, chat_completion
from tools.llm.safety import split_commands, SafetyError

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 5555
PORT_ENCODINGS = {5555: "gbk", 6666: "utf-8"}  # 见 .knowledge/architecture/master-object-callbacks.md
COMMAND_INTERVAL = 0.4  # 回写每条指令之间的间隔（秒），避免服务器状态未跟上
MAX_AI_COMMANDS = 8

SYSTEM_PROMPT = """你是一名 MUD 文字游戏的「自然语言 → 游戏指令」翻译器。玩家用自然语言描述意图，你把意图翻译成游戏可执行的指令序列。

【指令格式】
- 每条指令形如 `动词 宾语`，全部小写，宾语用物品的英文 id（如 jian、gold、yifu）。
- 移动：`go <方向>`，方向只能是 north/south/east/west/northup/southup/eastup/westup/northdown/southdown/eastdown/northeast/northwest/southeast/southwest/up/down/enter/out。
- 查看环境：`look`；查看背包：`inventory`；查看当铺货品：`list`；典当：`pawn <物品id>`；估价：`value <物品id>`；买：`buy <物品id>`；卖：`sell <物品id>`。

【可用动词白名单】（只能使用这些动词，禁止其他任何动词）
go, look, l, inventory, i, score, hp, check, list, value, pawn, dang, buy, sell, get, put, open, close, wear, wield, unwield, remove, say, tell, reply, smile, nod, ask, watch, listen, sit, sleep, eat, drink

【禁止事项】
- 绝对禁止输出：kill/hit/fight/steal/give/drop/quit/exit/suicide 等危险动词，以及 shutdown/exec/call/update/eval 等任何管理调试命令。玩家请求这类行为时，把该意图放入 confirm 数组而不是 commands。
- 不要编造玩家不知道的路线。若不确定路线，第一步输出 `look` 查看当前房间出口，由玩家根据结果再次发起。
- 不知道物品的确切 id 时，先输出 `inventory` 或 `look` 观察，不要凭空猜宾语。

【输出契约】
只输出一个 JSON 对象（不要 markdown 代码块，不要解释文字）：
{"commands": ["指令1", "指令2", ...], "confirm": ["高危意图对应的指令"], "reason": "一句话说明"}
- commands：可直接安全执行的指令，最多 6 条。
- confirm：玩家要求但属于高危（战斗/财产转移/退出等）的指令，最多 3 条。
- reason：简要中文说明这次翻译的依据。"""


# ── telnet 协商 ─────────────────────────────────────────────

def strip_telnet(raw: bytes) -> tuple[bytes, bytes]:
    """剥除 telnet IAC 协商字节。

    返回 (数据, 需回应字节)：数据打印给玩家；需回应字节发回服务器
    （对 DO/WILL 一律回应 WONT/DONT——本端自行控制回显与续行；
    SB...SE 子协商整段跳过；其余 IAC 命令忽略）。
    """
    data = bytearray()
    resp = bytearray()
    i = 0
    n = len(raw)
    while i < n:
        b = raw[i]
        if b != 0xFF:
            data.append(b)
            i += 1
            continue
        # IAC 序列
        if i + 1 >= n:
            break  # 不完整 IAC，丢弃
        cmd = raw[i + 1]
        if cmd == 0xFF:  # IAC IAC —— 数据中的 0xFF
            data.append(0xFF)
            i += 2
        elif cmd == 0xF0:  # SE
            i += 2
        elif cmd == 0xFA:  # SB ... SE
            j = i + 2
            while j + 1 < n:
                if raw[j] == 0xFF and raw[j + 1] == 0xF0:
                    j += 2
                    break
                j += 1
            i = j
        elif cmd in (0xFD, 0xFE):  # DO / DONT：回应 WONT(0xFC)
            if i + 2 < n:
                resp.extend(bytes([0xFF, 0xFC if cmd == 0xFD else 0xFE, raw[i + 2]]))
                i += 3
            else:
                i += 2
        elif cmd in (0xFB, 0xFC):  # WILL / WONT：回应 DONT(0xFE)
            if i + 2 < n:
                resp.extend(bytes([0xFF, 0xFE, raw[i + 2]]))
                i += 3
            else:
                i += 2
        else:  # 其他 IAC 命令（GA 等）
            i += 2
    return bytes(data), bytes(resp)


# ── 网关主类 ────────────────────────────────────────────────

class LlmGateway:
    def __init__(self, host: str, port: int, encoding: str | None = None,
                 mock: bool = False, allow_confirm: bool = False,
                 llm_cfg: LLMConfig | None = None):
        self.host = host
        self.port = port
        self.encoding = encoding or PORT_ENCODINGS.get(port, "utf-8")
        self.mock = mock
        self.allow_confirm = allow_confirm
        self.llm_cfg = llm_cfg or LLMConfig()
        self.sock: socket.socket | None = None
        self._rx_thread: threading.Thread | None = None
        self._stop = threading.Event()

    # ── 连接与收发 ──
    def connect(self) -> None:
        self.sock = socket.create_connection((self.host, self.port), timeout=10)
        self.sock.settimeout(1)
        self._rx_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self._rx_thread.start()
        print(f"[网关] 已连接 {self.host}:{self.port}（编码 {self.encoding}，"
              f"LLM {'mock' if self.mock else 'API'} 模式）")
        print("[网关] 输入 `ai 自然语言` 解析为指令；`ai help` 看帮助；`/quit` 退出网关")

    def send_line(self, line: str) -> None:
        if not self.sock:
            return
        data = line.encode(self.encoding, errors="replace") + b"\r\n"
        try:
            self.sock.sendall(data)
        except OSError as e:
            print(f"[网关] 发送失败: {e}")

    def _receive_loop(self) -> None:
        buf = bytearray()
        last_flush = time.time()
        while not self._stop.is_set():
            try:
                if not self.sock:
                    break
                chunk = self.sock.recv(4096)
            except socket.timeout:
                # 无新数据时，把超过 1s 未终结的残块 flush 出来
                # （MUD 登录提示「请输入你的 ID：」这类无换行提示需要即时可见）
                if buf and time.time() - last_flush > 1.0:
                    print(bytes(buf).decode(self.encoding, errors="replace"), end="", flush=True)
                    buf.clear()
                    last_flush = time.time()
                continue
            except OSError:
                break
            if not chunk:
                break
            data, response = strip_telnet(chunk)
            if response:
                try:
                    self.sock.sendall(response)
                except OSError:
                    pass
            if not data:
                continue
            buf += data
            # 按换行切分输出；残块保留在 buf 中等待终结或 flush
            while True:
                idx = buf.find(b"\n")
                if idx == -1:
                    break
                line = bytes(buf[:idx]).decode(self.encoding, errors="replace")
                del buf[:idx + 1]
                print(line)
                last_flush = time.time()
        self._stop.set()
        if buf:
            print(bytes(buf).decode(self.encoding, errors="replace"), end="", flush=True)
        print("\n[网关] 连接已断开")

    # ── ai 指令处理 ──
    def handle_ai(self, text: str) -> None:
        text = text.strip()
        if not text:
            return
        print("[网关] 思考中…")
        try:
            if self.mock:
                from tools.llm.mock_llm import parse_mock
                payload = parse_mock(text)
            else:
                if not self.llm_cfg.configured:
                    print("[网关] 未配置 LLM API 密钥——请在你自己主机上配置（见 README）"
                          "，或加 --mock 使用演示模式。")
                    return
                payload = chat_completion(self.llm_cfg, SYSTEM_PROMPT, text)
        except LLMError as e:
            print(f"[网关] LLM 调用失败，已安全中止（不执行任何指令）: {e}")
            return

        try:
            allowed, blocked = split_commands(payload, allow_confirm=self.allow_confirm)
        except SafetyError as e:
            print(f"[网关] 解析结果不合法，已安全中止（不执行任何指令）: {e}")
            return

        reason = payload.get("reason", "")
        if reason:
            print(f"[网关] 意图解析：{reason}")

        for b in blocked:
            print(f"[网关] ⛔ 拦截：{b}")

        if not allowed:
            print("[网关] 没有可安全执行的指令（已全部拦截或为空）。")
            return

        print("[网关] 计划执行：")
        for idx, cmd in enumerate(allowed, 1):
            print(f"      {idx}. {cmd}")
        print("[网关] 逐条回写连接执行…")
        for cmd in allowed:
            if self._stop.is_set():
                break
            self.send_line(cmd)
            time.sleep(COMMAND_INTERVAL)
        print("[网关] 执行完毕")

    def run(self) -> None:
        try:
            self.connect()
        except OSError as e:
            print(f"[网关] 连接失败: {e}", file=sys.stderr)
            sys.exit(1)

        while not self._stop.is_set():
            try:
                raw = input()
            except (EOFError, KeyboardInterrupt):
                print("\n[网关] 退出")
                break
            line = raw.rstrip("\r\n")
            if not line.strip():
                continue
            if line.strip() == "/quit":
                print("[网关] 退出")
                break
            if line.strip().lower() == "ai help":
                print("[网关] ai <自然语言> —— 解析为 MUD 指令并回写执行；"
                      "/quit —— 退出网关；其他输入原样透传给游戏。")
                continue
            if line.startswith("ai ") or line.strip().lower() == "ai":
                self.handle_ai(line[3:] if line.startswith("ai ") else "")
            else:
                self.send_line(line)
        self.close()

    def close(self) -> None:
        self._stop.set()
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
        if self._rx_thread:
            self._rx_thread.join(timeout=2)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="python -m tools.llm.gateway",
        description="Phase 0 外挂式 LLM 网关：自然语言 → MUD 指令解析并回写执行",
    )
    parser.add_argument("--host", default=DEFAULT_HOST, help=f"游戏服务器地址（默认 {DEFAULT_HOST}）")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"游戏端口（默认 {DEFAULT_PORT}，5555=GBK / 6666=UTF-8）")
    parser.add_argument("--encoding", choices=["gbk", "utf-8"],
                        help="覆盖端口默认编码（5555→gbk，6666→utf-8）")
    parser.add_argument("--mock", action="store_true",
                        help="mock 模式：不调 LLM，用内置规则解析（演示/自测，真实效果请配 API）")
    parser.add_argument("--allow-confirm", action="store_true",
                        help="放行模型标为高危(confirm)的指令（默认全部拦截；"
                             "commands 中的危险动词与管理命令仍一律拦截；PoC 不推荐开启）")
    parser.add_argument("--llm-base", help="OpenAI 兼容 API 地址（默认读环境变量 LLM_API_BASE）")
    parser.add_argument("--llm-key", help="API 密钥（默认读环境变量 LLM_API_KEY；建议用环境变量）")
    parser.add_argument("--model", help="模型名（默认读环境变量 LLM_MODEL）")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    cfg = LLMConfig(api_base=args.llm_base, api_key=args.llm_key, model=args.model)
    gw = LlmGateway(
        host=args.host, port=args.port, encoding=args.encoding,
        mock=args.mock, allow_confirm=args.allow_confirm, llm_cfg=cfg,
    )
    if not args.mock:
        print(f"[网关] LLM 配置：{cfg.describe()}")
        if not cfg.configured:
            print("[网关] 警告：未检测到 API 密钥，ai 指令将不可用（可加 --mock 演示）")
    gw.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
