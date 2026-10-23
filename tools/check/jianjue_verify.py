#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#77 青元剑诀进阶神通（剑芒/护体剑气/剑影分光术/巨剑术/大庚剑阵）静态校验 + 原文守卫 + 突变实证 + 同构模拟

运行：python3 tools/check/jianjue_verify.py

覆盖：
  一、静态校验：交付文件存在 / 括号配对 / jianjue.h 守卫与规范 / 契约对齐
                （perform_action_file → 神通文件、attack_val 走 sword+qingyuan-jianjue、
                 结算走 COMBAT_D->do_attack TYPE_SPECIAL = 自动接入 #78 境界压制）
                / 1F §2.2 层数表与 §2.5 神通表文档对齐 / 境界门槛与内力消耗
  二、LPC 原文守卫：层数门槛、境界门槛、内力扣除、剑影分光道数、巨剑术 apply/damage 加还对称、
                护体剑气 buff 撤销对称（week/flat 成对 add/remove）、大庚剑阵目标数上限
  三、突变验证：删门槛 / 只加不还 / 改目录 / 放大目标数 → 守卫必须变红
  四、同构模拟：层数换算与 #62 三段境界门槛对齐 / 剑芒道数 / 剑影分光道数 / 护体剑气数值曲线 /
                巨剑术加成曲线 / 大庚剑阵命中数 / 施展流程拒绝分支

退出码：0 全绿，1 有失败
"""
import re
import sys
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0

SKILL = 'kungfu/skill/qingyuan-jianjue.c'
HDR = 'kungfu/skill/qingyuan-jianjue/jianjue.h'
PERFORMS = {
    'jianmang': 'kungfu/skill/qingyuan-jianjue/jianmang.c',
    'huti_jianqi': 'kungfu/skill/qingyuan-jianjue/huti_jianqi.c',
    'yingfenshen': 'kungfu/skill/qingyuan-jianjue/yingfenshen.c',
    'juyuan': 'kungfu/skill/qingyuan-jianjue/juyuan.c',
    'dageng_jianzhen': 'kungfu/skill/qingyuan-jianjue/dageng_jianzhen.c',
}
DOC = '.knowledge/combat/1F-法术剑诀阵法战斗.md'

# 1F §2.2 层数表：神通 → (文档所在层段, 层段下界)
DOC_LAYER = {
    'jianmang': ('基础剑芒、护体剑气', 1),
    'huti_jianqi': ('基础剑芒、护体剑气', 1),
    'yingfenshen': ('青竹蜂云剑操控、剑影分光术', 6),
    'dageng_jianzhen': ('大庚剑阵、无形剑气', 10),
}


# ═══════════ 工具函数 ═══════════

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


# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

for p in [SKILL, HDR] + list(PERFORMS.values()):
    ck('exists %s' % p, os.path.isfile(p))

for p in [SKILL, HDR] + list(PERFORMS.values()):
    ok, detail = check_parens(p)
    ck('parens %s' % p, ok, detail)

hdr = readf(HDR)
ck('jianjue.h 有包含守卫', '#ifndef __QINGYUAN_JIANJUE_H__' in hdr and '#define __QINGYUAN_JIANJUE_H__' in hdr)
ck('jianjue.h 无 tab 缩进', '\t' not in hdr)
for sym in ['jianjue_layer', 'jianjue_ready', 'jianjue_target', 'jianjue_attack_val', 'jianjue_strike']:
    ck('jianjue.h 提供 %s' % sym, re.search(r'\b%s\s*\([^;]*\)\s*\{' % sym, strip_lpc(hdr)) is not None)
ck('attack_val 的 skill 指向青元剑诀（damage.h 读 attack_val/skill/weapon）',
   re.search(r'"skill"\s*:\s*\(\$$\s*"weapon"\s*:\s*QINGYUAN_SKILL', hdr.replace('([', '($$')) is not None
   or ('"skill" : ([ "weapon" : QINGYUAN_SKILL ])' in hdr))
ck('jianjue_strike 走 COMBAT_D->do_attack TYPE_SPECIAL',
   re.search(r'COMBAT_D->do_attack\(me,\s*target,\s*weapon,\s*TYPE_SPECIAL\)', hdr) is not None)
ck('jianjue_strike 设置并清理 attack_val',
   'set_temp("attack_val"' in hdr and 'delete_temp("attack_val")' in hdr)
ck('层数公式 = (lv - 1) / 10 + 1（与 #62 三段境界门槛对齐）',
   re.search(r'return\s*\(lv\s*-\s*1\)\s*/\s*10\s*\+\s*1', hdr) is not None)
ck('持剑校验（skill_type == "sword"）', re.search(r'weapon->query\("skill_type"\)\s*!=\s*"sword"', hdr) is not None)
ck('激发校验（query_skill_mapped("sword") == QINGYUAN_SKILL）',
   re.search(r'query_skill_mapped\("sword"\)\s*!=\s*QINGYUAN_SKILL', hdr) is not None)

skill = readf(SKILL)
ck('技能文件提供 perform_action_file',
   re.search(r'string\s+perform_action_file\s*\(string action\)', skill) is not None)
ck('perform_action_file 指向 qingyuan-jianjue/ 目录',
   re.search(r'return\s+__DIR__\s*"qingyuan-jianjue/"\s*\+\s*action;', skill) is not None)

# ---- 1F 文档对齐 ----
doc = readf(DOC)
print('1F 文档对齐:')
layer_rows = {}
for m in re.finditer(r'\|\s*第(\d+)-(\d+)层\s*\|\s*([^|]+)\|\s*([^|]+)\|', doc):
    layer_rows[(int(m.group(1)), int(m.group(2)))] = m.group(4).strip()
ck('文档有第1-4层行', (1, 4) in layer_rows)
ck('文档有第6-9层行', (6, 9) in layer_rows)
ck('文档有第10-13层行', (10, 13) in layer_rows)
for name, (unlock_text, low) in DOC_LAYER.items():
    row = layer_rows.get((low, {1: 4, 6: 9, 10: 13}[low]), '')
    ck('文档第%d层段含「%s」' % (low, unlock_text), unlock_text in row, 'row=%s' % row)
ck('文档 §2.5 含巨剑术', re.search(r'\|\s*\*\*巨剑术\*\*', doc) is not None)
ck('文档 §2.5 含剑影分光术', re.search(r'\|\s*\*\*剑影分光术\*\*', doc) is not None)
ck('文档 §2.4 含大庚剑阵', '大庚剑阵' in doc)

# ---- 每个神通的接线与门槛 ----
GATES = {
    'jianmang': (1, None, 60),
    'huti_jianqi': (1, None, 100),
    'yingfenshen': (6, 'SECT_TIER_ZHU', 200),
    'juyuan': (6, 'SECT_TIER_ZHU', 250),
    'dageng_jianzhen': (10, 'SECT_TIER_JIE', 400),
}
print('神通接线:')
for name, path in PERFORMS.items():
    src = readf(path)
    code = strip_lpc(src)
    low, tier_const, cost = GATES[name]
    ck('%s include jianjue.h' % name, re.search(r'#include\s+"jianjue\.h"', src) is not None)
    ck('%s inherit F_SSERVER' % name, 'inherit F_SSERVER' in code)
    ck('%s 定义 valid_target_arg' % name, re.search(r'int\s+valid_target_arg\s*\(string target\)', code) is not None)
    ck('%s version 返回 2（新格式 perform）' % name,
       re.search(r'target\s*==\s*"version"\s*\)\s*return\s*2', src) is not None)
    ck('%s 定义 perform' % name, re.search(r'int\s+perform\s*\(object me,\s*object target\)', code) is not None)
    ck('%s 层数门槛 = 第%d层' % (name, low),
       re.search(r'if\s*\(layer\s*<\s*%d\)' % low, code) is not None)
    if tier_const:
        ck('%s 境界门槛 %s' % (name, tier_const),
           re.search(r'query_cultivation_tier\(me\)\s*<\s*%s' % tier_const, code) is not None)
    ck('%s 内力消耗 %d' % (name, cost), re.search(r'cost\s*=\s*%d;' % cost, code) is not None)
    ck('%s 扣内力（add("neili", -cost)）' % name, re.search(r'me->add\("neili",\s*-cost\)', src) is not None)
    if name == 'huti_jianqi':
        ck('%s 为自身增益型神通（不结算攻击）' % name, 'jianjue_strike(me,' not in code)
        ck('%s 注册 buff 到期回调' % name, 'start_call_out(__FILE__, "remove_effect"' in src)
    else:
        ck('%s 经 jianjue_strike 结算' % name, 'jianjue_strike(me,' in code)
        ck('%s 清理 attack_val' % name, 'delete_temp("attack_val")' in src)
    ck('%s 起手文案（message_vision）' % name, 'message_vision(' in code)

# ═══════════ 二、LPC 原文守卫 ═══════════
print('== 二、LPC 原文守卫 ==')

# 剑芒：第5层强化为 3 道
body = func_body_raw(readf(PERFORMS['jianmang']), 'perform')
ck('剑芒默认 2 道、第5层起 3 道（1F §2.2 剑芒强化）',
   re.search(r'hits\s*=\s*\(layer\s*>=\s*5\)\s*\?\s*3\s*:\s*2', body) is not None)
ck('剑芒 busy（start_busy）', 'start_busy(' in body)

# 护体剑气：数值曲线 + 撤销对称
huti = readf(PERFORMS['huti_jianqi'])
body = func_body_raw(huti, 'perform')
ck('护体剑气 buff 命名空间', 'active_buffer/qingyuan-jianjue.huti_jianqi' in huti)
ck('护体剑气 百分比减伤 = 10+层（上限 20）',
   re.search(r'week\s*=\s*10\s*\+\s*layer;', body) is not None and
   re.search(r'if\s*\(week\s*>\s*20\)\s*week\s*=\s*20', body) is not None)
ck('护体剑气 固定减伤 = 30+层×10（上限 120）',
   re.search(r'flat\s*=\s*30\s*\+\s*layer\s*\*\s*10;', body) is not None and
   re.search(r'if\s*\(flat\s*>\s*120\)\s*flat\s*=\s*120', body) is not None)
ck('护体剑气 持续 = 20+层×5（上限 60）',
   re.search(r'duration\s*=\s*20\s*\+\s*layer\s*\*\s*5;', body) is not None and
   re.search(r'if\s*\(duration\s*>\s*60\)\s*duration\s*=\s*60', body) is not None)
ck('护体剑气 记录 week/flat 供撤销', '"/week", week' in huti and '"/flat", flat' in huti)
ck('护体剑气 注册到期回调', re.search(r'start_call_out\(__FILE__,\s*"remove_effect",\s*duration,\s*me\)', body) is not None)
rm = func_body_raw(huti, 'remove_effect')
ck('护体剑气撤销百分比减伤', re.search(r'add_temp\("apply/week_injure",\s*-week\)', rm) is not None)
ck('护体剑气撤销固定减伤', re.search(r'add_temp\("apply/defense_attack",\s*-flat\)', rm) is not None)
ck('护体剑气撤销后清理 buff 标记', 'delete_temp(HUTI_BUFF)' in rm)

# 剑影分光术：道数
body = func_body_raw(readf(PERFORMS['yingfenshen']), 'perform')
ck('剑影分光 道数 = 3+(层-6)（上限 6）',
   re.search(r'hits\s*=\s*3\s*\+\s*\(layer\s*-\s*6\);', body) is not None and
   re.search(r'if\s*\(hits\s*>\s*6\)\s*hits\s*=\s*6', body) is not None)

# 巨剑术：加成与撤销对称（只加不还是平衡事故）
juyuan = readf(PERFORMS['juyuan'])
body = func_body_raw(juyuan, 'perform')
ck('巨剑术 加成 = 40+层×10（上限 120）',
   re.search(r'bonus\s*=\s*40\s*\+\s*layer\s*\*\s*10;', body) is not None and
   re.search(r'if\s*\(bonus\s*>\s*120\)\s*bonus\s*=\s*120', body) is not None)
ck('巨剑术 加 apply/damage', re.search(r'add_temp\("apply/damage",\s*bonus\)', body) is not None)
ck('巨剑术 撤销 apply/damage（-bonus）',
   re.search(r'add_temp\("apply/damage",\s*-bonus\)', body) is not None)
ck('巨剑术 撤销在命中之后', body.find('add_temp("apply/damage", bonus)') < body.find('add_temp("apply/damage", -bonus)'))

# 大庚剑阵：多目标上限 + 结丹后期多一道
body = func_body_raw(readf(PERFORMS['dageng_jianzhen']), 'perform')
ck('大庚剑阵 场上目标上限 4', re.search(r'sizeof\(enemies\)\s*>=\s*4', body) is not None)
ck('大庚剑阵 结丹后期 2 道剑光', re.search(r'SECT_TIER_JIE_LATE\)\s*\?\s*2\s*:\s*1', body) is not None)
ck('大庚剑阵 只打同室存活敌人',
   'environment(all[i]) != environment(me)' in body and 'living(all[i])' in body)

# ═══════════ 三、突变验证 ═══════════
print('== 三、突变验证 ==')


def guard_layer_gate(src, low):
    return re.search(r'if\s*\(layer\s*<\s*%d\)' % low, func_body_raw(src, 'perform')) is not None


def guard_tier_gate(src, const):
    return re.search(r'query_cultivation_tier\(me\)\s*<\s*%s' % const, func_body_raw(src, 'perform')) is not None


def guard_damage_symmetry(src):
    body = func_body_raw(src, 'perform')
    return (re.search(r'add_temp\("apply/damage",\s*bonus\)', body) is not None and
            re.search(r'add_temp\("apply/damage",\s*-bonus\)', body) is not None)


def guard_buff_symmetry(src):
    add_part = func_body_raw(src, 'perform')
    rm_part = func_body_raw(src, 'remove_effect')
    return (re.search(r'add_temp\("apply/week_injure",\s*week\)', add_part) is not None and
            re.search(r'add_temp\("apply/week_injure",\s*-week\)', rm_part) is not None)


def guard_attack_skill(src):
    return 'QINGYUAN_SKILL' in func_body_raw(src, 'jianjue_attack_val')


def guard_action_file(src):
    return re.search(r'__DIR__\s*"qingyuan-jianjue/"\s*\+\s*action', src) is not None


def guard_target_cap(src):
    return re.search(r'sizeof\(enemies\)\s*>=\s*4', func_body_raw(src, 'perform')) is not None


mut = readf(PERFORMS['jianmang']).replace('if (layer < 1)', 'if (0)')
ck('突变1 剑芒层数门槛删除 → 守卫红', not guard_layer_gate(mut, 1))

mut = readf(PERFORMS['yingfenshen']).replace(
    'if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)', 'if (0)')
ck('突变2 剑影分光筑基门槛删除 → 守卫红', not guard_tier_gate(mut, 'SECT_TIER_ZHU'))

mut = readf(PERFORMS['juyuan']).replace('me->add_temp("apply/damage", -bonus);', '')
ck('突变3 巨剑术只加不还 → 守卫红', not guard_damage_symmetry(mut))

mut = readf(PERFORMS['huti_jianqi']).replace(
    'if (intp(week) && week > 0) me->add_temp("apply/week_injure", -week);', '')
ck('突变4 护体剑气不撤销减伤 → 守卫红', not guard_buff_symmetry(mut))

mut = readf(HDR).replace('"skill" : ([ "weapon" : QINGYUAN_SKILL ]),',
                         '"skill" : ([ "weapon" : "sword" ]),')
ck('突变5 attack_val 技能指向被改 → 守卫红', not guard_attack_skill(mut))

mut = readf(SKILL).replace('__DIR__ "qingyuan-jianjue/"', '__DIR__ "special/"')
ck('突变6 perform 目录改错 → 守卫红', not guard_action_file(mut))

mut = readf(PERFORMS['dageng_jianzhen']).replace('sizeof(enemies) >= 4', 'sizeof(enemies) >= 99')
ck('突变7 大庚剑阵目标上限放大 → 守卫红', not guard_target_cap(mut))

# ═══════════ 四、同构模拟 ═══════════
print('== 四、同构模拟 ==')

# 4.1 层数换算（与 #62 valid_learn 三段门槛对齐）
def layer_of(lv):
    if lv < 1:
        return 0
    return (lv - 1) // 10 + 1


ck('lv1 → 第1层', layer_of(1) == 1)
ck('lv10 → 第1层', layer_of(10) == 1)
ck('lv11 → 第2层', layer_of(11) == 2)
ck('lv30 → 第3层（#62 前三层=炼气段末）', layer_of(30) == 3)
ck('lv31 → 第4层（#62 中三层=筑基段首）', layer_of(31) == 4)
ck('lv60 → 第6层（#62 中三层末）', layer_of(60) == 6)
ck('lv61 → 第7层（#62 后三层=结丹段首）', layer_of(61) == 7)
ck('lv90 → 第9层（残本末层）', layer_of(90) == 9)
ck('lv91 → 第10层（大庚剑阵门槛）', layer_of(91) == 10)
ck('lv0 → 未修习（0 层）', layer_of(0) == 0)

# #62 valid_learn 分段：lv<=30 炼气、lv<=60 筑基、其余结丹
def learn_tier(lv):
    if lv <= 30:
        return 0      # SECT_TIER_QI（炼气段，tier>=0）
    if lv <= 60:
        return 3      # SECT_TIER_ZHU
    return 6          # SECT_TIER_JIE


ck('层1-3 全落在炼气段（#62 一致）', all(learn_tier(10 * (l - 1) + 1) == 0 for l in (1, 2, 3)))
ck('层4-6 全落在筑基段（#62 一致）', all(learn_tier(10 * (l - 1) + 1) == 3 for l in (4, 5, 6)))
ck('层7-9 全落在结丹段（#62 一致）', all(learn_tier(10 * (l - 1) + 1) == 6 for l in (7, 8, 9)))

# 4.2 剑芒道数
def jianmang_hits(layer):
    return 3 if layer >= 5 else 2


ck('剑芒 第1层 2 道', jianmang_hits(1) == 2)
ck('剑芒 第4层 2 道', jianmang_hits(4) == 2)
ck('剑芒 第5层起 3 道（剑芒强化）', jianmang_hits(5) == 3 and jianmang_hits(9) == 3)

# 4.3 剑影分光道数
def yingfenshen_hits(layer):
    return min(6, max(0, 3 + (layer - 6)))


ck('剑影分光 第6层 3 道', yingfenshen_hits(6) == 3)
ck('剑影分光 第7层 4 道', yingfenshen_hits(7) == 4)
ck('剑影分光 第9层 6 道（封顶）', yingfenshen_hits(9) == 6)
ck('剑影分光 第12层 仍封顶 6 道', yingfenshen_hits(12) == 6)

# 4.4 护体剑气数值曲线
def huti_effect(layer):
    week = min(20, 10 + layer)
    flat = min(120, 30 + layer * 10)
    duration = min(60, 20 + layer * 5)
    return week, flat, duration


ck('护体剑气 第1层：11% / 40 点 / 25 秒', huti_effect(1) == (11, 40, 25))
ck('护体剑气 第6层：16% / 90 点 / 50 秒', huti_effect(6) == (16, 90, 50))
ck('护体剑气 第10层：20% / 120 点 / 60 秒（三项封顶）', huti_effect(10) == (20, 120, 60))
ck('护体剑气 第20层仍封顶', huti_effect(20) == (20, 120, 60))

# 4.5 巨剑术加成曲线
def juyuan_bonus(layer):
    return min(120, 40 + layer * 10)


ck('巨剑术 第6层 +100', juyuan_bonus(6) == 100)
ck('巨剑术 第8层 +120（封顶）', juyuan_bonus(8) == 120)
ck('巨剑术 第15层仍 +120', juyuan_bonus(15) == 120)

# 4.6 大庚剑阵命中数（结丹期 1 道 / 结丹后期 2 道）
SECT_TIER_JIE = 6
SECT_TIER_JIE_LATE = 8


def dageng_hits(tier):
    return 2 if tier >= SECT_TIER_JIE_LATE else 1


ck('大庚剑阵 结丹初期 1 道', dageng_hits(6) == 1)
ck('大庚剑阵 结丹中期 1 道', dageng_hits(7) == 1)
ck('大庚剑阵 结丹后期 2 道', dageng_hits(8) == 2)
ck('大庚剑阵 元婴期 2 道', dageng_hits(9) == 2)

# 4.7 施展流程拒绝分支（perform 文件关键判定翻译）
def perform_flow(layer, tier, neili, req_layer, req_tier, cost, fighting=True):
    if not fighting:
        return False, '未战斗'
    if layer < req_layer:
        return False, '层数不足'
    if req_tier is not None and tier < req_tier:
        return False, '境界不足'
    if neili < cost:
        return False, '内力不足'
    return True, 'ok'


ck('未战斗被拒', perform_flow(5, 3, 999, 1, None, 60, fighting=False)[1] == '未战斗')
ck('剑芒 第1层可施展', perform_flow(1, 0, 100, 1, None, 60)[0])
ck('剑芒 内力 59 被拒', perform_flow(1, 0, 59, 1, None, 60)[1] == '内力不足')
ck('剑影分光 第5层被拒（层数不足）', perform_flow(5, 3, 999, 6, 3, 200)[1] == '层数不足')
ck('剑影分光 第6层+炼气被拒（境界不足）', perform_flow(6, 0, 999, 6, 3, 200)[1] == '境界不足')
ck('剑影分光 第6层+筑基可施展', perform_flow(6, 3, 999, 6, 3, 200)[0])
ck('大庚剑阵 第9层被拒（层数不足）', perform_flow(9, 6, 999, 10, 6, 400)[1] == '层数不足')
ck('大庚剑阵 第10层+结丹可施展', perform_flow(10, 6, 999, 10, 6, 400)[0])

# ═══════════ 结果 ═══════════
print('')
if fails or errors:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
