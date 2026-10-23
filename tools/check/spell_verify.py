#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#76 五行法术落地（SPELL_D + 8 个法术 skill + shifa 命令）静态校验 + 原文守卫 + 突变实证 + 同构模拟

运行：python3 tools/check/spell_verify.py

覆盖：
  一、静态校验：交付文件存在 / 括号配对状态机 / 头文件守卫与规范 / 契约对齐
                （SPELL_D ↔ skill query_spell_info、#78 combat_power_permille、#28 element.h）
                / 数据一致性（spell_ids ↔ 磁盘文件 ↔ 元数据 id / 元素 / 品阶 / 类型）
                / 1F §1.1 文档品阶对齐 / 门派传承接线（sect_d 配置 + 传功 NPC）
  二、LPC 原文守卫：cast_spell（学会/境界/冷却/法力/硬直/伤害/效果/开战）、
                query_spell_damage（等级系数/境界×灵根/五行/破甲）、query_spell_cost（境界递减）
  三、突变验证：删守卫分支 / 换回常量 / 改品阶 → 对应守卫必须变红
  四、同构模拟：技能等级系数档位 / 法力境界递减 / 五行相克 / 端到端伤害（含灼烧跳数与
                境界压制、灵根加成叠加）/ 施法流程拒绝分支

退出码：0 全绿，1 有失败
"""
import re
import sys
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0

SPELL_H = 'include/spell.h'
SPELL_D = 'adm/daemons/spell_d.c'
SHIFA = 'cmds/usr/shifa.c'
SECT_D = 'adm/daemons/sect_d.c'
ELEMENT_H = 'include/element.h'
REALM_H = 'include/combat/realm_power.h'
GLOBALS_H = 'include/globals.h'
DOC = '.knowledge/combat/1F-法术剑诀阵法战斗.md'

SPELLS = {
    # id: (中文名, 元素, 品阶, 类型, 附加效果)
    'huodan-shu':  ('火弹术', '火', 'SPELL_LOW',  'SPELL_TYPE_ATTACK',  'SPELL_FX_BURN'),
    'huohua-shu':  ('火花术', '火', 'SPELL_MID',  'SPELL_TYPE_ATTACK',  'SPELL_FX_BURN'),
    'chanrao-shu': ('缠绕术', '木', 'SPELL_MID',  'SPELL_TYPE_CONTROL', 'SPELL_FX_BIND'),
    'bingjian-shu': ('冰箭术', '水', 'SPELL_LOW', 'SPELL_TYPE_ATTACK',  'SPELL_FX_SLOW'),
    'liusha-shu':  ('流沙术', '水', 'SPELL_LOW',  'SPELL_TYPE_CONTROL', 'SPELL_FX_SLOW'),
    'dici-shu':    ('地刺术', '土', 'SPELL_MID',  'SPELL_TYPE_ATTACK',  'SPELL_FX_NONE'),
    'tufang-shu':  ('土墙术', '土', 'SPELL_HIGH', 'SPELL_TYPE_DEFEND',  'SPELL_FX_GUARD'),
    'jinren-shu':  ('金刃术', '金', 'SPELL_LOW',  'SPELL_TYPE_ATTACK',  'SPELL_FX_PIERCE'),
}

# 门派传承（sect_id, 传功 NPC 文件, 该宗法术）
SECT_SPELLS = [
    ('huangfeng_valley', 'd/yueguo/huangfeng/npc/lihuayuan.c', ['huodan-shu', 'huohua-shu', 'chanrao-shu']),
    ('yanyue_sect', 'd/yueguo/yanyue/npc/qionglao.c', ['bingjian-shu', 'liusha-shu']),
    ('tianque_fort', 'd/yueguo/tianque/npc/lanyiren.c', ['dici-shu', 'tufang-shu']),
    ('huadao_dock', 'd/yueguo/huadao/npc/huadao_npc_placeholder.c', ['jinren-shu']),
]
# 化刀坞传功 NPC 实际文件名（占位名上面会在下面被替换）
SECT_SPELLS[3] = ('huadao_dock', 'd/yueguo/huadao/npc/hantianya.c', ['jinren-shu'])


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


def spell_info(src):
    """解析 skill 文件 query_spell_info() 返回的 mapping 键值"""
    body = func_body_raw(src, 'query_spell_info')
    if not body:
        return None
    info = {}
    for k, v in re.findall(r'"([a-z_]+)"\s*:\s*"([^"]*)"', body):
        info[k] = v
    for k, v in re.findall(r'"([a-z_]+)"\s*:\s*(SPELL_[A-Z_]+)', body):
        info[k] = v
    for k, v in re.findall(r'"([a-z_]+)"\s*:\s*(\d+)', body):
        info[k] = int(v)
    return info


# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

delivery = [SPELL_H, SPELL_D, SHIFA, 'include/globals.h']
delivery += ['kungfu/skill/%s.c' % s for s in SPELLS]
for p in delivery:
    ck('exists %s' % p, os.path.isfile(p))

for p in [SPELL_H, SPELL_D, SHIFA] + ['kungfu/skill/%s.c' % s for s in SPELLS] + [SECT_D]:
    ok, detail = check_parens(p)
    ck('parens %s' % p, ok, detail)

spell_h = readf(SPELL_H)
ck('spell.h 有包含守卫', '#ifndef __SPELL_H__' in spell_h and '#define __SPELL_H__' in spell_h)
ck('spell.h 无 tab 缩进', '\t' not in spell_h)
ck('spell.h 复用 element.h 五行体系', re.search(r'#include\s+<element\.h>', spell_h) is not None)
ck('spell.h 提供 spell_element_id', re.search(r'\bint\s+spell_element_id\s*\(', spell_h) is not None)
ck('spell.h 提供 spell_tier_need', re.search(r'\bint\s+spell_tier_need\s*\(', spell_h) is not None)
for const in ['SPELL_LOW', 'SPELL_MID', 'SPELL_HIGH', 'SPELL_TYPE_ATTACK', 'SPELL_TYPE_CONTROL',
              'SPELL_TYPE_DEFEND', 'SPELL_FX_BURN', 'SPELL_FX_SLOW', 'SPELL_FX_BIND',
              'SPELL_FX_GUARD', 'SPELL_FX_PIERCE', 'SPELL_COEF_BASE', 'SPELL_COEF_PER_LEVEL',
              'SPELL_COEF_MAX', 'SPELL_BURN_RATIO', 'SPELL_BURN_TICKS', 'SPELL_BURN_INTERVAL',
              'SPELL_SLOW_DODGE', 'SPELL_BIND_DODGE', 'SPELL_BIND_PARRY', 'SPELL_GUARD_DEFENSE',
              'SPELL_PIERCE_RATIO', 'SPELL_CAST_BUSY', 'SPELL_IMPROVE_RATIO']:
    ck('spell.h 常量 %s' % const, re.search(r'#define\s+%s\b' % const, spell_h) is not None)

glob = readf(GLOBALS_H)
ck('globals.h 定义 SPELL_D 宏', re.search(r'#define\s+SPELL_D\s+"/adm/daemons/spell_d"', glob) is not None)

spell_d = readf(SPELL_D)
spell_d_code = strip_lpc(spell_d)
ck('SPELL_D 无 tab 缩进', '\t' not in spell_d)
ck('SPELL_D 读 skill 元数据（SKILL_D(id)->query_spell_info）',
   re.search(r'SKILL_D\([^)]*\)->query_spell_info\(\)', spell_d) is not None)
ck('SPELL_D 复用 #78 组合系数 combat_power_permille', 'combat_power_permille' in spell_d_code)
ck('SPELL_D 复用 #28 五行系数 calc_element_modifier', 'calc_element_modifier' in spell_d_code)
ck('SPELL_D 走 SECT_D 境界判定', 'SECT_D->query_cultivation_tier' in spell_d_code)
for fn in ['query_spell_ids', 'resolve_spell_id', 'query_spell_info', 'query_player_spells',
           'query_cooldown', 'query_spell_cost', 'query_spell_damage', 'cast_spell',
           'burn_tick', 'remove_spell_debuff']:
    ck('SPELL_D 定义 %s' % fn, re.search(r'\b%s\s*\([^;]*\)\s*\{' % fn, spell_d_code) is not None)

shifa = readf(SHIFA)
shifa_code = strip_lpc(shifa)
ck('shifa 命令继承 F_SSERVER（offensive_target）', 'inherit F_SSERVER' in shifa_code)
ck('shifa 调 SPELL_D->cast_spell', 'SPELL_D->cast_spell' in shifa_code)
ck('shifa 调 SPELL_D->resolve_spell_id', 'SPELL_D->resolve_spell_id' in shifa_code)
ck('shifa 调 SPELL_D->query_player_spells', 'SPELL_D->query_player_spells' in shifa_code)
ck('shifa 有 no_fight 场景守卫', 'no_fight' in shifa)
ck('shifa 有 uni_target 守卫', 'uni_target' in shifa)
ck('shifa 有 help', re.search(r'int\s+help\s*\(object me\)', shifa_code) is not None)

# ---- 数据一致性：spell_ids ↔ 磁盘 ↔ 元数据 ----
print('数据一致性:')
ids_in_daemon = re.findall(r'"([a-z_]+-shu)"', func_body_raw(spell_d, 'query_spell_ids') + spell_d)
# query_spell_ids 返回全局数组，直接解析数组定义
m = re.search(r'nosave string \*spell_ids = \(\{(.*?)\}\);', spell_d, re.S)
daemon_ids = re.findall(r'"([a-z_]+-shu)"', m.group(1)) if m else []
ck('SPELL_D spell_ids 数量 = 8', len(daemon_ids) == 8, str(daemon_ids))
ck('SPELL_D spell_ids 与交付法术一致', set(daemon_ids) == set(SPELLS),
   'daemon=%s' % sorted(daemon_ids))

for sid, (cname, elem, level, typ, fx) in SPELLS.items():
    path = 'kungfu/skill/%s.c' % sid
    src = readf(path)
    code = strip_lpc(src)
    info = spell_info(src)
    ck('%s 声明 query_spell_info()' % path, info is not None and info.get('id') == sid,
       'info=%s' % info)
    ck('%s 中文名 %s' % (path, cname), info and info.get('name') == cname)
    ck('%s 元素 %s' % (path, elem), info and info.get('element') == elem)
    ck('%s 品阶 %s' % (path, level), info and info.get('level') == level)
    ck('%s 类型 %s' % (path, typ), info and info.get('type') == typ)
    ck('%s 附加效果 %s' % (path, fx), info and info.get('fx') == fx)
    ck('%s 法力/威力/冷却为正' % path,
       isinstance(info.get('cost'), int) and info['cost'] > 0 and
       isinstance(info.get('power'), int) and info['power'] >= 0 and
       isinstance(info.get('cooldown'), int) and info['cooldown'] > 0)
    ck('%s valid_enable spells' % path, 'usage == "spells"' in src)
    ck('%s type() martial' % path, 'return "martial"' in src)
    ck('%s 继承 SKILL' % path, re.search(r'inherit\s+SKILL\s*;', src) is not None)
    ck('%s 引用 <spell.h>（元素/品阶常量同源）' % path, re.search(r'#include\s+<spell\.h>', src) is not None)
    ck('%s practice_skill 引导 learn/shifa' % path, 'practice_skill' in src and 'shifa' in src)

# ---- 1F §1.1 文档品阶对齐 ----
print('1F §1.1 文档对齐:')
doc = readf(DOC)
level_rows = {
    'SPELL_LOW': re.search(r'\|\s*\*\*初级下阶\*\*\s*\|([^|]+)\|', doc),
    'SPELL_MID': re.search(r'\|\s*\*\*初级中阶\*\*\s*\|([^|]+)\|', doc),
    'SPELL_HIGH': re.search(r'\|\s*\*\*初级上阶\*\*\s*\|([^|]+)\|', doc),
}
for lv, mm in level_rows.items():
    ck('文档有 %s 行' % lv, mm is not None)
if all(level_rows.values()):
    doc_low = re.findall(r'[\u4e00-\u9fa5]+术', level_rows['SPELL_LOW'].group(1))
    doc_mid = re.findall(r'[\u4e00-\u9fa5]+术', level_rows['SPELL_MID'].group(1))
    doc_high = re.findall(r'[\u4e00-\u9fa5]+术', level_rows['SPELL_HIGH'].group(1))
    doc_sets = {'SPELL_LOW': doc_low, 'SPELL_MID': doc_mid, 'SPELL_HIGH': doc_high}
    for sid, (cname, elem, level, typ, fx) in SPELLS.items():
        if level == 'SPELL_LOW' and cname == '金刃术':
            # 金刃术在「各属性法术详情」表里标为金的初级法术（不在分级表）
            ck('文档金系初级法术含金刃术',
               re.search(r'\|\s*\*\*金\*\*\s*\|[^|]*\|\s*金刃术', doc) is not None)
            continue
        ck('文档 %s 归属 %s' % (cname, level), cname in doc_sets[level],
           'doc=%s' % doc_sets[level])

# ---- 门派传承接线 ----
print('门派传承接线:')
sect_d = readf(SECT_D)
for sect_id, npc_path, spells in SECT_SPELLS:
    # 该宗门配置块
    mm = re.search(r'"%s": \(\[(.*?)\n  \]\),' % re.escape(sect_id), sect_d, re.S)
    ck('%s 配置块存在' % sect_id, mm is not None)
    block = mm.group(1) if mm else ''
    for sid in spells:
        ck('%s→%s（sect_config skills）' % (sect_id, sid), '"%s"' % sid in block)
    ck('传功 NPC %s 存在' % npc_path, os.path.isfile(npc_path))
    npc = readf(npc_path)
    for sid in spells:
        ck('传功 NPC %s 教 %s' % (os.path.basename(npc_path), sid),
           re.search(r'set_skill\("%s"' % sid, npc) is not None)

# ═══════════ 二、LPC 原文守卫 ═══════════
print('== 二、LPC 原文守卫 ==')

cast = func_body_raw(spell_d, 'cast_spell')
ck('cast_spell 存在', cast != '')
ck('未习得拒绝（query_skill <= 0）', re.search(r'lv\s*=\s*me->query_skill\(id,\s*1\)', cast) is not None)
ck('境界门槛校验（spell_tier_need）', re.search(r'tier\s*<\s*spell_tier_need\(', cast) is not None)
ck('忙碌校验（is_busy）', 'me->is_busy()' in cast)
ck('冷却校验（query_cooldown）', 'query_cooldown(me, id)' in cast)
ck('法力校验（neili < cost）', re.search(r'me->query\("neili"\)\s*<\s*cost', cast) is not None)
ck('扣法力（add("neili", -cost)）', re.search(r'me->add\("neili",\s*-cost\)', cast) is not None)
ck('写冷却（spell/cooldown/）', 'spell/cooldown/' in cast)
ck('施法硬直（start_busy(SPELL_CAST_BUSY)）', 'start_busy(SPELL_CAST_BUSY)' in cast)
ck('防御法术强制自身目标', re.search(r'SPELL_TYPE_DEFEND\)\s*target\s*=\s*me', cast) is not None)
ck('伤害经 receive_damage 落地', 'receive_damage("qi", dmg, me)' in cast)
ck('附加效果分派 switch (fx)', re.search(r'switch\s*\(fx\)', cast) is not None)
for fx in ['SPELL_FX_BURN', 'SPELL_FX_SLOW', 'SPELL_FX_BIND', 'SPELL_FX_PIERCE']:
    ck('附加效果 %s 落地' % fx, fx in cast)
ck('灼烧跳数经 call_out', re.search(r'call_out\("burn_tick"', cast) is not None)
ck('命中后开战（fight_ob）', 'fight_ob' in cast)
ck('施法积累熟练度（improve_skill）', 'improve_skill' in cast)

dmg_body = func_body_raw(spell_d, 'query_spell_damage')
ck('query_spell_damage 存在', dmg_body != '')
ck('等级系数 = BASE + lv×PER', re.search(r'SPELL_COEF_BASE\s*\+\s*skill_level\s*\*\s*SPELL_COEF_PER_LEVEL', dmg_body) is not None)
ck('等级系数上限钳制', 'SPELL_COEF_MAX' in dmg_body)
ck('境界×灵根走 combat_power_permille(...,1,elem)', 'combat_power_permille(me, victim, 1, elem)' in dmg_body)
ck('五行相克走 calc_element_modifier', 'calc_element_modifier(elem, query_character_element(victim))' in dmg_body)
ck('破甲按 SPELL_PIERCE_RATIO', 'SPELL_PIERCE_RATIO' in dmg_body)
ck('伤害下限 1', re.search(r'if\s*\(dmg\s*<\s*1\)\s*dmg\s*=\s*1', dmg_body) is not None)

cost_body = func_body_raw(spell_d, 'query_spell_cost')
ck('query_spell_cost 存在', cost_body != '')
ck('结丹（tier≥6）消耗 70%', re.search(r'tier\s*>=\s*6\)\s*cost\s*=\s*cost\s*\*\s*70\s*/\s*100', cost_body) is not None)
ck('筑基（tier≥3）消耗 85%', re.search(r'tier\s*>=\s*3\)\s*cost\s*=\s*cost\s*\*\s*85\s*/\s*100', cost_body) is not None)

burn_body = func_body_raw(spell_d, 'burn_tick')
ck('burn_tick 校验目标存活', 'living(target)' in burn_body)
ck('burn_tick 自我续跳（ticks-1）', re.search(r'ticks\s*-\s*1', burn_body) is not None)

debuff_body = func_body_raw(spell_d, 'remove_spell_debuff')
ck('remove_spell_debuff 校验 objectp', 'objectp(target)' in debuff_body)

# ═══════════ 三、突变验证 ═══════════
print('== 三、突变验证 ==')


def guard_gate(src):
    return re.search(r'tier\s*<\s*spell_tier_need\(', func_body_raw(src, 'cast_spell')) is not None


def guard_cd(src):
    return 'query_cooldown(me, id)' in func_body_raw(src, 'cast_spell')


def guard_cost(src):
    return re.search(r'me->add\("neili",\s*-cost\)', func_body_raw(src, 'cast_spell')) is not None


def guard_power(src):
    return 'combat_power_permille(me, victim, 1, elem)' in func_body_raw(src, 'query_spell_damage')


def guard_elem(src):
    body = func_body_raw(src, 'query_spell_damage')
    return ('calc_element_modifier(elem, query_character_element(victim))' in body
            and re.search(r'elem_pm\s*=\s*to_int\(elem_mod\s*\*\s*1000\)', body) is not None
            and re.search(r'dmg\s*=\s*dmg\s*\*\s*elem_pm\s*/\s*1000', body) is not None)


def guard_ids_match(src):
    m = re.search(r'nosave string \*spell_ids = \(\{(.*?)\}\);', src, re.S)
    if not m:
        return False
    ids = re.findall(r'"([a-z_]+-shu)"', m.group(1))
    return set(ids) == set(SPELLS)


# 突变1：删境界门槛 → 守卫红
mut = spell_d.replace('if (tier < spell_tier_need(info["level"]))', 'if (0)')
ck('突变1 境界门槛删除 → 守卫红', not guard_gate(mut))

# 突变2：删冷却检查 → 守卫红
mut = spell_d.replace('cd = query_cooldown(me, id);', 'cd = 0;')
ck('突变2 冷却检查删除 → 守卫红', not guard_cd(mut))

# 突变3：删法力扣除 → 守卫红
mut = spell_d.replace('me->add("neili", -cost);', '/* no cost */')
ck('突变3 法力扣除删除 → 守卫红', not guard_cost(mut))

# 突变4：境界×灵根改为常量 → 守卫红
mut = spell_d.replace('pm = combat_power_permille(me, victim, 1, elem);', 'pm = 1000;')
ck('突变4 境界/灵根系数改常量 → 守卫红', not guard_power(mut))

# 突变5：删五行相克乘算 → 守卫红
mut = spell_d.replace('dmg = dmg * elem_pm / 1000;', '/* no element */')
ck('突变5 五行相克乘算删除 → 守卫红', not guard_elem(mut))

# 突变6：注册表混入不存在法术 → 数据一致性红
mut = spell_d.replace('"jinren-shu",\n});', '"jinren-shu", "bingcan-shu",\n});')
ck('突变6 注册表混入未落地法术 → 守卫红', not guard_ids_match(mut))

# 突变7：改品阶（火弹术改中级）→ 文档对齐红
mut = readf('kungfu/skill/huodan-shu.c').replace('"level":    SPELL_LOW', '"level":    SPELL_MID')
mut_info = spell_info(mut)
ck('突变7 火弹术品阶改中阶 → 守卫红', mut_info.get('level') != SPELLS['huodan-shu'][2])

# ═══════════ 四、同构模拟 ═══════════
print('== 四、同构模拟 ==')

C_BASE = hdr_int(spell_h, 'SPELL_COEF_BASE')
C_PER = hdr_int(spell_h, 'SPELL_COEF_PER_LEVEL')
C_MAX = hdr_int(spell_h, 'SPELL_COEF_MAX')
BURN_RATIO = hdr_int(spell_h, 'SPELL_BURN_RATIO')
BURN_TICKS = hdr_int(spell_h, 'SPELL_BURN_TICKS')
PIERCE_RATIO = hdr_int(spell_h, 'SPELL_PIERCE_RATIO')
SLOW_DODGE = hdr_int(spell_h, 'SPELL_SLOW_DODGE')
BIND_DODGE = hdr_int(spell_h, 'SPELL_BIND_DODGE')
BIND_PARRY = hdr_int(spell_h, 'SPELL_BIND_PARRY')
GUARD_DEF = hdr_int(spell_h, 'SPELL_GUARD_DEFENSE')
LOW, MID, HIGH = 1, 2, 3
ATTACK, CONTROL, DEFEND = 1, 2, 3
FX_NONE, FX_BURN, FX_SLOW, FX_BIND, FX_GUARD, FX_PIERCE = 0, 1, 2, 3, 4, 5

# 4.1 技能等级系数（1F §2.2 四档）
def spell_coef(lv):
    c = C_BASE + lv * C_PER
    if c > C_MAX:
        c = C_MAX
    return c


ck('Lv1 系数 0.51（1F 0.3-0.8 档）', spell_coef(1) == 510)
ck('Lv20 系数 0.70（1F 0.3-0.8 档）', spell_coef(20) == 700)
ck('Lv60 系数 1.10（1F 0.8-1.2 档）', spell_coef(60) == 1100)
ck('Lv100 系数 1.50（1F 1.2-1.8 档）', spell_coef(100) == 1500)
ck('Lv200 系数 2.50（1F 1.8-2.5 档）', spell_coef(200) == 2500)
ck('Lv400 系数封顶 2.50', spell_coef(400) == C_MAX)

# 4.2 法力随境界递减（1F §2.3 境界消耗系数）
def spell_cost(base, tier):
    c = base
    if tier >= 6:
        c = c * 70 // 100
    elif tier >= 3:
        c = c * 85 // 100
    return max(1, c)


ck('炼气期 火弹术消耗 20', spell_cost(20, 0) == 20)
ck('筑基期 火弹术消耗 17（85%）', spell_cost(20, 3) == 17)
ck('结丹期 火弹术消耗 14（70%）', spell_cost(20, 6) == 14)
ck('消耗下限 1', spell_cost(1, 6) == 1)

# 4.3 五行相克（解析 element.h 相克表，避免与源文件漂移）
elem_src = readf(ELEMENT_H)
ELE = {}
for name in ['GOLD', 'WOOD', 'WATER', 'FIRE', 'EARTH', 'THUNDER', 'ICE', 'WIND']:
    v = hdr_int(elem_src, 'ELE_%s' % name)
    ELE[name] = v
COUNTER_ADV = float(re.search(r'#define\s+ELEM_COUNTER_ADV\s+([\d.]+)', elem_src).group(1))
COUNTER_DIS = float(re.search(r'#define\s+ELEM_COUNTER_DIS\s+([\d.]+)', elem_src).group(1))
ELE_NAME2ID = {'金': ELE['GOLD'], '木': ELE['WOOD'], '水': ELE['WATER'],
               '火': ELE['FIRE'], '土': ELE['EARTH'], '雷': ELE['THUNDER'],
               '冰': ELE['ICE'], '风': ELE['WIND']}
counter_table = {}
for m2 in re.finditer(r'ELE_([A-Z]+):\s*\(\[([^\]]*)\]\)', elem_src):
    attacker = ELE[m2.group(1)]
    row = {}
    for dname, val in re.findall(r'ELE_([A-Z]+):\s*(ELEM_COUNTER_ADV|ELEM_COUNTER_DIS|[\d.]+)', m2.group(2)):
        if val == 'ELEM_COUNTER_ADV':
            row[ELE[dname]] = COUNTER_ADV
        elif val == 'ELEM_COUNTER_DIS':
            row[ELE[dname]] = COUNTER_DIS
        else:
            row[ELE[dname]] = float(val)
    counter_table[attacker] = row


def element_mod(att, defn):
    return counter_table.get(att, {}).get(defn, 1.0)


ck('火克金 1.5', element_mod(ELE['FIRE'], ELE['GOLD']) == COUNTER_ADV)
ck('水克火 1.5', element_mod(ELE['WATER'], ELE['FIRE']) == COUNTER_ADV)
ck('火被水克 0.7', element_mod(ELE['FIRE'], ELE['WATER']) == COUNTER_DIS)
ck('火对土 1.0', element_mod(ELE['FIRE'], ELE['EARTH']) == 1.0)

# 4.4 伤害公式（query_spell_damage 翻译）
def spell_damage(power, lv, pm, att_elem, def_elem, pierce=False):
    if power <= 0:
        return 0
    d = power * spell_coef(lv) // 1000
    d = d * pm // 1000
    d = d * int(round(element_mod(att_elem, def_elem) * 1000)) // 1000
    if pierce:
        d = d * (1000 + PIERCE_RATIO) // 1000
    return max(1, d)


POWER = {sid: spell_info(readf('kungfu/skill/%s.c' % sid))['power'] for sid in SPELLS}
ck('火弹术威力 80（取自 skill 文件）', POWER['huodan-shu'] == 80)
ck('土墙术无伤害（威力 0）', POWER['tufang-shu'] == 0)

# 无境界差、无灵根（pm=1000）时的基准值
ck('火弹术 Lv50 打同属性目标 = 80', spell_damage(POWER['huodan-shu'], 50, 1000, ELE['FIRE'], ELE['FIRE']) == 80)
ck('火弹术 Lv50 克金属性 = 120（×1.5）',
   spell_damage(POWER['huodan-shu'], 50, 1000, ELE['FIRE'], ELE['GOLD']) == 120)
ck('火弹术 Lv50 被水属性克 = 56（×0.7）',
   spell_damage(POWER['huodan-shu'], 50, 1000, ELE['FIRE'], ELE['WATER']) == 56)
ck('金刃术破甲 +30% = 110', spell_damage(POWER['jinren-shu'], 50, 1000, ELE['GOLD'], ELE['GOLD'], pierce=True) == 110)
ck('等级越高伤害越高（Lv1 < Lv100）',
   spell_damage(POWER['huodan-shu'], 1, 1000, ELE['FIRE'], ELE['EARTH']) <
   spell_damage(POWER['huodan-shu'], 100, 1000, ELE['FIRE'], ELE['EARTH']))

# 4.5 境界压制 × 灵根加成叠加（#78 原语）
realm_h = readf(REALM_H)
SUP_ADV = hdr_int(realm_h, 'REALM_SUP_MAJOR_ADV')
SUP_DIS = hdr_int(realm_h, 'REALM_SUP_MAJOR_DIS')
ROOT_BASE = [int(x) for x in re.findall(r'(-?\d+)', re.search(
    r'realm_root_base_pm = \(\{(.*?)\}\)', realm_h, re.S).group(1))]
ROOT_SAME = [int(x) for x in re.findall(r'(-?\d+)', re.search(
    r'realm_root_same_pm = \(\{(.*?)\}\)', realm_h, re.S).group(1))]
ROOT_PEN = [int(x) for x in re.findall(r'(-?\d+)', re.search(
    r'realm_root_penalty_pm = \(\{(.*?)\}\)', realm_h, re.S).group(1))]


def root_pm(quality, strength, same):
    pm = ROOT_BASE[quality]
    if same:
        pm += ROOT_SAME[quality] * strength // 100
    return pm


def combo_pm(realm_permille, quality, strength, same):
    return realm_permille * root_pm(quality, strength, same) // 1000


# 天灵根·火（品质 0、强度 98）同系
pm_t0 = combo_pm(1000, 0, 98, True)
ck('天灵根火修同系组合系数 = 1794', pm_t0 == 1794)
ck('天灵根火修同系火弹术 Lv50 vs 土属性 = 80×1.794 = 143',
   spell_damage(POWER['huodan-shu'], 50, pm_t0, ELE['FIRE'], ELE['EARTH']) == 143)
pm_adv = combo_pm(SUP_ADV, 0, 98, True)
ck('高 1 大境界压制组合系数 = 2000×1.794/1000 = 3588', pm_adv == 3588)
ck('压制下单发火弹术 = 80×3.588 = 287',
   spell_damage(POWER['huodan-shu'], 50, pm_adv, ELE['FIRE'], ELE['EARTH']) == 287)
pm_dis = combo_pm(SUP_DIS, 0, 98, True)
ck('越级 1 大境界组合系数 = 500×1.794/1000 = 897', pm_dis == 897)

# 4.6 灼烧总量：初始 + 3 跳 × 25%
def burn_total(hit, ticks=BURN_TICKS, ratio=BURN_RATIO):
    tick = hit * ratio // 1000
    return hit + tick * ticks


ck('灼烧总计 = 初始 ×1.75（3 跳）', burn_total(100) == 175)
ck('灼烧跳数取自 spell.h（3）', BURN_TICKS == 3)
ck('灼烧占比取自 spell.h（25%）', BURN_RATIO == 250)

# 4.7 施法流程拒绝分支（cast_spell 关键判定翻译）
def cast_flow(skills, tier, neili, cooldown_left, info, busy=False):
    """返回 (是否成功, 原因)"""
    sid = info['id']
    if skills.get(sid, 0) <= 0:
        return False, '未习得'
    if busy:
        return False, '忙碌'
    if cooldown_left > 0:
        return False, '冷却'
    need = {LOW: 0, MID: 1, HIGH: 2}[info['level']]
    if tier < need:
        return False, '境界不足'
    cost = spell_cost(info['cost'], tier)
    if neili < cost:
        return False, '法力不足'
    return True, 'ok'


HODAN = {'id': 'huodan-shu', 'level': LOW, 'cost': 20}
TUFANG = {'id': 'tufang-shu', 'level': HIGH, 'cost': 40}
ck('未习得被拒', cast_flow({}, 0, 100, 0, HODAN)[1] == '未习得')
ck('忙碌被拒', cast_flow({'huodan-shu': 1}, 0, 100, 0, HODAN, busy=True)[1] == '忙碌')
ck('冷却中被拒', cast_flow({'huodan-shu': 1}, 0, 100, 3, HODAN)[1] == '冷却')
ck('炼气初期可施火弹术', cast_flow({'huodan-shu': 1}, 0, 100, 0, HODAN)[0])
ck('炼气初期不可施土墙术（需后期）', cast_flow({'tufang-shu': 1}, 0, 100, 0, TUFANG)[1] == '境界不足')
ck('炼气后期可施土墙术', cast_flow({'tufang-shu': 1}, 2, 100, 0, TUFANG)[0])
ck('法力不足被拒', cast_flow({'huodan-shu': 1}, 0, 10, 0, HODAN)[1] == '法力不足')

# 4.8 附加效果数值（取自 spell.h）
ck('减速 debuff 量 40 / 持续 12 秒', SLOW_DODGE == 40 and hdr_int(spell_h, 'SPELL_SLOW_DURATION') == 12)
ck('束缚同时降身法/招架', BIND_DODGE > 0 and BIND_PARRY > 0)
ck('护体减伤 60', GUARD_DEF == 60)
ck('破甲 30%', PIERCE_RATIO == 300)

# ═══════════ 结果 ═══════════
print('')
if fails or errors:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
