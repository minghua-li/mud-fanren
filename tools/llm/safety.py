from __future__ import annotations

"""指令安全过滤：动词白名单 / 危险指令拦截 / 条数上限 / 格式校验。

对齐 #68 风险表的口径：
- 动词白名单校验：非白名单动词的指令绝不注入
- 高危命令二次确认后才注入（本 PoC 中 confirm 指令默认拒绝，玩家可显式放行）
- wiz/admin 路径绝对排除
- 单次最多 N 条指令（防多命令连续执行失控）
"""

# ── 白名单分层 ──────────────────────────────────────────────

# 安全动词：可直接执行。均为 cmds/std|usr 下常见且无破坏性的指令。
# 判定标准：不改变他人/世界状态、不产生伤害、不丢弃/转移玩家财产、
# 不是管理/调试命令。词条一律小写，命中时按词条前缀匹配（e.g. "look" 命中 "look xxx"）。
SAFE_VERBS = {
    # 移动与观察
    "go", "look", "l", "inventory", "i", "score", "hp", "check",
    # 当铺/商店交易（只读或等价交换，PoC 阶段视为安全；pawn 是当铺主流程）
    "list", "value", "pawn", "dang", "buy", "sell",
    # 物品管理（非破坏性）
    "get", "put", "open", "close", "wear", "wield", "unwield", "remove",
    # 社交（只影响自己输出）
    "say", "tell", "reply", "smile", "nod", "ask", "watch", "listen",
    # 基本动作
    "sit", "sleep", "eat", "drink",
}

# 危险动词：出现在 LLM 输出中时，放进 confirm 列表（默认不自动执行）。
# 玩家可在交互中显式确认（本 PoC 提供 --allow-confirm 开关后才放行）。
DANGEROUS_VERBS = {
    # 战斗/伤害
    "kill", "hit", "fight", "steal", "attack",
    # 财产转移/破坏
    "give", "drop", "discard",
    # 会话控制（退出登录/切角色）
    "quit", "exit", "relogin", "suicide",
    # 洗点/降级等不可逆
    "reset", "destruct",
}

# 绝对禁止动词：管理/调试/wiz/arch 命令，出现即整批拒绝，连 confirm 都不给。
# 防御纵深：即使模型输出这些，也绝不可能被注入连接。
DENIED_VERBS = {
    # 驱动/管理
    "shutdown", "exec", "call", "debug", "update", "reload", "clone",
    # 文件系统（wiz 侧）
    "cd", "pwd", "ls", "rm", "mv", "cp", "tail", "cat", "ed", "more",
    "mkdir", "rmdir", "chmod", "patch", "diff", "grep",
    # 提权/审计绕过
    "snoop", "chinese", "adm", "arch", "wiz", "immortal",
    # 直接操作驱动内部
    "eval", "efun", "dump", "profile", "mem", "netstat", "ps",
}

# 指令形态校验：只允许 小写字母开头 + 小写字母/数字/下划线/连字符 + 可选空格参数（同字符集）


class SafetyError(Exception):
    """指令未通过安全校验（含原因，供降级提示）。"""


def check_command(cmd: str) -> str:
    """单条指令校验，返回动词；不合法抛 SafetyError。

    - 空/超长/含特殊字符（; | & $ ` 引号 括号等）→ 拒绝
    - 动词不在白名单 → 按分层处理：DENIED 直接拒绝整批；DANGEROUS 进 confirm
    """
    cmd = cmd.strip()
    if not cmd:
        raise SafetyError("空指令")
    if len(cmd) > 80:
        raise SafetyError("指令过长")
    if any(c in cmd for c in (';', '|', '&', '$', '`', '\\', '"', "'", '(', ')', '<', '>', '\n', '\r')):
        raise SafetyError(f"指令含非法字符: {cmd[:40]}")
    parts = cmd.split()
    verb = parts[0]
    if not all(len(p) > 0 and all(c.isalnum() or c in "_-" for c in p) for p in parts):
        raise SafetyError(f"指令含非法字符: {cmd[:40]}")
    if verb in DENIED_VERBS:
        raise SafetyError(f"危险指令被拒绝: {verb}")
    if verb in SAFE_VERBS:
        return verb
    if verb in DANGEROUS_VERBS:
        raise SafetyError(f"高危指令需确认(默认拒绝): {verb}")
    raise SafetyError(f"非白名单动词被拒绝: {verb}")


def split_commands(payload: dict) -> tuple[list[str], list[str]]:
    """从 LLM 返回的 JSON 中提取并分类指令。

    返回 (可执行列表, 被拦截列表)。任何一条触线都不会被注入连接——
    拦截信息（动词 + 原因）进拦截列表，由调用方展示给玩家。
    """
    raw_commands = payload.get("commands") or []
    if not isinstance(raw_commands, list):
        raise SafetyError("LLM 输出缺少 commands 数组")
    if len(raw_commands) > 8:
        raise SafetyError(f"指令条数超限: {len(raw_commands)} > 8")

    allowed: list[str] = []
    blocked: list[str] = []
    for cmd in raw_commands:
        if not isinstance(cmd, str):
            blocked.append(f"{cmd!r} 非字符串")
            continue
        try:
            check_command(cmd)
            allowed.append(cmd)
        except SafetyError as e:
            blocked.append(f"{cmd} —— {e}")
    # confirm 列表（模型自报的危险意图）：PoC 默认不执行，全部进拦截提示
    raw_confirm = payload.get("confirm") or []
    if isinstance(raw_confirm, list):
        for cmd in raw_confirm:
            if isinstance(cmd, str) and cmd.strip():
                blocked.append(f"{cmd} —— 高危意图，已拦截(confirm)")
    return allowed, blocked
