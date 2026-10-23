#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#79 体修路径（淬体/肉身搏击/体剑双修）静态校验 + 原文守卫 + 突变实证 + 同构模拟

运行：python3 tools/check/body_refine_verify.py

覆盖：
  一、静态校验：交付文件存在 / 括号配对 / body.h 守卫与规范 / 契约对齐
                （BODY_D 接口、damage.h 读 body_refine/fist|sword|defense、attribute.c 读
                 body_refine/qi、poison.c 读 body_refine/stage、attack.c 毒体带毒）
                / COMBAT_SYSTEM §三 四段路径文档对齐 / 门派传承接线（巨剑门三功法 + 鬼灵门毒体）
  二、LPC 原文守卫：query_stage 阶梯（每段需上一段 50 级）、refresh 阶段钳制与派生换算、
                train_body（阶段/上限/消耗/收益/夹取）、damage.h 空手与重剑接线、defatk 叠加、
                attribute.c 气血接线、三头六臂 exert 加减对称与时长/数值封顶
  三、突变验证：删阶梯/删钳制/不扣内力/空手加成错位/防御不叠加/法相只加不还 → 守卫必须变红
  四、同构模拟：阶段推导（各功法组合）/ 上限表 / 淬体收益与夹取 / 加成换算 /
                空手·重剑·普通兵器伤害差异 / 固定减伤钳制 / 三头六臂曲线 / 毒体抗毒

退出码：0 全绿，1 有失败
"""
import re
import sys
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0

BODY_H = 'include/body.h'
BODY_D = 'adm/daemons/body_d.c'
CUINA = 'cmds/usr/cuina.c'
DAMAGE_H = 'include/combat/damage.h'
ATTRIBUTE = 'feature/attribute.c'
POISON = 'feature/poison.c'
ATTACK = 'feature/attack.c'
SECT_D = 'adm/daemons/sect_d.c'
GLOBALS_H = 'include/globals.h'
DOC = '.knowledge/combat/COMBAT_SYSTEM.md'

SKILLS = {
    'xiangjia-gong': ('象甲功', '基础炼体', 'iron-cloth'),
    'fansheng-zhenmo-gong': ('梵圣真魔功', '法体双修', 'force'),
    'niepan-shengti': ('涅槃圣体', '极限炼体', 'iron-cloth'),
    'wandu-hunyuan-shen': ('万毒混元身', '毒体修炼', 'iron-cloth'),
}
EXERT = 'kungfu/skill/fansheng-zhenmo-gong/santou_liubi.c'


# ═══════════ 工具 ═══════════

def strip_lpc(src):
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
                return False, 'mismatch at %r' % c
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


def func_body_raw(src, fname):
    m = re.search(r'\b%s\s*\([^)]*\)\s*\{' % fname, src)
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


def hdr_int(src, name):
    m = re.search(r'#define\s+%s\s+(-?\d+)' % name, src)
    return int(m.group(1)) if m else None


def hdr_str(src, name):
    m = re.search(r'#define\s+%s\s+"([^"]*)"' % name, src)
    return m.group(1) if m else None


# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

delivery = [BODY_H, BODY_D, CUINA, EXERT, 'kungfu/skill/fansheng-zhenmo-gong.c']
delivery += ['kungfu/skill/%s.c' % s for s in SKILLS]
for p in delivery:
    ck('exists %s' % p, os.path.isfile(p))

for p in delivery + [DAMAGE_H, ATTRIBUTE, POISON, ATTACK, SECT_D]:
    ok, detail = check_parens(p)
    ck('parens %s' % p, ok, detail)

body_h = readf(BODY_H)
ck('body.h 有包含守卫', '#ifndef __BODY_H__' in body_h and '#define __BODY_H__' in body_h)
ck('body.h 无 tab 缩进', '\t' not in body_h)
for const in ['BODY_STAGE_NONE', 'BODY_STAGE_BASIC', 'BODY_STAGE_DUAL', 'BODY_STAGE_ULTIMATE',
              'BODY_STAGE_POISON', 'BODY_PREREQ_SKILL', 'BODY_CAP_BASIC', 'BODY_CAP_DUAL',
              'BODY_CAP_ULTIMATE', 'BODY_CAP_POISON', 'BODY_QI_PER_POINT', 'BODY_DEF_DIVISOR',
              'BODY_FIST_DIVISOR', 'BODY_SWORD_DIVISOR', 'BODY_COST_NEILI', 'BODY_COST_JING',
              'BODY_GAIN_BASE', 'BODY_GAIN_PER_LEVEL', 'BODY_GAIN_MAX', 'BODY_POISON_RESIST',
              'BODY_POISON_ID', 'BODY_FAXIANG_BUFF', 'BODY_FAXIANG_NEILI', 'BODY_FAXIANG_MAX_TIME']:
    ck('body.h 常量 %s' % const, re.search(r'#define\s+%s\b' % const, body_h) is not None)

for macro, sid in [('BODY_SKILL_BASIC', 'xiangjia-gong'), ('BODY_SKILL_DUAL', 'fansheng-zhenmo-gong'),
                   ('BODY_SKILL_ULTIMATE', 'niepan-shengti'), ('BODY_SKILL_POISON', 'wandu-hunyuan-shen')]:
    ck('body.h %s = %s' % (macro, sid), hdr_str(body_h, macro) == sid, 'got %s' % hdr_str(body_h, macro))

glob = readf(GLOBALS_H)
ck('globals.h 定义 BODY_D 宏', re.search(r'#define\s+BODY_D\s+"/adm/daemons/body_d"', glob) is not None)

body_d = readf(BODY_D)
body_code = strip_lpc(body_d)
ck('BODY_D 无 tab 缩进', '\t' not in body_d)
for fn in ['query_stage_cap', 'query_stage_name', 'query_stage', 'query_max_body_skill',
           'query_points', 'refresh', 'query_body_info', 'train_body']:
    ck('BODY_D 定义 %s' % fn, re.search(r'\b%s\s*\([^;]*\)\s*\{' % fn, body_code) is not None)

cuina = readf(CUINA)
ck('cuina 调 BODY_D->train_body', 'BODY_D->train_body(me)' in cuina)
ck('cuina 调 BODY_D->query_body_info', 'BODY_D->query_body_info(me)' in cuina)
ck('cuina 有 help', re.search(r'int\s+help\s*\(object me\)', cuina) is not None)

# ---- 消费方接线 ----
dh = readf(DAMAGE_H)
ck('damage.h 空手吃体修拳劲', re.search(r'if\s*\(\s*!objectp\(weapon\)\s*\)\s*\n?\s*damage\s*\+=\s*\(int\)me->query\("body_refine/fist"\)', dh) is not None)
ck('damage.h 重剑（体剑双修）吃半额', re.search(r'martial_skill\s*==\s*"zhongjian-jianfa"\s*\)\s*\n?\s*damage\s*\+=\s*\(int\)me->query\("body_refine/sword"\)', dh) is not None)
ck('damage.h 固定减伤叠加体修防御', re.search(r'defatk\s*=\s*victim->query_temp\("apply/defense_attack"\)\s*\+\s*\(int\)victim->query\("body_refine/defense"\)', dh) is not None)

attr = readf(ATTRIBUTE)
ck('attribute.c query_max_qi 读 body_refine/qi',
   re.search(r'query_max_qi[\s\S]{0,4000}?body_refine/qi', attr) is not None)
ck('attribute.c 增益在 apply/max_qi 之前累加',
   attr.find('body_refine/qi') < attr.find('apply/max_qi'))

poison = readf(POISON)
ck('poison.c 毒体抗毒（stage 判定）', re.search(r'body_refine/stage"\s*\)\s*>=\s*BODY_STAGE_POISON', poison) is not None)
ck('poison.c 抗毒量取 body.h 常量', 'BODY_POISON_RESIST' in poison)

atk = readf(ATTACK)
ck('attack.c 毒体开战时带毒', 'F_POISON->empoison(this_object(), BODY_POISON_ID' in atk)

# ---- 文档对齐：COMBAT_SYSTEM §三 四段路径 ----
doc = readf(DOC)
print('COMBAT_SYSTEM §三 文档对齐:')
for sid, (cname, stage, slot) in SKILLS.items():
    path = 'kungfu/skill/%s.c' % sid
    src = readf(path)
    ck('%s 继承 %s' % (path, 'FORCE' if slot == 'force' else 'SKILL'),
       ('inherit FORCE;' if slot == 'force' else 'inherit SKILL;') in src)
    ck('%s valid_enable %s' % (path, slot), re.search(r'return usage == "%s"' % slot, src) is not None)
    ck('%s 头注释声明「%s」' % (path, stage), cname in src and stage in src)
    ck('文档含代表功法 %s' % cname, cname in doc)
    ck('文档含路径 %s' % stage, stage in doc)

# ---- 门派传承 ----
print('门派传承接线:')
sect_d = readf(SECT_D)
m = re.search(r'"jujian_gate": \(\[(.*?)\n  \]\),', sect_d, re.S)
jj = m.group(1) if m else ''
for sid in ['xiangjia-gong', 'fansheng-zhenmo-gong', 'niepan-shengti']:
    ck('巨剑门→%s（sect_config）' % sid, '"%s"' % sid in jj)
m = re.search(r'"guiling_sect": \(\[(.*?)\n  \]\),', sect_d, re.S)
gl = m.group(1) if m else ''
ck('鬼灵门→wandu-hunyuan-shen（sect_config）', '"wandu-hunyuan-shen"' in gl)
gaoren = readf('d/yueguo/jujian/npc/gaoren.c')
for sid in ['xiangjia-gong', 'fansheng-zhenmo-gong', 'niepan-shengti']:
    ck('传功长老 gaoren 教 %s' % sid, re.search(r'set_skill\("%s"' % sid, gaoren) is not None)

# ---- 阶段阶梯接线 ----
print('阶段阶梯:')
for sid, prev, need in [
        ('fansheng-zhenmo-gong', 'BODY_SKILL_BASIC', 'SECT_TIER_ZHU'),
        ('niepan-shengti', 'BODY_SKILL_DUAL', 'SECT_TIER_YING'),
        ('wandu-hunyuan-shen', 'BODY_SKILL_ULTIMATE', 'SECT_TIER_JIE')]:
    src = readf('kungfu/skill/%s.c' % sid)
    vl = func_body_raw(src, 'valid_learn')
    ck('%s 需上一段功法 %s' % (sid, prev), prev in vl)
    ck('%s 需 %s 级' % (sid, 'BODY_PREREQ_SKILL'), 'BODY_PREREQ_SKILL' in vl)
    ck('%s 境界门槛 %s' % (sid, need), need in vl)

# ═══════════ 二、LPC 原文守卫 ═══════════
print('== 二、LPC 原文守卫 ==')

stage_body = func_body_raw(body_d, 'query_stage')
ck('query_stage 取四门功法等级',
   all(k in stage_body for k in ['BODY_SKILL_BASIC', 'BODY_SKILL_DUAL', 'BODY_SKILL_ULTIMATE', 'BODY_SKILL_POISON']))
ck('query_stage 基础功法未学 → 未淬体', re.search(r'if\s*\(lv_basic\s*<\s*1\)\s*return\s+BODY_STAGE_NONE', stage_body) is not None)
ck('query_stage 法体双修需象甲功 50 级',
   re.search(r'lv_dual\s*>=\s*1\s*&&\s*lv_basic\s*>=\s*BODY_PREREQ_SKILL', stage_body) is not None)
ck('query_stage 极限炼体需梵圣 50 级',
   re.search(r'lv_ultimate\s*>=\s*1\s*&&\s*lv_dual\s*>=\s*BODY_PREREQ_SKILL', stage_body) is not None)
ck('query_stage 毒体需涅槃 50 级',
   re.search(r'lv_poison\s*>=\s*1\s*&&\s*lv_ultimate\s*>=\s*BODY_PREREQ_SKILL', stage_body) is not None)

refresh_body = func_body_raw(body_d, 'refresh')
ck('refresh 按阶段取上限', re.search(r'cap\s*=\s*query_stage_cap\(stage\)', refresh_body) is not None)
ck('refresh 点数夹在上限内', re.search(r'if\s*\(points\s*>\s*cap\)\s*points\s*=\s*cap', refresh_body) is not None)
ck('refresh 写回 points/stage', 'set("body_refine/points"' in refresh_body and 'set("body_refine/stage"' in refresh_body)
ck('refresh 派生 qi/defense/fist',
   'set("body_refine/qi"' in refresh_body and 'set("body_refine/defense"' in refresh_body
   and 'set("body_refine/fist"' in refresh_body)

train_body = func_body_raw(body_d, 'train_body')
ck('train_body 校验体修阶段', re.search(r'stage\s*<\s*BODY_STAGE_BASIC', train_body) is not None)
ck('train_body 校验上限', re.search(r'points\s*>=\s*cap', train_body) is not None)
ck('train_body 校验内力', re.search(r'query\("neili"\)\s*<\s*BODY_COST_NEILI', train_body) is not None)
ck('train_body 校验精神', re.search(r'query\("jing"\)\s*<\s*BODY_COST_JING', train_body) is not None)
ck('train_body 扣内力与精神',
   re.search(r'add\("neili",\s*-BODY_COST_NEILI\)', train_body) is not None
   and re.search(r'add\("jing",\s*-BODY_COST_JING\)', train_body) is not None)
ck('train_body 收益随功法等级（BASE + lv/PER）',
   re.search(r'gain\s*=\s*BODY_GAIN_BASE\s*\+\s*lv\s*/\s*BODY_GAIN_PER_LEVEL', train_body) is not None)
ck('train_body 收益封顶 BODY_GAIN_MAX', 'BODY_GAIN_MAX' in train_body)
ck('train_body 收益夹到阶段上限', re.search(r'if\s*\(points\s*\+\s*gain\s*>\s*cap\)\s*gain\s*=\s*cap\s*-\s*points', train_body) is not None)
ck('train_body 落盘后 refresh', 'refresh(me)' in train_body)

exert = readf(EXERT)
ex_body = func_body_raw(exert, 'exert')
ck('三头六臂需梵圣真魔功为内功', 'query_skill_mapped("force") != BODY_SKILL_DUAL' in ex_body)
ck('三头六臂防重复叠加', re.search(r'query_temp\(BODY_FAXIANG_BUFF\)', ex_body) is not None)
ck('三头六臂时长封顶', 'BODY_FAXIANG_MAX_TIME' in ex_body)
ck('三头六臂加 apply/attack 与 apply/damage',
   'add_temp("apply/attack", attack)' in ex_body and 'add_temp("apply/damage", damage)' in ex_body)
rm = func_body_raw(exert, 'remove_effect')
ck('三头六臂撤销命中加成', re.search(r'add_temp\("apply/attack",\s*-attack\)', rm) is not None)
ck('三头六臂撤销伤害加成', re.search(r'add_temp\("apply/damage",\s*-damage\)', rm) is not None)

# ═══════════ 三、突变验证 ═══════════
print('== 三、突变验证 ==')


def guard_ladder(src):
    b = func_body_raw(src, 'query_stage')
    return (re.search(r'lv_dual\s*>=\s*1\s*&&\s*lv_basic\s*>=\s*BODY_PREREQ_SKILL', b) is not None
            and re.search(r'lv_ultimate\s*>=\s*1\s*&&\s*lv_dual\s*>=\s*BODY_PREREQ_SKILL', b) is not None
            and re.search(r'lv_poison\s*>=\s*1\s*&&\s*lv_ultimate\s*>=\s*BODY_PREREQ_SKILL', b) is not None)


def guard_cap_clamp(src):
    b = func_body_raw(src, 'refresh')
    return re.search(r'if\s*\(points\s*>\s*cap\)\s*points\s*=\s*cap', b) is not None


def guard_limit_check(src):
    return re.search(r'points\s*>=\s*cap', func_body_raw(src, 'train_body')) is not None


def guard_cost(src):
    b = func_body_raw(src, 'train_body')
    return (re.search(r'add\("neili",\s*-BODY_COST_NEILI\)', b) is not None
            and re.search(r'add\("jing",\s*-BODY_COST_JING\)', b) is not None)


def guard_unarmed_fist(dh_src):
    return re.search(r'if\s*\(\s*!objectp\(weapon\)\s*\)\s*\n?\s*damage\s*\+=\s*\(int\)me->query\("body_refine/fist"\)', dh_src) is not None


def guard_defatk(dh_src):
    return re.search(r'body_refine/defense', dh_src) is not None


def guard_faxiang_symmetry(src):
    add = func_body_raw(src, 'exert')
    rem = func_body_raw(src, 'remove_effect')
    return (re.search(r'add_temp\("apply/damage",\s*damage\)', add) is not None
            and re.search(r'add_temp\("apply/damage",\s*-damage\)', rem) is not None)


def guard_max_qi_wiring(attr_src):
    return re.search(r'query_max_qi[\s\S]{0,4000}?body_refine/qi', attr_src) is not None


mut = body_d.replace('if (lv_dual >= 1 && lv_basic >= BODY_PREREQ_SKILL)', 'if (lv_dual >= 1)')
ck('突变1 法体双修阶梯去掉根基要求 → 守卫红', not guard_ladder(mut))

mut = body_d.replace('if (points > cap) points = cap;', '')
ck('突变2 refresh 去掉阶段钳制 → 守卫红', not guard_cap_clamp(mut))

mut = body_d.replace('if (points >= cap)', 'if (0)')
ck('突变3 train_body 去掉上限判断 → 守卫红', not guard_limit_check(mut))

mut = body_d.replace('me->add("neili", -BODY_COST_NEILI);', '')
ck('突变4 train_body 不扣内力 → 守卫红', not guard_cost(mut))

mut = dh.replace('damage += (int)me->query("body_refine/fist");', '')
ck('突变5 空手体修伤害接线删除 → 守卫红', not guard_unarmed_fist(mut))

mut = dh.replace('+(int)victim->query("body_refine/defense")', '')
ck('突变6 固定减伤不叠加体修防御 → 守卫红', not guard_defatk(mut))

mut = exert.replace('me->add_temp("apply/damage", -damage);', '')
ck('突变7 三头六臂只加不还 → 守卫红', not guard_faxiang_symmetry(mut))

mut = attr.replace('body_refine/qi', 'nouse_field')
ck('突变8 气血不接体修 → 守卫红', not guard_max_qi_wiring(mut))

# ═══════════ 四、同构模拟 ═══════════
print('== 四、同构模拟 ==')

PREREQ = hdr_int(body_h, 'BODY_PREREQ_SKILL')
CAP = {0: 0, 1: hdr_int(body_h, 'BODY_CAP_BASIC'), 2: hdr_int(body_h, 'BODY_CAP_DUAL'),
       3: hdr_int(body_h, 'BODY_CAP_ULTIMATE'), 4: hdr_int(body_h, 'BODY_CAP_POISON')}
QI_PER = hdr_int(body_h, 'BODY_QI_PER_POINT')
DEF_DIV = hdr_int(body_h, 'BODY_DEF_DIVISOR')
FIST_DIV = hdr_int(body_h, 'BODY_FIST_DIVISOR')
SWORD_DIV = hdr_int(body_h, 'BODY_SWORD_DIVISOR')
C_NEILI = hdr_int(body_h, 'BODY_COST_NEILI')
C_JING = hdr_int(body_h, 'BODY_COST_JING')
G_BASE = hdr_int(body_h, 'BODY_GAIN_BASE')
G_PER = hdr_int(body_h, 'BODY_GAIN_PER_LEVEL')
G_MAX = hdr_int(body_h, 'BODY_GAIN_MAX')
P_RESIST = hdr_int(body_h, 'BODY_POISON_RESIST')
FX_NEILI = hdr_int(body_h, 'BODY_FAXIANG_NEILI')
FX_ATT = hdr_int(body_h, 'BODY_FAXIANG_ATTACK')
FX_DMG = hdr_int(body_h, 'BODY_FAXIANG_DAMAGE')
FX_LV = hdr_int(body_h, 'BODY_FAXIANG_LEVEL')
FX_PER = hdr_int(body_h, 'BODY_FAXIANG_PER_LEVEL')
FX_MAXT = hdr_int(body_h, 'BODY_FAXIANG_MAX_TIME')


# 4.1 阶段推导（query_stage 翻译）
def stage_of(basic, dual, ultimate, poison):
    if basic < 1:
        return 0
    if dual >= 1 and basic >= PREREQ:
        if ultimate >= 1 and dual >= PREREQ:
            if poison >= 1 and ultimate >= PREREQ:
                return 4
            return 3
        return 2
    return 1


ck('无炼体功法 → 未淬体', stage_of(0, 0, 0, 0) == 0)
ck('仅象甲功 → 基础炼体', stage_of(1, 0, 0, 0) == 1)
ck('象甲功未满 50 级学梵圣 → 仍是基础炼体（阶梯拦截）', stage_of(49, 1, 0, 0) == 1)
ck('象甲功 50 + 梵圣 → 法体双修', stage_of(50, 1, 0, 0) == 2)
ck('梵圣未满 50 学涅槃 → 仍是法体双修', stage_of(50, 49, 1, 0) == 2)
ck('梵圣 50 + 涅槃 → 极限炼体', stage_of(50, 50, 1, 0) == 3)
ck('涅槃未满 50 学万毒 → 仍是极限炼体', stage_of(50, 50, 49, 1) == 3)
ck('涅槃 50 + 万毒 → 毒体修炼', stage_of(50, 50, 50, 1) == 4)

# 4.2 上限表与加成换算
ck('上限表 200/450/800/1200', [CAP[1], CAP[2], CAP[3], CAP[4]] == [200, 450, 800, 1200])


def bonuses(points):
    return points * QI_PER, points // DEF_DIV, points // FIST_DIV, points // SWORD_DIV


ck('200 点：气血 +600、减伤 +66、空手 +66、重剑 +33', bonuses(200) == (600, 66, 66, 33))
ck('450 点：气血 +1350、减伤 +150、空手 +150、重剑 +75', bonuses(450) == (1350, 150, 150, 75))
ck('1200 点：气血 +3600、减伤 +400、空手 +400、重剑 +200', bonuses(1200) == (3600, 400, 400, 200))

# 4.3 淬体收益与夹取（train_body 翻译）
def gain_of(max_skill):
    return min(G_MAX, G_BASE + max_skill // G_PER)


ck('功法 0 级收益 1 点', gain_of(0) == 1)
ck('功法 19 级收益 1 点', gain_of(19) == 1)
ck('功法 20 级收益 2 点', gain_of(20) == 2)
ck('功法 100 级收益 6 点', gain_of(100) == 6)
ck('功法 200 级收益封顶 8 点', gain_of(200) == G_MAX)


def train(points, stage, max_skill, neili, jing):
    if stage < 1:
        return False, '未修体', points
    if neili < C_NEILI:
        return False, '内力不足', points
    if jing < C_JING:
        return False, '精神不足', points
    cap = CAP[stage]
    if points >= cap:
        return False, '已达上限', points
    gain = gain_of(max_skill)
    if points + gain > cap:
        gain = cap - points
    return True, 'ok', points + gain


ck('未修体功被拒', train(0, 0, 0, 999, 999)[1] == '未修体')
ck('内力不足被拒', train(0, 1, 20, 59, 999)[1] == '内力不足')
ck('精神不足被拒', train(0, 1, 20, 999, 39)[1] == '精神不足')
ck('阶段上限拦截（200 点满）', train(200, 1, 200, 999, 999)[1] == '已达上限')
ck('临近上限时收益夹取（195+8→200）', train(195, 1, 200, 999, 999)[2] == 200)
ck('阶段升级后上限放宽（201 点可继续增长）', train(200, 2, 200, 999, 999)[2] > 200)

# 4.4 战斗数值接线（damage.h 翻译）
def hit_damage(base, weapon_type, martial, fist, sword):
    d = base
    if weapon_type is None:
        d += fist
    elif martial == 'zhongjian-jianfa':
        d += sword
    return d


ck('空手：基数 100 + 体修拳劲 66 = 166', hit_damage(100, None, 'unarmed', 66, 33) == 166)
ck('重剑（体剑双修）：100 + 33 = 133', hit_damage(100, 'sword', 'zhongjian-jianfa', 66, 33) == 133)
ck('普通剑法：不吃体修加成（仍 100）', hit_damage(100, 'sword', 'qingyuan-jianjue', 66, 33) == 100)
ck('重剑加成是空手的一半（33 < 66）', hit_damage(0, 'sword', 'zhongjian-jianfa', 66, 33) <
   hit_damage(0, None, 'unarmed', 66, 33))


def flat_reduction(damage, temp_def, body_def):
    defatk = temp_def + body_def
    if defatk < 0:
        defatk = 0
    if defatk > damage // 2:
        defatk = damage // 2
    return defatk


ck('固定减伤 = 临时 + 体修（40+66=106 → 上限 damage/2）', flat_reduction(200, 40, 66) == 100)
ck('伤害较低时减伤按半额钳制（100 伤害 → 最多 50）', flat_reduction(100, 0, 66) == 50)
ck('无体修时沿用原逻辑', flat_reduction(200, 40, 0) == 40)

# 4.5 三头六臂曲线
def faxiang(lv):
    duration = min(FX_MAXT, FX_LV + lv // 10 * FX_PER)
    attack = FX_ATT + lv // 10
    damage = FX_DMG + lv // 5
    return duration, attack, damage


ck('梵圣 1 级：20 秒 / +30 / +40', faxiang(1) == (20, 30, 40))
ck('梵圣 10 级：24 秒 / +31 / +42', faxiang(10) == (24, 31, 42))
ck('梵圣 100 级：60 秒（封顶）/ +40 / +60', faxiang(100) == (60, 40, 60))
ck('梵圣 200 级：仍封顶 60 秒 / +50 / +80', faxiang(200) == (60, 50, 80))
ck('法相内力消耗 200', FX_NEILI == 200)

# 4.6 毒体抗毒（poison.c ObtainProof 翻译；AdjustCoefficient 钳 0..99）
def anti_poison(base, stage):
    r = base
    if stage >= 4:
        r += P_RESIST
    r = max(0, min(99, r))
    return r


ck('未到毒体无额外抗毒（30）', anti_poison(30, 3) == 30)
ck('毒体抗毒 +30（30 → 60）', anti_poison(30, 4) == 60)
ck('抗毒上限仍钳 99', anti_poison(90, 4) == 99)

# ═══════════ 结果 ═══════════
print('')
if fails or errors:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
