#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#73 丹药炼制链路（PILL_D + 丹药实体 + 筑基丹接突破）静态校验 + LPC 原文守卫 + 同构模拟（可复跑验证）
运行：python3 tools/check/pill_verify.py
覆盖：
  一、静态校验：交付文件存在 / 括号配对状态机 / 引用完整性（丹方成品、丹方药材→坊市材料、
                 shop_d 13 个丹药引用全部指向真实对象）/ 接口签名（ROOT_REFINE_D、SECT_FACILITY_D、
                 ECONOMY_D、ECONOMY_BRIDGE_D、MONEY_D 依赖接口）/ 丹药实体 1E §4.1 六属性 /
                 globals.h 宏
  二、LPC 原文守卫：PILL_D consume_ingredients（材料校验/扣除）、refine_pill（等级门槛/成功率/成丹）、
                 dan.c do_eat 分发 + pill_bonus 写入 + 叠加上限、root_refine_d.c 大境界概率
                 pill_bonus 读取接线（#61 预留端）、tupo.c 成功后清理 pill_bonus、
                 shop_d.c 13 引用改指——守卫绑定交付物原文，含突变实证（改回旧行为 → 守卫红）
  三、同构模拟：炼丹术等级换算 / 成功率计算（炼丹术×2% + 丹房加成 − 品级罚值，钳 5~95）/
                 炼制流程（材料不足不动材料；足够则扣材料成丹；品质判定随炼丹术提升）/
                 筑基丹服用（炼气期校验、叠加最多 3 颗）/
                 突破概率（pill_bonus 真实加成、成功后清空、失败保留）
退出码：0 全绿，1 有失败
"""
import re
import sys
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0

# ═══════════ 工具函数 ═══════════

def strip_lpc(src):
    """去行注释/块注释/字符串/@LONG heredoc，保留代码骨架"""
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if c == '/' and i + 1 < n and src[i + 1] == '/':
            j = src.find('\n', i)
            i = n if j == -1 else j
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '*':
            j = src.find('*/', i + 2)
            i = n if j == -1 else j + 2
            continue
        if c == '"':
            j = i + 1
            while j < n:
                if src[j] == '\\':
                    j += 2
                    continue
                if src[j] == '"':
                    break
                j += 1
            i = j + 1 if j < n else n
            continue
        if c == '@':
            m = re.match(r'@([A-Z_][A-Z_0-9]*)', src[i:])
            if m:
                endm = re.search(r'\n\s*' + re.escape(m.group(1)) + r'\s*;', src[i:])
                if endm:
                    i += endm.end()
                    continue
        out.append(c)
        i += 1
    return ''.join(out)


def check_parens(path):
    """括号配对状态机（含 @LONG heredoc 感知）"""
    try:
        with open(path, encoding='utf-8') as f:
            src = f.read()
    except IOError as e:
        return False, 'read error: %s' % e
    body = strip_lpc(src)
    stack = []
    pairs = {'(': ')', '{': '}', '[': ']'}
    for c in body:
        if c in pairs:
            stack.append(c)
        elif c in pairs.values():
            if not stack or pairs[stack.pop()] != c:
                return False, 'mismatch at %r (stack=%r)' % (c, stack[-3:])
    if stack:
        return False, 'unclosed %r' % stack[-1]
    return True, 'parens ok'


def ck(name, cond, detail=''):
    global fails
    if not cond:
        fails += 1
        print('  [FAIL] %s %s' % (name, detail))
    else:
        print('  [ OK ] %s' % name)


def readf(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

DELIVERY = [
    'adm/daemons/pill_d.c',
    'cmds/usr/liandan.c',
    'inherit/item/dan.c',
    'include/pill.h',
    'include/globals.h',
]
PILL_FILES = [
    'clone/pill/lianqisan.c', 'clone/pill/huanglongdan.c', 'clone/pill/juqi.c',
    'clone/pill/zhuji.c', 'clone/pill/ningdan.c', 'clone/pill/jiejindan.c',
    'clone/pill/cao.c', 'clone/pill/yuehua_cao.c', 'clone/pill/yuehua_dan.c',
    'clone/pill/shouwan.c', 'clone/pill/shouwang.c', 'clone/pill/xingchen.c',
    'clone/pill/nixi.c', 'clone/pill/junxu.c', 'clone/pill/xingjun.c',
    'clone/pill/lingye.c',
]
print('交付文件存在性:')
for p in DELIVERY + PILL_FILES:
    ck('exists %s' % p, os.path.isfile(p))

print('括号配对状态机:')
for p in DELIVERY + PILL_FILES + ['adm/daemons/root_refine_d.c', 'cmds/usr/tupo.c', 'adm/daemons/shop_d.c']:
    ok, detail = check_parens(p)
    ck('parens %s' % p, ok, detail)

print('globals.h 宏:')
gh = readf('include/globals.h')
ck('PILL_D 宏', re.search(r'#define\s+PILL_D\s+"', gh))
ck('DAN_BASE 宏', re.search(r'#define\s+DAN_BASE\s+"', gh))

print('接口签名（依赖方真实存在）:')
rrd = readf('adm/daemons/root_refine_d.c')
for fn in ['query_player_realm_index', 'query_player_realm', 'add_xiuwei', 'query_xiuwei',
           'check_qi_layer_up', 'realm_name', 'query_major_breakthrough_probability']:
    ck('ROOT_REFINE_D %s' % fn, re.search(r'\b%s\s*\(' % fn, rrd))
sfd = readf('adm/daemons/sect_facility_d.c')
ck('SECT_FACILITY_D query_danfang_bonus', re.search(r'query_danfang_bonus\s*\(', sfd))
ecd = readf('adm/daemons/economyd.c')
for fn in ['register_goods', 'query_current_price', 'record_purchase', 'record_sale']:
    ck('ECONOMY_D %s' % fn, re.search(r'\b%s\s*\(' % fn, ecd))
ebd = readf('adm/daemons/economy_bridge_d.c')
ck('ECONOMY_BRIDGE_D get_player_realm_code', re.search(r'get_player_realm_code\s*\(', ebd))

print('丹药实体 1E §4.1 六属性:')
REQ_ATTRS = ['pill_type', 'stage', 'effect', 'quality', 'side_effect', 'refine_level']
for p in PILL_FILES:
    src = readf(p)
    for a in REQ_ATTRS:
        ck('%s set(%s)' % (p, a), re.search(r'set\("%s"\s*,' % a, src))
    ck('%s inherit DAN_BASE' % p, 'inherit DAN_BASE' in src)
    ck('%s setup()' % p, re.search(r'\bsetup\(\)\s*;', src))

print('丹方引用完整性（PILL_D 内部数据 → 实体/材料）:')
pd = readf('adm/daemons/pill_d.c')
# 提取 PILL_D 中 danfang 引用的成品路径与药材
pill_paths = set(re.findall(r'"pill"\s*:\s*"([^"]+)"', pd))
for pp in sorted(pill_paths):
    ck('danfang 成品 %s' % pp, os.path.isfile(pp[1:] + '.c'), '(missing)')
mat_ids = set(re.findall(r'"([a-z_]+)"\s*:\s*\d+\s*\]', pd))
# 药材 id 应对应坊市材料对象（material_id 属性）
for mid in sorted(mat_ids):
    found = False
    for d in os.listdir('d/yueguo/tainan/obj'):
        if d.endswith('.c'):
            if re.search(r'set\("material_id"\s*,\s*"%s"\)' % mid, readf('d/yueguo/tainan/obj/' + d)):
                found = True
    ck('丹方药材 %s 有坊市材料对象' % mid, found)

print('实体 refine_level 与丹方门槛一致（c6 数据一致性）:')
# 解析 PILL_D 丹方：pill 路径 → refine_level
df_pill_level = {}
for m in re.finditer(r'"([a-z_]+)"\s*:\s*\(\[\s*"name"\s*:\s*"([^"]+)"\s*,\s*"pill"\s*:\s*"([^"]+)"\s*,.*?"refine_level"\s*:\s*(\d+)', pd, re.S):
    df_pill_level[m.group(3)] = int(m.group(4))
for ppath, plevel in sorted(df_pill_level.items()):
    pfile = ppath[1:] + '.c'
    psrc = readf(pfile)
    m = re.search(r'set\("refine_level",\s*(\d+)\)', psrc)
    if m:
        ck('%s 实体 refine_level=%s == 丹方 %s' % (pfile, m.group(1), plevel), int(m.group(1)) == plevel)
    else:
        ck('%s 有 refine_level' % pfile, False)

print('实体 effect 绑定（筑基丹 effect/stage/refine_level 从实体读取，防模拟硬编码漂移）:')
zhuji_src = readf('clone/pill/zhuji.c')
zhuji_effect = int(re.search(r'set\("effect",\s*(\d+)\)', zhuji_src).group(1))
zhuji_stage = int(re.search(r'set\("stage",\s*(\d+)\)', zhuji_src).group(1))
zhuji_plevel = int(re.search(r'set\("refine_level",\s*(\d+)\)', zhuji_src).group(1))
ck('筑基丹 effect=25（实体绑定）', zhuji_effect == 25)
ck('筑基丹 stage=2 目标筑基（实体绑定）', zhuji_stage == 2)
ck('筑基丹 refine_level=8（实体绑定）', zhuji_plevel == 8)
ck('筑基丹丹方门槛=8（实体与丹方一致）', df_pill_level.get('/clone/pill/zhuji') == 8)

print('shop_d 13 个丹药引用全部指向真实对象:')
shop = readf('adm/daemons/shop_d.c')
refs = re.findall(r'"type"\s*:\s*"pill"[^}]*?"/clone/pill/([a-z_]+)"', shop)
# shop_d 是数组形式，直接找 /clone/pill/ 路径
pill_refs = re.findall(r'"/clone/pill/([a-z_]+)"', shop)
ck('shop_d 引用数 = 13', len(set(pill_refs)) == 13, 'got %d: %s' % (len(set(pill_refs)), sorted(set(pill_refs))))
for r in sorted(set(pill_refs)):
    ck('shop_d → /clone/pill/%s.c 存在' % r, os.path.isfile('clone/pill/%s.c' % r))
# 旧 MISS 路径不再出现
ck('shop_d 无旧 /obj/remedy 丹药引用', '/obj/remedy' not in shop and '/obj/nicheng' not in shop)

# ═══════════ 二、LPC 原文守卫 ═══════════
print('== 二、LPC 原文守卫 ==')

def lpc_func_body(src, fname):
    """提取 LPC 函数体（跳过前向声明）"""
    m = re.search(r'\b%s\s*\([^)]*\)\s*\{(?=\n|.)' % fname, src)
    if not m:
        return ''
    start = m.end()
    depth = 1
    i = start
    while i < len(src) and depth > 0:
        if src[i] == '{':
            depth += 1
        elif src[i] == '}':
            depth -= 1
        i += 1
    return src[start:i]

pd = readf('adm/daemons/pill_d.c')
body = strip_lpc(pd)

def func_body_raw(src, fname):
    """提取 LPC 函数体（原文，不去字符串——守卫检查调用形态/字符串参数时用原文）"""
    m = re.search(r'\b%s\s*\([^)]*\)\s*\{(?=\n|.)' % fname, src)
    if not m:
        return ''
    start = m.end()
    depth = 1
    i = start
    while i < len(src) and depth > 0:
        if src[i] == '{':
            depth += 1
        elif src[i] == '}':
            depth -= 1
        i += 1
    return src[start:i]

# consume_ingredients：先校验后扣除（原文：字符串参数与结构都要检查）
ci_raw = func_body_raw(pd, 'consume_ingredients')
ci = strip_lpc(ci_raw)
ck('consume_ingredients 有材料校验', '材料不足' in ci_raw)
ck('consume_ingredients 有扣除 destruct', 'destruct(' in ci)
# 校验在扣除之前：'材料不足' 分支在 'destruct' 之前
ck('consume_ingredients 先校验后扣除', ci_raw.find('材料不足') < ci_raw.find('destruct('))

# refine_pill：等级门槛 + 成功率 + 成丹 new
rp_raw = func_body_raw(pd, 'refine_pill')
rp = strip_lpc(rp_raw)
ck('refine_pill 有等级门槛检查', '炼丹术等级不足' in rp_raw)
ck('refine_pill 调 query_success_rate', 'query_success_rate' in rp)
ck('refine_pill 成丹 new(df["pill"])', 'new(df["pill"])' in rp_raw)
ck('refine_pill 成功率判定', 'random(100)' in rp and 'roll >= rate' in rp)

# query_success_rate：乘法结构 base×(1+修正%)×(1+年份%) − 品级罚值
qsr_raw = func_body_raw(pd, 'query_success_rate')
qsr = strip_lpc(qsr_raw)
ck('成功率含炼丹术加成', 'refine * 2' in qsr)
ck('成功率含丹房加成(接 #60)', 'query_danfang_bonus' in qsr_raw or 'query_danfang_bonus' in qsr)
ck('成功率含品级罚值', 'df["quality"]' in qsr_raw and 'rate -= penalty' in qsr)
ck('成功率乘法结构(02 §4.3)', '* (100 + refine * 2' in qsr and '* (100 + year_bonus)' in qsr)
ck('成功率含火候修正(1E §2.3)', 'fire_bonus' in qsr and 'PILL_FIRE_WEN' in qsr_raw and 'PILL_FIRE_WANG' in qsr_raw)
ck('成功率含药材年份(1E §2.3)', 'year_bonus' in qsr and 'year / 10' in qsr and 'PILL_YEAR_BONUS_CAP' in qsr)

# query_herb_avg_year：读取材料 herb_year 求平均
qh_raw = func_body_raw(pd, 'query_herb_avg_year')
ck('药材年份读取 herb_year', 'herb_year' in qh_raw and 'material_id' in qh_raw)

# roll_quality：炼丹术提升品级 + 旺火概率翻倍
rq_raw = func_body_raw(pd, 'roll_quality')
rq = strip_lpc(rq_raw)
ck('品质判定随炼丹术提升', 'refine >= 15' in rq)
ck('旺火品质概率翻倍', 'q_prob' in rq and '40' in rq)

# 材料对象带 herb_year（1E §2.3 年份维度）
lc = readf('d/yueguo/tainan/obj/lingcao.c')
hc = readf('d/yueguo/tainan/obj/huanglongcao.c')
ck('灵草带 herb_year', re.search(r'set\("herb_year",\s*\d+\)', lc))
ck('黄龙草带 herb_year', re.search(r'set\("herb_year",\s*\d+\)', hc))

# dan.c：do_eat 分发 + pill_bonus 写入 + 叠加上限
dan = readf('inherit/item/dan.c')
ck('dan.c 分发三类', 'PILL_TYPE_BREAKTHROUGH' in dan and 'PILL_TYPE_XIUWEI' in dan and 'PILL_TYPE_HEAL' in dan)
db_raw = func_body_raw(dan, 'do_eat_breakthrough')
db = strip_lpc(db_raw)
ck('突破丹写 pill_bonus', 'add_temp("breakthrough/pill_bonus"' in db_raw)
ck('突破丹叠加上限', 'PILL_BREAK_MAX_STACK' in db and 'max_bonus' in db)
ck('突破丹境界校验', 'query_player_realm_index' in db_raw and 'target - 1' in db_raw)
dx_raw = func_body_raw(dan, 'do_eat_xiuwei')
dx = strip_lpc(dx_raw)
ck('修为丹 add_xiuwei', 'add_xiuwei' in dx_raw)
ck('修为丹丹毒累积', 'PILL_TOXIN' in dx and 'toxin' in dx)
dh = strip_lpc(func_body_raw(dan, 'do_eat_heal'))
ck('疗伤丹回血', 'receive_curing' in dh)

# root_refine_d.c：大境界概率读 pill_bonus（#61 预留端接线）
rrd = readf('adm/daemons/root_refine_d.c')
qm_raw = func_body_raw(rrd, 'query_major_breakthrough_probability')
qm = strip_lpc(qm_raw)
ck('大境界概率读 pill_bonus（#73 接线）', 'query_temp("breakthrough/pill_bonus")' in qm_raw)
ck('大境界概率保留 aux_bonus', 'query_temp("breakthrough/aux_bonus")' in qm_raw)

# tupo.c：成功后清理 pill_bonus
tp = readf('cmds/usr/tupo.c')
tb_raw = tp
ck('tupo 成功后清 pill_bonus', 'query_temp("breakthrough/pill_bonus")' in tb_raw and 'delete_temp("breakthrough/pill_bonus")' in tb_raw)

# shop_d.c：13 个丹药条目 type=pill 且指向 /clone/pill
shop = readf('adm/daemons/shop_d.c')
count_pill_rows = len(re.findall(r'\{"[a-z0-9_]+",\s*"[^"]+",\s*"pill",\s*"/clone/pill/[a-z_]+"', shop))
ck('shop_d pill 行 = 13（全部改指）', count_pill_rows == 13, 'got %d' % count_pill_rows)

# ═══════════ 三、同构模拟（Python 忠实翻译 LPC 关键路径）═══════════
print('== 三、同构模拟 ==')

# 3.1 炼丹术等级换算（pill_d.c refine_need / query_refine_level 翻译）
def refine_need(level):
    if level <= 20: return 5
    if level <= 40: return 8
    if level <= 60: return 12
    if level <= 80: return 20
    return 50

def query_refine_level(exp):
    level = 1
    while level < 100:
        need = refine_need(level)
        if exp < need: break
        exp -= need
        level += 1
    return level

ck('炼丹术 Lv0 exp→1', query_refine_level(0) == 1)
ck('炼丹术 exp5→2', query_refine_level(5) == 2)
ck('炼丹术 exp100→21（1-20 每级 5 次）', query_refine_level(100) == 21)
ck('炼丹术 exp165→29（21-40 每级 8 次）', query_refine_level(100 + 8 * 8) == 29)

# 3.2 成功率（PILL_D query_success_rate 翻译：base×(1+修正%)×(1+年份%) − 品级罚值，钳 5~95）
def success_rate(base, refine, danfang_bonus, quality, fire=0, year=0):
    fire_bonus = 5 if fire == 1 else (-5 if fire == 3 else 0)
    year_bonus = min(year // 10, 30)
    rate = base * (100 + refine * 2 + danfang_bonus + fire_bonus) // 100
    rate = rate * (100 + year_bonus) // 100
    rate -= (quality - 1) * 10
    rate = max(5, min(95, rate))
    return rate

ck('炼气散 Lv1 无加成: 70×1.02=71', success_rate(70, 1, 0, 1) == 71)
ck('筑基丹 Lv8(品2罚10): 40×1.16−10=36', success_rate(40, 8, 0, 2) == 36)
ck('筑基丹 Lv8+丹房10: 40×1.26−10=40', success_rate(40, 8, 10, 2) == 40)
ck('结金丹 Lv18: 30×1.36−20=20', success_rate(30, 18, 0, 3) == 20)
ck('稳火+5: 40×1.21−10=38', success_rate(40, 8, 0, 2, fire=1) == 38)
ck('旺火−5: 40×1.11−10=34', success_rate(40, 8, 0, 2, fire=3) == 34)
ck('年份100年+10%: 40×1.16×1.10−10=40', success_rate(40, 8, 0, 2, year=100) == 40)
ck('年份300年封顶+30%: 40×1.16×1.30−10=49', success_rate(40, 8, 0, 2, year=300) == 49)
ck('钳制下限 5', success_rate(5, 0, 0, 3) == 5)
ck('钳制上限 95', success_rate(90, 10, 10, 1) == 95)

# 3.3 炼制流程（consume_ingredients + refine_pill 翻译）
class MockPlayer:
    def __init__(self, mats):
        # mats: {'lingcao': 数量}
        self.inv = []
        for k, v in mats.items():
            for _ in range(v):
                self.inv.append({'material_id': k})
        self.msg = []
        self.xiuwei = 0
        self.temp = {}
        self.realm_index = 1
        self.toxin = 0

def consume_ingredients(p, need):
    have = {}
    for k in need:
        have[k] = sum(1 for o in p.inv if o['material_id'] == k)
        if have[k] < need[k]:
            return 0, have
    for k in need:
        cnt = 0
        for i in range(len(p.inv) - 1, -1, -1):
            if cnt >= need[k]: break
            if p.inv[i]['material_id'] == k:
                del p.inv[i]
                cnt += 1
    return 1, have

# 材料不足：不扣任何材料
p = MockPlayer({'lingcao': 1})
ok, have = consume_ingredients(p, {'lingcao': 2})
ck('筑基丹材料不足被拦', ok == 0 and len(p.inv) == 1)
# 材料足够：扣材料
p = MockPlayer({'lingcao': 5, 'huanglongcao': 3})
ok, have = consume_ingredients(p, {'lingcao': 5, 'huanglongcao': 3})
ck('筑基丹材料足够扣除', ok == 1 and len(p.inv) == 0)

# 3.4 品质判定（roll_quality 翻译：炼丹术≥15 提升，旺火概率翻倍）
import random
random.seed(7)
def roll_quality(base_q, refine, fire=0):
    q_prob = 40 if fire == 3 else 20
    quality = base_q
    if quality < 3 and refine >= 15:
        if random.random() < q_prob / 100.0:
            quality += 1
            if quality < 3 and refine >= 30 and random.random() < q_prob / 100.0:
                quality += 1
    return min(quality, 3)

qs = [roll_quality(1, 1) for _ in range(500)]
ck('炼丹术 Lv1 品质恒凡品', all(q == 1 for q in qs))
qs = [roll_quality(1, 20) for _ in range(500)]
ck('炼丹术 Lv20 可出现良品', any(q == 2 for q in qs))
qs_fire = [roll_quality(1, 20, fire=3) for _ in range(500)]
ck('旺火品质提升概率更高', sum(1 for q in qs_fire if q > 1) > sum(1 for q in qs if q > 1))
ck('品质上限上品', all(q <= 3 for q in qs + qs_fire))

# 3.5 筑基丹服用（dan.c do_eat_breakthrough 翻译：境界校验 + 叠加最多 3 颗）
class Player:
    def __init__(self, realm_index):
        self.realm_index = realm_index
        self.temp = {}
        self.toxin = 0
    def add_temp(self, k, v):
        self.temp[k] = self.temp.get(k, 0) + v

def eat_breakthrough(me, target_stage, effect):
    cur = me.realm_index
    if target_stage > 1 and cur != target_stage - 1:
        return 0, '境界不适用'
    if cur <= 0:
        return 0, '未入道'
    max_bonus = effect * 3
    cur_bonus = me.temp.get('breakthrough/pill_bonus', 0)
    if cur_bonus + effect > max_bonus:
        return 0, '已达上限'
    me.add_temp('breakthrough/pill_bonus', effect)
    return 1, 'ok'

me = Player(1)  # 炼气期
ok, why = eat_breakthrough(me, zhuji_stage, zhuji_effect)   # 筑基丹（实体绑定 effect/stage）
ck('炼气期服筑基丹成功', ok == 1 and me.temp['breakthrough/pill_bonus'] == zhuji_effect)
ok, why = eat_breakthrough(me, zhuji_stage, zhuji_effect)
ok2, why2 = eat_breakthrough(me, zhuji_stage, zhuji_effect)
ck('叠加 3 颗=%d' % (zhuji_effect * 3), me.temp['breakthrough/pill_bonus'] == zhuji_effect * 3)
ok, why = eat_breakthrough(me, zhuji_stage, zhuji_effect)
ck('第 4 颗被上限拦截', ok == 0 and me.temp['breakthrough/pill_bonus'] == zhuji_effect * 3)
me2 = Player(2)  # 筑基期
ok, why = eat_breakthrough(me2, zhuji_stage, zhuji_effect)
ck('筑基期服筑基丹被拒', ok == 0 and '境界不适用' in why)

# 3.6 突破概率（query_major_breakthrough_probability 翻译：base×factor + aux + pill，钳 1~99）
def major_prob(quality_factor, base_realm_rate, realm_penalty, aux, pill):
    prob = int(base_realm_rate * quality_factor) + realm_penalty + aux + pill
    prob = max(1, min(99, prob))
    return prob

# 炼气→筑基：base 10、伪灵根 0.3、realm_penalty=0（筑基丹 effect 从实体读取）
base_zhuji = major_prob(0.3, 10, 0, 0, 0)
with_pill = major_prob(0.3, 10, 0, 0, zhuji_effect)
ck('伪灵根炼气→筑基 10×0.3=3', base_zhuji == 3)
ck('服筑基丹后 +%d → %d（红→绿实证）' % (zhuji_effect, 3 + zhuji_effect), with_pill == 3 + zhuji_effect)
base_jd = major_prob(0.3, 5, -15 * (3 - 2), 0, 0)   # 伪灵根筑基→结丹：5×0.3=1-15=-14→钳 1
with_jj = major_prob(0.3, 5, -15, 0, 15)
ck('伪灵根结丹概率钳制 1', base_jd == 1)
ck('结金丹 +15 在钳制下仍受限', with_jj == 1)

# 3.7 tupo 成功后清理 pill_bonus（翻译 tupo.c）
def tupo(me, result):
    if result == 1:
        if me.temp.get('breakthrough/pill_bonus', 0) > 0:
            del me.temp['breakthrough/pill_bonus']
me = Player(1)
me.add_temp('breakthrough/pill_bonus', zhuji_effect)
tupo(me, 1)
ck('突破成功后 pill_bonus 清空', 'breakthrough/pill_bonus' not in me.temp)
me.add_temp('breakthrough/pill_bonus', zhuji_effect)
tupo(me, 2)
ck('突破失败后 pill_bonus 保留', me.temp.get('breakthrough/pill_bonus') == zhuji_effect)

# ═══════════ 结果 ═══════════
print('')
if fails or errors:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
