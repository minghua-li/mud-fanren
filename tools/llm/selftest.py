from __future__ import annotations

"""#69 三条验收的自测脚本（可重复运行，作为交付实证）。

运行：python tools/llm/selftest.py
退出码 0 = 全部通过；非 0 = 有失败项。

覆盖（对照票面验收）：
  c1 网关可独立运行：连接端口 + 透传 + ai 拦截 + 回写执行（fake MUD 服务器实证）
  c2 `ai 去当铺把金条当了` → go/list/pawn 序列被回写执行
  c3 无效/危险输入失败安全：危险指令不注入、LLM 垃圾响应安全中止、降级路径实测

不依赖真实游戏服务（5555/6666 未就绪也可跑）；不依赖真实 LLM API（mock/注入替身）。
"""

import socket
import sys
import threading
import time
from pathlib import Path

# 允许直接 `python tools/llm/selftest.py` 运行（也支持 python -m tools.llm.selftest）
_SYSROOT = str(Path(__file__).resolve().parents[2])
if _SYSROOT not in sys.path:
    sys.path.insert(0, _SYSROOT)

from tools.llm.safety import check_command, split_commands, SafetyError
from tools.llm.mock_llm import parse_mock
from tools.llm.llm_client import LLMConfig, LLMError
from tools.llm.gateway import LlmGateway, strip_telnet

# ── fake MUD 服务器 ─────────────────────────────────────────

class FakeMudServer:
    """本地模拟 MUD 服务器：telnet 协商 + 按行回显 + 记录收到的指令行。"""

    def __init__(self, encoding: str = "utf-8"):
        self.encoding = encoding
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("127.0.0.1", 0))
        self.sock.listen(1)
        self.port = self.sock.getsockname()[1]
        self.received: list[str] = []          # 服务器收到的指令行（剔除协商）
        self.greeting_sent = False
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()

    def _serve(self) -> None:
        conn, _ = self.sock.accept()
        conn.settimeout(0.2)
        # 发 telnet 协商（模拟真实服务器）
        conn.sendall(b"\xff\xfd\x18\xff\xfd\x1f\xff\xfd'\xff\xfbV\xff\xfb\xc9\xff\xfbF\xff\xfb*\xff\xfbZ")
        conn.sendall(("\r\n欢迎来到模拟 MUD 世界。\r\n").encode(self.encoding))
        buf = bytearray()
        while not self._stop.is_set():
            try:
                chunk = conn.recv(4096)
            except socket.timeout:
                continue
            except OSError:
                break
            if not chunk:
                break
            buf += chunk
            # 剥 telnet 协商（网关发回的 WONT/DONT 回应直接丢弃）
            clean, _resp = strip_telnet(bytes(buf))
            buf = bytearray()
            # strip_telnet 只返回纯数据；但要保留残块，这里简化：整块按行解析
            if clean:
                for raw_line in clean.split(b"\r\n"):
                    line = raw_line.strip()
                    if not line:
                        continue
                    text = line.decode(self.encoding, errors="replace")
                    self.received.append(text)
                    conn.sendall((f"服务器收到：{text}\r\n").encode(self.encoding))
        try:
            conn.close()
        except OSError:
            pass

    def wait_received(self, count: int, timeout: float = 5.0) -> list[str]:
        """等待服务器收到至少 count 条指令行。"""
        deadline = time.time() + timeout
        while time.time() < deadline:
            if len(self.received) >= count:
                break
            time.sleep(0.05)
        return list(self.received)

    def close(self) -> None:
        self._stop.set()
        try:
            self.sock.close()
        except OSError:
            pass


# ── 测试框架 ────────────────────────────────────────────────

_PASS = []
_FAIL = []


def check(name: str, cond: bool, detail: str = "") -> None:
    if cond:
        _PASS.append(name)
        print(f"  ✓ {name}")
    else:
        _FAIL.append(name)
        print(f"  ✗ {name}  {detail}")


def section(title: str) -> None:
    print(f"\n── {title} ──")


# ── 单元测试 ────────────────────────────────────────────────

def test_telnet_strip() -> None:
    section("单元：telnet 协商剥离")
    raw = (b"\xff\xfd\x18\xff\xfd\x1f\xff\xfd'\xff\xfbV\xff\xfb\xc9"
           b"hello\r\n")
    data, _resp = strip_telnet(raw)
    check("IAC 序列被剥离，数据保留", data == b"hello\r\n", repr(data))
    # 回应 DO → WONT，WILL → DONT（在第二返回值里）
    _d, resp = strip_telnet(b"\xff\xfd\x18")
    check("DO 被回应 WONT", resp == b"\xff\xfc\x18", repr(resp))
    _d2, resp2 = strip_telnet(b"\xff\xfb\x56")
    check("WILL 被回应 DONT", resp2 == b"\xff\xfe\x56", repr(resp2))
    # SB...SE 整段跳过
    data3, _r3 = strip_telnet(b"\xff\xfa\x18\x00abc\xff\xf0data")
    check("SB...SE 整段跳过", data3 == b"data", repr(data3))


def test_safety_unit() -> None:
    section("单元：指令安全过滤")
    # 白名单通过
    for ok in ("go east", "look", "list", "pawn jin tiao", "inventory", "say 你好"):
        try:
            check_command(ok)
            check(f"白名单通过: {ok!r}", True)
        except SafetyError as e:
            check(f"白名单通过: {ok!r}", False, str(e))
    # 危险/越权拒绝
    for bad in ("kill npc", "hit npc", "quit", "drop gold", "give gold to npc",
                "shutdown", "exec /bin/sh", "eval 1+1", "ls", "update /d/city",
                "snoop", "call out", "rm file"):
        try:
            check_command(bad)
            check(f"拒绝: {bad!r}", False, "未抛 SafetyError")
        except SafetyError:
            check(f"拒绝: {bad!r}", True)
    # 注入字符拒绝
    for inj in ("go east; kill npc", "go | shutdown", "look $(whoami)", "say `pwd`"):
        try:
            check_command(inj)
            check(f"拒绝注入: {inj!r}", False, "未抛 SafetyError")
        except SafetyError:
            check(f"拒绝注入: {inj!r}", True)


def test_split_commands() -> None:
    section("单元：LLM 输出分类（commands/confirm/非法）")
    allowed, blocked = split_commands({
        "commands": ["go east", "list", "pawn gold"],
        "confirm": ["kill npc"],
        "reason": "去当铺",
    })
    check("安全指令放行", allowed == ["go east", "list", "pawn gold"], repr(allowed))
    check("confirm 高危被拦截", any("kill" in b for b in blocked), repr(blocked))
    allowed2, blocked2 = split_commands({
        "commands": ["go east", "shutdown", "quit"],
        "confirm": [],
    })
    check("危险指令从 commands 中被拦截", allowed2 == ["go east"], repr(allowed2))
    check("拦截条目含原因", any("shutdown" in b and "拒绝" in b for b in blocked2), repr(blocked2))
    try:
        split_commands({"commands": "not-a-list"})
        check("非数组 commands 抛错", False, "未抛 SafetyError")
    except SafetyError:
        check("非数组 commands 抛错", True)


def test_mock_parser() -> None:
    section("单元：mock 解析器")
    payload = parse_mock("去当铺把金条当了")
    cmds = payload["commands"]
    check("产出移动指令", any(c.startswith("go ") for c in cmds), repr(cmds))
    check("产出 list", "list" in cmds, repr(cmds))
    check("产出 pawn", any(c.startswith("pawn ") for c in cmds), repr(cmds))
    # mock 决不产出危险指令
    for text in ("杀掉眼前这个人", "把金条给别人", "退出游戏", "rm 所有文件"):
        p = parse_mock(text)
        for c in p["commands"]:
            try:
                check_command(c)
                check(f"mock({text!r}) 指令合法", True)
            except SafetyError:
                check(f"mock({text!r}) 指令合法", False, f"产出非法指令 {c!r}")


# ── 集成测试：网关 ↔ fake MUD 服务器 ───────────────────────

def make_gateway(server: FakeMudServer, **kw) -> LlmGateway:
    gw = LlmGateway(
        host="127.0.0.1", port=server.port,
        encoding=kw.pop("encoding", "utf-8"), **kw,
    )
    gw.connect()
    # 等 fake 服务器协商完成
    server.wait_received(0, timeout=2)
    return gw


def test_c1_gateway_run() -> None:
    section("验收 c1：网关可独立运行（连接/透传/拦截/回写）")
    server = FakeMudServer()
    try:
        gw = make_gateway(server, mock=True)
        check("TCP 连接成功", gw.sock is not None)
        # 普通输入透传
        gw.send_line("say 你好")
        got = server.wait_received(1)
        check("普通输入透传", got == ["say 你好"], repr(got))
        # ai 拦截 → mock 解析 → 回写执行
        gw.handle_ai("去当铺把金条当了")
        got = server.wait_received(5)
        moves = [c for c in got if c.startswith("go ")]
        check("ai 输入被拦截并回写指令序列", len(got) >= 4, repr(got))
        check("序列含移动指令", len(moves) >= 1, repr(got))
        check("序列含 list", "list" in got, repr(got))
        check("序列含 pawn", any(c.startswith("pawn ") for c in got), repr(got))
        gw.close()
    finally:
        server.close()


def test_c2_pawn_sequence() -> None:
    section("验收 c2：`ai 去当铺把金条当了` 产出并执行 go/list/pawn 序列")
    server = FakeMudServer()
    try:
        gw = make_gateway(server, mock=True)
        gw.handle_ai("去当铺把金条当了")
        got = server.wait_received(5)
        # 顺序：go... → list → pawn
        go_idx = [i for i, c in enumerate(got) if c.startswith("go ")]
        list_idx = [i for i, c in enumerate(got) if c == "list"]
        pawn_idx = [i for i, c in enumerate(got) if c.startswith("pawn ")]
        check("移动在 list 之前", go_idx and list_idx and go_idx[-1] < list_idx[0], repr(got))
        check("list 在 pawn 之前", list_idx and pawn_idx and list_idx[0] < pawn_idx[0], repr(got))
        check("完整序列=go+list+pawn", bool(go_idx and list_idx and pawn_idx), repr(got))
        gw.close()
    finally:
        server.close()


def test_c3_fail_safe() -> None:
    section("验收 c3：无效/危险输入失败安全（拦截/降级实测）")
    # 3a：LLM 返回含危险指令的 JSON → 危险指令不注入
    server = FakeMudServer()
    from tools.llm import gateway as gw_mod
    orig_chat = gw_mod.chat_completion

    def evil_chat(cfg, system, user):
        return {
            "commands": ["go east", "kill npc", "quit", "shutdown"],
            "confirm": ["drop gold", "give gold to npc"],
            "reason": "恶意测试",
        }

    gw_mod.chat_completion = evil_chat
    try:
        gw = make_gateway(server, mock=False, llm_cfg=LLMConfig(api_key="test-key"))
        gw.handle_ai("杀掉眼前这个人")
        got = server.wait_received(2)
        check("只注入白名单指令", got == ["go east"], repr(got))
        check("kill 未注入", all("kill" not in c for c in got), repr(got))
        check("quit/shutdown 未注入", all("quit" not in c and "shutdown" not in c for c in got), repr(got))
        check("drop/give 未注入", all("drop" not in c and "give" not in c for c in got), repr(got))
        gw.close()
    finally:
        gw_mod.chat_completion = orig_chat
        server.close()

    # 3b：LLM 返回垃圾（非 JSON）→ 抛 LLMError → 安全中止，零注入
    server2 = FakeMudServer()

    def garbage_chat(cfg, system, user):
        raise LLMError("模型返回了不可解析的内容")

    gw_mod.chat_completion = garbage_chat
    try:
        gw = make_gateway(server2, mock=False, llm_cfg=LLMConfig(api_key="test-key"))
        gw.handle_ai("随便说点什么")
        time.sleep(0.5)
        check("LLM 失败时零注入", server2.received == [], repr(server2.received))
        gw.close()
    finally:
        gw_mod.chat_completion = orig_chat
        server2.close()

    # 3c：命令条数超限 → 整体拒绝
    try:
        split_commands({"commands": ["go east"] * 9, "confirm": []})
        check("超限拒绝", False, "未抛 SafetyError")
    except SafetyError:
        check("超限拒绝", True)

    # 3d：未配置 API key + 非 mock → 拒绝调用（提示配置），零注入
    server3 = FakeMudServer()
    try:
        gw = make_gateway(server3, mock=False, llm_cfg=LLMConfig(api_key=""))
        gw.handle_ai("去当铺")
        time.sleep(0.3)
        check("无 key 时零注入", server3.received == [], repr(server3.received))
        gw.close()
    finally:
        server3.close()


def main() -> int:
    print("tools/llm selftest —— #69 验收实证")
    test_telnet_strip()
    test_safety_unit()
    test_split_commands()
    test_mock_parser()
    test_c1_gateway_run()
    test_c2_pawn_sequence()
    test_c3_fail_safe()
    print(f"\n结果：{len(_PASS)} 通过，{len(_FAIL)} 失败")
    if _FAIL:
        print("失败项：")
        for f in _FAIL:
            print(f"  - {f}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
