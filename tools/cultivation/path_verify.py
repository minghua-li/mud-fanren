#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
#61 修炼系统关键路径自测（LPC 逻辑的 Python 忠实翻译）

环境无 fluffos driver（AGENTS.md 已知限制），无法真跑 LPC 运行时验证。
本脚本把 #61 交付的关键路径 LPC 逻辑逐函数翻译为 Python，端到端可复现地验证：

    新玩家创建 → 打坐修炼（升层）→ 境界突破（含失败惩罚/保底）→ sect promote 卡门槛

每段翻译在函数 docstring 标注对应 LPC 源文件与行号，可对照审查。
运行：python3 tools/cultivation/path_verify.py   （全部断言通过输出 PASS，exit 0；任一失败 exit 1）

注意：这是逻辑翻译模拟，非 LPC 本体执行；真实运行时验证需人工安装 fluffos v2019 后补。
"""

import sys


# ======================================================================
# 常量（对齐 include/spirit_root.h 与 adm/daemons/root_refine_d.c）
# ======================================================================
SPIRIT_ROOT_NONE = 0
SPIRIT_ROOT_PSEUDO = 1
SPIRIT_ROOT_FAKE = 2
SPIRIT_ROOT_TRUE = 3
SPIRIT_ROOT_VARIANT = 4
SPIRIT_ROOT_HEAVENLY = 5

REALM_QI_REFINERY = 1       # 炼气期（spirit_root.h:178）
REALM_FOUNDATION = 2        # 筑基期（spirit_root.h:179）
REALM_CORE_FORMATION = 3    # 结丹期（spirit_root.h:180）
REALM_NASCENT_SOUL = 4      # 元婴期（spirit_root.h:181）

PSEUDO_MAX_BREAK_RATE = 50  # 伪灵根结丹率上限（spirit_root.h:229）

# root_refine_d.c:1720-1733
REALM_STAGE_NAMES = ["凡人", "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘"]
REALM_SUB_STAGE_NAMES = ["初期", "中期", "后期", "巅峰", "大圆满"]
REALM_QI_MAX_LAYER = 13
XIUWEI_QI_TO_ZHU = 10000
XIUWEI_ZHU_TO_JIE = 30000
XIUWEI_JIE_TO_YING = 100000
XIUWEI_YING_TO_HUA = 500000

# root_refine_d.c:1736-1740
XIUWEI_BASE_QI = 10
XIUWEI_BASE_ZHU = 30
XIUWEI_BASE_JIE = 100
XIUWEI_BASE_YING = 400
XIUWEI_BASE_HUA = 1500

# root_refine_d.c:1744-1754
BREAK_CD_QI_TO_ZHU = 86400
BREAK_CD_ZHU_TO_JIE = 86400
BREAK_CD_JIE_TO_YING = 259200
BREAK_CD_YING_TO_HUA = 259200
MAJOR_BREAK_STREAK_THRESHOLD = 3
MAJOR_BREAK_STREAK_BONUS = 15
BREAK_FAIL_XIUWEI_KEEP = 500   # 千分比：失败保留 50% 突破门槛

# sect_d.c:17（sect 侧境界名，炼气=0；与 root_refine 的 index 差 1，经 realm 字符串衔接）
SECT_REALM_NAMES = ["炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘"]


# ======================================================================
# 玩家对象（模拟 LPC 对象 dbase 属性）
# ======================================================================
class Player:
    def __init__(self, is_user=True, combat_exp=0, quality=SPIRIT_ROOT_PSEUDO,
                 purity=30, realm=None, realm_index=None, realm_sub=None, xiuwei=None):
        self.is_user = is_user
        self.combat_exp = combat_exp
        self.dbase = {}
        self.temp = {}
        # 伪灵根初始 purity=30（attribute.c:1145 ROOT_QUALITY_T4），可覆盖
        self.spirit_root = {"quality": quality, "purity": purity,
                            "strength": 0, "fail_streak": 0, "debuff": 0}
        if realm is not None:
            self.dbase["realm"] = realm
        if realm_index is not None:
            self.dbase["realm_index"] = realm_index
        if realm_sub is not None:
            self.dbase["realm_sub"] = realm_sub
        if xiuwei is not None:
            self.dbase["xiuwei"] = xiuwei

    # F_DBASE 接口
    def query(self, key):
        if key == "realm":
            return self.dbase.get("realm", 0)
        if key == "realm_sub":
            return self.dbase.get("realm_sub", 0)
        if key == "realm_index":
            return self.dbase.get("realm_index", 0)
        if key == "xiuwei":
            return self.dbase.get("xiuwei", 0)
        if key == "combat_exp":
            return self.combat_exp
        if key == "spirit_root/resonance":
            return 0
        return self.dbase.get(key, 0)

    def set(self, key, val):
        self.dbase[key] = val

    def add(self, key, n):
        if key.startswith("spirit_root/"):
            sub = key.split("/")[1]
            self.spirit_root[sub] = self.spirit_root.get(sub, 0) + n
        else:
            self.dbase[key] = self.query(key) + n


def _to_int(f):
    """LPC to_int()：截断取整"""
    return int(f)


# ======================================================================
# human.c setup_human 新段翻译（human.c:80-87）—— c1
# ======================================================================
def setup_human_realm(ob):
    """新玩家初始境界=炼气1层（userp 且无 realm）"""
    if ob.is_user and ob.query("realm") == 0:
        ob.set("realm", "炼气1层")
        ob.set("realm_sub", "初期")
        ob.set("realm_index", 1)
        if ob.query("xiuwei") == 0:
            ob.set("xiuwei", 0)


# ======================================================================
# root_refine_d.c realm 读写翻译（1756-2057）—— c2/c3
# ======================================================================
def realm_name(index, sub):
    """root_refine_d.c:1840-1858 境界索引+子阶段 → 中文境界字符串"""
    if index <= 0:
        return "凡人"
    if index >= len(REALM_STAGE_NAMES):
        index = len(REALM_STAGE_NAMES) - 1
    if index == 1:  # 炼气：sub 为层数，层数用 ASCII 数字（sect_d.extract_layer 依赖）
        if sub < 1:
            sub = 1
        if sub > REALM_QI_MAX_LAYER:
            sub = REALM_QI_MAX_LAYER
        return "炼气" + str(sub) + "层"
    if sub < 0:
        sub = 0
    if sub > 2:
        sub = 2
    return REALM_STAGE_NAMES[index] + REALM_SUB_STAGE_NAMES[sub]


def qi_layer_sub_name(layer):
    """root_refine_d.c:1861-1868 炼气层数 → 子阶段名"""
    if layer <= 3:
        return "初期"
    if layer <= 6:
        return "中期"
    if layer <= 9:
        return "后期"
    if layer <= 12:
        return "巅峰"
    return "大圆满"


def query_player_realm_index(ob):
    """root_refine_d.c:1759-1768"""
    idx = ob.query("realm_index")
    if idx > 0:
        return idx
    return query_realm_index_from_string(ob.query("realm"))


def query_realm_index_from_string(realm):
    """root_refine_d.c:1771-1780"""
    if realm == 0 or realm == "":
        return 0
    for i in range(1, len(REALM_STAGE_NAMES)):
        if REALM_STAGE_NAMES[i] in realm:
            return i
    return 0


def extract_ascii_layer(realm):
    """提取 realm 字符串中 ASCII 数字层数（root_refine_d.c:1783-1814 同 sect_d.extract_layer）"""
    if realm == 0 or realm == "":
        return 0
    digits = ""
    for ch in realm:
        if ch.isdigit():
            digits += ch
            break
    if not digits:
        return 1
    num = ""
    for ch in realm[realm.index(digits[0]):]:
        if ch.isdigit():
            num += ch
        else:
            break
    return int(num) if num else 1


def query_player_realm_layer(ob):
    """root_refine_d.c:1783-1814"""
    if query_player_realm_index(ob) != 1:
        return 0
    return extract_ascii_layer(ob.query("realm"))


def set_player_realm(ob, index, sub):
    """root_refine_d.c:1879-1889 —— c2 统一存储（realm/realm_index/realm_sub）"""
    ob.set("realm", realm_name(index, sub))
    ob.set("realm_index", index)
    if index == 1:
        ob.set("realm_sub", qi_layer_sub_name(sub))
    else:
        ob.set("realm_sub", REALM_SUB_STAGE_NAMES[sub])


def query_xiuwei(ob):
    """root_refine_d.c:1894-1902"""
    x = ob.query("xiuwei")
    return x if isinstance(x, int) else 0


def add_xiuwei(ob, n):
    """root_refine_d.c:1905-1914"""
    if n == 0:
        return query_xiuwei(ob)
    cur = query_xiuwei(ob) + n
    if cur < 0:
        cur = 0
    ob.set("xiuwei", cur)
    return cur


def spend_xiuwei(ob, n):
    """root_refine_d.c:1917-1923"""
    if n <= 0:
        return 0
    if query_xiuwei(ob) < n:
        return 0
    add_xiuwei(ob, -n)
    return 1


def query_layer_xiuwei_need(layer):
    """root_refine_d.c:1926-1930 炼气第 N 层 → N+1 层所需修为 = layer*100"""
    if layer < 1:
        layer = 1
    return layer * 100


def query_next_layer_need(ob):
    """root_refine_d.c:1933-1942"""
    if query_player_realm_index(ob) != 1:
        return 0
    layer = query_player_realm_layer(ob)
    if layer >= REALM_QI_MAX_LAYER:
        return 0
    return query_layer_xiuwei_need(layer)


def query_player_sub_stage(ob):
    """root_refine_d.c:1988-2005"""
    if query_player_realm_index(ob) == 1:
        layer = query_player_realm_layer(ob)
        if layer <= 3:
            return 0
        if layer <= 6:
            return 1
        return 2
    sub = ob.query("realm_sub")
    if sub == 0 or sub == "":
        return 0
    if "中期" in sub:
        return 1
    if "后期" in sub:
        return 2
    return 0


def query_major_break_need(ob):
    """root_refine_d.c:1961-1985 大境界突破修为门槛"""
    idx = query_player_realm_index(ob)
    if idx == 1:
        if query_player_realm_layer(ob) >= REALM_QI_MAX_LAYER:
            return XIUWEI_QI_TO_ZHU
        return 0
    if idx == 2:
        return XIUWEI_ZHU_TO_JIE if query_player_sub_stage(ob) >= 2 else 0
    if idx == 3:
        return XIUWEI_JIE_TO_YING if query_player_sub_stage(ob) >= 2 else 0
    if idx == 4:
        return XIUWEI_YING_TO_HUA if query_player_sub_stage(ob) >= 2 else 0
    return 0


def query_cultivation_base(ob):
    """root_refine_d.c:2010-2025 当前境界打坐基础修为"""
    idx = query_player_realm_index(ob)
    if idx == 0:
        return 0
    if idx == 1:
        return XIUWEI_BASE_QI
    if idx == 2:
        return XIUWEI_BASE_ZHU
    if idx == 3:
        return XIUWEI_BASE_JIE
    if idx == 4:
        return XIUWEI_BASE_YING
    return XIUWEI_BASE_HUA


def query_cultivation_speed_factor(ob):
    """root_refine_d.c:1530-1556（简化为无 debuff/无共鸣场景）"""
    quality = ob.spirit_root["quality"]
    if quality <= SPIRIT_ROOT_NONE:
        return 0.0
    if quality == SPIRIT_ROOT_PSEUDO:
        base_speed = 0.3
    elif quality == SPIRIT_ROOT_FAKE:
        base_speed = 0.6
    elif quality == SPIRIT_ROOT_TRUE:
        base_speed = 1.0
    elif quality == SPIRIT_ROOT_VARIANT:
        base_speed = 2.3
    elif quality == SPIRIT_ROOT_HEAVENLY:
        base_speed = 2.5
    else:
        base_speed = 0.0
    purity = ob.spirit_root.get("purity", 50)
    purity_factor = 1.0 - (1.0 - purity / 100.0) * 0.3
    if purity_factor < 0.7:
        purity_factor = 0.7
    return base_speed * purity_factor


def query_heartbeat_cultivation_gain(ob):
    """root_refine_d.c:2028-2039 单次心跳修为 = 境界基准 × 灵根速度系数"""
    base = query_cultivation_base(ob)
    if base <= 0:
        return 0
    factor = query_cultivation_speed_factor(ob)
    if factor <= 0.0:
        return 0
    return _to_int(base * factor)


def check_qi_layer_up(ob):
    """root_refine_d.c:2043-2064 炼气层数自动提升"""
    if query_player_realm_index(ob) != 1:
        return 0
    layer = query_player_realm_layer(ob)
    if layer >= REALM_QI_MAX_LAYER:
        return 0
    need = query_layer_xiuwei_need(layer)
    if query_xiuwei(ob) < need:
        return 0
    new_layer = layer + 1
    spend_xiuwei(ob, need)
    set_player_realm(ob, 1, new_layer)
    return 1


def do_heartbeat_cultivation(ob):
    """root_refine_d.c:2067-2086 一次心跳修炼，返回实际获得修为"""
    if not ob.is_user:
        return 0
    gain = query_heartbeat_cultivation_gain(ob)
    if gain <= 0:
        return 0
    add_xiuwei(ob, gain)
    # gain_exp_from_cultivation（root_refine_d.c:267-283）：灵根经验成长，与修为路径无关，跳过
    check_qi_layer_up(ob)
    return gain


def query_break_cd(ob):
    """root_refine_d.c:2091-2104 大境界突破失败冷却（秒）"""
    idx = query_player_realm_index(ob)
    if idx == 1:
        return BREAK_CD_QI_TO_ZHU
    if idx == 2:
        return BREAK_CD_ZHU_TO_JIE
    if idx == 3:
        return BREAK_CD_JIE_TO_YING
    return BREAK_CD_YING_TO_HUA


def query_break_cooldown_remaining(ob, now):
    """root_refine_d.c:2107-2117"""
    last = ob.spirit_root.get("last_break", 0)
    if not last:
        return 0
    remain = last + query_break_cd(ob) - now
    if remain < 0:
        return 0
    return remain


def query_major_breakthrough_probability(ob, realm, aux_bonus=0):
    """root_refine_d.c:930-989（截取关键分支；未含 990 之后的质量/共鸣修正段——新玩家无）"""
    quality = ob.spirit_root["quality"]
    if quality == SPIRIT_ROOT_HEAVENLY and realm == REALM_CORE_FORMATION:
        return 100
    if realm == REALM_QI_REFINERY:
        base_realm_rate = 90
    elif realm == REALM_FOUNDATION:
        base_realm_rate = 10
    elif realm == REALM_CORE_FORMATION:
        base_realm_rate = 5
    elif realm == REALM_NASCENT_SOUL:
        base_realm_rate = 3
    else:
        base_realm_rate = 1
    if quality == SPIRIT_ROOT_PSEUDO:
        quality_factor = 0.3
    elif quality == SPIRIT_ROOT_FAKE:
        quality_factor = 0.7
    elif quality == SPIRIT_ROOT_TRUE:
        quality_factor = 1.0
    elif quality == SPIRIT_ROOT_VARIANT:
        quality_factor = 1.2
    elif quality == SPIRIT_ROOT_HEAVENLY:
        quality_factor = 5.0
    else:
        quality_factor = 0.0
    realm_penalty = 0
    if quality == SPIRIT_ROOT_PSEUDO:
        realm_penalty = -15 * (realm - REALM_FOUNDATION)
    elif quality == SPIRIT_ROOT_FAKE:
        realm_penalty = -5 * (realm - REALM_FOUNDATION)
    if realm_penalty < -50:
        realm_penalty = -50
    prob = _to_int(base_realm_rate * quality_factor) + realm_penalty
    prob += aux_bonus
    if quality == SPIRIT_ROOT_PSEUDO and realm == REALM_CORE_FORMATION:
        if prob > PSEUDO_MAX_BREAK_RATE:
            prob = PSEUDO_MAX_BREAK_RATE
    # 钳制对齐 LPC：root_refine_d.c:990-991 `if (prob < 1) prob = 1; if (prob > 99) prob = 99;`
    if prob < 1:
        prob = 1
    if prob > 99:
        prob = 99
    return prob


def major_breakthrough_success(ob, now):
    """root_refine_d.c:2120-2153 —— c2 突破成功真实写入境界"""
    idx = query_player_realm_index(ob)
    new_idx = idx + 1
    set_player_realm(ob, new_idx, 0)
    ob.spirit_root["last_break"] = 0
    ob.spirit_root["strength"] = ob.spirit_root.get("strength", 0) + 10
    ob.spirit_root["purity"] = min(ob.spirit_root.get("purity", 50) + 3, 100)
    ob.spirit_root["fail_streak"] = 0
    return 1


def major_breakthrough_failure(ob, now):
    """root_refine_d.c:2156-2192 失败：修为回退 50% + 冷却 + 连续失败计数"""
    need = query_major_break_need(ob)
    keep = int(need * BREAK_FAIL_XIUWEI_KEEP / 1000.0)
    penalty = 0
    if keep > 0:
        penalty = need - keep
        add_xiuwei(ob, -penalty)
    ob.spirit_root["last_break"] = now
    ob.spirit_root["fail_streak"] = ob.spirit_root.get("fail_streak", 0) + 1
    # 对齐 LPC root_refine_d.c:2173-2179：连续失败 ≥2 触发灵根震荡 debuff（简化：只标记生效）
    streak = ob.spirit_root["fail_streak"]
    if streak >= 2 and not ob.spirit_root.get("debuff"):
        ob.spirit_root["debuff"] = 1
    return 1


def do_major_breakthrough(ob, method, now, rng_roll=None):
    """root_refine_d.c:2196-2257 执行大境界突破（1=成功 2=失败 0=条件不满足）
    rng_roll：注入 random(100) 结果（None 表示由场景内定）"""
    idx = query_player_realm_index(ob)
    if idx <= 0 or idx >= len(REALM_STAGE_NAMES) - 1:
        return 0
    need = query_major_break_need(ob)
    if need <= 0:
        return 0
    if query_xiuwei(ob) < need:
        return 0
    cd = query_break_cooldown_remaining(ob, now)
    if cd > 0:
        return 0
    target = idx + 1
    aux = ob.temp.get("aux_bonus", 0)
    prob = query_major_breakthrough_probability(ob, target, aux)
    streak = ob.spirit_root.get("fail_streak", 0)
    if streak >= MAJOR_BREAK_STREAK_THRESHOLD:
        prob += MAJOR_BREAK_STREAK_BONUS
        if prob > 99:
            prob = 99
    if rng_roll is None:
        rng_roll = 0  # 默认成功路径（场景内显式控制）
    if rng_roll < prob:
        spend_xiuwei(ob, need)
        # LPC: `spend_xiuwei(ob, need); return major_breakthrough_success(ob, method) ? 1 : 0;`（2250-2251）
        return 1 if major_breakthrough_success(ob, now) else 0
    # LPC: `return major_breakthrough_failure(ob, method) ? 2 : 0;`（2255）
    return 2 if major_breakthrough_failure(ob, now) else 0


# ======================================================================
# sect_d.c 门槛判定翻译（sect_d.c:297-344 / 540-592）—— c6
# ======================================================================
def parse_realm(realm):
    """sect_d.c:297-313 解析境界字符串 → (境界索引, 层数)（sect 侧炼气=0）"""
    if realm == 0 or realm == "":
        return (0, 0)
    index = 0
    for i, name in enumerate(SECT_REALM_NAMES):
        if name in realm:
            index = i
            break
    layer = extract_ascii_layer(realm)
    return (index, layer)


def query_cultivation_tier(player):
    """sect_d.c:317-344 玩家境界 tier = 境界索引*3 + 小阶段（无 realm 时 exp_to_tier 兜底）"""
    realm = player.query("realm")
    if realm == 0 or realm == "":
        return exp_to_tier(player.combat_exp)
    index, layer = parse_realm(realm)
    stage = 1  # 默认中期
    if "初期" in realm:
        stage = 0
    elif "后期" in realm:
        stage = 2
    elif layer > 0:
        if layer <= 3:
            stage = 0
        elif layer <= 6:
            stage = 1
        else:
            stage = 2
    return index * 3 + stage


def exp_to_tier(exp):
    """sect_d.c exp_to_tier（兜底；sect_d.c:253-265 区间）"""
    if exp < 100000:
        return 0
    if exp < 1000000:
        return 3
    if exp < 10000000:
        return 6
    if exp < 50000000:
        return 9
    if exp < 200000000:
        return 12
    return 15


def sect_promote_tier_gate(player, require_tier):
    """sect_d.c promote:571 `query_cultivation_tier(player) < tier` 卡门槛"""
    return query_cultivation_tier(player) >= require_tier


# ======================================================================
# 断言辅助
# ======================================================================
_failures = []


def check(name, cond, detail=""):
    if cond:
        print("  PASS  %s" % name)
    else:
        print("  FAIL  %s  %s" % (name, detail))
        _failures.append(name)


# ======================================================================
# 场景 1：c1 新玩家初始境界
# ======================================================================
def scenario_c1_new_player():
    print("[场景1] c1 新玩家初始境界=炼气1层（human.c:80-87）")
    p = Player(is_user=True)
    setup_human_realm(p)
    check("新玩家 realm == 炼气1层", p.query("realm") == "炼气1层", p.query("realm"))
    check("realm_sub == 初期", p.query("realm_sub") == "初期", p.query("realm_sub"))
    check("realm_index == 1", p.query("realm_index") == 1, p.query("realm_index"))
    check("xiuwei == 0", p.query("xiuwei") == 0, p.query("xiuwei"))
    # 已带 realm 的玩家不被覆盖
    q = Player(is_user=True, realm="筑基初期", realm_index=2)
    setup_human_realm(q)
    check("存量有 realm 玩家不被覆盖", q.query("realm") == "筑基初期", q.query("realm"))


# ======================================================================
# 场景 2：c3 打坐修炼 → 炼气升层 → 13 层大圆满
# ======================================================================
def scenario_c3_dazuo():
    print("[场景2] c3 打坐修炼（伪灵根 speed=0.3，base=10，purity=30 → 每心跳 +2 修为）")
    p = Player(is_user=True)
    setup_human_realm(p)
    # 伪灵根 purity=30 → purity_factor=0.79（root_refine_d.c:1545-1552）→ int(10*0.3*0.79)=2
    gain = query_heartbeat_cultivation_gain(p)
    check("单心跳修为 = 2", gain == 2, gain)

    # 打坐到炼气13层大圆满（升层累计消耗 100+200+…+1200=7800，还需攒满 10000 突破门槛）
    ticks = 0
    prev_layer = 1
    total_spent = 0
    while query_major_break_need(p) == 0 or query_xiuwei(p) < XIUWEI_QI_TO_ZHU:
        if ticks > 200000:
            check("打坐心跳未失控", False, "ticks=%d" % ticks)
            return
        do_heartbeat_cultivation(p)
        ticks += 1
        layer = query_player_realm_layer(p)
        if layer > prev_layer:  # 升层：累计该层消耗（check_qi_layer_up spend）
            total_spent += query_layer_xiuwei_need(prev_layer)
            prev_layer = layer
    check("达到炼气13层大圆满", query_player_realm_layer(p) == 13, query_player_realm_layer(p))
    check("realm == 炼气13层", p.query("realm") == "炼气13层", p.query("realm"))
    check("修为达到突破门槛", query_xiuwei(p) >= XIUWEI_QI_TO_ZHU, query_xiuwei(p))
    # 升层消耗真实核算：1→2 需100 … 12→13 需1200，累计 7800
    check("升层累计消耗 7800", total_spent == 7800, total_spent)
    print("      （%d 心跳，当前修为 %d，realm=%s）" % (ticks, query_xiuwei(p), p.query("realm")))


# ======================================================================
# 场景 3：c4 突破流程（失败惩罚 + 冷却 + 连续失败保底 + 成功写入）
# ======================================================================
def scenario_c4_tupo():
    print("[场景3] c4 突破流程（伪灵根炼气→筑基自然成功率 3%）")
    p = Player(is_user=True)
    setup_human_realm(p)
    now = 1000000
    while query_major_break_need(p) == 0 or query_xiuwei(p) < XIUWEI_QI_TO_ZHU:
        do_heartbeat_cultivation(p)
    xiuwei_at_full = query_xiuwei(p)
    check("突破前修为 ≥ 10000", xiuwei_at_full >= XIUWEI_QI_TO_ZHU, xiuwei_at_full)

    # 注入随机值模拟失败（roll=99 > prob=3）：验证失败惩罚路径
    result = do_major_breakthrough(p, 1, now, rng_roll=99)
    check("失败返回 2", result == 2, result)
    check("失败后修为回退至门槛 50%（5000）", query_xiuwei(p) == 5000, query_xiuwei(p))
    check("失败后冷却生效", query_break_cooldown_remaining(p, now) == BREAK_CD_QI_TO_ZHU,
          query_break_cooldown_remaining(p, now))
    check("连续失败计数 +1", p.spirit_root["fail_streak"] == 1, p.spirit_root["fail_streak"])
    # 把修为补回门槛，独立验证「修为充足但冷却中」被拦（返回 0）
    while query_xiuwei(p) < XIUWEI_QI_TO_ZHU:
        do_heartbeat_cultivation(p)
    result2 = do_major_breakthrough(p, 1, now, rng_roll=0)
    check("冷却中突破被拦（返回 0）", result2 == 0, result2)

    # 冷却过后：连续失败 2 次 → 保底生效（streak=3 时 +15%）
    now += BREAK_CD_QI_TO_ZHU + 1
    for _ in range(2):
        # 需先攒回修为（失败回退到 5000，需再刷 5000 才能尝试）
        while query_xiuwei(p) < XIUWEI_QI_TO_ZHU:
            do_heartbeat_cultivation(p)
        do_major_breakthrough(p, 1, now, rng_roll=99)
        now += BREAK_CD_QI_TO_ZHU + 1
    check("连续失败 3 次触发保底计数", p.spirit_root["fail_streak"] == 3, p.spirit_root["fail_streak"])

    # 保底后成功（roll=10，prob=3+15=18 → 10<18 成功）
    while query_xiuwei(p) < XIUWEI_QI_TO_ZHU:
        do_heartbeat_cultivation(p)
    result3 = do_major_breakthrough(p, 1, now, rng_roll=10)
    check("保底后突破成功（返回 1）", result3 == 1, result3)
    check("成功写入 realm=筑基初期", p.query("realm") == "筑基初期", p.query("realm"))
    check("realm_index=2", p.query("realm_index") == 2, p.query("realm_index"))
    check("realm_sub=初期", p.query("realm_sub") == "初期", p.query("realm_sub"))
    check("失败计数清零", p.spirit_root["fail_streak"] == 0, p.spirit_root["fail_streak"])


# ======================================================================
# 场景 4：c6 sect promote 按真实境界卡门槛
# ======================================================================
def scenario_c6_sect_promote():
    print("[场景4] c6 sect promote 按真实境界卡门槛（yanyue 内门 tier≥2）")
    # 炼气1层新玩家：tier=0，被拒
    fresh = Player(is_user=True)
    setup_human_realm(fresh)
    t0 = query_cultivation_tier(fresh)
    check("炼气1层 tier=0", t0 == 0, t0)
    check("炼气1层无法晋升内门（tier<2）", not sect_promote_tier_gate(fresh, 2))

    # 炼气13层：tier=2，可通过内门
    full13 = Player(is_user=True, realm="炼气13层", realm_index=1)
    t13 = query_cultivation_tier(full13)
    check("炼气13层 tier=2", t13 == 2, t13)
    check("炼气13层可通过内门（tier≥2）", sect_promote_tier_gate(full13, 2))

    # 突破后筑基初期：tier=3，可通过内门，亦可晋升真传（需 tier≥3 等）
    zhuji = Player(is_user=True, realm="筑基初期", realm_index=2, realm_sub="初期")
    tz = query_cultivation_tier(zhuji)
    check("筑基初期 tier=3", tz == 3, tz)
    check("筑基初期通过内门", sect_promote_tier_gate(zhuji, 2))

    # 存量高 exp 无 realm 玩家走 exp_to_tier 兜底（5M exp → tier 6）
    legacy = Player(is_user=True, combat_exp=5000000)
    check("存量无 realm 玩家兜底 tier=6", query_cultivation_tier(legacy) == 6,
          query_cultivation_tier(legacy))


# ======================================================================
# 场景 5：审查修复验证——tupo lingshi 修为不足不再白扣灵石（tupo.c 前置检查）
# ======================================================================
def tupo_lingshi_precheck(ob, now, stone_cost=5000):
    """tupo.c main 前置检查链（修复后）——返回 (拦截提示 or None, 是否扣灵石)
    对应 tupo.c:39-72 顺序：idx → need → 冷却 → 修为 → 扣灵石 → do_major_breakthrough"""
    idx = query_player_realm_index(ob)
    if idx <= 0:
        return ("尚未踏入修仙之路", False)
    if idx >= 8:
        return ("已至最高境界", False)
    need = query_major_break_need(ob)
    if need <= 0:
        return ("尚未修炼至大圆满", False)
    cd = query_break_cooldown_remaining(ob, now)
    if cd > 0:
        return ("冷却中", False)
    # 审查第 1 轮修复：扣灵石前先查修为
    if query_xiuwei(ob) < need:
        return ("修为不足（不扣灵石）", False)
    # 通过前置检查 → 进入扣灵石分支（模拟 player_pay 成功）
    ob.temp["aux_bonus"] = 10
    return (None, True)


def scenario_5_tupo_lingshi_fix():
    print("[场景5] 审查修复：tupo lingshi 修为不足不白扣灵石（tupo.c:65 前置修为检查）")
    # 炼气13层大圆满但修为只有 5000（<10000）：lingshi 应被前置拦截，不扣灵石
    poor = Player(is_user=True, realm="炼气13层", realm_index=1, xiuwei=5000)
    msg, paid = tupo_lingshi_precheck(poor, now=1000000)
    check("修为不足被拦截", msg == "修为不足（不扣灵石）", msg)
    check("灵石未被扣", not paid)
    # 修为充足（≥10000）：通过前置检查，允许扣灵石尝试
    rich = Player(is_user=True, realm="炼气13层", realm_index=1, xiuwei=10000)
    msg2, paid2 = tupo_lingshi_precheck(rich, now=1000000)
    check("修为充足放行", msg2 is None, msg2)
    check("进入扣灵石分支", paid2)
    # 冷却中：前置拦截，不扣灵石（既有修复）
    cd_player = Player(is_user=True, realm="炼气13层", realm_index=1, xiuwei=10000)
    cd_player.spirit_root["last_break"] = 1000000
    msg3, paid3 = tupo_lingshi_precheck(cd_player, now=1000000)
    check("冷却中被拦截", msg3 == "冷却中", msg3)
    check("冷却中灵石未被扣", not paid3)


def main():
    print("=" * 60)
    print("#61 修炼系统关键路径自测（LPC 逻辑 Python 翻译）")
    print("=" * 60)
    scenario_c1_new_player()
    scenario_c3_dazuo()
    scenario_c4_tupo()
    scenario_c6_sect_promote()
    scenario_5_tupo_lingshi_fix()
    print("=" * 60)
    if _failures:
        print("RESULT: FAIL (%d) %s" % (len(_failures), ",".join(_failures)))
        sys.exit(1)
    print("RESULT: PASS 全部断言通过")
    sys.exit(0)


if __name__ == "__main__":
    main()
