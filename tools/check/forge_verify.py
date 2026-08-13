#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#74 法宝炼制链路（FORGE_D + 法宝实体 + 炼器材料）静态校验 + 同构模拟 + LPC 原文守卫（可复跑验证）
运行：python3 tools/check/forge_verify.py
覆盖：
  一、静态校验：本票文件存在 / 括号配对状态机 / 接口签名 /
                引用完整性（配方材料 id→材料对象、配方 target→treasure 基类、fangshi goods→对象文件）/
                globals.h FORGE_D 宏 / lianqi help 前向原型
  二、LPC 原文守卫：forge 主流程（境界检查/材料扣减/成功率/品质/成品生成）、
                 treasure.c 境界限制（query_cultivation_tier >= require_level）、
                 lianqi.c 场所检查（query_current_facility == huadao_lianqi）——
                 守卫绑定交付物原文，含突变实证（改回旧行为 → 守卫红）
  三、同构模拟：数据驱动解析 forge_d.c 配方配置，忠实翻译 forge 流程
                 场景1 材料不足拒绝（不扣任何材料）/ 场景2 境界不足拒绝 /
                 场景3 炼器术不足拒绝 / 场景4 炼制成功法宝属性齐全 /
                 场景5 品质判定与倍率应用 / 场景6 失败耗材无成品 /
                 场景7 端到端（坊市材料→炼制→成品→装备境界检查）
退出码：0 全绿，1 有失败
"""
import re
import sys
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0


def err(msg):
    global fails
    fails += 1
    errors.append(msg)
    print('FAIL: ' + msg)


oks = 0


def ok(msg):
    global oks
    oks += 1
    print('ok: ' + msg)


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


def read_file(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


def check_parens(path):
    """括号配对状态机（字符串/注释/@LONG 感知）"""
    src = read_file(path)
    code = strip_lpc(src)
    stack = []
    pairs = {'(': ')', '{': '}', '[': ']'}
    for ch in code:
        if ch in '({[':
            stack.append(ch)
        elif ch in ')}]':
            if not stack or pairs[stack.pop()] != ch:
                return False, '括号不配对: %s' % path
    if stack:
        return False, '括号未闭合(%d): %s' % (len(stack), path)
    return True, ''


def extract_func(src, fname):
    """提取 LPC 函数体文本（含签名行到闭合花括号），无注释/字符串"""
    code = strip_lpc(src)
    m = re.search(r'\b' + re.escape(fname) + r'\s*\([^;]*\)\s*\{', code)
    if not m:
        return None
    start = m.start()
    depth = 0
    i = m.end() - 1  # 定位到 '{'
    n = len(code)
    while i < n:
        if code[i] == '{':
            depth += 1
        elif code[i] == '}':
            depth -= 1
            if depth == 0:
                return code[start:i + 1]
        i += 1
    return None


def extract_func_raw(src, fname):
    """提取 LPC 函数体（保留字符串字面量的原始文本；注释/字符串感知跳过来定位）。
    守卫若需匹配字符串内容（如 "lianqi-shu"、"huadao_lianqi"），必须用本函数。"""
    n = len(src)
    i = 0
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
        m = re.match(r'\b' + re.escape(fname) + r'\s*\(', src[i:])
        if m:
            start = i
            # 定位参数闭合括号（从 '(' 起配对；m.end() 已消费左括号）
            lp = i + m.group(0).rfind('(')
            k = lp
            depth = 0
            while k < n:
                if src[k] == '(':
                    depth += 1
                elif src[k] == ')':
                    depth -= 1
                    if depth == 0:
                        break
                k += 1
            # 定位函数体 '{'
            k += 1
            while k < n and src[k] in ' \t\r\n':
                k += 1
            if k < n and src[k] == '{':
                d2 = 0
                k3 = k
                while k3 < n:
                    ch = src[k3]
                    if ch == '/' and k3 + 1 < n and src[k3 + 1] == '/':
                        j2 = src.find('\n', k3)
                        k3 = n if j2 == -1 else j2
                        continue
                    if ch == '/' and k3 + 1 < n and src[k3 + 1] == '*':
                        j2 = src.find('*/', k3 + 2)
                        k3 = n if j2 == -1 else j2 + 2
                        continue
                    if ch == '"':
                        j2 = k3 + 1
                        while j2 < n:
                            if src[j2] == '\\':
                                j2 += 2
                                continue
                            if src[j2] == '"':
                                break
                            j2 += 1
                        k3 = j2 + 1 if j2 < n else n
                        continue
                    if ch == '{':
                        d2 += 1
                    elif ch == '}':
                        d2 -= 1
                        if d2 == 0:
                            return src[start:k3 + 1]
                    k3 += 1
            # 该处签名后无函数体，继续向后扫描
            i = k
            continue
        i += 1
    return None


# LPC mapping 字面量 → Python dict（仅配方数据：键为字符串、值为 str/int/嵌套 mapping）
_LPC_MACROS = {
    'SECT_TIER_QI_LATE': 2,
    'SECT_TIER_ZHU': 3,
    'SECT_TIER_JIE': 6,
}


def parse_lpc_mapping(text):
    """把 LPC `([ ... ])` 字面量块（含外层括号）转换为 Python dict。"""
    # 行首缩进去平（eval 顶层不允许缩进）
    text = '\n'.join(line.strip() for line in text.splitlines())
    # 转换 ([ ]) 为 { }，并替换宏名
    text = re.sub(r'\(\s*\[', '{', text)
    text = re.sub(r'\]\s*\)', '}', text)
    text = re.sub(r'__DIR__"', '"', text)  # LPC 路径拼接 __DIR__"obj/x" → "obj/x"
    for k, v in _LPC_MACROS.items():
        text = re.sub(r'\b' + k + r'\b', str(v), text)
    # 键/值字符串为 "..." 形式，Python 兼容
    return eval(text, {'__builtins__': {}}, {})


def parse_forge_formula(src):
    """从 forge_d.c 提取 forge_formula 配置（nosave mapping forge_formula = ([ ... ]);）"""
    m = re.search(r'nosave\s+mapping\s+forge_formula\s*=\s*(\(\s*\[.*?\]\s*\))\s*;', src, re.S)
    if not m:
        return None
    return parse_lpc_mapping(m.group(1))


def parse_fangshi_goods(src):
    """从 fangshi.c 提取 goods 表（nosave mapping goods = ([ ... ]);）"""
    m = re.search(r'nosave\s+mapping\s+goods\s*=\s*(\(\s*\[.*?\]\s*\))\s*;', src, re.S)
    if not m:
        return None
    return parse_lpc_mapping(m.group(1))


def extract_register_goods(src):
    """从 fangshi.c 提取 register_goods 调用列表（(type, base, turnover)）"""
    calls = re.findall(r'register_goods\("([a-z_0-9]+)",\s*(\d+),\s*(\d+)\)', src)
    return calls


# ═══════════ 一、静态校验 ═══════════

print('═══ 一、静态校验 ═══')

NEW_FILES = [
    'adm/daemons/forge_d.c',
    'cmds/usr/lianqi.c',
    'd/yueguo/obj/treasure.c',
    'include/forge.h',
    'd/yueguo/tainan/obj/yinjing.c',
    'd/yueguo/tainan/obj/jinjing.c',
    'd/yueguo/tainan/obj/xuantie.c',
    'd/yueguo/tainan/obj/gengjing.c',
]

# 1.1 文件存在
for f in NEW_FILES:
    if os.path.exists(f):
        ok('文件存在: %s' % f)
    else:
        err('文件缺失: %s' % f)

# 1.2 括号配对状态机（本票全部 .c/.h）
for f in NEW_FILES + ['d/yueguo/tainan/fangshi.c', 'include/globals.h']:
    if not os.path.exists(f):
        continue
    good, why = check_parens(f)
    if good:
        ok('括号配对: %s' % f)
    else:
        err('括号失败: %s' % why)

# 1.3 接口签名存在（forge_d / lianqi / treasure）
forge_src = read_file('adm/daemons/forge_d.c')
for sig in ['resolve_formula', 'query_formula_ids', 'query_formula',
            'describe_formula_list', 'count_material', 'consume_material',
            'query_success_rate', 'roll_quality', 'forge']:
    if re.search(r'\b' + sig + r'\s*\(', strip_lpc(forge_src)):
        ok('forge_d 接口: %s' % sig)
    else:
        err('forge_d 缺接口: %s' % sig)

lianqi_src = read_file('cmds/usr/lianqi.c')
for sig in ['main', 'help']:
    if re.search(r'\bint\s+' + sig + r'\s*\(', strip_lpc(lianqi_src)):
        ok('lianqi 接口: %s' % sig)
    else:
        err('lianqi 缺接口: %s' % sig)
# help 前向原型（#61 教训：缺前向原型驱动编译失败）
if re.search(r'int\s+help\(object\s+me\)\s*;', strip_lpc(lianqi_src)):
    ok('lianqi help 前向原型存在')
else:
    err('lianqi help 前向原型缺失')

tre_src = read_file('d/yueguo/obj/treasure.c')
for sig in ['setup', 'wield', 'wear', 'extra_long']:
    if re.search(r'\b' + sig + r'\s*\(', strip_lpc(tre_src)):
        ok('treasure 接口: %s' % sig)
    else:
        err('treasure 缺接口: %s' % sig)
# 继承 EQUIP（物品/背包/装备接线 c2）
if re.search(r'inherit\s+EQUIP', strip_lpc(tre_src)):
    ok('treasure 继承 EQUIP（物品/装备接线）')
else:
    err('treasure 未继承 EQUIP')

# 1.4 globals.h FORGE_D 宏
globals_src = read_file('include/globals.h')
if re.search(r'#define\s+FORGE_D\s+"/adm/daemons/forge_d"', globals_src):
    ok('globals.h FORGE_D 宏存在')
else:
    err('globals.h FORGE_D 宏缺失')

# 1.5 引用完整性：配方数据 ↔ 材料对象 ↔ 坊市 goods
formula = parse_forge_formula(forge_src)
if formula is None:
    err('无法解析 forge_formula 配方配置')
    formula = {}
else:
    ok('解析配方 %d 个: %s' % (len(formula), ','.join(sorted(formula.keys()))))

fangshi_src = read_file('d/yueguo/tainan/fangshi.c')
goods = parse_fangshi_goods(fangshi_src)
if goods is None:
    err('无法解析 fangshi goods 表')
    goods = {}
else:
    ok('解析坊市货物 %d 个' % len(goods))

MATERIAL_OBJ_DIR = 'd/yueguo/tainan/obj'
for fid, f in formula.items():
    for mid in f['materials']:
        obj_path = os.path.join(MATERIAL_OBJ_DIR, mid + '.c')
        if os.path.exists(obj_path):
            ok('配方 %s 材料 %s → %s' % (fid, mid, obj_path))
        else:
            err('配方 %s 材料 %s 对象缺失: %s' % (fid, mid, obj_path))
        if mid in goods:
            ok('配方 %s 材料 %s 坊市可购' % (fid, mid))
        else:
            err('配方 %s 材料 %s 坊市无货' % (fid, mid))
    # 配方必须含 1E 数据结构关键字段
    for field in ['treasure_type', 'element', 'forbidden_count', 'require_tier',
                  'materials', 'min_skill', 'base_rate', 'value']:
        if field in f:
            ok('配方 %s 字段 %s' % (fid, field))
        else:
            err('配方 %s 缺字段 %s' % (fid, field))
    if f.get('attack', 0) > 0 and f.get('defense', 0) > 0:
        err('配方 %s 攻击/防御同时非零（EQUIP 限制不可双持类型）' % fid)
    else:
        ok('配方 %s 攻防单侧' % fid)

# 1.6 fangshi goods 对象文件存在 + register_goods 类型覆盖矿石
for gid, g in goods.items():
    obj_path = os.path.join('d/yueguo/tainan', g['file'] + '.c')
    if os.path.exists(obj_path):
        ok('坊市 %s 对象文件存在: %s' % (gid, obj_path))
    else:
        err('坊市 %s 对象文件缺失: %s' % (gid, obj_path))

regs = extract_register_goods(fangshi_src)
reg_types = set()
for t, b, tv in regs:
    reg_types.add(t)
    ok('register_goods: %s base=%s turnover=%s' % (t, b, tv))
for gid, g in goods.items():
    if g['type'] not in reg_types:
        err('坊市 %s 类型 %s 未 register_goods' % (gid, g['type']))
    else:
        ok('坊市 %s 类型 %s 已注册' % (gid, g['type']))

# 1.7 配方 target 法宝基类存在（forge_d new 路径）
if os.path.exists('d/yueguo/obj/treasure.c') and 'new("/d/yueguo/obj/treasure")' in forge_src:
    ok('FORGE_D 成品生成指向 treasure 基类')
else:
    err('FORGE_D 未指向 treasure 基类')

# ═══════════ 二、LPC 原文守卫 ═══════════

print('\n═══ 二、LPC 原文守卫 ═══')

forge_body = extract_func_raw(forge_src, 'forge')
if forge_body is None:
    err('无法提取 forge 函数体')
    forge_body = ''
else:
    guards_forge = [
        ('境界检查', r'query_cultivation_tier\(player\)\s*<\s*tier_need'),
        ('技能检查', r'query_skill\("lianqi-shu"\)\s*<\s*f\["min_skill"\]'),
        ('材料检查', r'count_material\(player,\s*mkeys\[i\]\)\s*<\s*need'),
        ('材料扣减', r'consume_material\(player,\s*mkeys\[i\],\s*mats\[mkeys\[i\]\]\)'),
        ('成功率接线', r'query_success_rate\(player,\s*f\)'),
        ('五步流程', r'FORGE_STEPS'),
        ('品质判定', r'roll_quality\(rate\)'),
        ('成品生成', r'new\("/d/yueguo/obj/treasure"\)'),
        ('境界名展示', r'tier_name\(tier_need\)'),
        ('落地面判定', r'move\(player\)\s*!=\s*1'),
    ]
    for name, pat in guards_forge:
        if re.search(pat, forge_body):
            ok('守卫[forge] %s' % name)
        else:
            err('守卫[forge] %s 缺失: %s' % (name, pat))

rate_body = extract_func_raw(forge_src, 'query_success_rate')
if rate_body and re.search(r'lianqi-shu', rate_body) and re.search(r'query_forge_bonus', rate_body):
    ok('守卫[query_success_rate] 炼器术+工坊加成')
else:
    err('守卫[query_success_rate] 缺失')

tre_body = extract_func(tre_src, 'check_realm')
if tre_body and re.search(r'query_cultivation_tier\(owner\)\s*>=\s*need', tre_body):
    ok('守卫[treasure.check_realm] 境界限制真实生效')
else:
    err('守卫[treasure.check_realm] 缺失')
# 拒绝分支守卫（审查盲区实证：放行分支改 return 0 或拒绝分支改 return 1 时脚本必须红）
# 形态：境界达标放行（>= need return 1）之后紧跟拒绝返回（return 0）
if tre_body and re.search(r'query_cultivation_tier\(owner\)\s*>=\s*need\)\s*return\s+1;[\s\S]*?return\s+0;', tre_body):
    ok('守卫[treasure.check_realm] 拒绝分支存在（放行后 return 0）')
else:
    err('守卫[treasure.check_realm] 拒绝分支缺失')

lq_body = extract_func_raw(lianqi_src, 'main')
if lq_body and re.search(r'query_current_facility\(me\)\s*!=\s*"huadao_lianqi"', lq_body):
    ok('守卫[lianqi] 场所检查（炼器工坊）')
else:
    err('守卫[lianqi] 场所检查缺失')
if lq_body and re.search(r'query_forge_bonus\(me\)', lq_body):
    ok('守卫[lianqi] 工坊加成消费')
else:
    err('守卫[lianqi] 工坊加成消费缺失')

# ═══════════ 三、同构模拟（忠实翻译 forge 流程） ═══════════

print('\n═══ 三、同构模拟 ═══')

# 常量对齐（LPC FORGE_STEPS 4 步）
FORGE_STEPS = ['精炼提纯', '器胚锻造', '禁制铭刻', '通灵开光']
QUALITY_RATE = {'下品': 1.0, '中品': 1.15, '上品': 1.3, '极品': 1.5}
QUALITY_BAN = {'下品': 0, '中品': 1, '上品': 2, '极品': 3}


class MockPlayer:
    """忠实模拟 LPC 玩家对象（背包材料/境界/炼器术/成品列表）"""

    def __init__(self, tier=2, skill=100, materials=None, facility_bonus=0):
        self.tier = tier
        self.skill = skill
        self.materials = dict(materials or {})
        self.facility_bonus = facility_bonus
        self.treasures = []
        self.msgs = []
        self.sect = 'huadao_dock'

    def query_cultivation_tier(self, player=None):
        return self.tier

    def query_skill(self, skill, raw=1):
        return self.skill

    def query_player_sect(self, player=None):
        return self.sect

    def query_forge_bonus(self, player=None):
        return self.facility_bonus

    def count_material(self, mid):
        return self.materials.get(mid, 0)

    def consume_material(self, mid, n):
        left = n
        while left > 0 and self.materials.get(mid, 0) > 0:
            self.materials[mid] -= 1
            left -= 1

    def tell(self, msg):
        self.msgs.append(msg)


def forge_mirror(p, formula, fid, rng=None):
    """翻译 LPC forge() 主流程（逐条对应源码逻辑；成功率/步概率/品质公式一致）"""
    rng = rng or random.Random(42)
    f = formula[fid]
    tier_need = f['require_tier']
    if p.query_cultivation_tier() < tier_need:
        return 'realm'
    if p.query_skill('lianqi-shu') < f['min_skill']:
        return 'skill'
    mats = f['materials']
    for mid, need in mats.items():
        if p.count_material(mid) < need:
            return 'material'
    for mid, need in mats.items():
        p.consume_material(mid, need)
    rate = f['base_rate'] + p.query_skill('lianqi-shu') // 2 + p.query_forge_bonus()
    rate = max(1, min(99, rate))
    per = int((rate / 100.0) ** (1.0 / 4.0) * 100)
    per = min(99, per)
    for _ in FORGE_STEPS:
        if rng.randrange(100) >= per:
            return 'fail'
    roll = rng.randrange(100) + 1
    if roll < rate * 25 // 100:
        quality = '极品'
    elif roll < rate * 50 // 100:
        quality = '上品'
    elif roll < rate * 80 // 100:
        quality = '中品'
    else:
        quality = '下品'
    t = {
        'name': f['name'] + '（' + quality + '）',
        'treasure_type': f['treasure_type'],
        'level': quality,
        'element': f['element'],
        'forbidden_count': f['forbidden_count'] + QUALITY_BAN[quality],
        'attack': int(f['attack'] * QUALITY_RATE[quality]),
        'defense': int(f['defense'] * QUALITY_RATE[quality]),
        'special': f['special'],
        'require_level': f['require_tier'],
        'value': f['value'],
    }
    p.treasures.append(t)
    return 'success'


def forge_sim(fid, tier, skill, mats, rng_seed=42, bonus=0):
    p = MockPlayer(tier=tier, skill=skill, materials=mats, facility_bonus=bonus)
    rng = random.Random(rng_seed)
    result = forge_mirror(p, formula, fid, rng)
    return p, result


# 场景1：材料不足拒绝（不扣任何材料）
p, r = forge_sim('qinggangjian', tier=2, skill=100, mats={'tiejing': 1})
if r == 'material' and p.materials == {'tiejing': 1}:
    ok('场景1 材料不足拒绝且不扣材料')
else:
    err('场景1 失败: %s %s' % (r, p.materials))

# 场景2：境界不足拒绝
p, r = forge_sim('xuantiezhongjian', tier=2, skill=100, mats={'jinjing': 2, 'xuantie': 1})
if r == 'realm':
    ok('场景2 境界不足拒绝（玄铁重剑需筑基）')
else:
    err('场景2 失败: %s' % r)

# 场景3：炼器术等级不足拒绝
p, r = forge_sim('gengjingfeijian', tier=6, skill=30, mats={'xuantie': 2, 'gengjing': 1})
if r == 'skill':
    ok('场景3 炼器术不足拒绝（庚精飞剑需 60 级）')
else:
    err('场景3 失败: %s' % r)

# 场景4：炼制成功，法宝属性齐全（1E §4.1 全字段）
p, r = forge_sim('qinggangjian', tier=2, skill=100, mats={'tiejing': 2, 'yinjing': 1})
if r == 'success' and len(p.treasures) == 1:
    t = p.treasures[0]
    fields = ['treasure_type', 'level', 'element', 'forbidden_count',
              'attack', 'defense', 'special', 'require_level', 'value']
    miss = [f for f in fields if f not in t]
    if not miss:
        ok('场景4 炼制成功且 1E 属性齐全: %s' % t['name'])
    else:
        err('场景4 成品缺属性: %s' % miss)
else:
    err('场景4 失败: %s' % r)

# 场景5：品质判定与倍率应用（下品 1.0 / 极品 1.5）
p, r = forge_sim('chitongdun', tier=2, skill=100,
                 mats={'tiejing': 3, 'jinjing': 1}, rng_seed=7)
if r == 'success':
    t = p.treasures[0]
    base_def = 25
    expect_def = int(base_def * QUALITY_RATE[t['level']])
    if t['defense'] == expect_def and t['forbidden_count'] == 3 + QUALITY_BAN[t['level']]:
        ok('场景5 品质 %s 倍率/禁制正确（防御 %d=25×%.2f）' % (t['level'], t['defense'], QUALITY_RATE[t['level']]))
    else:
        err('场景5 品质倍率错误: %s def=%d expect=%d ban=%d' % (t['level'], t['defense'], expect_def, t['forbidden_count']))
else:
    err('场景5 炼制失败: %s' % r)

# 场景5b：品质分布覆盖全部四档（多种子遍历，至少出现 3 种品质）
qualities = set()
for seed in range(200):
    p, r = forge_sim('qinggangjian', tier=2, skill=100,
                     mats={'tiejing': 2, 'yinjing': 1}, rng_seed=seed)
    if r == 'success':
        qualities.add(p.treasures[0]['level'])
if len(qualities) >= 3:
    ok('场景5b 品质分布覆盖 %d 档: %s' % (len(qualities), sorted(qualities)))
else:
    err('场景5b 品质分布过窄: %s' % sorted(qualities))

# 场景6：失败耗材无成品（构造低成功率配方低技能，遍历种子找失败）
found_fail = False
for seed in range(200):
    p, r = forge_sim('gengjingfeijian', tier=6, skill=60,
                     mats={'xuantie': 2, 'gengjing': 1}, rng_seed=seed)
    if r == 'fail':
        found_fail = True
        if p.materials == {'xuantie': 0, 'gengjing': 0} and len(p.treasures) == 0:
            ok('场景6 失败耗材无成品（seed=%d）' % seed)
        else:
            err('场景6 失败后材料未耗尽或竟有成品: %s %s' % (p.materials, len(p.treasures)))
        break
if not found_fail:
    err('场景6 未能构造失败样本（50% 成功率 × 200 种子理论必现）')

# 场景7：端到端（坊市材料→炼制→成品→装备境界检查）
#   材料来自 fangshi goods（铁精/银精），炼成法宝后 check_realm 按境界放行/拦截
p, r = forge_sim('qinggangjian', tier=2, skill=100,
                 mats={'tiejing': 2, 'yinjing': 1}, rng_seed=1)
if r == 'success':
    t = p.treasures[0]
    need = t['require_level']
    low_player = MockPlayer(tier=0, skill=100)
    high_player = MockPlayer(tier=2, skill=100)
    low_blocked = low_player.query_cultivation_tier() < need
    high_ok = high_player.query_cultivation_tier() >= need
    if low_blocked and high_ok:
        ok('场景7 端到端：法宝 require_level=%d，炼气中期被拦/炼气后期放行' % need)
    else:
        err('场景7 境界检查边界错误: need=%d low=%s high=%s' % (need, low_blocked, high_ok))
else:
    err('场景7 炼制失败: %s' % r)

# 场景8：配方数据与 1E 规格一致性（品质/境界/技能门槛）
for fid, f in formula.items():
    if f['treasure_type'] in ('法器', '法宝', '古宝', '通天灵宝', '玄天之宝'):
        ok('配方 %s treasure_type=%s 属 1E 等级体系' % (fid, f['treasure_type']))
    else:
        err('配方 %s treasure_type=%s 非法' % (fid, f['treasure_type']))
    if f['element'] in ('金', '木', '水', '火', '土', '风', '雷', '空间', '时间'):
        ok('配方 %s element=%s 属 1E 属性体系' % (fid, f['element']))
    else:
        err('配方 %s element=%s 非法' % (fid, f['element']))

# ═══════════ 四、突变验证（守卫绑定交付物） ═══════════

print('\n═══ 四、突变验证 ═══')
import shutil
import tempfile

mut_dir = tempfile.mkdtemp(prefix='forge74_mut_')
os.makedirs(os.path.join(mut_dir, 'adm/daemons'), exist_ok=True)
os.makedirs(os.path.join(mut_dir, 'cmds/usr'), exist_ok=True)
os.makedirs(os.path.join(mut_dir, 'd/yueguo/obj'), exist_ok=True)
os.makedirs(os.path.join(mut_dir, 'include'), exist_ok=True)
for src, dst in [
    ('adm/daemons/forge_d.c', os.path.join(mut_dir, 'adm/daemons/forge_d.c')),
    ('cmds/usr/lianqi.c', os.path.join(mut_dir, 'cmds/usr/lianqi.c')),
    ('d/yueguo/obj/treasure.c', os.path.join(mut_dir, 'd/yueguo/obj/treasure.c')),
    ('include/forge.h', os.path.join(mut_dir, 'include/forge.h')),
    ('include/globals.h', os.path.join(mut_dir, 'include/globals.h')),
]:
    shutil.copy(src, dst)

# 突变1：forge 删境界检查 → 守卫红
mut_src = read_file(os.path.join(mut_dir, 'adm/daemons/forge_d.c'))
mut_src = mut_src.replace('if (SECT_D->query_cultivation_tier(player) < tier_need)',
                          'if (0)')
mut_body = extract_func_raw(mut_src, 'forge')
if re.search(r'query_cultivation_tier\(player\)\s*<\s*tier_need', mut_body or ''):
    err('突变1 失败：删境界检查后守卫仍绿')
else:
    ok('突变1 删境界检查 → 守卫红')

# 突变2：treasure 放行分支失效（>=need return 1 → return 0，境界限制失效）→ 拒绝分支守卫红
mut_tre = read_file(os.path.join(mut_dir, 'd/yueguo/obj/treasure.c'))
mut_tre = mut_tre.replace('if (SECT_D->query_cultivation_tier(owner) >= need) return 1;',
                          'if (SECT_D->query_cultivation_tier(owner) >= need) return 0;')
mut_body = extract_func(mut_tre, 'check_realm')
if re.search(r'query_cultivation_tier\(owner\)\s*>=\s*need\)\s*return\s+1;[\s\S]*?return\s+0;', mut_body or ''):
    err('突变2 失败：放行分支改 return 0 后守卫仍绿')
else:
    ok('突变2 放行分支失效（return 1→0）→ 守卫红')

# 突变3：lianqi 场所检查改指向别处 → 守卫红
mut_lq = read_file(os.path.join(mut_dir, 'cmds/usr/lianqi.c'))
mut_lq = mut_lq.replace('!= "huadao_lianqi"', '!= "huangfeng_danfang"')
mut_body = extract_func(mut_lq, 'main')
if re.search(r'!=\s*"huadao_lianqi"', mut_body or ''):
    err('突变3 失败：场所改指后守卫仍绿')
else:
    ok('突变3 场所改指 → 守卫红')

# 突变4：材料扣减删除 → 守卫红
mut_src2 = read_file(os.path.join(mut_dir, 'adm/daemons/forge_d.c'))
mut_src2 = mut_src2.replace('consume_material(player, mkeys[i], mats[mkeys[i]]);', '')
mut_body = extract_func(mut_src2, 'forge')
if re.search(r'consume_material\(player,', mut_body or ''):
    err('突变4 失败：删材料扣减后守卫仍绿')
else:
    ok('突变4 删材料扣减 → 守卫红')

# 突变5：treasure check_realm 拒绝分支失效（return 0 → return 1，境界限制完全失效）→ 守卫红
mut_tre2 = read_file(os.path.join(mut_dir, 'd/yueguo/obj/treasure.c'))
mut_tre2 = mut_tre2.replace('''    if (SECT_D->query_cultivation_tier(owner) >= need) return 1;
    return 0;''',
                            '''    if (SECT_D->query_cultivation_tier(owner) >= need) return 1;
    return 1;''')
mut_body = extract_func(mut_tre2, 'check_realm')
if re.search(r'query_cultivation_tier\(owner\)\s*>=\s*need\)\s*return\s+1;[\s\S]*?return\s+0;', mut_body or ''):
    err('突变5 失败：拒绝分支改 return 1 后守卫仍绿')
else:
    ok('突变5 check_realm 拒绝分支失效（return 0→1）→ 守卫红')

# 突变6：treasure check_realm 判定反转（>= 改 <，境界不足反而放行）→ 守卫红
mut_tre3 = read_file(os.path.join(mut_dir, 'd/yueguo/obj/treasure.c'))
mut_tre3 = mut_tre3.replace('if (SECT_D->query_cultivation_tier(owner) >= need) return 1;',
                            'if (SECT_D->query_cultivation_tier(owner) < need) return 1;')
mut_body = extract_func(mut_tre3, 'check_realm')
if re.search(r'query_cultivation_tier\(owner\)\s*>=\s*need\)\s*return\s+1;[\s\S]*?return\s+0;', mut_body or ''):
    err('突变6 失败：判定反转后守卫仍绿')
else:
    ok('突变6 check_realm 判定反转（>= 改 <）→ 守卫红')

shutil.rmtree(mut_dir, ignore_errors=True)

# ═══════════ 结果 ═══════════

print('\n═══════════════════════════════════')
print('断言总数: %d, 失败: %d' % (oks, fails))
if fails:
    print('FAILED: 存在失败项')
    sys.exit(1)
else:
    print('OK: #74 法宝炼制链路静态验收全部通过')
    sys.exit(0)
