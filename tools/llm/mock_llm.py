from __future__ import annotations

"""Mock 解析器：无 LLM API 时的演示/自测替身。

只用于两条路：
1. 玩家没有配置 API key 时跑通「自然语言 → 指令序列 → 回写」链路演示；
2. selftest 自测时确定性验证（不受外部 LLM 服务可用性影响）。

它不是产品功能：真实解析效果由 LLM 提供（见 llm_client.chat_completion）。
mock 按简单关键词规则启发式产出指令序列，规则刻意保守——
识别不了就返回 look（观察环境），绝不产出危险指令。
"""

import re

from tools.llm.safety import check_command, SafetyError


def parse_mock(user_text: str) -> dict:
    """把自然语言输入解析为 #68 输出契约的 JSON。

    返回 {"commands": [...], "confirm": [], "reason": "..."}
    任何分支都先经过 check_command 自检，保证 mock 产出的指令必然合法。
    """
    text = user_text.strip().lower()

    def safe(cmd: str) -> bool:
        try:
            check_command(cmd)
            return True
        except SafetyError:
            return False

    reason = "mock 规则解析"

    # 当铺典当：go(盲走两步入城区) + list + pawn
    if re.search(r"当铺|典当|当了|pawn|dang", text):
        plan = ["go east", "go north", "list", "pawn jin tiao"]
        # 若文本里出现明确物品词，替换 pawn 宾语（仍限白名单字符）
        m = re.search(r"(金条|黄金|金子|宝剑|宝剑|剑|装备|物品)\s*(?:当|卖了)?", text)
        item = "jin tiao"  # 默认：金条(id 猜测，真实游戏以背包 id 为准)
        if m:
            word = m.group(1)
            if word in ("黄金", "金条", "金子"):
                item = "gold"
            elif word in ("宝剑", "剑"):
                item = "jian"
            elif word == "装备":
                item = "armor"
        plan = [c for c in plan if safe(c)]
        plan[3] = f"pawn {item}" if safe(f"pawn {item}") else "look"
        return {"commands": plan, "confirm": [], "reason": reason + ": 当铺典当"}

    # 买东西
    if re.search(r"买|buy", text):
        plan = ["go east", "go north", "list"]
        return {"commands": [c for c in plan if safe(c)], "confirm": [], "reason": reason + ": 去商店"}

    # 查看背包/状态
    if re.search(r"背包|身上|物品|inventory|baobei|看看我", text):
        return {"commands": ["inventory"], "confirm": [], "reason": reason + ": 查看背包"}

    # 打招呼
    if re.search(r"打招呼|hi|你好", text):
        return {"commands": ["say 你好"], "confirm": [], "reason": reason + ": 打招呼"}

    # 默认：观察环境（安全兜底，绝不猜危险指令）
    return {"commands": ["look"], "confirm": [], "reason": reason + ": 默认观察"}
