#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
#60 门派设施系统 端到端路径验证（LPC 关键路径忠实翻译）
数据驱动：从 adm/daemons/sect_facility_d.c 解析 facility_config / seed_config，
按 LPC 实现逐函数翻译（标注源行号），覆盖：
  入宗 → 灵田种植/收获 → 丹房加成 → 藏经阁阅读/抄录 → 坊市购买
  → 每日修行 → 设施升级 → 每日限额 → 异派/不在房间/贡献不足拦截
用法：python3 tools/facility/path_verify.py  （exit 0 = 全部断言通过）
"""
import os, re, sys, random

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
DAEMON = os.path.join(BASE, "adm/daemons/sect_facility_d.c")
HDR = os.path.join(BASE, "include/sect_facility.h")

# ---------- LPC 常量宏解析 ----------
def parse_define_mapping(text, macro):
    m = re.search(r'#define\s+' + macro + r'\s+\(\[(.*?)\n\]\)', text, re.S)
    if not m: raise SystemExit("宏 %s 解析失败" % macro)
    body = m.group(1)
    out = {}
    for item in re.finditer(r'(\d+)\s*:\s*\(\{\s*(\d+)\s*,\s*(\d+)\s*\}\)', body):
        out[int(item.group(1))] = [int(item.group(2)), int(item.group(3))]
    return out

def parse_define_int(text, macro):
    m = re.search(r'#define\s+' + macro + r'\s+(\d+)', text)
    return int(m.group(1)) if m else None


hdr = open(HDR, encoding="utf-8").read()
USE_STONE_D = parse_define_int(hdr, "SECT_USE_STONE_DEFAULT")
USE_CONTRIB_D = parse_define_int(hdr, "SECT_USE_CONTRIB_DEFAULT")
SPAR_CONTRIB = parse_define_int(hdr, "SECT_SPAR_CONTRIB")

def parse_buff_macros(text):
    out = {}
    for m in re.finditer(r'#define\s+(SECT_BUFF_\w+)\s+"([^"]+)"', text):
        out[m.group(1)] = m.group(2)
    return out
BUFF = parse_buff_macros(hdr)
TYPE = {}
for name, num in [("SECT_FACILITY_PLANT",1),("SECT_FACILITY_ALCHEMY",2),("SECT_FACILITY_LIBRARY",3),
                  ("SECT_FACILITY_DEFENSE",4),("SECT_FACILITY_TRAINING",5),("SECT_FACILITY_SPECIAL",6)]:
    TYPE[name] = num
UPG_COMMON = parse_define_mapping(hdr, "SECT_UPGRADE_COMMON")
UPG_MARKET = parse_define_mapping(hdr, "SECT_UPGRADE_MARKET")
UPG_LIBRARY = parse_define_mapping(hdr, "SECT_UPGRADE_LIBRARY")
UPG_DEFENSE = parse_define_mapping(hdr, "SECT_UPGRADE_DEFENSE")
UPG_SPECIAL = parse_define_mapping(hdr, "SECT_UPGRADE_SPECIAL")
UPG_PLANT = parse_define_mapping(hdr, "SECT_UPGRADE_PLANT")
PLANT_PLOTS_BASE = parse_define_int(hdr, "SECT_PLANT_PLOTS_BASE")
PLANT_PLOTS_PER = parse_define_int(hdr, "SECT_PLANT_PLOTS_PER_LEVEL")
PLANT_MAINT = parse_define_int(hdr, "SECT_PLANT_MAINTENANCE")
SPAR_EXP_BASE = parse_define_int(hdr, "SECT_SPAR_EXP_BASE")
SPAR_EXP_PER = parse_define_int(hdr, "SECT_SPAR_EXP_PER_LEVEL")
GROWTH = {"GROWTH_BASE_SHORT": 3600, "GROWTH_BASE_MEDIUM": 14400, "GROWTH_BASE_LONG": 86400}

# ---------- facility_config 解析 ----------
daemon = open(DAEMON, encoding="utf-8").read()

def parse_entry(entry_text, key):
    """解析单个设施条目（简化的 LPC 字面量解析）"""
    def g(field):
        m = re.search(r'"%s"\s*:\s*"([^"]*)"' % field, entry_text)
        return m.group(1) if m else None
    def gi(field):
        m = re.search(r'"%s"\s*:\s*(-?\d+)' % field, entry_text)
        if m: return int(m.group(1))
        m2 = re.search(r'"%s"\s*:\s*([A-Za-z_]+)' % field, entry_text)
        if m2:
            nm = {"SECT_USE_STONE_DEFAULT": USE_STONE_D, "SECT_USE_CONTRIB_DEFAULT": USE_CONTRIB_D}.get(m2.group(1))
            if nm is not None: return nm
        return None
    e = {}
    e["sect"] = g("sect"); e["name"] = g("name"); e["room"] = g("room")
    tm = re.search(r'"type"\s*:\s*(SECT_FACILITY_\w+)', entry_text)
    e["type"] = TYPE[tm.group(1)] if tm else None
    e["level"] = gi("level"); e["max_level"] = gi("max_level")
    e["duration"] = gi("duration") or 0
    e["use_stone"] = gi("use_stone") or 0
    e["use_contrib"] = gi("use_contrib") or 0
    e["daily_limit"] = gi("daily_limit") or 0
    e["market"] = gi("market") or 0
    em = re.search(r'"effect"\s*:\s*\(\[\s*"type"\s*:\s*(?:"([^"]+)"|([A-Za-z_]+))\s*,\s*"base"\s*:\s*(\d+)\s*,\s*"per_level"\s*:\s*(\d+)', entry_text)
    if em:
        etype = em.group(1) if em.group(1) else BUFF.get(em.group(2), em.group(2))
        e["effect"] = {"type": etype, "base": int(em.group(3)), "per_level": int(em.group(4))}
    um = re.search(r'"upgrade"\s*:\s*(SECT_UPGRADE_\w+)', entry_text)
    e["upgrade"] = {"SECT_UPGRADE_COMMON": UPG_COMMON, "SECT_UPGRADE_DEFENSE": UPG_DEFENSE,
                    "SECT_UPGRADE_SPECIAL": UPG_SPECIAL, "SECT_UPGRADE_PLANT": UPG_PLANT,
                    "SECT_UPGRADE_MARKET": UPG_MARKET, "SECT_UPGRADE_LIBRARY": UPG_LIBRARY}[um.group(1)] if um else {}
    drm = re.search(r'"daily_reward"\s*:\s*\(\[\s*"exp"\s*:\s*([A-Za-z_]+|\d+)\s*,\s*"contrib"\s*:\s*([A-Za-z_]+|\d+)\s*,\s*"verb"\s*:\s*"([^"]+)"', entry_text)
    if drm:
        def num_or_macro(s):
            if s.isdigit(): return int(s)
            return {"SECT_SPAR_EXP_BASE": SPAR_EXP_BASE, "SECT_SPAR_CONTRIB": SPAR_CONTRIB}.get(s, 0)
        e["daily_reward"] = {"exp": num_or_macro(drm.group(1)), "contrib": num_or_macro(drm.group(2)), "verb": drm.group(3)}
    return e

facility_config = {}
for m in re.finditer(r'"([a-z_]+)"\s*:\s*\(\[(.*?)\n  \]\)', daemon, re.S):
    key, body = m.group(1), m.group(2)
    if '"sect"' in body and '"room"' in body:
        facility_config[key] = parse_entry("(" + body + ")", key)

# seed_config 解析
seed_config = {}
for m in re.finditer(r'"([a-z]+)"\s*:\s*\(\[(.*?)\n  \]\)', daemon, re.S):
    key, body = m.group(1), m.group(2)
    if '"growth"' in body:
        def g(f):
            mm = re.search(r'"%s"\s*:\s*"?([^",\s]+)' % f, body)
            return mm.group(1) if mm else None
        sc = {"name": g("name"), "id": g("id"), "unit": g("unit"),
              "growth": GROWTH[g("growth")], "yield": int(g("yield")),
              "cost": int(g("cost")), "value": int(g("value"))}
        seed_config[key] = sc

assert len(facility_config) == 18, "设施配置条目数应为 18，实际 %d" % len(facility_config)
assert set(seed_config) == {"lingcao", "huanglongcao", "zidanshen"}, "种子配置缺失"

# ---------- 玩家状态模拟 ----------
class Player:
    def __init__(self, pid):
        self.pid = pid
        self.sect = None
        self.rank = 0
        self.contribution = 0
        self.balance_wen = 100000   # 初始 1000 灵石（文）
        self.combat_exp = 0
        self.learned = {}
        self.plots = {}             # facility_key -> {idx: plot}
        self.buffs = {}             # facility_key -> {expire, value}
        self.daily = {}             # facility_key -> {day, count}
        self.room = None
        self.now = 1000000          # 模拟时钟
        self.inv = []               # 物品

    def query(self, path):
        if path == "sect/id": return self.sect
        if path == "sect/rank": return self.rank
        if path == "sect/contribution": return self.contribution
        if path == "sect/learned": return self.learned
        if path == "combat_exp": return self.combat_exp
        if path.startswith("sect_facility/plots"):
            return self.plots
        if path.startswith("sect_facility/buffs"):
            return self.buffs
        if path.startswith("sect_facility/daily"):
            return self.daily
        return None

# ---------- 依赖 daemon 模拟（对齐 LPC） ----------
class SECT_D:
    @staticmethod
    def query_player_sect(p): return p.sect
    @staticmethod
    def query_rank(p): return p.rank
    @staticmethod
    def query_contribution(p): return p.contribution
    @staticmethod
    def add_contribution(p, amount, reason):   # sect_d.c:507
        if p.sect is None: return 0
        p.contribution += amount
        return p.contribution
    @staticmethod
    def query_sect_skill_info(sect, sid):
        # 黄枫谷功法（sect_d.c 配置）
        skills = {
            "huangfeng_valley": {
                "changchun-gong": {"name": "长春功", "rank": 0, "cost": 500, "desc": "木系基础功法"},
                "qingyuan-jianjue": {"name": "青元剑诀", "rank": 0, "cost": 500, "desc": "剑修功法"},
                "zhenyang-jue": {"name": "真阳诀", "rank": 1, "cost": 2000, "desc": "火系顶阶功法"},
            }
        }
        return skills.get(sect, {}).get(sid)
    @staticmethod
    def query_sect_skills(sect):
        # sect_d.c:query_sect_skills —— 返回本门技能键列表
        return list({
            "huangfeng_valley": {
                "changchun-gong": 1, "qingyuan-jianjue": 1, "zhenyang-jue": 1,
            },
            "yanyue_sect": {},
        }.get(sect, {}).keys())
    @staticmethod
    def query_sect_ranks(sect):
        return ["外门弟子", "内门弟子", "真传弟子", "长老", "副宗主", "宗主"]
    @staticmethod
    def query_sect_name(sect):
        return {"huangfeng_valley": "黄枫谷", "yanyue_sect": "掩月宗"}.get(sect, sect)

class MONEY_D:
    @staticmethod
    def player_pay(p, amount):   # moneyd.c:275，amount 单位为文
        if p.balance_wen < amount: return 0
        p.balance_wen -= amount
        return 1

# ---------- 设施 daemon 逻辑翻译（对齐 sect_facility_d.c） ----------
def eff_value(cfg):     # sect_facility_d.c eff_value
    e = cfg["effect"]
    return e["base"] + (cfg["level"] - 1) * e["per_level"]

def query_plot_count(key):   # sect_facility_d.c:query_plot_count
    return PLANT_PLOTS_BASE + (facility_config[key]["level"] - 1) * PLANT_PLOTS_PER

def check_access(p, key):    # sect_facility_d.c:check_access
    cfg = facility_config.get(key)
    if cfg is None: return "不存在的设施"
    if p.sect is None: return "你尚未拜入任何门派，无法使用门派设施。"
    if p.sect != cfg["sect"]: return "此设施不属于你所在的门派。"
    if p.room != key: return "请先进入「" + cfg["name"] + "」再使用。"
    return None

def pay_cost(p, key, stone, contrib):   # sect_facility_d.c:pay_cost（先查贡献后扣灵石）
    if contrib > 0 and SECT_D.query_contribution(p) < contrib:
        return "门派贡献不足，需要 %d 点贡献。" % contrib
    if stone > 0 and not MONEY_D.player_pay(p, stone * 100):
        return "灵石不足，需要 %d 灵石。" % stone
    if contrib > 0:
        SECT_D.add_contribution(p, -contrib, "使用设施：" + facility_config[key]["name"])
    return None

def today(p):
    return p.now // 86400

def query_daily_count(p, key):   # sect_facility_d.c:query_daily_count
    d = p.daily.get(key)
    if not d or d["day"] != today(p): return 0
    return d["count"]

def add_daily_count(p, key):     # sect_facility_d.c:add_daily_count
    d = p.daily.get(key)
    if not d or d["day"] != today(p):
        p.daily[key] = {"day": today(p), "count": 1}
    else:
        d["count"] += 1

def use_facility(p, key):        # sect_facility_d.c:use_facility
    err = check_access(p, key)
    if err: return err
    cfg = facility_config[key]
    if cfg["type"] in (1, 3) or cfg["market"]:
        return "此处设施请使用专项指令"
    if cfg["daily_limit"] > 0 and query_daily_count(p, key) >= cfg["daily_limit"]:
        return "今日已使用过「%s」，明日再来。" % cfg["name"]
    err = pay_cost(p, key, cfg["use_stone"], cfg["use_contrib"])
    if err: return err
    if cfg["daily_limit"] > 0: add_daily_count(p, key)
    value = eff_value(cfg)
    p.buffs[key] = {"expire": p.now + cfg["duration"], "value": value}
    return "ok:" + str(value)

def query_effect_value(p, buff_type):   # sect_facility_d.c:query_effect_value（同类型多设施取最大值）
    maxv = 0
    for k, cfg in facility_config.items():
        if cfg["effect"]["type"] != buff_type: continue
        b = p.buffs.get(k)
        if b and b["expire"] > p.now:
            maxv = max(maxv, b["value"])
    return maxv

def query_buff(p, key):          # sect_facility_d.c:query_buff（过期返回 0）
    b = p.buffs.get(key)
    if not b or b["expire"] <= p.now: return 0
    return b["value"]

def plant(p, key, seed_id):      # sect_facility_d.c:plant
    cfg = facility_config[key]
    if cfg["type"] != 1: return "此处不是灵田，无法种植。"
    err = check_access(p, key)
    if err: return err
    sc = seed_config.get(seed_id)
    if not sc: return "没有这种灵种。"
    total = query_plot_count(key)
    plots = p.plots.get(key, {})
    found = -1
    for i in range(total):
        pl = plots.get(i)
        if not pl or pl["status"] in (0, 4):
            found = i; break
    if found == -1: return "灵田已满，没有空闲地块。"
    cost = sc["cost"] + PLANT_MAINT
    if not MONEY_D.player_pay(p, cost * 100):
        return "灵石不足，种植需要 %d 灵石。" % cost
    speed_factor = 100 + (cfg["level"] - 1) * 25
    growth = int(sc["growth"] * 100.0 / speed_factor)
    plots[found] = {"status": 2, "seed": sc["name"], "seed_id": seed_id,
                    "planted": p.now, "mature": p.now + growth, "yield": 1}
    p.plots[key] = plots
    return "ok"

def harvest(p, key, idx):        # sect_facility_d.c:harvest
    cfg = facility_config[key]
    if cfg["type"] != 1: return "此处不是灵田。"
    err = check_access(p, key)
    if err: return err
    plots = p.plots.get(key, {})
    pl = plots.get(idx)
    if not pl: return "该地块没有种植作物。"
    if pl["status"] not in (2, 3): return "该地块当前没有可收获的作物。"
    if pl["status"] == 2 and p.now < pl["mature"]: return "作物尚未成熟。"
    sc = seed_config[pl["seed_id"]]
    bonus = eff_value(cfg)
    base = sc["yield"]
    total = base + int(base * bonus / 100.0)
    count = total if total > 0 else 1
    if random.randrange(100) < 30: count += 1
    for _ in range(count):
        p.inv.append(sc["name"])
    plots[idx] = 0
    p.plots[key] = plots
    return count

def read_skill(p, key, sid):     # sect_facility_d.c:read_skill（显示功法信息，无副作用）
    cfg = facility_config[key]
    if cfg["type"] != 3: return "此处不是藏经阁。"
    err = check_access(p, key)
    if err: return err
    info = SECT_D.query_sect_skill_info(p.sect, sid)
    if not info: return "本门无此功法。"
    return "ok:" + info["name"]

def transcribe(p, key, sid):     # sect_facility_d.c:transcribe_skill
    cfg = facility_config[key]
    if cfg["type"] != 3: return "此处不是藏经阁。"
    err = check_access(p, key)
    if err: return err
    info = SECT_D.query_sect_skill_info(p.sect, sid)
    if not info: return "本门无此功法。"
    if sid in p.learned: return "你已学过这门功法。"
    reduce = eff_value(cfg)
    cost = info["cost"]
    if reduce > 0:
        cost = int(cost * (100.0 - reduce) / 100.0)
    if SECT_D.query_contribution(p) < cost:
        return "贡献不足"
    SECT_D.add_contribution(p, -cost, "藏经阁抄录")
    p.learned[sid] = p.now
    return "ok"

def market_buy(p, key, gid):     # sect_facility_d.c:market_buy
    cfg = facility_config[key]
    if not cfg["market"]: return "此处不是坊市。"
    err = check_access(p, key)
    if err: return err
    sc = seed_config.get(gid)
    if not sc: return "坊市没有这种货物。"
    discount = eff_value(cfg)
    unit = sc["cost"]
    if discount > 0:
        unit = int(unit * (100.0 - discount) / 100.0)
    if not MONEY_D.player_pay(p, unit * 100):
        return "灵石不足"
    p.inv.append(sc["name"])
    return "ok"

def practice(p, key):            # sect_facility_d.c:practice
    cfg = facility_config[key]
    if "daily_reward" not in cfg: return "此处没有可进行的修行。"
    err = check_access(p, key)
    if err: return err
    if query_daily_count(p, key) >= 1: return "今日已在此修行过，明日再来。"
    rw = cfg["daily_reward"]
    exp_gain = rw["exp"] + (cfg["level"] - 1) * SPAR_EXP_PER
    p.combat_exp += exp_gain
    if rw["contrib"] > 0:
        SECT_D.add_contribution(p, rw["contrib"], "设施修行")
    add_daily_count(p, key)
    return "ok"

def upgrade(p, key):             # sect_facility_d.c:upgrade_facility
    err = check_access(p, key)
    if err: return err
    cfg = facility_config[key]
    if cfg["level"] >= cfg["max_level"]: return "已升至最高等级。"
    upg = cfg["upgrade"]
    nxt = cfg["level"] + 1
    if nxt not in upg: return "无法继续升级。"
    stone, contrib = upg[nxt]
    if p.rank < 1: return "只有内门弟子以上方可主持门派设施的升级。"
    err = pay_cost(p, key, stone, contrib)
    if err: return err
    cfg["level"] = nxt
    return "ok"

# ---------- 命令层模拟（对齐 cmds/usr/facility.c main() 分发 + daemon 中文名解析） ----------
# 审查发现（第 3 轮小厮·2）：帮助/列表展示中文名而实现按拼音 id 直查——玩家照帮助输入即失败。
# 修复：daemon 新增 resolve_seed_id / resolve_skill_id（中文名或拼音 id 双向解析），
# facility.c 子命令参数原样传给 daemon。此处模拟命令层完整调用链（中文名→id→命令层调用成功）。

def resolve_seed_id(arg):
    """对齐 sect_facility_d.c resolve_seed_id：中文名或拼音 id → seed_config 键"""
    if arg is None: return None
    for sid, sc in seed_config.items():
        if sid == arg or sc["name"] == arg:
            return sid
    return None

def resolve_skill_id(sect_id, arg):
    """对齐 sect_facility_d.c resolve_skill_id：中文名或拼音 id → SECT_D 技能键"""
    if arg is None: return None
    for sid in SECT_D.query_sect_skills(sect_id):
        info = SECT_D.query_sect_skill_info(sect_id, sid)
        if sid == arg or (info and info["name"] == arg):
            return sid
    return None

def cmd_facility(p, arg):
    """对齐 cmds/usr/facility.c main()：子命令分发，参数（中文名或拼音 id）传给 daemon"""
    if not arg or " " not in arg:
        return None  # 无参/单参子命令（use/upgrade/practice/list/help）不在命令面测试范围
    cmd, subarg = arg.split(" ", 1)
    if cmd == "plant":
        sid = resolve_seed_id(subarg)
        if sid is None: return "没有这种灵种。输入 facility list 查看可种灵种。"
        return plant(p, p.room, sid)
    if cmd == "harvest":
        idx = int(subarg) - 1      # 地块编号
        if idx < 0: idx = 0
        return harvest(p, p.room, idx)
    if cmd == "read":
        sid2 = resolve_skill_id(p.sect, subarg)
        if sid2 is None: return "本门无此功法。输入 facility list 查看本门功法。"
        return read_skill(p, p.room, sid2)
    if cmd == "copy":
        sid2 = resolve_skill_id(p.sect, subarg)
        if sid2 is None: return "本门无此功法。"
        return transcribe(p, p.room, sid2)
    if cmd == "buy":
        gid = resolve_seed_id(subarg)
        if gid is None: return "坊市没有这种货物。输入 facility list 查看货物。"
        return market_buy(p, p.room, gid)
    return "未知子命令"

# ---------- 断言框架 ----------
PASS = 0; FAIL = []
def check(name, cond, detail=""):
    global PASS
    if cond:
        PASS += 1
    else:
        FAIL.append("%s%s" % (name, ("：" + detail) if detail else ""))

# ---------- 场景 1：入宗前/异派/位置拦截 ----------
p = Player("test_hanli")
check("入宗前使用丹房被拒", use_facility(p, "huangfeng_danfang") == "你尚未拜入任何门派，无法使用门派设施。")
check("入宗前贡献查询为 0", SECT_D.query_contribution(p) == 0)
p.sect = "huangfeng_valley"
check("未在设施房间使用被拒", use_facility(p, "huangfeng_danfang") == "请先进入「岳麓殿丹房」再使用。")
p.room = "huangfeng_danfang"
p2 = Player("test_mo"); p2.sect = "yanyue_sect"
check("异派弟子使用被拒", use_facility(p2, "huangfeng_danfang") == "此设施不属于你所在的门派。")

# ---------- 场景 2：丹房炼丹加成（c6 核心） ----------
p.contribution = 1000   # 测试资源
before = p.contribution
r = use_facility(p, "huangfeng_danfang")
check("丹房使用成功", r.startswith("ok:"), str(r))
check("丹房扣贡献（走 add_contribution）", p.contribution == before - 100, "before=%d after=%d" % (before, p.contribution))
check("丹房扣灵石 10", p.balance_wen == 100000 - 10*100)
check("炼丹加成生效 query_danfang_bonus=10（§4.4.1 炼丹阁 +10%）", query_effect_value(p, "alchemy_success") == 10)
check("buff 记录在玩家属性", query_buff(p, "huangfeng_danfang") == 10)

# 贡献不足：先查贡献后扣灵石
p.contribution = 0
p.balance_wen = 100000
r = use_facility(p, "huangfeng_danfang")
check("贡献不足被拒", "门派贡献不足" in r, r)
check("贡献不足不扣灵石", p.balance_wen == 100000)
p.contribution = 1000

# ---------- 场景 3：灵田种植/收获（c1/c6） ----------
p.room = "huangfeng_baiyaoyuan"
r = plant(p, "huangfeng_baiyaoyuan", "lingcao")
check("百药园播种成功", r == "ok", str(r))
check("播种扣灵石 20（种子10+维护10）", p.balance_wen == 100000 - 20*100)
pl = p.plots["huangfeng_baiyaoyuan"][0]
check("地块状态生长中", pl["status"] == 2)
check("未成熟收获被拒", "尚未成熟" in harvest(p, "huangfeng_baiyaoyuan", 0))
p.now = pl["mature"] + 1
before_inv = len(p.inv)
cnt = harvest(p, "huangfeng_baiyaoyuan", 0)
check("成熟收获成功且产灵草", isinstance(cnt, int) and cnt >= 1, str(cnt))
check("收获物进背包", len(p.inv) >= before_inv + 1)
check("地块回到空闲", p.plots["huangfeng_baiyaoyuan"].get(0) is None or p.plots["huangfeng_baiyaoyuan"].get(0) == 0)

# 灵田升级→更多地块：外门弟子不能升级，内门可以（Lv1→2 = 1000 灵石 + 5000 贡献，对齐 §4.4.1 灵药园）
p.balance_wen += 300000   # 升级测试资源
p.contribution += 6000
p.room = "huangfeng_baiyaoyuan"
r = upgrade(p, "huangfeng_baiyaoyuan")
check("外门弟子升级被拒", r == "只有内门弟子以上方可主持门派设施的升级。")
before_bal, before_contrib = p.balance_wen, p.contribution
p.rank = 1
r = upgrade(p, "huangfeng_baiyaoyuan")
check("内门弟子升级灵田成功", r == "ok", str(r))
check("灵田升 2 级", facility_config["huangfeng_baiyaoyuan"]["level"] == 2)
check("灵田地块增至 5 块", query_plot_count("huangfeng_baiyaoyuan") == 5)
check("升级实扣灵石 1000", p.balance_wen == before_bal - 1000*100)
check("升级实扣贡献 5000", p.contribution == before_contrib - 5000)
p.rank = 0  # 还原

# ---------- 场景 4：藏经阁阅读/抄录 ----------
# 注：真实实现用 base_name(environment(player)) 与配置 room 字段匹配识别设施房间
# （sect_facility_d.c query_current_facility，由下方『LPC 原文守卫』做机器验证）；
# 此处 Python 翻译以 p.room=key 模拟该匹配结果。
p.room = "huangfeng_cangjing"
r = transcribe(p, "huangfeng_cangjing", "changchun-gong")
check("藏经阁抄录成功（耗贡献500）", r == "ok", str(r))
check("抄录写入 sect/learned", "changchun-gong" in p.learned)
check("重复抄录被拒", transcribe(p, "huangfeng_cangjing", "changchun-gong") == "你已学过这门功法。")

# ---------- 场景 5：坊市购买 ----------
p.room = "huangfeng_fangshi"
before_bal = p.balance_wen
r = market_buy(p, "huangfeng_fangshi", "zidanshen")
check("坊市购买紫丹参成功", r == "ok", str(r))
check("坊市实扣灵石 100", p.balance_wen == before_bal - 100*100)

# ---------- 场景 6：每日修行（切磋） ----------
p.room = "huangfeng_qingjianchang"
before_exp, before_contrib = p.combat_exp, p.contribution
r = practice(p, "huangfeng_qingjianchang")
check("演武场切磋成功", r == "ok", str(r))
check("切磋加经验 500", p.combat_exp == before_exp + 500)
check("切磋加贡献 20（add_contribution）", p.contribution == before_contrib + 20)
r = practice(p, "huangfeng_qingjianchang")
check("当日第二次切磋被拒", "今日已在此修行过" in r, r)

# ---------- 场景 7：每日限额（天月神舟 daily_limit=1） ----------
p2.contribution = 1000
p2.room = "yanyue_tianyuezhou"
r = use_facility(p2, "yanyue_tianyuezhou")
check("天月神舟使用成功（扣50灵石+500贡献）", r.startswith("ok:"), str(r))
r = use_facility(p2, "yanyue_tianyuezhou")
check("天月神舟当日第二次被拒（每日1次）", "今日已使用过" in r, r)

# ---------- 场景 8：护山大阵防御加成 ----------
p.room = "huangfeng_hushan"
r = use_facility(p, "huangfeng_hushan")
check("护山大阵使用成功", r.startswith("ok:"), str(r))
check("防御加成生效 query_defense_bonus=20", query_effect_value(p, "defense_power") == 20)

# ---------- 场景 9：九宗设施配置完整性 ----------
nine = ["yanyue_sect","huangfeng_valley","lingshou_mountain","qingxu_sect","huadao_dock",
        "tianque_fort","jujian_gate","guiling_sect","yuling_sect"]
for s in nine:
    check("门派 %s 有设施" % s, any(c["sect"] == s for c in facility_config.values()))
check("通用五类设施齐备",
      {1,2,3,4,5} <= {c["type"] for c in facility_config.values()})
check("18 个设施条目", len(facility_config) == 18)

# ---------- 场景 10：命令层中文名→id 解析（c6 修复，第 3 轮小厮·2 not_met 项） ----------
# 玩家照帮助输入中文名（facility plant <灵草|黄龙草|紫丹参>、read/copy <功法名>、buy <货物>）
# 必须解析到正确拼音 id 并走通命令层调用；拼音 id 直传（既有调用方）仍可用。
p3 = Player("cmd_hanli"); p3.sect = "huangfeng_valley"; p3.contribution = 1000

p3.room = "huangfeng_baiyaoyuan"
r = cmd_facility(p3, "plant 灵草")
check("命令层：facility plant 灵草 → 解析 lingcao → 种植成功", r == "ok", str(r))
pl = p3.plots["huangfeng_baiyaoyuan"][0]
check("命令层：中文名种植地块 seed_id=lingcao", pl["seed_id"] == "lingcao", str(pl.get("seed_id")))
r = cmd_facility(p3, "plant 黄龙草")
check("命令层：facility plant 黄龙草 → 解析 huanglongcao → 种植成功", r == "ok", str(r))
p3.now = p3.plots["huangfeng_baiyaoyuan"][1]["mature"] + 1
r = cmd_facility(p3, "harvest 2")
check("命令层：facility harvest 2（地块编号）可收获", isinstance(r, int) and r >= 1, str(r))
r = cmd_facility(p3, "plant 紫丹参")
check("命令层：facility plant 紫丹参 → 解析 zidanshen → 种植成功", r == "ok", str(r))
r = cmd_facility(p3, "plant 不存在的灵草")
check("命令层：中文名不存在被拒", "没有这种灵种" in r, r)

p3.room = "huangfeng_cangjing"
r = cmd_facility(p3, "read 长春功")
check("命令层：facility read 长春功 → 解析 changchun-gong → 阅读成功", r.startswith("ok:"), str(r))
r = cmd_facility(p3, "copy 长春功")
check("命令层：facility copy 长春功 → 解析 changchun-gong → 抄录成功", r == "ok", str(r))
check("命令层：抄录写入 learned[changchun-gong]", "changchun-gong" in p3.learned)
r = cmd_facility(p3, "read 不存在功法")
check("命令层：功法中文名不存在被拒", "本门无此功法" in r, r)

p3.room = "huangfeng_fangshi"
before_bal = p3.balance_wen
r = cmd_facility(p3, "buy 紫丹参")
check("命令层：facility buy 紫丹参 → 解析 zidanshen → 购买成功", r == "ok", str(r))
check("命令层：中文名购买实扣灵石 100", p3.balance_wen == before_bal - 100*100)

# 拼音 id 直传仍然可用（兼容既有调用方与 daemon 直接调用）
p3.room = "huangfeng_baiyaoyuan"
r = cmd_facility(p3, "plant lingcao")
check("命令层：拼音 id 直传仍可用（facility plant lingcao）", r == "ok", str(r))

# ---------- LPC 原文守卫（机器验证绑定 LPC 原文，非仅 Python 翻译） ----------
# 针对审查意见：path_verify 的 Python 翻译不绑定 LPC 原文（改 LPC 不改翻译仍绿）。
# 此段直接解析 LPC 原文关键函数体并断言顺序/匹配逻辑，另做一次 LPC 原文突变
# 证明守卫对原文敏感（红→绿）。
def extract_func(daemon_src, func_name):
    m = re.search(r'\b' + func_name + r'\s*\([^)]*\)\s*\{(.*?)\n\}', daemon_src, re.S)
    return m.group(1) if m else None

def pay_order_ok(body):
    """先查贡献（query_contribution<contrib）必须出现在 player_pay 之前（sect_facility_d.c:pay_cost）"""
    if body is None: return False
    i_contrib = body.find("query_contribution(player) < contrib")
    i_pay = body.find("player_pay(player, stone * 100)")
    return i_contrib != -1 and i_pay != -1 and i_contrib < i_pay

def base_name_match_ok(body):
    """设施房间识别用 base_name(environment(player)) 匹配配置 room（sect_facility_d.c:query_current_facility）"""
    if body is None: return False
    return "base_name(env)" in body and 'facility_config[key]["room"]' in body

pay_body = extract_func(daemon, "pay_cost")
cur_body = extract_func(daemon, "query_current_facility")
check("LPC 原文 pay_cost：先查贡献（query_contribution）后扣灵石（player_pay）",
      pay_order_ok(pay_body))
check("LPC 原文 query_current_facility：base_name(env) 匹配配置 room",
      base_name_match_ok(cur_body))

def contrib_via_api_ok(body):
    """贡献消耗/奖励必须走 SECT_D->add_contribution 接口（c3 核心性质，非直接写 sect/contribution）"""
    if body is None: return False
    return "SECT_D->add_contribution(player" in body

check("LPC 原文 pay_cost：贡献扣减走 SECT_D->add_contribution", contrib_via_api_ok(pay_body))
check("LPC 原文 transcribe_skill：抄录贡献走 SECT_D->add_contribution",
      contrib_via_api_ok(extract_func(daemon, "transcribe_skill")))
check("LPC 原文 practice：奖励贡献走 SECT_D->add_contribution",
      contrib_via_api_ok(extract_func(daemon, "practice")))

# 突变实证②（作用于 LPC 原文）：把 pay_cost 的 add_contribution 换成直接写 sect/contribution
# → 守卫必须转红（证明 c3 接口绑定原文，回应审查突变④）
mut2 = daemon.replace('SECT_D->add_contribution(player, -contrib, reason)',
                      'player->add("sect/contribution", -contrib)')
mut2_pay = extract_func(mut2, "pay_cost")
check("LPC 原文突变：贡献改直接写 sect/contribution 后守卫转红（c3 接口绑定）",
      mut2_pay is not None and not contrib_via_api_ok(mut2_pay))

# 突变实证（作用于 LPC 原文，非 Python 翻译）：颠倒 pay_cost 扣费顺序 → 守卫必须转红
old_seq = ('    if (contrib > 0 && SECT_D->query_contribution(player) < contrib)\n'
           '        return "门派贡献不足，需要 " + contrib + " 点贡献。";\n\n'
           '    if (stone > 0 && !MONEY_D->player_pay(player, stone * 100))\n'
           '        return "灵石不足，需要 " + stone + " 灵石。";')
new_seq = ('    if (stone > 0 && !MONEY_D->player_pay(player, stone * 100))\n'
           '        return "灵石不足，需要 " + stone + " 灵石。";\n\n'
           '    if (contrib > 0 && SECT_D->query_contribution(player) < contrib)\n'
           '        return "门派贡献不足，需要 " + contrib + " 点贡献。";')
assert old_seq in daemon, "LPC pay_cost 顺序片段未找到，无法突变"
mut_daemon = daemon.replace(old_seq, new_seq)
mut_pay = extract_func(mut_daemon, "pay_cost")
check("LPC 原文突变：颠倒扣费顺序后守卫转红（证明守卫绑定 LPC 原文）",
      mut_pay is not None and not pay_order_ok(mut_pay))

# ======== c6 路径原文守卫（第 3 轮小厮·1 MUT-B/MUT-C 缺口） ========
# MUT-B：删掉 plant 的灵石扣费 → 此前脚本仍全绿；MUT-C：use_facility 的 grant_buff 加成被移除 → 同样全绿。
# 补守卫：解析 plant / use_facility 函数体，断言扣费与加成授予真实存在。

def plant_pay_ok(body):
    """plant 函数体：种植必须真实扣灵石（MONEY_D->player_pay）"""
    if body is None: return False
    return "MONEY_D->player_pay(player, cost * 100)" in body

def use_grant_ok(body):
    """use_facility 函数体：buff 类设施加成必须经 grant_buff 授予（丹房/护山大阵生效）"""
    if body is None: return False
    return "grant_buff(player, key, value, dur)" in body

plant_body = extract_func(daemon, "plant")
use_body = extract_func(daemon, "use_facility")
check("LPC 原文 plant：种植扣灵石走 MONEY_D->player_pay", plant_pay_ok(plant_body))
check("LPC 原文 use_facility：buff 加成经 grant_buff 授予", use_grant_ok(use_body))

# 突变实证（复刻 MUT-B）：删掉 plant 函数的灵石扣费 → 守卫必须转红
mut_plant = daemon.replace("if (!MONEY_D->player_pay(player, cost * 100))",
                           "if (0) /* MUT-B: 灵石扣费被删 */")
mut_plant_body = extract_func(mut_plant, "plant")
check("LPC 原文突变：plant 灵石扣费被删后守卫转红（MUT-B）",
      mut_plant_body is not None and not plant_pay_ok(mut_plant_body))

# 突变实证（复刻 MUT-C）：use_facility 的 grant_buff 被移除（加成不再生效）→ 守卫必须转红
mut_use = daemon.replace("grant_buff(player, key, value, dur);", "/* MUT-C: 加成不再生效 */")
mut_use_body = extract_func(mut_use, "use_facility")
check("LPC 原文突变：use_facility grant_buff 被移除后守卫转红（MUT-C）",
      mut_use_body is not None and not use_grant_ok(mut_use_body))

# ======== c6 命令面中文名解析原文守卫（第 3 轮小厮·2 not_met 项） ========
# 帮助/列表展示中文名（灵草/黄龙草/紫丹参/长春功），实现必须按中文名或拼音 id 双向解析。

def resolve_seed_name_ok(body):
    """resolve_seed_id 函数体：必须含中文名匹配分支（sc["name"] == arg）"""
    if body is None: return False
    return 'sc["name"] == arg' in body

def resolve_skill_name_ok(body):
    """resolve_skill_id 函数体：必须含中文名匹配分支（info["name"] == arg）"""
    if body is None: return False
    return 'info["name"] == arg' in body

seed_res_body = extract_func(daemon, "resolve_seed_id")
skill_res_body = extract_func(daemon, "resolve_skill_id")
check("LPC 原文 resolve_seed_id：中文名或拼音 id 双向匹配", resolve_seed_name_ok(seed_res_body))
check("LPC 原文 resolve_skill_id：中文名或拼音 id 双向匹配", resolve_skill_name_ok(skill_res_body))

# 突变实证：把 resolve_seed_id 改成只按拼音 id 匹配（删中文名分支）→ 守卫转红
mut_seed = daemon.replace("if (sid == arg || (mapp(sc) && sc[\"name\"] == arg))",
                          "if (sid == arg)")
mut_seed_body = extract_func(mut_seed, "resolve_seed_id")
check("LPC 原文突变：resolve_seed_id 去掉中文名匹配后守卫转红（c6 命令面）",
      mut_seed_body is not None and not resolve_seed_name_ok(mut_seed_body))

# ======== 命令层 dispatch 静态守卫（命令层测试绑定真实 facility.c） ========
# 命令层必须把子命令参数（中文名或拼音 id）原样传给 daemon，中文名解析在 daemon 侧完成。
facility_c = open(os.path.join(BASE, "cmds/usr/facility.c"), encoding="utf-8").read()
check("命令层 facility.c：plant 子命令传参给 SECT_FACILITY_D->plant",
      "SECT_FACILITY_D->plant(me, key, subarg)" in facility_c)
check("命令层 facility.c：read 子命令传参给 SECT_FACILITY_D->read_skill",
      "SECT_FACILITY_D->read_skill(me, key, subarg)" in facility_c)
check("命令层 facility.c：copy 子命令传参给 SECT_FACILITY_D->transcribe_skill",
      "SECT_FACILITY_D->transcribe_skill(me, key, subarg)" in facility_c)
check("命令层 facility.c：buy 子命令传参给 SECT_FACILITY_D->market_buy",
      "SECT_FACILITY_D->market_buy(me, key, subarg, 1)" in facility_c)

# 突变实证：命令层把 plant 参数硬编码成拼音 id（中文名输入将失效）→ 守卫转红
mut_fac = facility_c.replace("SECT_FACILITY_D->plant(me, key, subarg)",
                             'SECT_FACILITY_D->plant(me, key, "lingcao")')
check("命令层突变：facility.c plant 硬编码 id 后守卫转红（命令层绑定）",
      "SECT_FACILITY_D->plant(me, key, subarg)" not in mut_fac)

# ---------- 汇总 ----------
print("===== #60 门派设施系统 路径验证 =====")
print("设施配置 %d 条 / 种子 %d 种" % (len(facility_config), len(seed_config)))
print("断言：%d 通过 / %d 失败" % (PASS, len(FAIL)))
for f in FAIL:
    print("  FAIL: " + f)
sys.exit(1 if FAIL else 0)
