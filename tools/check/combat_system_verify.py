#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#63 P5-3 凡人战斗系统 整体验收（五行法术 / 剑诀 / 阵法 / 体修 / 伤害公式）

运行：python3 tools/check/combat_system_verify.py

这是父票 #63 的「整体验收」脚本，不是子票脚本的重复：
  一、子票交付与自检：四张子票的校验脚本逐个复跑（#76 法术 / #77 剑诀 / #78 伤害公式 /
                #79 体修），任何一个红 → 本脚本红
  二、c1~c6 逐条验收：五行法术落地（五系覆盖）/ 剑诀神通可施展 / 阵法布阵·效果·解除 /
                战斗流程接线（calc_damage 修正链顺序、法术与剑诀真实结算）/ 静态方法自检 /
                目录命名与复用风格
  三、验收修复守卫：#63 验收时发现并修掉的两处阵法缺陷（布阵进度推进、境界门槛字段），
                以及阵灵修为口径——守卫绑定修复后的原文，含突变实证
  四、集成同构模拟：阵法 tick 激活时序、阵法五行减伤、法术端到端伤害链、剑诀与普攻同一结算口径

退出码：0 全绿，1 有失败
"""
import re
import sys
import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

errors = []
fails = 0

SUBTICKETS = [
    ('#76 五行法术', 'tools/check/spell_verify.py'),
    ('#77 剑诀神通', 'tools/check/jianjue_verify.py'),
    ('#78 境界/灵根伤害', 'tools/check/combat_realm_verify.py'),
    ('#79 体修路径', 'tools/check/body_refine_verify.py'),
]
FORMATION = 'feature/formation.c'
BUZHEN = 'cmds/std/buzhen.c'
CHEZHEN = 'cmds/std/chezhen.c'
DAMAGE_H = 'include/combat/damage.h'
COMBATD = 'adm/daemons/combatd.c'
SPELL_D = 'adm/daemons/spell_d.c'


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


# ═══════════ 一、子票自检复跑 ═══════════
print('== 一、子票交付与自检复跑 ==')

for label, script in SUBTICKETS:
    ck('%s 校验脚本存在 %s' % (label, script), os.path.isfile(script))
    if os.path.isfile(script):
        proc = subprocess.run([sys.executable, script], capture_output=True, text=True)
        ck('%s 自检复跑 PASS' % label, proc.returncode == 0,
           'exit=%d tail=%s' % (proc.returncode, (proc.stdout or '')[-300:]))

# ═══════════ 二、c1~c6 逐条验收 ═══════════
print('== 二、c1~c6 逐条验收 ==')

# ---- c1 五行法术：五系覆盖 ----
spell_d = readf(SPELL_D)
m = re.search(r'nosave string \*spell_ids = \(\{(.*?)\}\);', spell_d, re.S)
spell_ids = re.findall(r'"([a-z_]+-shu)"', m.group(1)) if m else []
ck('c1 法术注册表存在（%d 个）' % len(spell_ids), len(spell_ids) >= 5)

ELEMENTS = set()
for sid in spell_ids:
    path = 'kungfu/skill/%s.c' % sid
    if not os.path.isfile(path):
        continue
    src = readf(path)
    mm = re.search(r'"element"\s*:\s*"([^"]+)"', src)
    if mm:
        ELEMENTS.add(mm.group(1))
ck('c1 五系齐全（金木水火土）：%s' % '、'.join(sorted(ELEMENTS)),
   {'金', '木', '水', '火', '土'}.issubset(ELEMENTS))

damage_h = readf(DAMAGE_H)
ck('c1 五行相克接进伤害（calc_element_modifier）', 'calc_element_modifier(' in damage_h)
ck('c1 法术结算走 SPELL_D->cast_spell', 'cast_spell' in readf('cmds/usr/shifa.c'))

# ---- c2 剑诀神通 ----
jal = readf('kungfu/skill/qingyuan-jianjue.c')
ck('c2 剑诀 perform 解析入口存在', 'perform_action_file' in jal)
perform_files = []
for fn in sorted(os.listdir('kungfu/skill/qingyuan-jianjue')):
    if fn.endswith('.c'):
        perform_files.append(fn)
ck('c2 剑诀神通文件 >= 3（实际 %d）' % len(perform_files), len(perform_files) >= 3)
ck('c2 神通走战斗接口 do_attack(TYPE_SPECIAL)', 'TYPE_SPECIAL' in readf('kungfu/skill/qingyuan-jianjue/jianjue.h'))

# ---- c3 阵法：布阵 / 效果 / 解除 ----
fsc = readf(FORMATION)
ck('c3 阵型数据表存在', 'nosave mapping formation_data' in fsc)
found = re.findall(r'"(name)"\s*:\s*"([^"]+阵)"', fsc)
ck('c3 阵型数量 >= 5（实际 %d）' % len(found), len(found) >= 5)
for fname in ['聚灵阵', '颠倒五行阵', '天罡北斗阵', '大庚剑阵']:
    ck('c3 阵型 %s' % fname, fname in fsc)
ck('c3 布阵命令 buzhen → start_formation', 'start_formation' in readf(BUZHEN))
ck('c3 撤阵命令 chezhen → dismiss_formation', 'dismiss_formation' in readf(CHEZHEN))
ck('c3 阵法效果 tick 存在（formation_tick）', 'void formation_tick(object me)' in fsc)
ck('c3 战斗流程每 tick 驱动阵法', 'formation_tick' in readf(COMBATD))
ck('c3 阵法伤害减免接入 combatd', 'formation_damage_modify' in readf(COMBATD))

# ---- c4 战斗系统接入 ----
ck('c4 伤害结算入口 calc_damage 被 combatd 调用', 'calc_damage(' in readf(COMBATD))
pos_elem = damage_h.find('五行克制修正')
pos_realm = damage_h.find('境界压制修正')
pos_root = damage_h.find('灵根法术加成')
pos_armor = damage_h.find('special_armor_effect(victim,me,damage)')
ck('c4 修正链顺序：五行 → 境界 → 灵根 → 护甲',
   -1 < pos_elem < pos_realm < pos_root < pos_armor,
   'elem=%d realm=%d root=%d armor=%d' % (pos_elem, pos_realm, pos_root, pos_armor))
ck('c4 境界压制消费 realm_index（#61）', 'realm_index' in readf('include/combat/realm_power.h'))
ck('c4 体修接线（空手/减伤）', 'body_refine/fist' in damage_h and 'body_refine/defense' in damage_h)
ck('c4 法术真实扣血（receive_damage）', 'receive_damage("qi"' in spell_d)
ck('c4 剑诀真实扣血（do_attack）', 'do_attack' in readf('kungfu/skill/qingyuan-jianjue/jianjue.h'))

# ---- c5 静态验收方法 ----
ck('c5 子票校验脚本统一落 tools/check/', len(SUBTICKETS) == 4 and all(p.startswith('tools/check/') for _, p in SUBTICKETS))
kb_idx = readf('document/knowledge/INDEX-by-cluster.md')
for entry in ['combat-realm-spirit-root-damage', 'elemental-spell-casting',
              'sword-art-perform-unlock', 'body-refinement-cultivation']:
    ck('c5 KB 条目已沉淀 %s' % entry, entry in kb_idx)

# ---- c6 风格一致 ----
ck('c6 法术 skill 命名 kebab-case', all(re.match(r'^[a-z0-9]+(-[a-z0-9]+)*-shu$', s) for s in spell_ids))
ck('c6 剑诀神通目录复用 skills/<skill>/<action>.c', os.path.isdir('kungfu/skill/qingyuan-jianjue'))
ck('c6 新 daemon 统一 adm/daemons/', os.path.isfile('adm/daemons/body_d.c') and os.path.isfile(SPELL_D))
ck('c6 无硬编码仓库专属路径（/Users/ /playground）',
   '/Users/' not in damage_h + spell_d + fsc and 'playground' not in damage_h + spell_d + fsc)

# ═══════════ 三、验收修复守卫（#63 发现的两处阵法缺陷）═══════════
print('== 三、验收修复守卫 ==')

tick_body = func_body_raw(fsc, 'formation_tick')
ck('修复1 formation_tick 未激活时推进 progress_formation',
   re.search(r'if\s*\(\s*!formation_active\s*\)[\s\S]{0,200}?progress_formation\(\)', tick_body) is not None)
ck('修复1 进度推进在效果结算之前', tick_body.find('progress_formation()') < tick_body.find('formation_tick_count++'))

buzhen = readf(BUZHEN)
ck('修复2 布阵境界门槛改用 SECT_D->query_cultivation_tier',
   'SECT_D->query_cultivation_tier(me)' in buzhen)
ck('修复2 不再使用不存在的 realm_level 字段', 'realm_level' not in buzhen)
ck('修复2 大庚剑阵门槛 = 筑基（SECT_TIER_ZHU）',
   re.search(r'formation_type\s*==\s*4\s*&&\s*tier\s*<\s*SECT_TIER_ZHU', buzhen) is not None)
ck('修复2 颠倒五行阵门槛 = 炼气后期（SECT_TIER_QI_LATE）',
   re.search(r'formation_type\s*==\s*2\s*&&\s*tier\s*<\s*SECT_TIER_QI_LATE', buzhen) is not None)

spirit_body = func_body_raw(fsc, 'summon_formation_spirit')
ck('修复3 阵灵强度按 SECT_D tier 计算', 'SECT_D->query_cultivation_tier(ob)' in spirit_body)
ck('修复3 阵灵不再读 realm_level 字段', 'query("realm_level")' not in spirit_body)


# 突变实证
def guard_tick_progress(src):
    b = func_body_raw(src, 'formation_tick')
    return re.search(r'if\s*\(\s*!formation_active\s*\)[\s\S]{0,200}?progress_formation\(\)', b) is not None


def guard_tier_gate(src):
    return ('SECT_D->query_cultivation_tier(me)' in src and 'realm_level' not in src)


def guard_spirit_tier(src):
    b = func_body_raw(src, 'summon_formation_spirit')
    return 'SECT_D->query_cultivation_tier(ob)' in b and 'query("realm_level")' not in b


def guard_damage_chain(src):
    return -1 < src.find('五行克制修正') < src.find('境界压制修正') < src.find('灵根法术加成')


def guard_five_elements(ids, reader):
    els = set()
    for sid in ids:
        mm = re.search(r'"element"\s*:\s*"([^"]+)"', reader('kungfu/skill/%s.c' % sid))
        if mm:
            els.add(mm.group(1))
    return {'金', '木', '水', '火', '土'}.issubset(els)


mut = fsc.replace('            progress_formation();\n', '')
ck('突变1 formation_tick 不推进进度 → 守卫红', not guard_tick_progress(mut))

mut = buzhen.replace('int tier = SECT_D->query_cultivation_tier(me);',
                     'int tier = me->query("realm_level");')
ck('突变2 布阵门槛改回 realm_level → 守卫红', not guard_tier_gate(mut))

mut = fsc.replace('total_realm += SECT_D->query_cultivation_tier(ob);',
                  'total_realm += ob->query("realm_level");')
ck('突变3 阵灵改回 realm_level → 守卫红', not guard_spirit_tier(mut))

mut = damage_h.replace('// ===== 境界压制修正 =====', '// ===== 境界压制修正（挪位）=====')
mut = mut.replace('// ===== 五行克制修正 =====', '// ===== X =====')
ck('突变4 修正链顺序被打乱 → 守卫红', not guard_damage_chain(mut))

mut_ids = [s for s in spell_ids if s != 'jinren-shu']
ck('突变5 金系法术缺失 → 五系覆盖守卫红', not guard_five_elements(mut_ids, readf))

# ═══════════ 四、集成同构模拟 ═══════════
print('== 四、集成同构模拟 ==')

# 4.1 阵法激活时序（formation_tick / progress_formation 翻译）
def formation_activate(setup_time):
    """返回 (首次生效 tick 序号, 总 tick 数)"""
    setup = setup_time
    active = 0
    first_effect = None
    for tick in range(1, 20):
        if not active:
            if setup > 0:
                setup -= 1
                if setup <= 0:
                    active = 1
            continue
        if first_effect is None:
            first_effect = tick
            break
    return first_effect


ck('聚灵阵（3 回合）第 4 tick 起生效', formation_activate(3) == 4)
ck('天罡北斗阵（2 回合）第 3 tick 起生效', formation_activate(2) == 3)
ck('大庚剑阵（4 回合）第 5 tick 起生效', formation_activate(4) == 5)

# 4.2 阵法五行减伤（formation_damage_modify 翻译）
def formed_damage(damage, ftype, resist, active):
    if not active:
        return damage
    if ftype == 2:      # 颠倒五行阵
        return damage * (100 - resist) // 100
    return damage


ck('未激活时阵法不减伤（伤害不变）', formed_damage(1000, 2, 30, False) == 1000)
ck('颠倒五行阵激活后五行伤害 -30%', formed_damage(1000, 2, 30, True) == 700)
ck('非颠倒阵不减伤', formed_damage(1000, 3, 30, True) == 1000)

# 4.3 布阵境界门槛（SECT tier 口径）
SECT_TIER_QI_LATE = 2
SECT_TIER_ZHU = 3
SECT_TIER_JIE = 6


def can_setup(ftype, tier):
    if ftype == 4 and tier < SECT_TIER_ZHU:
        return False
    if ftype == 2 and tier < SECT_TIER_QI_LATE:
        return False
    return True


ck('炼气初期（tier 0）不可布颠倒五行阵', not can_setup(2, 0))
ck('炼气后期（tier 2）可布颠倒五行阵', can_setup(2, 2))
ck('炼气后期（tier 2）不可布大庚剑阵', not can_setup(4, 2))
ck('筑基初期（tier 3）可布大庚剑阵', can_setup(4, 3))
ck('结丹期（tier 6）两类阵皆可布', can_setup(2, 6) and can_setup(4, 6))

# 4.4 法术端到端伤害链（与 #76/#78 同口径）
COEF_BASE, COEF_PER, COEF_MAX = 500, 10, 2500
SUP_ADV, ROOT_T0_SAME = 2000, 1794


def spell_end_to_end(power, lv, realm_pm, root_pm, elem_pm):
    coef = min(COEF_MAX, COEF_BASE + lv * COEF_PER)
    d = power * coef // 1000
    d = d * realm_pm // 1000
    d = d * root_pm // 1000
    d = d * elem_pm // 1000
    return max(1, d)


ck('法术端到端：80 威力 Lv50 同境界同灵根同属性 = 80',
   spell_end_to_end(80, 50, 1000, 1000, 1000) == 80)
ck('法术端到端：天灵根同系（1.794）→ 143', spell_end_to_end(80, 50, 1000, ROOT_T0_SAME, 1000) == 143)
ck('法术端到端：压制 1 大境界（2.0）+ 天灵根同系 → 287',
   spell_end_to_end(80, 50, SUP_ADV, ROOT_T0_SAME, 1000) == 287)
ck('法术端到端：五行克制 1.5 倍 → 214',
   spell_end_to_end(80, 50, 1000, ROOT_T0_SAME, 1500) == 214)

# 4.5 剑诀与普攻同一结算口径
ck('剑诀经 do_attack → calc_damage（同一境界/五行口径）',
   'calc_damage' in readf(COMBATD) and 'do_attack' in readf('kungfu/skill/qingyuan-jianjue/jianjue.h'))
ck('体修加成只在普攻链（空手/重剑），法术不吃（互不污染）',
   'body_refine/fist' in damage_h and 'body_refine/fist' not in spell_d)

# ═══════════ 结果 ═══════════
print('')
if fails or errors:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
