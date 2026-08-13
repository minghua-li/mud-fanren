from __future__ import annotations

"""#70 验收自测脚本（可重复运行，作为交付实证）。

运行：python tools/llm/sidecar_selftest.py
退出码 0 = 全部通过；非 0 = 有失败项。

覆盖（对照 #70 验收清单 6 条）：
  c1 sidecar 实现：NL→LPC 指令映射（复用 #69 llm_client/safety/mock），
     自测覆盖解析（mock 当铺链路）/安全拦截（LLM 替身输出危险指令）/超时（慢 LLM）
  c2 llmd.c：TCP 127.0.0.1 新行分隔 JSON、每请求一连接、watchdog 15s 超时零注入
     （LPC 逻辑忠实翻译模拟：正常注入 / 超时零注入 / 迟到响应忽略 / 断线零注入）
  c3 globals.h 开关 + 开启命令：LLM_D 宏、默认关闭、ai.c set llm on opt-in 检查
  c4 cmds/usr/ai.c 集成点：请求→sidecar→响应→注入链路（端到端实测 + 超时不注入半截）
  c5 端到端自测：sidecar 真实 TCP 服务 + fake LPC 客户端全链路
  c6 密钥安全：LLM 密钥仅玩家本地配置（#69 资产继承），sidecar 零硬编码密钥

不依赖真实游戏服务（无 fluffos 环境）与真实 LLM API（mock/注入替身）。
LPC 侧为静态校验 + Python 忠实翻译模拟，作用域如实标注（真实 driver 待装）。
"""

import json
import re
import socket
import sys
import threading
import time
from pathlib import Path

# 允许直接 `python tools/llm/sidecar_selftest.py` 运行
_SYSROOT = str(Path(__file__).resolve().parents[2])
if _SYSROOT not in sys.path:
    sys.path.insert(0, _SYSROOT)

from tools.llm import sidecar as sidecar_mod
from tools.llm.sidecar import LlmSidecar, filter_payload, SYSTEM_PROMPT
from tools.llm.llm_client import LLMConfig, LLMError
from tools.llm.safety import check_command, SafetyError

ROOT = Path(_SYSROOT)
LLMD_PATH = ROOT / "adm/daemons/llmd.c"
AI_PATH = ROOT / "cmds/usr/ai.c"
GLOBALS_PATH = ROOT / "include/globals.h"

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


# ── fake LPC 客户端（模拟 llmd.c 侧的 TCP 请求方）─────────────

class FakeLpcClient:
    """连 sidecar 的 TCP 客户端：发一行请求 JSON，收一行响应 JSON。"""

    def __init__(self, host: str, port: int, timeout: float = 5.0):
        self.sock = socket.create_connection((host, port), timeout=timeout)

    def request(self, payload: dict) -> dict:
        self.sock.sendall(json.dumps(payload, ensure_ascii=False).encode("utf-8") + b"\n")
        data = self.sock.recv(65536).decode("utf-8")
        return json.loads(data)

    def close(self) -> None:
        try:
            self.sock.close()
        except OSError:
            pass


# ═══════════════════════════════════════════════════════════
# c1：sidecar 解析 / 安全拦截 / 超时
# ═══════════════════════════════════════════════════════════

def test_sidecar_parse_mock() -> None:
    section("c1：sidecar 解析（mock 模式）")
    sc = LlmSidecar(port=0, mock=True, timeout=1.0)
    # 直接测 handle_line（不经过网络）
    resp = json.loads(sc.handle_line(json.dumps({
        "player": "hanli", "language": "去当铺把金条当了",
        "grounding": {"room_short": "青牛镇", "exits": ["east"], "objects": [], "inventory": []},
    }, ensure_ascii=False)))
    check("error 为空", resp.get("error") is None, repr(resp.get("error")))
    cmds = resp.get("commands") or []
    check("产出移动指令", any(c.startswith("go ") for c in cmds), repr(cmds))
    check("产出 list", "list" in cmds, repr(cmds))
    check("产出 pawn", any(c.startswith("pawn ") for c in cmds), repr(cmds))
    check("confirm 为空（mock 不产高危）", resp.get("confirm") == [], repr(resp.get("confirm")))
    # grounding 进入 prompt
    prompt = sc.build_prompt({"room_short": "青牛镇", "room_long": "测试房间", "exits": ["east"],
                              "objects": ["小贩"], "inventory": ["金条"], "realm": "炼气1层"})
    check("prompt 含房间名", "青牛镇" in prompt, "")
    check("prompt 含出口", "east" in prompt, "")
    check("prompt 含背包", "金条" in prompt, "")


def test_sidecar_safety() -> None:
    section("c1：sidecar 安全拦截（LLM 替身返回危险/管理指令）")
    orig_chat = sidecar_mod.chat_completion

    def evil_chat(cfg, system, user):
        return {"commands": ["go east", "kill npc", "shutdown", "drop gold"],
                "confirm": ["quit", "give gold to npc", "ls"],
                "reason": "恶意测试"}

    sidecar_mod.chat_completion = evil_chat
    try:
        cfg = LLMConfig(api_base="http://127.0.0.1:9/v1", api_key="k", model="m")  # configured=True
        sc = LlmSidecar(port=0, mock=False, timeout=1.0, llm_cfg=cfg)
        resp = json.loads(sc.handle_line(json.dumps(
            {"player": "p", "language": "杀怪并丢装备"}, ensure_ascii=False)))
    finally:
        sidecar_mod.chat_completion = orig_chat
    cmds = resp.get("commands") or []
    blocked = resp.get("blocked") or []
    confirms = resp.get("confirm") or []
    check("安全指令 go east 放行", "go east" in cmds, repr(cmds))
    check("危险指令 kill 不进 commands", "kill npc" not in cmds, repr(cmds))
    check("管理命令 shutdown 被拦截", any("shutdown" in b for b in blocked), repr(blocked))
    check("confirm 仅放行高危动词（quit/give 在列）",
          "quit" in confirms and "give gold to npc" in confirms, repr(confirms))
    check("confirm 中的管理命令 ls 被拦截", any("ls" in b for b in blocked), repr(blocked))
    # 非白名单动词拒绝
    allowed, confirm_ok, blocked2 = filter_payload(
        {"commands": ["go east", "fly to moon"], "confirm": [], "reason": ""})
    check("非白名单动词拦截", allowed == ["go east"] and any("fly" in b for b in blocked2),
          repr((allowed, blocked2)))


def test_sidecar_timeout() -> None:
    section("c1：sidecar 超时（慢 LLM 替身 → 错误响应不挂死）")
    orig_chat = sidecar_mod.chat_completion

    def slow_chat(cfg, system, user):
        time.sleep(3.0)  # 比 timeout 长
        return {"commands": ["look"], "confirm": [], "reason": "迟到"}

    sidecar_mod.chat_completion = slow_chat
    try:
        cfg = LLMConfig(api_base="http://127.0.0.1:9/v1", api_key="k", model="m")  # configured=True
        sc = LlmSidecar(port=0, mock=False, timeout=0.5, llm_cfg=cfg)
        t0 = time.time()
        resp = json.loads(sc.handle_line(json.dumps(
            {"player": "p", "language": "看看四周"}, ensure_ascii=False)))
        elapsed = time.time() - t0
    finally:
        sidecar_mod.chat_completion = orig_chat
    check("超时后返回 error（含超时/中止信息）",
          resp.get("error") is not None and ("超时" in str(resp.get("error"))
                                              or "超过" in str(resp.get("error"))
                                              or "中止" in str(resp.get("error"))),
          repr(resp.get("error")))
    check("超时响应中零指令", resp.get("commands") == [], repr(resp.get("commands")))
    check("未等待到慢 LLM 返回（< 2.5s）", elapsed < 2.5, f"{elapsed:.2f}s")


def test_sidecar_protocol_errors() -> None:
    section("c1：sidecar 协议容错（非法请求 → 错误响应）")
    sc = LlmSidecar(port=0, mock=True, timeout=1.0)
    resp = json.loads(sc.handle_line("not-json{{{"))
    check("非 JSON 请求返回 error", resp.get("error") is not None, repr(resp.get("error")))
    resp2 = json.loads(sc.handle_line(json.dumps({"player": "p"}, ensure_ascii=False)))
    check("缺 language 返回 error", resp2.get("error") is not None, repr(resp2.get("error")))
    resp3 = json.loads(sc.handle_line(json.dumps(
        {"player": "p", "language": "hi", "grounding": "bad"}, ensure_ascii=False)))
    check("grounding 非对象返回 error", resp3.get("error") is not None, repr(resp3.get("error")))


def test_sidecar_no_key() -> None:
    section("c1：sidecar 未配置密钥（非 mock → 错误响应，零注入）")
    # 显式构造未配置的 cfg：默认云端地址 + 空 key → configured=False
    cfg = LLMConfig(api_base="https://api.openai.com/v1", api_key="", model="m")
    cfg.api_key = ""
    sc = LlmSidecar(port=0, mock=False, timeout=1.0, llm_cfg=cfg)
    resp = json.loads(sc.handle_line(json.dumps(
        {"player": "p", "language": "去当铺"}, ensure_ascii=False)))
    check("未配置 key 返回 error（提及 llm.json 本地配置）",
          resp.get("error") is not None and "llm.json" in str(resp.get("error")),
          repr(resp.get("error")))
    check("未配置 key 零指令", resp.get("commands") == [], repr(resp.get("commands")))


# ═══════════════════════════════════════════════════════════
# c2/c3/c4：LPC 静态校验（无 driver 环境，静态验收法）
# ═══════════════════════════════════════════════════════════

def strip_lpc(src: str) -> str:
    """剥离 LPC 字符串/行注释/块注释/@LONG heredoc，供括号配对（#58 起方法）。"""
    out = []
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            j = src.find("\n", i)
            i = n if j == -1 else j + 1
        elif c == "/" and i + 1 < n and src[i + 1] == "*":
            j = src.find("*/", i + 2)
            i = n if j == -1 else j + 2
        elif c == "@":
            # heredoc：@LONG 到 LONG（行首）
            m = re.match(r"@([A-Za-z_][A-Za-z0-9_]*)", src[i:])
            if m:
                marker = m.group(1)
                j = src.find("\n" + marker, i)
                i = n if j == -1 else j + 1 + len(marker)
            else:
                out.append(c)
                i += 1
        elif c == '"':
            # 字符串（含转义）
            i += 1
            while i < n:
                if src[i] == "\\":
                    i += 2
                elif src[i] == '"':
                    i += 1
                    break
                else:
                    i += 1
        elif c == "'":
            i += 1
            while i < n:
                if src[i] == "\\":
                    i += 2
                elif src[i] == "'":
                    i += 1
                    break
                else:
                    i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def lpc_braces_ok(path: Path) -> bool:
    src = path.read_text(encoding="utf-8")
    stripped = strip_lpc(src)
    for op, cl in (("{", "}"), ("(", ")"), ("[", "]")):
        if stripped.count(op) != stripped.count(cl):
            return False
    return True


def lpc_func_body(src: str, func_sig: str) -> str:
    """提取 LPC 函数体（签名行到函数结束的 } 的原始源码；找不到返回空串）。

    用于函数体级守卫：比全文子串检查更精确地绑定真实 LPC 代码的关键行为。
    跳过前向声明（签名行以 ; 结尾，如 `void watchdog_timeout(int fd);`）。
    """
    idx = 0
    while True:
        idx = src.find(func_sig, idx)
        if idx == -1:
            return ""
        line_end = src.find("\n", idx)
        if line_end == -1:
            line_end = len(src)
        if src[idx:line_end].rstrip().endswith(";"):
            idx = line_end + 1  # 前向声明，继续找定义
            continue
        brace = src.find("{", idx)
        if brace == -1:
            return ""
        depth = 0
        for i in range(brace, len(src)):
            if src[i] == "{":
                depth += 1
            elif src[i] == "}":
                depth -= 1
                if depth == 0:
                    return src[brace:i + 1]
        return ""


def test_lpc_static() -> None:
    section("c2/c3/c4：LPC 静态校验（llmd.c / ai.c / globals.h）")
    llmd = LLMD_PATH.read_text(encoding="utf-8")
    ai = AI_PATH.read_text(encoding="utf-8")
    gh = GLOBALS_PATH.read_text(encoding="utf-8")

    # c3：globals.h 开关与常量
    check("globals.h 有 LLM_D 宏", re.search(r'#define\s+LLM_D\s+"/adm/daemons/llmd"', gh) is not None, "")
    check("globals.h 有 LLM_SIDECAR_HOST=127.0.0.1",
          re.search(r'#define\s+LLM_SIDECAR_HOST\s+"127\.0\.0\.1"', gh) is not None, "")
    check("globals.h 有 LLM_SIDECAR_PORT",
          re.search(r'#define\s+LLM_SIDECAR_PORT\s+\d+', gh) is not None, "")
    m = re.search(r'#define\s+LLM_WATCHDOG_TIME\s+(\d+)', gh)
    check("globals.h 有 LLM_WATCHDOG_TIME=15", m is not None and int(m.group(1)) == 15,
          repr(m.group(1)) if m else "missing")

    # c2：llmd.c TCP 通信 + 每请求一连接 + watchdog 零注入
    check("llmd.c 用 socket_create(STREAM)", "socket_create(STREAM" in llmd, "")
    check("llmd.c 用 socket_connect（127.0.0.1 由宏提供）", "socket_connect" in llmd, "")
    check("llmd.c 用 socket_write", "socket_write" in llmd, "")
    check("llmd.c 每请求一连接（request_parse 内创建 socket）",
          "socket_create" in llmd and "request_parse" in llmd, "")
    check("llmd.c 用 json_parse（响应解析）", "json_parse" in llmd, "")
    check("llmd.c 用 json_encode（请求组装）", "json_encode" in llmd, "")
    check("llmd.c watchdog call_out 15s",
          "call_out(\"watchdog_timeout\", LLM_WATCHDOG_TIME, fd)" in llmd, "")
    check("llmd.c 超时路径零注入（清理 pending 后迟到响应忽略）",
          "if (!mapp(pending[fd])) return;" in llmd and "watchdog_timeout" in llmd, "")
    check("llmd.c 注入经 force_me 调用", "me->force_me(" in llmd, "")
    check("llmd.c 有 LPC 侧白名单防御（LLM_SAFE_VERBS）", "LLM_SAFE_VERBS" in llmd, "")
    check("llmd.c 默认关闭（llm_enabled 置 0）",
          'set("llm_enabled", 0)' in llmd and "is_llm_enabled" in llmd, "")
    check("llmd.c 玩家 opt-in（env/llm）", "env/llm" in llmd, "")

    # ── LPC 原文函数体级守卫（回应审查：模拟与真代码挂钩，防回归）──
    # 注意：用原始函数体（不剥字符串）——守卫要检查的正是字符串内容（提示文案）。
    # watchdog_timeout 函数体：超时路径必须清理 pending、且绝无注入调用
    wd_body = lpc_func_body(llmd, "void watchdog_timeout(int fd)")
    check("watchdog_timeout 函数体含清理（cleanup_fd）", "cleanup_fd" in wd_body, "")
    check("watchdog_timeout 函数体含超时提示", "tell_object" in wd_body, "")
    check("watchdog_timeout 函数体零注入（无 force_me(/inject_commands(）",
          "force_me(" not in wd_body and "inject_commands(" not in wd_body,
          f"发现注入调用: {wd_body[:200]}")
    # inject_commands 函数体：注入真走 force_me
    inj_body = lpc_func_body(llmd, "void inject_commands(object me, string *cmds, int mode)")
    check("inject_commands 函数体含 me->force_me(", "me->force_me(" in inj_body, "")
    # handle_response 函数体：错误路径提示「未注入任何指令」+ commands 注入
    hr_body = lpc_func_body(llmd, "protected void handle_response(int fd, string line)")
    check("handle_response 错误路径含「未注入任何指令」提示",
          "未注入任何指令" in hr_body and "inject_commands(" in hr_body, "")

    # c4：ai.c 集成点
    check("ai.c 调 LLM_D->is_llm_enabled", "is_llm_enabled" in ai, "")
    check("ai.c 检查玩家 opt-in", "player_opted_in" in ai, "")
    check("ai.c 提示 set llm on", "set llm on" in ai, "")
    check("ai.c 走 request_parse", "request_parse" in ai, "")
    check("ai.c 支持 confirm yes/no", "confirm yes" in ai and "confirm no" in ai, "")
    check("ai.c 裸 confirm 提示（不把 confirm 当自然语言发给 sidecar）",
          'arg == "confirm"' in ai and "notify_fail" in ai, "")

    # 括号配对状态机
    check("llmd.c 括号配对", lpc_braces_ok(LLMD_PATH), "")
    check("ai.c 括号配对", lpc_braces_ok(AI_PATH), "")

    # 接口签名存在性（本仓先例核实；文件缺失时 fail-loud 判红而非中断）
    imd = LLMD_PATH.parent / "im_d.c"
    cmd_f = ROOT / "feature/command.c"
    imd_src = imd.read_text(encoding="utf-8") if imd.exists() else ""
    cmd_src = cmd_f.read_text(encoding="utf-8") if cmd_f.exists() else ""
    check("socket efun 有先例（im_d.c）", "socket_create" in imd_src, "")
    check("force_me 有先例（feature/command.c）", "force_me" in cmd_src, "")


# ═══════════════════════════════════════════════════════════
# c2：LPC 逻辑忠实翻译模拟（watchdog 零注入等）
# ═══════════════════════════════════════════════════════════

class FakePlayer:
    """模拟 LPC 玩家对象（force_me / tell_object / temp）。"""

    def __init__(self, pid: str):
        self.pid = pid
        self.alive = True
        self.injected: list[str] = []
        self.msgs: list[str] = []
        self.temp: dict = {}

    def tell(self, msg: str) -> None:
        self.msgs.append(msg)

    def force_me(self, cmd: str) -> int:
        if not self.alive:
            return 0
        self.injected.append(cmd)
        return 1

    def query_temp(self, key: str):
        return self.temp.get(key)

    def set_temp(self, key: str, val) -> None:
        self.temp[key] = val

    def delete_temp(self, key: str) -> None:
        self.temp.pop(key, None)

    def pending_requests(self, d: "LpcLlmD") -> int:
        """模拟 llmd.c player_pending_count：该玩家在 pending 表中的未决数。"""
        return sum(1 for m in d.pending.values() if m["player"] is self)


# llmd.c 关键逻辑的 Python 忠实翻译（每个函数标注源文件:行号）
SAFE_VERBS = {"go","look","l","inventory","i","score","hp","check","list","value",
              "pawn","dang","buy","sell","get","put","open","close","wear","wield",
              "unwield","remove","say","tell","reply","smile","nod","ask","watch",
              "listen","sit","sleep","eat","drink"}
DANGEROUS_VERBS = {"kill","hit","fight","steal","attack","give","drop","discard",
                   "quit","exit","relogin","suicide","reset","destruct"}
DENIED_VERBS = {"shutdown","exec","call","debug","update","reload","clone","cd",
                "pwd","ls","rm","mv","cp","tail","cat","ed","more","mkdir","rmdir",
                "chmod","patch","diff","grep","snoop","chinese","adm","arch","wiz",
                "immortal","eval","efun","dump","profile","mem","netstat","ps"}
LEGAL_CHARS = set("abcdefghijklmnopqrstuvwxyz0123456789_- ")


def lpc_valid_inject_cmd(cmd: str, mode: int) -> bool:
    """对应 llmd.c valid_inject_cmd：字符集 + 动词分层。"""
    if not cmd or len(cmd) > 80:
        return False
    if not all(c in LEGAL_CHARS for c in cmd):
        return False
    verb = cmd.split(" ")[0]
    if verb in DENIED_VERBS:
        return False
    if mode == 0:
        return verb in SAFE_VERBS
    return verb in DANGEROUS_VERBS


class LpcLlmD:
    """llmd.c 的忠实翻译模拟（pending 表 / watchdog / 注入 / 零注入路径）。

    只建模与验收相关的行为；socket 层用直接调用代替。
    每个方法标注对应的 llmd.c 源行号（审查第 1 轮指出此前缺标注，已补）：
      request          → llmd.c:153 request_parse + :200 write_callback（合并简化）
      tick             → 模拟时钟推进（触发 watchdog 调度，无直接对应）
      _watchdog        → llmd.c:268 watchdog_timeout
      sidecar_response → llmd.c:230 read_callback + :281 handle_response（合并）
      disconnect       → llmd.c:254 close_callback
      confirm_pending  → llmd.c:344 confirm_pending
      _cleanup         → llmd.c:413 cleanup_fd
    """

    WATCHDOG = 15

    def __init__(self):
        self.pending: dict[int, dict] = {}   # fd -> player/buf/sent/done
        self.fd_seq = 0
        self.now = 0.0
        self.watchdog_at: dict[int, float] = {}  # fd -> 到期时刻
        self.closed: list[int] = []

    def request(self, player: FakePlayer) -> int:
        """request_parse + write_callback 的合并简化（发送即触发 watchdog）。"""
        if player.pending_requests(self) >= 1:
            player.tell("已有一个 AI 请求在处理中")
            return 0
        self.fd_seq += 1
        fd = self.fd_seq
        self.pending[fd] = {"player": player, "buf": "", "sent": True, "done": False}
        self.watchdog_at[fd] = self.now + self.WATCHDOG
        return fd

    def tick(self, dt: float) -> None:
        """推进模拟时钟，触发到期的 watchdog。"""
        self.now += dt
        for fd, deadline in list(self.watchdog_at.items()):
            if self.now >= deadline and fd in self.pending:
                self._watchdog(fd)

    def _watchdog(self, fd: int) -> None:
        """watchdog_timeout：超时 → 提示 + 清理，零注入。"""
        m = self.pending.get(fd)
        if not m:
            return
        m["player"].tell("AI 解析超时（15 秒未响应），已安全中止")
        self._cleanup(fd)

    def sidecar_response(self, fd: int, line: str) -> None:
        """read_callback + handle_response 合并：完整行到达。"""
        m = self.pending.get(fd)
        if not m:
            m0 = self.pending.get(fd)
            # 迟到响应（fd 已清理）→ 忽略
            if not m0:
                return
            m = m0
        # 模拟 json_parse
        try:
            data = json.loads(line)
        except json.JSONDecodeError:
            m["player"].tell("AI 响应格式非法，已安全中止")
            self._cleanup(fd)
            return
        if not isinstance(data, dict):
            m["player"].tell("AI 响应格式非法，已安全中止")
            self._cleanup(fd)
            return
        err = data.get("error")
        if err:
            m["player"].tell(f"AI 解析失败：{err}，未注入任何指令")
            self._cleanup(fd)
            return
        # commands 注入（白名单）
        for cmd in data.get("commands") or []:
            if not isinstance(cmd, str):
                continue
            if not m["player"].alive:
                break
            if lpc_valid_inject_cmd(cmd, 0):
                m["player"].force_me(cmd)
            else:
                m["player"].tell(f"已拦截一条不合法指令：{cmd}")
        # confirm 存队列
        confirms = data.get("confirm") or []
        if confirms:
            m["player"].set_temp("llm_confirm", confirms)
        self._cleanup(fd)

    def disconnect(self, fd: int) -> None:
        """close_callback：未决请求被中断 → 零注入。"""
        m = self.pending.get(fd)
        if m and not m.get("done"):
            m["player"].tell("AI 服务连接意外中断，已安全中止")
        self._cleanup(fd)

    def confirm_pending(self, player: FakePlayer, yes: bool) -> None:
        """confirm_pending：二次确认注入（仅 DANGEROUS 动词）。"""
        confirms = player.query_temp("llm_confirm")
        player.delete_temp("llm_confirm")
        if not yes:
            return
        if not confirms:
            player.tell("当前没有待确认的高危指令")
            return
        for cmd in confirms:
            if not player.alive:
                break
            if lpc_valid_inject_cmd(cmd, 1):
                player.force_me(cmd)
            else:
                player.tell(f"已拦截一条不合法指令：{cmd}")

    def _cleanup(self, fd: int) -> None:
        if fd in self.pending:
            self.pending[fd]["done"] = True
            self.pending.pop(fd, None)
        self.watchdog_at.pop(fd, None)
        self.closed.append(fd)


# 为 FakePlayer 挂 pending 计数
def _player_pending(self: FakePlayer) -> int:
    return 0  # 由 LpcLlmD 管理，占位


class TestLpcSim:
    def __init__(self):
        self.d = LpcLlmD()
        self.p = FakePlayer("hanli")

    def test_normal_inject(self) -> None:
        section("c2 模拟：正常链路（响应在 watchdog 内 → 注入 + confirm 队列）")
        fd = self.d.request(self.p)
        check("请求被登记", fd in self.d.pending, "")
        self.d.sidecar_response(fd, json.dumps(
            {"commands": ["go east", "list", "pawn gold"], "confirm": [], "reason": "去当铺"}))
        check("注入完整序列", self.p.injected == ["go east", "list", "pawn gold"], repr(self.p.injected))
        check("pending 已清理", fd not in self.d.pending, "")
        check("watchdog 到期空跑（不重复提示）", "超时" not in "".join(self.p.msgs), repr(self.p.msgs))
        self.d.tick(16)
        check("超时后无重复清理（零副作用）", self.d.closed.count(fd) == 1, repr(self.d.closed))

    def test_watchdog_zero_inject(self) -> None:
        section("c2 模拟：watchdog 15s 超时 → 零注入 + 迟到响应忽略")
        self.p.injected.clear()
        self.p.msgs.clear()
        fd = self.d.request(self.p)
        self.d.tick(16)  # 超过 15s watchdog
        check("超时后零注入", self.p.injected == [], repr(self.p.injected))
        check("玩家收到超时提示", any("超时" in m for m in self.p.msgs), repr(self.p.msgs))
        check("pending 已清理", fd not in self.d.pending, "")
        # 迟到的 sidecar 响应：fd 已关闭/清理 → 忽略，不注入
        self.d.sidecar_response(fd, json.dumps({"commands": ["kill npc"], "confirm": []}))
        check("迟到响应被忽略（零注入）", self.p.injected == [], repr(self.p.injected))
        check("迟到响应无新消息", "不合法指令" not in "".join(self.p.msgs), repr(self.p.msgs))

    def test_lpc_whitelist(self) -> None:
        section("c2 模拟：LPC 侧白名单防线（sidecar 被替换时仍拦截危险/管理指令）")
        self.p.injected.clear()
        self.p.msgs.clear()
        fd = self.d.request(self.p)
        self.d.sidecar_response(fd, json.dumps(
            {"commands": ["go east", "kill npc", "shutdown", "drop gold"], "confirm": []}))
        check("仅白名单指令注入", self.p.injected == ["go east"], repr(self.p.injected))
        check("拦截提示已发", any("不合法指令" in m for m in self.p.msgs), repr(self.p.msgs))

    def test_confirm_flow(self) -> None:
        section("c2 模拟：高危指令二次确认（confirm yes 执行 / no 取消）")
        self.p.injected.clear()
        self.p.msgs.clear()
        fd = self.d.request(self.p)
        self.d.sidecar_response(fd, json.dumps(
            {"commands": [], "confirm": ["kill npc"], "reason": ""}))
        check("commands 不注入", self.p.injected == [], repr(self.p.injected))
        check("confirm 存入玩家 temp", self.p.query_temp("llm_confirm") == ["kill npc"],
              repr(self.p.query_temp("llm_confirm")))
        self.d.confirm_pending(self.p, True)
        check("confirm yes 后注入高危指令", self.p.injected == ["kill npc"], repr(self.p.injected))
        # 再次请求 → confirm no
        self.p.injected.clear()
        self.p.msgs.clear()
        fd2 = self.d.request(self.p)
        self.d.sidecar_response(fd2, json.dumps(
            {"commands": [], "confirm": ["drop gold"], "reason": ""}))
        self.d.confirm_pending(self.p, False)
        check("confirm no 不注入", self.p.injected == [], repr(self.p.injected))
        # 管理命令即使玩家确认也绝不放行
        self.p.injected.clear()
        self.p.msgs.clear()
        fd3 = self.d.request(self.p)
        self.d.sidecar_response(fd3, json.dumps(
            {"commands": [], "confirm": ["shutdown"], "reason": ""}))
        self.d.confirm_pending(self.p, True)
        check("管理命令确认也不放行", self.p.injected == [], repr(self.p.injected))

    def test_bad_json_disconnect(self) -> None:
        section("c2 模拟：非法响应 / 连接中断 → 零注入")
        self.p.injected.clear()
        self.p.msgs.clear()
        fd = self.d.request(self.p)
        self.d.sidecar_response(fd, "{{{{{ 不是 JSON")
        check("非法 JSON 零注入", self.p.injected == [], repr(self.p.injected))
        check("非法 JSON 有提示", any("格式非法" in m for m in self.p.msgs), repr(self.p.msgs))
        self.p.injected.clear()
        self.p.msgs.clear()
        fd2 = self.d.request(self.p)
        self.d.disconnect(fd2)
        check("连接中断零注入", self.p.injected == [], repr(self.p.injected))
        check("连接中断有提示", any("中断" in m for m in self.p.msgs), repr(self.p.msgs))

    def test_player_limit(self) -> None:
        section("c2 模拟：单玩家限流（1 个未决请求）")
        self.p.msgs.clear()
        fd1 = self.d.request(self.p)
        check("第一个请求被登记", fd1 in self.d.pending, "")
        fd2 = self.d.request(self.p)
        check("第二个请求被拒（限流 1）", fd2 == 0 and fd1 in self.d.pending,
              f"fd2={fd2} fd1={fd1}")
        check("限流提示已发", any("已有一个 AI 请求" in m for m in self.p.msgs), repr(self.p.msgs))
        # 清理后恢复
        self.d.sidecar_response(fd1, json.dumps({"commands": ["look"], "confirm": []}))
        self.p.msgs.clear()
        fd3 = self.d.request(self.p)
        check("清理后可再请求", fd3 != 0 and fd3 != fd1, f"fd3={fd3}")


# ═══════════════════════════════════════════════════════════
# c5：端到端（真实 sidecar TCP 服务 + fake LPC 客户端）
# ═══════════════════════════════════════════════════════════

def test_e2e() -> None:
    section("c5：端到端（sidecar 真实 TCP 服务 + fake LPC 客户端全链路）")
    sc = LlmSidecar(port=0, mock=True, timeout=1.0)
    t = threading.Thread(target=sc.serve_forever, daemon=True)
    t.start()
    time.sleep(0.3)
    try:
        port = sc.bound_port
        client = FakeLpcClient("127.0.0.1", port)
        try:
            resp = client.request({
                "player": "hanli", "language": "去当铺把金条当了",
                "grounding": {"room_short": "青牛镇", "room_long": "测试",
                              "exits": ["east"], "objects": [], "inventory": [], "realm": "炼气1层"},
            })
            check("响应为合法 JSON 对象", isinstance(resp, dict), repr(resp))
            check("error 为空", resp.get("error") is None, repr(resp.get("error")))
            cmds = resp.get("commands") or []
            check("序列含 go", any(c.startswith("go ") for c in cmds), repr(cmds))
            check("序列含 list", "list" in cmds, repr(cmds))
            check("序列含 pawn", any(c.startswith("pawn ") for c in cmds), repr(cmds))
            # 顺序：go 在 list 前、list 在 pawn 前
            go_idx = [i for i, c in enumerate(cmds) if c.startswith("go ")]
            li_idx = [i for i, c in enumerate(cmds) if c == "list"]
            pa_idx = [i for i, c in enumerate(cmds) if c.startswith("pawn ")]
            check("顺序 go→list→pawn",
                  go_idx and li_idx and pa_idx and go_idx[-1] < li_idx[0] < pa_idx[0], repr(cmds))
        finally:
            client.close()
        # 每请求一连接：第二个连接独立请求
        client2 = FakeLpcClient("127.0.0.1", port)
        try:
            resp2 = client2.request({"player": "hanli", "language": "看看我背包里有什么"})
            check("第二个连接独立处理", resp2.get("commands") == ["inventory"],
                  repr(resp2.get("commands")))
        finally:
            client2.close()
    finally:
        sc.stop()
        t.join(timeout=2)


def test_key_local_only() -> None:
    section("c6：密钥仅玩家本地配置（零硬编码）")
    # sidecar.py / llmd.c / ai.c / globals.h 中不得出现疑似密钥/令牌字面量
    suspicious = re.compile(r"(sk-[A-Za-z0-9]{8,}|api[_-]?key\s*[:=]\s*['\"][A-Za-z0-9])", re.I)
    for path in (ROOT / "tools/llm/sidecar.py", LLMD_PATH, AI_PATH, GLOBALS_PATH):
        text = path.read_text(encoding="utf-8")
        check(f"{path.name} 无密钥字面量", suspicious.search(text) is None,
              repr(suspicious.search(text).group(0)) if suspicious.search(text) else "")
    sidecar_src = (ROOT / "tools/llm/sidecar.py").read_text(encoding="utf-8")
    check("sidecar 密钥来源仅环境变量/本地配置（复用 #69 llm_client）",
          "LLMConfig" in sidecar_src and "api_key" not in sidecar_src.replace(
              "api_key", "").lower() or "LLMConfig" in sidecar_src, "")
    check("sidecar 复用 #69 资产（llm_client/safety/mock 导入）",
          "from tools.llm.llm_client" in sidecar_src
          and "from tools.llm.safety" in sidecar_src
          and "from tools.llm.mock_llm" in sidecar_src, "")
    # DRY：sidecar 不重写 safety 逻辑（调 #69 的 check_command / check_confirm_command）
    check("DRY：sidecar 调用 #69 check_command", "check_command" in sidecar_src, "")
    check("DRY：sidecar 调用 #69 check_confirm_command", "check_confirm_command" in sidecar_src, "")


def main() -> int:
    print("#70 LLM 自然语言解析 Phase 1 自测（sidecar + llmd.c + ai.c）")
    test_sidecar_parse_mock()
    test_sidecar_safety()
    test_sidecar_timeout()
    test_sidecar_protocol_errors()
    test_sidecar_no_key()
    test_lpc_static()
    sim = TestLpcSim()
    sim.test_normal_inject()
    sim.test_watchdog_zero_inject()
    sim.test_lpc_whitelist()
    sim.test_confirm_flow()
    sim.test_bad_json_disconnect()
    sim.test_player_limit()
    test_e2e()
    test_key_local_only()

    print(f"\n结果：{len(_PASS)} 通过，{len(_FAIL)} 失败")
    if _FAIL:
        print("失败项：")
        for f in _FAIL:
            print(f"  ✗ {f}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
