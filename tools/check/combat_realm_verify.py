#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#78 战斗伤害公式接境界/灵根加成 静态校验 + LPC 原文守卫 + 突变实证 + 同构模拟（可复跑验证）

运行：python3 tools/check/combat_realm_verify.py

覆盖：
  一、静态校验：交付文件存在 / 括号配对状态机 / 头文件包含守卫 / 契约对齐
                （damage.h 引用的函数在 realm_power.h 真实定义、境界与灵根数据键
                 与 #61 realm 约定和 #26 灵根结构一致）/ 设计文档数值契约
                （02-战斗机制与平衡.md §2.1 境界压制系数表、§2.2 灵根加成系数表）
  二、LPC 原文守卫：realm_power_modifier_permille（未声明境界中性/大境界分支/小阶段分支）、
                spirit_root_damage_info（灵根数据读取/强度折算/异系惩罚只限常规五行）、
                damage.h（境界块在五行块之后、灵根块只在法术攻击生效、乘除钳制形态）
  三、突变验证：改回旧行为/削弱守卫 → 对应守卫必须变红（证明守卫绑定交付物原文）
  四、同构模拟：忠实翻译 LPC 关键路径 → 境界压制系数表逐档核对、灵根系数逐档核对、
                物理攻击不受灵根倍率影响、端到端伤害随境界/灵根变化的单调性与数值断言

退出码：0 全绿，1 有失败
"""
import re
import sys
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0

REALM_H = 'include/combat/realm_power.h'
DAMAGE_H = 'include/combat/damage.h'
ELEMENT_H = 'include/element.h'
ROOT_DAEMON = 'adm/daemons/root_refine_d.c'
COMBATD = 'adm/daemons/combatd.c'
DESIGN_DOC = '02-扩充内容/02-战斗机制与平衡.md'


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


def func_body_raw(src, fname):
    """提取 LPC 函数体（原文）"""
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
    """解析 #define NAME 123"""
    m = re.search(r'#define\s+%s\s+(-?\d+)' % name, src)
    return int(m.group(1)) if m else None


def hdr_permille_array(src, name):
    """解析 nosave int *name = ({ 1500, 1300, ... });"""
    m = re.search(r'%s\s*=\s*\(\{([^}]*)\}\)' % re.escape(name), src)
    if not m:
        return None
    return [int(x) for x in re.findall(r'-?\d+', m.group(1))]


# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

for p in [REALM_H, DAMAGE_H, ELEMENT_H, ROOT_DAEMON, COMBATD, DESIGN_DOC,
          'include/spirit_root.h', 'include/globals.h']:
    ck('exists %s' % p, os.path.isfile(p))

for p in [REALM_H, DAMAGE_H]:
    ok, detail = check_parens(p)
    ck('parens %s' % p, ok, detail)

rp = readf(REALM_H)
rp_code = strip_lpc(rp)

ck('realm_power.h 有包含守卫', '#ifndef __COMBAT_REALM_POWER__' in rp and '#define __COMBAT_REALM_POWER__' in rp)
ck('realm_power.h 无 tab 缩进（项目规范）', '\t' not in rp)
ck('realm_power.h 显式 include <element.h>（自包含，不依赖包含者）', re.search(r'#include\s+<element\.h>', rp))

# ---- 契约对齐：damage.h 引用的符号必须在 realm_power.h 真实定义 ----
dh = readf(DAMAGE_H)
dh_code = strip_lpc(dh)
ck('damage.h include <combat/realm_power.h>', re.search(r'#include\s+<combat/realm_power\.h>', dh))

USED_SYMBOLS = ['realm_power_modifier_permille', 'combat_is_spell_attack',
                'combat_is_spell_slot', 'combat_attack_element',
                'spirit_root_damage_info', 'REALM_SUP_SAME']
for sym in USED_SYMBOLS:
    ck('damage.h 使用 %s' % sym, sym in dh_code)
    if sym.isupper():
        ck('realm_power.h 定义常量 %s' % sym, re.search(r'#define\s+%s\b' % sym, rp))
    else:
        ck('realm_power.h 定义函数 %s' % sym, re.search(r'\b%s\s*\([^;]*\)\s*\{' % sym, rp_code))

# ---- 契约对齐：#61 境界存储键 / #26 灵根数据键 ----
ck('读取 realm_index（#61 统一境界入口）', 'query("realm_index")' in rp)
ck('读取 realm_sub（#61 小阶段）', 'query("realm_sub")' in rp)
ck('读取 realm 字符串（#61 兜底解析）', 'query("realm")' in rp)
ck('读取 spirit_root 灵根数据（#26 结构）', 'query("spirit_root")' in rp)
for k in ['quality_idx', 'variant', 'strength', 'elements']:
    ck('灵根键 %s（#26 结构）' % k, re.search(r'root\["%s"\]' % k, rp) is not None)

rrd = readf(ROOT_DAEMON)
ck('#61 提供 set_player_realm 写 realm_index', re.search(r'void\s+set_player_realm\s*\(', rrd))
ck('#61 set_player_realm 写 realm_index', 'ob->set("realm_index", index)' in rrd)
ck('#61 写入 realm_sub 小阶段', 'ob->set("realm_sub"' in rrd)

elem = readf(ELEMENT_H)
for sym in ['ELE_GOLD', 'ELE_EARTH', 'ELE_TOTAL_ALL', 'query_character_element', 'query_element_name']:
    ck('element.h 提供 %s（#28 五行体系）' % sym, re.search(r'\b%s\b' % sym, elem) is not None)

# ---- 设计文档数值契约（02 §2.1 境界压制系数表 / §2.2 灵根加成系数表）----
print('设计文档数值契约:')
doc = readf(DESIGN_DOC)


def doc_ratio(text):
    """解析 '0.85' 形式的倍率 → 千分比"""
    return int(round(float(text) * 1000))


m = re.search(r'\|\s*差 1 小层\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)\s*\|', doc)
ck('文档有「差 1 小层」行', m is not None)
if m:
    ck('小阶段越级系数 = 文档 %.2f' % float(m.group(1)),
       hdr_int(rp, 'REALM_SUP_STEP_DOWN') == doc_ratio(m.group(1)),
       'hdr=%s' % hdr_int(rp, 'REALM_SUP_STEP_DOWN'))
    ck('小阶段压制系数 = 文档 %.2f' % float(m.group(2)),
       hdr_int(rp, 'REALM_SUP_STEP_UP') == doc_ratio(m.group(2)),
       'hdr=%s' % hdr_int(rp, 'REALM_SUP_STEP_UP'))

m = re.search(r'\|\s*差 1 大境界\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)\s*\|', doc)
ck('文档有「差 1 大境界」行', m is not None)
if m:
    ck('大境界越级系数 = 文档 %.2f' % float(m.group(1)),
       hdr_int(rp, 'REALM_SUP_MAJOR_DIS') == doc_ratio(m.group(1)))
    ck('大境界压制系数 = 文档 %.2f' % float(m.group(2)),
       hdr_int(rp, 'REALM_SUP_MAJOR_ADV') == doc_ratio(m.group(2)))

m = re.search(r'\|\s*差 2 大境界\s*\|\s*([\d.]+)\s*\|', doc)
ck('文档有「差 2 大境界」行', m is not None)
if m:
    ck('越级 2 大境界下限 = 文档 %.2f' % float(m.group(1)),
       hdr_int(rp, 'REALM_SUP_FLOOR') == doc_ratio(m.group(1)))

DOC_ROOT = [
    ('天灵根', 0, 1.5, 30),
    ('真灵根', 2, 1.0, 15),
    ('伪灵根', 4, 0.6, 5),
    ('变异灵根', 1, 1.3, 25),
]
base_arr = hdr_permille_array(rp, 'realm_root_base_pm')
same_arr = hdr_permille_array(rp, 'realm_root_same_pm')
pen_arr = hdr_permille_array(rp, 'realm_root_penalty_pm')
ck('解析灵根基础倍率表（6 档）', base_arr is not None and len(base_arr) == 6, str(base_arr))
ck('解析灵根同系加成表（6 档）', same_arr is not None and len(same_arr) == 6, str(same_arr))
ck('解析灵根异系惩罚表（6 档）', pen_arr is not None and len(pen_arr) == 6, str(pen_arr))
for label, qidx, base, same in DOC_ROOT:
    ck('文档 %s 基础倍率 %.1f = 表[%d]' % (label, base, qidx), base_arr[qidx] == doc_ratio(base),
       'hdr=%s' % base_arr[qidx])
    ck('文档 %s 同系加成 +%d%% = 表[%d]' % (label, same, qidx), same_arr[qidx] == same * 10,
       'hdr=%s' % same_arr[qidx])
ck('文档 变异灵根常规属性 -10% = 惩罚表[1]', pen_arr[1] == -100, 'hdr=%s' % pen_arr[1])
ck('伪灵根无额外惩罚（表[4]=0）', pen_arr[4] == 0)

# ═══════════ 二、LPC 原文守卫 ═══════════
print('== 二、LPC 原文守卫 ==')

# ---- realm_power_modifier_permille ----
body = func_body_raw(rp, 'realm_power_modifier_permille')
ck('realm_power_modifier_permille 存在', body != '')
ck('未声明境界 → 中性（不按 combat_exp 猜测）',
   re.search(r'if\s*\(\s*ai\s*<\s*0\s*\|\|\s*di\s*<\s*0\s*\)\s*return\s+REALM_SUP_SAME', body) is not None)
ck('自身对自身 → 中性', re.search(r'att\s*==\s*def', body) is not None)
ck('大境界分支读 ai - di', re.search(r'big\s*=\s*ai\s*-\s*di', body) is not None)
ck('大境界压制返回 MAJOR_ADV', 'REALM_SUP_MAJOR_ADV' in body)
ck('大境界越级返回 MAJOR_DIS', 'REALM_SUP_MAJOR_DIS' in body)
ck('越级 2 档及以上返回 FLOOR', 'REALM_SUP_FLOOR' in body)
ck('小阶段差按 STEP_UP/STEP_DOWN 累乘', 'REALM_SUP_STEP_UP' in body and 'REALM_SUP_STEP_DOWN' in body)
ck('小阶段累计有上限 STEP_CAP', 'REALM_SUP_STEP_CAP' in body)
ck('缺小阶段按中期中性处理', re.search(r'if\s*\(\s*as\s*<\s*0\s*\)\s*as\s*=\s*1', body) is not None)

# ---- spirit_root_damage_info ----
body = func_body_raw(rp, 'spirit_root_damage_info')
ck('spirit_root_damage_info 存在', body != '')
ck('无灵根数据返回中性 1000', re.search(r'return\s*\(\{\s*1000\s*,\s*0', body) is not None)
ck('品质下标越界返回中性', re.search(r'q\s*<\s*0\s*\|\|\s*q\s*>=\s*6', body) is not None)
ck('灵根强度折算（* strength / 100）', re.search(r'realm_root_same_pm\[q\]\s*\*\s*strength\s*/\s*100', body) is not None)
ck('强度上限钳制 100', re.search(r'strength\s*>\s*100\s*\)\s*strength\s*=\s*100', body) is not None)
ck('变异属性按 variant 判同系', 'root["variant"]' in body)
ck('变异灵根同系只认变异属性（基础五行吃 -10%，§2.2）',
   re.search(r'q\s*==\s*1\s*&&\s*stringp\(variant\)', body) is not None)
ck('常规五行按 elements 判同系', 'root["elements"]' in body and 'member_array(name, elems)' in body)
ck('异系惩罚仅限常规五行（ELE_GOLD..ELE_EARTH）',
   re.search(r'elem\s*>=\s*ELE_GOLD\s*&&\s*elem\s*<=\s*ELE_EARTH', body) is not None)
ck('基础倍率每次都会参与（无 same 时也用 base）', 'realm_root_base_pm[q]' in body)

# ---- combat_is_spell_attack / combat_attack_element ----
body = func_body_raw(rp, 'combat_is_spell_attack')
ck('法术判定读 combat/spell_elem', 'combat/spell_elem' in body)
ck('法术判定读 combat/attack_type == "spell"', 'combat/attack_type' in body and '"spell"' in body)
body = func_body_raw(rp, 'combat_is_spell_slot')
ck('法术招式槽判定为 spells', 'attack_skill == "spells"' in body)
body = func_body_raw(rp, 'combat_attack_element')
ck('法术元素优先读 spell_elem', 'combat/spell_elem' in body)
ck('无 spell_elem 时回退角色主属性（query_character_element）', 'query_character_element' in body)

# ---- damage.h 接线守卫 ----
ck('damage.h 有境界压制修正块', '境界压制修正' in dh)
ck('damage.h 调用 realm_power_modifier_permille', 'realm_power_modifier_permille(me, victim)' in dh_code)
ck('damage.h 灵根块受法术门禁（temp 声明 或 spells 招式槽）',
   re.search(r'if\s*\(\s*combat_is_spell_attack\(me\)\s*\|\|\s*combat_is_spell_slot\(attack_skill\)\s*\)', dh_code) is not None)
ck('damage.h 伤害按千分比折算（realm）', re.search(r'damage\s*=\s*damage\s*\*\s*realm_pm\s*/\s*1000', dh_code) is not None)
ck('damage.h 伤害按千分比折算（灵根）', re.search(r'damage\s*=\s*damage\s*\*\s*root_pm\s*/\s*1000', dh_code) is not None)
ck('damage.h 灵根倍率只对法术攻击的元素取值', 'combat_attack_element(me)' in dh_code)
ck('damage.h 保留玩家可见提示（境界压制）', '【境界压制】' in dh)
ck('damage.h 保留玩家可见提示（灵根共鸣）', '【灵根共鸣】' in dh)

# 顺序：五行克制块 → 境界块 → 灵根块 → special_armor_effect
pos_elem = dh.find('五行克制修正')
pos_realm = dh.find('境界压制修正')
pos_root = dh.find('灵根法术加成')
pos_armor = dh.find('special_armor_effect(victim,me,damage)')
ck('五行块在境界块之前', -1 < pos_elem < pos_realm)
ck('境界块在灵根块之前', -1 < pos_realm < pos_root)
ck('境界/灵根修正发生在护甲特效之前', -1 < pos_root < pos_armor,
   'root=%d armor=%d' % (pos_root, pos_armor))

# 幂等性：同一函数内不应重复乘算
ck('realm_pm 只乘算一次', dh_code.count('damage = damage * realm_pm') == 1)
ck('root_pm 只乘算一次', dh_code.count('damage = damage * root_pm') == 1)

# ═══════════ 三、突变验证（守卫绑定交付物） ═══════════
print('== 三、突变验证 ==')


def guard_realm_major_adv(src):
    return hdr_int(src, 'REALM_SUP_MAJOR_ADV') == 2000


def guard_unknown_neutral(src):
    body = func_body_raw(src, 'realm_power_modifier_permille')
    return re.search(r'if\s*\(\s*ai\s*<\s*0\s*\|\|\s*di\s*<\s*0\s*\)\s*return\s+REALM_SUP_SAME', body) is not None


def guard_strength_cap(src):
    body = func_body_raw(src, 'spirit_root_damage_info')
    return re.search(r'strength\s*>\s*100\s*\)\s*strength\s*=\s*100', body) is not None


def guard_spell_gate(src):
    return re.search(r'if\s*\(\s*combat_is_spell_attack\(me\)\s*\|\|\s*combat_is_spell_slot\(attack_skill\)\s*\)', src) is not None


def guard_penalty_range(src):
    body = func_body_raw(src, 'spirit_root_damage_info')
    return re.search(r'elem\s*>=\s*ELE_GOLD\s*&&\s*elem\s*<=\s*ELE_EARTH', body) is not None


def guard_variant_same(src):
    body = func_body_raw(src, 'spirit_root_damage_info')
    return re.search(r'q\s*==\s*1\s*&&\s*stringp\(variant\)', body) is not None


# 突变1：压制系数 2.0 → 1.2（削弱境界压制）
mut = rp.replace('#define REALM_SUP_MAJOR_ADV     2000', '#define REALM_SUP_MAJOR_ADV     1200')
ck('突变1 压制系数 2000→1200 → 守卫红', not guard_realm_major_adv(mut))

# 突变2：删掉「未声明境界 → 中性」分支（历史 NPC 平衡被静默改写）
mut = rp.replace('''    if (ai < 0 || di < 0) return REALM_SUP_SAME;''',
                 '''    if (ai < 0 || di < 0) return 9999;''')
ck('突变2 未声明境界中性分支被改 → 守卫红', not guard_unknown_neutral(mut))

# 突变3：灵根强度钳制 100 → 1000（强度可夸大数值）
mut = rp.replace('''    if (strength > 100) strength = 100;''',
                 '''    if (strength > 1000) strength = 1000;''')
ck('突变3 强度钳制上限被改 → 守卫红', not guard_strength_cap(mut))

# 突变4：灵根块门禁被去掉（物理攻击也吃灵根倍率）
mut = dh.replace('if (combat_is_spell_attack(me) || combat_is_spell_slot(attack_skill))', 'if (1)')
ck('突变4 灵根门禁改 if(1) → 守卫红', not guard_spell_gate(mut))

# 突变5：异系惩罚范围判断被去掉（变异属性也吃 -10% 惩罚）
mut = rp.replace('else if (elem >= ELE_GOLD && elem <= ELE_EARTH)',
                 'else if (1)')
ck('突变5 异系惩罚范围判断被改 → 守卫红', not guard_penalty_range(mut))

# 突变6：变异灵根同系判定退回「基础五行也算同系」（雷灵根的金法被误加成）
mut = rp.replace('if (q == 1 && stringp(variant) && variant != "")',
                 'if (0)')
ck('突变6 变异灵根同系专判被去掉 → 守卫红', not guard_variant_same(mut))

# 突变7：千分比折算写错（/1000 → /100，伤害被放大 10 倍）
mut = dh.replace('damage = damage * realm_pm / 1000;', 'damage = damage * realm_pm / 100;')
ck('突变7 境界折算 /1000→/100 → 守卫红',
   re.search(r'damage\s*=\s*damage\s*\*\s*realm_pm\s*/\s*1000', mut) is None)

# ═══════════ 四、同构模拟（Python 忠实翻译 LPC 关键路径）═══════════
print('== 四、同构模拟 ==')

REALM_NAMES = ['炼气', '筑基', '结丹', '元婴', '化神', '炼虚', '合体', '大乘']
SUP_SAME = hdr_int(rp, 'REALM_SUP_SAME')
SUP_UP = hdr_int(rp, 'REALM_SUP_STEP_UP')
SUP_DOWN = hdr_int(rp, 'REALM_SUP_STEP_DOWN')
SUP_CAP = hdr_int(rp, 'REALM_SUP_STEP_CAP')
SUP_ADV = hdr_int(rp, 'REALM_SUP_MAJOR_ADV')
SUP_DIS = hdr_int(rp, 'REALM_SUP_MAJOR_DIS')
SUP_FLOOR = hdr_int(rp, 'REALM_SUP_FLOOR')
ELE_NAMES = {0: '无', 1: '金', 2: '木', 3: '水', 4: '火', 5: '土',
             6: '雷', 7: '冰', 8: '风'}
ELE_GOLD, ELE_EARTH, ELE_TOTAL_ALL = 1, 5, 8


class Ob:
    """模拟 LPC 对象：query() 走 dbase，query_temp() 走临时属性"""

    def __init__(self, dbase=None, temp=None):
        self.db = dict(dbase or {})
        self.tmp = dict(temp or {})

    def query(self, key):
        return self.db.get(key)

    def query_temp(self, key):
        return self.tmp.get(key)


def realm_index(ob):
    idx = ob.query('realm_index')
    if isinstance(idx, int) and idx > 0:
        return idx - 1
    realm = ob.query('realm')
    if not isinstance(realm, str) or realm == '':
        return -1
    for i, name in enumerate(REALM_NAMES):
        if name in realm:
            return i
    return -1


def realm_stage(ob):
    sub = ob.query('realm_sub')
    if isinstance(sub, str) and sub != '':
        if '初期' in sub:
            return 0
        if '中期' in sub:
            return 1
        if '后期' in sub or '巅峰' in sub or '大圆满' in sub:
            return 2
    realm = ob.query('realm')
    if not isinstance(realm, str):
        return -1
    if '初期' in realm:
        return 0
    if '中期' in realm:
        return 1
    if '后期' in realm:
        return 2
    return -1


def realm_pm(att, defn):
    if not isinstance(att, Ob) or not isinstance(defn, Ob) or att is defn:
        return SUP_SAME
    ai, di = realm_index(att), realm_index(defn)
    if ai < 0 or di < 0:
        return SUP_SAME
    as_, ds = realm_stage(att), realm_stage(defn)
    if as_ < 0:
        as_ = 1
    if ds < 0:
        ds = 1
    big = ai - di
    if big == 0:
        step = as_ - ds
        if step == 0:
            return SUP_SAME
        if step > 0:
            v = SUP_SAME
            for _ in range(step):
                v = v * SUP_UP // 1000
                if v > SUP_CAP:
                    return SUP_CAP
            return v
        v = SUP_SAME
        for _ in range(-step):
            v = v * SUP_DOWN // 1000
        return v
    if big > 0:
        return SUP_ADV
    if big == -1:
        return SUP_DIS
    return SUP_FLOOR


def spirit_root_info(ob, elem):
    root = ob.query('spirit_root')
    if not isinstance(root, dict):
        return 1000, 0, -1
    q = root.get('quality_idx')
    if not isinstance(q, int) or q < 0 or q >= 6:
        return 1000, 0, -1
    strength = root.get('strength')
    if not isinstance(strength, int) or strength < 0:
        strength = 0
    if strength > 100:
        strength = 100
    name = ELE_NAMES.get(elem, '无')
    if name == '无':
        return 1000, 0, q
    same = 0
    variant = root.get('variant')
    elems = root.get('elements') or []
    if q == 1 and isinstance(variant, str) and variant != '':
        # 变异灵根：同系只认变异属性（构成的基础五行按常规属性吃惩罚）
        if name == variant:
            same = 1
    elif isinstance(variant, str) and name == variant:
        same = 1
    elif name in elems:
        same = 1
    pm = base_arr[q]
    if same:
        pm += same_arr[q] * strength // 100
    elif ELE_GOLD <= elem <= ELE_EARTH:
        pm += pen_arr[q]
    return pm, same, q


def spell_attack(ob):
    if isinstance(ob.query_temp('combat/spell_elem'), int):
        return True
    return ob.query_temp('combat/attack_type') == 'spell'


def attack_element(ob):
    e = ob.query_temp('combat/spell_elem')
    if isinstance(e, int) and 0 < e <= ELE_TOTAL_ALL:
        return e
    sr = ob.query('spirit_root') or {}
    me = sr.get('main_element')
    return {v: k for k, v in ELE_NAMES.items()}.get(me, 0)


def calc_damage(me, victim, base, attack_skill='sword'):
    """calc_damage 的伤害修正路径翻译：境界 → 灵根（与 damage.h 顺序一致）"""
    dmg = base
    pm = realm_pm(me, victim)
    if pm != SUP_SAME:
        dmg = dmg * pm // 1000
    if spell_attack(me) or attack_skill == 'spells':
        rp_pm, same, q = spirit_root_info(me, attack_element(me))
        if rp_pm != 1000:
            dmg = dmg * rp_pm // 1000
    return dmg


def realm(idx, sub=None, string=None):
    db = {'realm_index': idx + 1}
    if sub is not None:
        db['realm_sub'] = sub
    if string is not None:
        db['realm'] = string
    return Ob(db)


def root(kind, elements, variant=None, strength=100, main=None):
    return Ob({'spirit_root': {
        'quality_idx': kind, 'elements': elements, 'variant': variant,
        'strength': strength, 'main_element': main or (elements[0] if elements else ''),
    }})


# 4.1 境界压制系数表逐档核对
qi_early = realm(1, sub='初期')     # realm_index=1 炼气 + 初期
qi_mid = realm(1, sub='中期')
qi_late = realm(1, sub='后期')
zhu_early = realm(2, sub='初期')
jie_early = realm(3, sub='初期')
ck('同境界同阶段 = 1.0', realm_pm(qi_early, qi_early) == 1000)
ck('同境界 +1 小阶段 = 1.2', realm_pm(qi_mid, qi_early) == 1200)
ck('同境界 +1 小阶段（反向）= 0.85', realm_pm(qi_early, qi_mid) == 850)
ck('同境界 -2 小阶段 = 0.7225→722', realm_pm(qi_early, qi_late) == 722)
ck('同境界 +2 小阶段触顶 1.44', realm_pm(qi_late, qi_early) == 1440)
ck('高 1 大境界 = 2.0（压制）', realm_pm(zhu_early, qi_mid) == 2000)
ck('低 1 大境界 = 0.5（越级）', realm_pm(qi_mid, zhu_early) == 500)
ck('低 2 大境界 = 0.25（下限）', realm_pm(qi_mid, jie_early) == 250)
ck('未声明境界一方 → 中性 1.0', realm_pm(qi_mid, Ob()) == 1000)
ck('双方未声明 → 中性 1.0', realm_pm(Ob(), Ob()) == 1000)

# realm 字符串兜底解析（#61 存储口径）
r_str = Ob({'realm': '筑基后期'})
ck('realm 字符串解析：筑基后期 → 大境界序号 1', realm_index(r_str) == 1)
ck('realm 字符串解析：筑基后期 → 小阶段 2', realm_stage(r_str) == 2)
ck('凡人（realm_index=0）→ 未声明', realm_index(Ob({'realm_index': 0})) == -1)

# 4.2 灵根加成逐档核对（02 §2.2 表）
t0_metal = root(0, ['金'], strength=98, main='金')
t1_thunder = root(1, ['金', '水'], variant='雷', strength=90, main='金')
t2_wood = root(2, ['木'], strength=75, main='木')
t4_fire = root(4, ['火'], strength=35, main='火')

ck('天灵根·金 金系法术 = 1500+300×98%=1794', spirit_root_info(t0_metal, 1)[0] == 1794)
ck('天灵根·金 火系法术 = 1500-150=1350', spirit_root_info(t0_metal, 4)[0] == 1350)
ck('天灵根·金 金系法术判同系', spirit_root_info(t0_metal, 1)[1] == 1)
ck('天灵根·金 火系法术判异系', spirit_root_info(t0_metal, 4)[1] == 0)
ck('变异雷灵根 雷法 = 1300+250×90%=1525', spirit_root_info(t1_thunder, 6)[0] == 1525)
ck('变异雷灵根 雷法判同系（variant）', spirit_root_info(t1_thunder, 6)[1] == 1)
ck('变异雷灵根 金法 = 1300-100=1200', spirit_root_info(t1_thunder, 1)[0] == 1200)
ck('变异雷灵根 雷法无常规五行惩罚（变异属性走同系判定）',
   spirit_root_info(t1_thunder, 6)[0] > spirit_root_info(t1_thunder, 5)[0])
ck('真灵根·木 木法 = 1000+150×75%=1112', spirit_root_info(t2_wood, 2)[0] == 1112)
ck('伪灵根·火 火法 = 600+50×35%=617', spirit_root_info(t4_fire, 4)[0] == 617)
ck('伪灵根 异系无惩罚（600）', spirit_root_info(t4_fire, 1)[0] == 600)
ck('无灵根数据 → 中性 1000', spirit_root_info(Ob(), 1)[0] == 1000)

# 4.3 物理攻击不吃灵根倍率（门禁守卫生效）
fighter = Ob({
    'realm_index': 1, 'realm_sub': '初期', 'spirit_root': {
        'quality_idx': 0, 'elements': ['金'], 'strength': 100, 'main_element': '金'}})
target = Ob({'realm_index': 1, 'realm_sub': '初期'})
physical = calc_damage(fighter, target, 1000)
ck('物理攻击不吃灵根倍率（伤害 = 基数）', physical == 1000, 'got %d' % physical)

# 法术攻击：写 temp 元素后生效
spellman = Ob({'realm_index': 1, 'realm_sub': '初期', 'spirit_root': {
    'quality_idx': 0, 'elements': ['金'], 'strength': 100, 'main_element': '金'}},
    temp={'combat/spell_elem': 1})
spell_hit = calc_damage(spellman, target, 1000)
ck('天灵根·金 金系法术伤害 = 1000×1.8=1800', spell_hit == 1800, 'got %d' % spell_hit)
spellman.tmp['combat/spell_elem'] = 4
fire_hit = calc_damage(spellman, target, 1000)
ck('天灵根·金 火系法术伤害 = 1000×1.35=1350', fire_hit == 1350, 'got %d' % fire_hit)

# 4.3b 法术招式槽（spells）无 temp 声明也吃灵根倍率
slot_ob = Ob({'realm_index': 1, 'realm_sub': '初期', 'spirit_root': {
    'quality_idx': 2, 'elements': ['金'], 'strength': 75, 'main_element': '金'}})
slot_hit = calc_damage(slot_ob, target, 1000, attack_skill='spells')
ck('spells 招式槽无 temp 声明也吃灵根倍率（1112）', slot_hit == 1112, 'got %d' % slot_hit)
ck('非 spells 招式槽不吃灵根倍率（1000）', calc_damage(slot_ob, target, 1000) == 1000)

# 4.4 境界压制端到端（同一法术，境界不同）
low = Ob({'realm_index': 1, 'realm_sub': '初期'}, temp={'combat/spell_elem': 1})
high = Ob({'realm_index': 2, 'realm_sub': '初期'})
ck('炼气 vs 筑基：伤害打半（1000×0.5=500）', calc_damage(low, high, 1000) == 500)
ck('筑基 vs 炼气：伤害翻倍（1000×2=2000）', calc_damage(high, low, 1000) == 2000)

# 4.5 单调性：境界越高伤害越高（同基数）
seq = [calc_damage(realm(i, sub='初期'), realm(1, sub='初期'), 1000) for i in range(1, 6)]
ck('境界提升伤害单调不减：%s' % seq, all(seq[i] <= seq[i + 1] for i in range(len(seq) - 1)))
ck('同境界对照伤害 = 基数', seq[0] == 1000)

# 4.6 灵根品质单调性：同系法术伤害随品质提升
by_quality = []
for q, strength in [(4, 35), (3, 55), (2, 75), (0, 100)]:
    ob = Ob({'realm_index': 1, 'realm_sub': '初期',
             'spirit_root': {'quality_idx': q, 'elements': ['金'],
                             'strength': strength, 'main_element': '金'}},
            temp={'combat/spell_elem': 1})
    by_quality.append(calc_damage(ob, target, 1000))
ck('灵根品质越高同系法术伤害越高：%s' % by_quality,
   all(by_quality[i] < by_quality[i + 1] for i in range(len(by_quality) - 1)))

# 4.7 综合场景：结丹天灵根·金 施展金系法术 打 炼气中期修士
att = Ob({'realm_index': 3, 'realm_sub': '初期',
          'spirit_root': {'quality_idx': 0, 'elements': ['金'],
                          'strength': 98, 'main_element': '金'}},
         temp={'combat/spell_elem': 1})
vic = Ob({'realm_index': 1, 'realm_sub': '中期'})
# 攻方高 2 大境界 → 压制 2.0；灵根 1.794 → 1000×2=2000 → ×1.794=3588
ck('结丹天灵根金法 vs 炼气中期 = 3588（2.0×1.794）', calc_damage(att, vic, 1000) == 3588,
   'got %d' % calc_damage(att, vic, 1000))

# 4.8 反向：炼气伪灵根 火法 打 结丹 修士（越级 + 灵根弱势）
att2 = Ob({'realm_index': 1, 'realm_sub': '初期',
           'spirit_root': {'quality_idx': 4, 'elements': ['火'],
                           'strength': 35, 'main_element': '火'}},
          temp={'combat/spell_elem': 4})
vic2 = Ob({'realm_index': 3, 'realm_sub': '初期'})
# 0.25 → 250，再 ×0.617 = 154
ck('炼气伪灵根火法 vs 结丹 = 154（0.25×0.617）', calc_damage(att2, vic2, 1000) == 154,
   'got %d' % calc_damage(att2, vic2, 1000))

# ═══════════ 结果 ═══════════
print('')
if fails or errors:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
