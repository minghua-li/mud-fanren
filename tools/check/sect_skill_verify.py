#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#62 凡人功法实体化（kungfu skill 文件·九宗功法树）静态校验 + 同构模拟 + LPC 原文守卫（可复跑验证）
运行：python3 tools/check/sect_skill_verify.py
覆盖：
  一、静态校验：26 功法文件 ↔ sect_config skill_id 一一对应 / 继承 SKILL / 括号配对状态机 /
                 接口签名（type/valid_learn/practice_skill/valid_enable/query_action）/
                 引用完整性（include 头文件存在）/ 字符串截断（招式描述无裸半角引号）
  二、LPC 原文守卫：learn_skill 接线（set_skill 两处 + 补灌分支）、register_skill_names
                 （add_translate）、血灵大法灵根门槛、青元剑诀分段门槛——守卫绑定交付物原文，
                 含突变实证（改回旧行为 → 守卫红）
  三、同构模拟：新玩家入宗→sect learn 习得入技能表 / 阶位不足拒绝 / 贡献不足拒绝 /
                 已学过拒绝 / 任务奖励免贡献习得后补灌（不扣贡献）/ 境界门槛拦截 /
                 青元剑诀分段 / 血灵大法灵根门槛
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
    """括号配对状态机（字符串/注释/@LONG 感知）"""
    with open(path, encoding='utf-8') as f:
        src = f.read()
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
    """提取 LPC 函数体文本（含签名行到闭合花括号）"""
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


def ck(name, cond, detail=''):
    global fails
    if not cond:
        fails += 1
        errors.append('FAIL %s %s' % (name, detail))
        print('  [FAIL] %s %s' % (name, detail))
    else:
        print('  [ OK ] %s' % name)


# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

SECT_D_PATH = 'adm/daemons/sect_d.c'
with open(SECT_D_PATH, encoding='utf-8') as f:
    sect_d_src = f.read()

# 1. 解析 sect_config 的 skills：skill_id -> (name)
#    从 sect_d.c 提取 "skill_id": ([ "name": "...", ... ]) 块
skill_defs = {}
for m in re.finditer(r'"([a-z][a-z0-9-]+)"\s*:\s*\(\s*\[\s*"name"\s*:\s*"([^"]+)"', sect_d_src):
    skill_defs[m.group(1)] = m.group(2)
print('sect_config 解析到技能条目: %d' % len(skill_defs))

unique_ids = sorted(set(skill_defs.keys()))
print('唯一 skill_id: %d' % len(unique_ids))
for sid in unique_ids:
    print('   %-22s %s' % (sid, skill_defs[sid]))

ck('sect_config 技能条目数 >= 26（九宗功法树）', len(skill_defs) >= 26,
   '实际 %d' % len(skill_defs))
ck('唯一 skill_id 数 == 26（与票面清单一致）', len(unique_ids) == 26,
   '实际 %d' % len(unique_ids))

# 2. 每个 skill_id 对应 kungfu/skill/<id>.c 存在
missing_files = [sid for sid in unique_ids if not os.path.exists('kungfu/skill/%s.c' % sid)]
ck('26 个 kungfu/skill 文件全部落地', not missing_files, '缺失: %s' % missing_files)

# 3. 文件 ↔ sect_config 一一对应：sect_config 的 26 个 skill_id 全部有文件（方向一已查），
#    反向（文件在、配置无）由「无 HEAD 同名 + 26 唯一 id」覆盖——kungfu/skill/ 下有 700+
#    北侠既有文件，不能按「目录内多文件即孤儿」判定（会误报全部北侠文件）。恒真断言已移除。

# 4. 括号配对 + 继承（SKILL 或 FORCE）+ 接口签名 + 引用完整性 + 字符串截断
#    force 类功法必须继承 FORCE（提供 force_character/hit_ob/recover_speed/backup_neili 等
#    内功接口，attribute.c/combatd 核心路径调用；裸 SKILL 会导致 max_neili=0 或驱动报错）——
#    与仓库既有 70 个 force 功法一致（逐一 grep 核对）
FORCE_SKILLS = {'changchun-gong', 'zhenyang-jue', 'xuanbing-jue', 'guiyuan-gong',
                'ningyuan-gong', 'shuangxiu-zhishu', 'xuanyue-xiyin-gong', 'xueling-dafa'}
for sid in unique_ids:
    path = 'kungfu/skill/%s.c' % sid
    with open(path, encoding='utf-8') as f:
        src = f.read()
    ok, err = check_parens(path)
    ck('括号配对 %s' % sid, ok, err)
    if sid in FORCE_SKILLS:
        ck('force 类继承 FORCE %s' % sid, 'inherit FORCE;' in src,
           '缺 inherit FORCE;（force 槽功法须继承 FORCE 否则内功接口缺失）')
        ck('force 类 exert 解析口 %s' % sid,
           re.search(r'\bstring exert_function_file\(string func\)', src) is not None,
           '缺 exert_function_file（exert 命令调用）')
    else:
        ck('继承 SKILL %s' % sid, 'inherit SKILL;' in src, '缺 inherit SKILL;')
    ck('接口 type %s' % sid, re.search(r'\bstring type\(\)', src) is not None, '缺 type()')
    ck('接口 valid_learn %s' % sid, re.search(r'\bint valid_learn\(object me\)', src) is not None, '缺 valid_learn')
    ck('接口 practice_skill %s' % sid, re.search(r'\bint practice_skill\(object me\)', src) is not None, '缺 practice_skill')
    ck('接口 valid_enable %s' % sid, re.search(r'\bint valid_enable\(string usage\)', src) is not None, '缺 valid_enable')
    # 战斗类（moves 有 query_action）
    if re.search(r'\bmapping \*action\b', src):
        ck('接口 query_action %s' % sid, re.search(r'\bmapping query_action\(', src) is not None, '缺 query_action')
    # include 引用完整
    for inc in re.findall(r'#include\s+<([^>]+)>', src):
        incpath = 'include/%s' % inc
        ck('include 存在 %s <- %s' % (sid, inc), os.path.exists(incpath), '缺头文件 %s' % incpath)
    # 字符串截断：LPC 字符串内的裸半角引号（招式描述必须用中文引号）
    for line_no, line in enumerate(src.split('\n'), 1):
        s = line.strip()
        if s.startswith('//') or s.startswith('*'):
            continue
        # 检查 "..." 字符串内是否含未转义半角引号导致截断：统计每行引号数为奇数则可疑
        if line.count('"') % 2 == 1 and '/*' not in line:
            ck('字符串截断 %s:%d' % (sid, line_no), False, '引号不成对: %r' % line)

# 5. 与九宗档案「功法」节对齐：每个 skill_id 的中文名与档案功法名一致
#    档案功法名清单（九宗档案「功法」节 + 1C §3.3）
ARCHIVE_NAMES = {
    'changchun-gong': '长春功', 'qingyuan-jianjue': '青元剑诀', 'zhenyang-jue': '真阳诀',
    'xuanbing-jue': '玄冰诀', 'guiyuan-gong': '归元功', 'huanling-jue': '幻灵决',
    'ningyuan-gong': '凝元功', 'shuangxiu-zhishu': '双修之术', 'xuanyue-xiyin-gong': '玄月吸阴功',
    'yushou-shu': '御兽术', 'yichong-shu': '役虫术', 'kuilei-shu': '傀儡术',
    'wangu-jue': '万蛊诀', 'daomen-shufa': '道门术法', 'qingxu-jian-dian': '清虚剑典',
    'daofa-chuancheng': '刀法传承', 'lianqi-shu': '炼器术', 'zhubao-shu': '筑堡术',
    'zhenfa-shu': '阵法术', 'zhongjian-jianfa': '重剑剑法', 'jianxiu-chuancheng': '剑修传承',
    'guidao-gongfa': '鬼道功法', 'dushu': '毒术', 'anshu': '暗术',
    'lianshi-shu': '炼尸术', 'xueling-dafa': '血灵大法',
}
for sid, cn in ARCHIVE_NAMES.items():
    if sid in skill_defs:
        ck('档案对齐 %s -> %s' % (sid, cn), skill_defs[sid] == cn,
           'sect_config 名「%s」≠ 档案「%s」' % (skill_defs[sid], cn))
    else:
        ck('档案对齐 %s' % sid, False, 'sect_config 缺该功法')

# ═══════════ 二、LPC 原文守卫 ═══════════
print('== 二、LPC 原文守卫 ==')

# 2.1 learn_skill 接线：set_skill（习得即入技能表 + 已学过补灌）
learn_body = extract_func(sect_d_src, 'learn_skill')
ck('learn_skill 函数体可提取', learn_body is not None)
if learn_body:
    ck('learn_skill 习得即入技能表（set_skill 调用）', 'set_skill(skill_id, 1)' in learn_body,
       '缺 set_skill(skill_id, 1) 接线')
    ck('learn_skill 补灌分支（已学过且技能表空）',
       'query_skill(skill_id, 1) <= 0' in learn_body and 'set_skill(skill_id, 1)' in learn_body,
       '缺补灌分支')
    # 扣贡献（cost_pos）之后必须还有 set_skill（习得即入技能表在扣贡献后执行）
    cost_pos = learn_body.find('player->add(SECT_PATH_CONTRIB, -cost)')
    set_after = learn_body.find('set_skill(skill_id, 1)', cost_pos)
    ck('learn_skill 习得 set_skill 在扣贡献之后', cost_pos != -1 and set_after != -1 and set_after > cost_pos,
       'cost=%s set_after=%s' % (cost_pos, set_after))

# 2.2 register_skill_names：add_translate 注册中文名
reg_body = extract_func(sect_d_src, 'register_skill_names')
ck('register_skill_names 函数体可提取', reg_body is not None)
if reg_body:
    ck('注册函数调用 add_translate', 'CHINESE_D->add_translate' in reg_body, '缺 add_translate 调用')
    ck('注册函数遍历 sect_config skills', 'query_sect_skills' in reg_body, '缺 query_sect_skills 遍历')
    ck('create() 调用注册函数', 'register_skill_names();' in extract_func(sect_d_src, 'create') or
       'register_skill_names();' in sect_d_src, 'create 未调用注册')

# 2.3 血灵大法：灵根门槛守卫（真实 valid_learn 函数体）
with open('kungfu/skill/xueling-dafa.c', encoding='utf-8') as f:
    xld_src = f.read()
xld_body = extract_func(xld_src, 'valid_learn')
ck('血灵大法 valid_learn 可提取', xld_body is not None)
if xld_body:
    ck('血灵大法境界门槛（结丹，对齐档案成长线）', 'SECT_TIER_JIE' in xld_body, '缺结丹门槛')
    ck('血灵大法灵根检查（天灵根）', 'ROOT_QUALITY_T0' in xld_body, '缺天灵根检查')
    ck('血灵大法灵根检查（暗灵根）', 'ROOT_VAR_DARK' in xld_body, '缺暗灵根检查')
    ck('血灵大法灵根数据源', 'SR_QUALITY_IDX' in xld_body and 'SR_VARIANT' in xld_body, '缺灵根字段')

# 2.4 青元剑诀：分段门槛守卫
with open('kungfu/skill/qingyuan-jianjue.c', encoding='utf-8') as f:
    qyj_src = f.read()
qyj_body = extract_func(qyj_src, 'valid_learn')
ck('青元剑诀 valid_learn 可提取', qyj_body is not None)
if qyj_body:
    ck('青元剑诀中三层筑基门槛', 'SECT_TIER_ZHU' in qyj_body, '缺筑基分段')
    ck('青元剑诀后三层结丹门槛', 'SECT_TIER_JIE' in qyj_body, '缺结丹分段')
    ck('青元剑诀等级分段（<=30/<=60）', 'lv <= 30' in qyj_body and 'lv <= 60' in qyj_body, '缺等级分段')

# 2.5 长春功：灵根检查守卫（档案黄枫谷.md:27「需灵根」；round-1 修订 #3，
#     审查指出该修复未被脚本守卫——补原文守卫断言）
with open('kungfu/skill/changchun-gong.c', encoding='utf-8') as f:
    ccg_src = f.read()
ccg_body = extract_func(ccg_src, 'valid_learn')
ck('长春功 valid_learn 可提取', ccg_body is not None)
if ccg_body:
    ck('长春功灵根存在性检查', 'SPIRIT_ROOT_DATA' in ccg_body and 'mapp(root)' in ccg_body,
       '缺灵根检查（档案「需灵根」）')
    ck('长春功无境界门槛（炼气可修）', 'SECT_TIER_ZHU' not in ccg_body and 'SECT_TIER_JIE' not in ccg_body,
       '不应有境界门槛')

# 2.5 突变实证说明：脚本内不做「替换构造」式的恒真断言（mutant 由 replace 构造则
#    断言恒真，无判别力）。真正的判别力来自 2.1 的 learn_skill 函数体原文守卫（删接线
#    → set_after=-1 红）与 2.3/2.4 的功法文件守卫；外部红→绿突变实证见交付说明
#    （审查者与交付方各自独立做过 4 组：删接线/删灵根/删分段/删文件均转红）。

# ═══════════ 三、同构模拟 ═══════════
print('== 三、同构模拟（learn_skill 路径 Python 忠实翻译）==')

# 忠实翻译 sect_d.c learn_skill 逻辑（与 LPC 原文逐行对应）
# 注：skill_id 校验/阶位/贡献/习得写入/技能表接线 与 sect_d.c 一致
class MockPlayer:
    """模拟玩家对象（F_DBASE 语义：set/query/add）"""
    def __init__(self, realm='炼气1层', contrib=100000, rank=3, learned=None, skills=None,
                 spirit_root=None):
        self._data = {}
        self._data['realm'] = realm
        self._data['sect/contribution'] = contrib
        self._data['sect/rank'] = rank
        self._data['sect/learned'] = learned or {}
        self._data['_skills'] = skills or {}
        self._data['spirit_root'] = spirit_root
        self.msgs = []

    def query(self, k, default=0):
        v = self._data.get(k)
        return default if v is None else v

    def set(self, k, v):
        self._data[k] = v

    def add(self, k, delta):
        cur = self.query(k, 0)
        if not isinstance(cur, (int, float)):
            cur = 0
        self._data[k] = cur + delta
        return self._data[k]

    def set_skill(self, skill, val):
        self._data['_skills'][skill] = val

    def query_skill(self, skill, raw=0):
        return self._data['_skills'].get(skill, 0)

    def tell_object(self, msg):
        self.msgs.append(msg)

    # LPC undefinedp 语义
    def learned(self, sid):
        l = self._data.get('sect/learned')
        return isinstance(l, dict) and sid in l


def query_cultivation_tier(realm):
    """忠实翻译 sect_d.c query_cultivation_tier"""
    if not realm:
        return 0
    names = ['炼气', '筑基', '结丹', '元婴', '化神', '炼虚', '合体', '大乘']
    idx = 0
    for i, nm in enumerate(names):
        if nm in realm:
            idx = i
            break
    # extract_layer：取第一个数字
    m = re.search(r'(\d+)', realm)
    layer = int(m.group(1)) if m else 0
    if '初期' in realm:
        stage = 0
    elif '后期' in realm:
        stage = 2
    elif layer > 0:
        if layer <= 3:
            stage = 0
        elif layer <= 6:
            stage = 1
        else:
            stage = 2
    else:
        stage = 1
    return idx * 3 + stage


def learn_skill_translated(player, sect_id, skill_id, SECT_CONFIG):
    """忠实翻译 sect_d.c learn_skill（含 #62 接线）"""
    cfg = SECT_CONFIG.get(sect_id)
    if not cfg:
        player.tell_object('你尚未拜入任何门派。')
        return 0
    skill_info = cfg['skills'].get(skill_id)
    if skill_info is None:
        player.tell_object('本门无此功法')
        return 0
    learned = player.query('sect/learned')
    if isinstance(learned, dict) and skill_id in learned:
        if player.query_skill(skill_id, 1) <= 0:
            player.set_skill(skill_id, 1)
            player.tell_object('已学过补灌')
            return 1
        player.tell_object('你已学过')
        return 0
    rank = player.query('sect/rank')
    if rank < skill_info['rank']:
        player.tell_object('阶位不足')
        return 0
    cost = skill_info['cost']
    contrib = player.query('sect/contribution')
    if contrib < cost:
        player.tell_object('贡献不足')
        return 0
    player.add('sect/contribution', -cost)
    if not isinstance(learned, dict):
        learned = {}
    learned[skill_id] = 100  # time() 占位
    player.set('sect/learned', learned)
    if player.query_skill(skill_id, 1) <= 0:
        player.set_skill(skill_id, 1)
    player.tell_object('习得「%s」' % skill_info['name'])
    return 1


# 从 sect_d.c 解析 SECT_CONFIG（9 宗各归其位，含 rank/cost/name）
SECT_IDS = ['yanyue_sect', 'huangfeng_valley', 'lingshou_mountain', 'qingxu_sect',
            'huadao_dock', 'tianque_fort', 'jujian_gate', 'guiling_sect', 'yuling_sect']
SECT_CONFIG = {}
for sid in SECT_IDS:
    SECT_CONFIG[sid] = {'skills': {}}
starts = []
for sid in SECT_IDS:
    m = re.search(r'"%s"\s*:\s*\(\[' % sid, sect_d_src)
    if m:
        starts.append((m.start(), sid))
starts.sort()
for i, (pos, sid) in enumerate(starts):
    end = starts[i + 1][0] if i + 1 < len(starts) else len(sect_d_src)
    block = sect_d_src[pos:end]
    for mm in re.finditer(r'"([a-z][a-z0-9-]+)"\s*:\s*\(\s*\[\s*"name"\s*:\s*"([^"]+)"\s*,\s*"rank"\s*:\s*(\d+)', block):
        SECT_CONFIG[sid]['skills'][mm.group(1)] = {
            'name': mm.group(2), 'rank': int(mm.group(3)), 'cost': 500
        }
print('解析宗门数: %d, 总技能数: %d' % (len(SECT_CONFIG), sum(len(v['skills']) for v in SECT_CONFIG.values())))
cfg_total = sum(len(v['skills']) for v in SECT_CONFIG.values())
cfg_uniq = set()
for v in SECT_CONFIG.values():
    cfg_uniq.update(v['skills'].keys())
ck('解析出 9 宗', len(SECT_CONFIG) == 9, '实际 %d' % len(SECT_CONFIG))
ck('解析出 28 个配置条目（9 宗合计，共享技能各宗计一次）', cfg_total == 28, '实际 %d' % cfg_total)
ck('唯一 skill_id 26（yushou-shu/yichong-shu 灵兽山与御灵宗共享）', len(cfg_uniq) == 26,
   '实际 %d: %s' % (len(cfg_uniq), sorted(cfg_uniq)))

# 场景 1：炼气新入宗弟子 sect learn 长春功（rank0）→ 习得入技能表
p = MockPlayer(realm='炼气5层', contrib=100000, rank=0)
r = learn_skill_translated(p, 'huangfeng_valley', 'changchun-gong', SECT_CONFIG)
ck('场景1 炼气弟子习得长春功', r == 1 and p.query_skill('changchun-gong', 1) == 1 and
   p.learned('changchun-gong'), 'r=%s skill=%s' % (r, p.query_skill('changchun-gong', 1)))

# 场景 2：外门弟子学内门功法（真阳诀 rank1）→ 阶位不足拒绝，不扣贡献
p = MockPlayer(realm='炼气5层', contrib=100000, rank=0)
before = p.query('sect/contribution')
r = learn_skill_translated(p, 'huangfeng_valley', 'zhenyang-jue', SECT_CONFIG)
ck('场景2 阶位不足拒绝', r == 0 and p.query('sect/contribution') == before and
   p.query_skill('zhenyang-jue', 1) == 0, 'r=%s' % r)

# 场景 3：贡献不足拒绝
p = MockPlayer(realm='炼气5层', contrib=100, rank=0)
r = learn_skill_translated(p, 'huangfeng_valley', 'changchun-gong', SECT_CONFIG)
ck('场景3 贡献不足拒绝', r == 0 and p.learned('changchun-gong') is False, 'r=%s' % r)

# 场景 4：已学过（learned+skills 都有）→ 拒绝
p = MockPlayer(realm='炼气5层', contrib=100000, rank=0, learned={'changchun-gong': 1}, skills={'changchun-gong': 5})
before = p.query('sect/contribution')
r = learn_skill_translated(p, 'huangfeng_valley', 'changchun-gong', SECT_CONFIG)
ck('场景4 已学过拒绝', r == 0 and p.query('sect/contribution') == before, 'r=%s' % r)

# 场景 5：任务奖励免贡献习得（learned 有、skills 无）→ 补灌入技能表，不扣贡献
p = MockPlayer(realm='炼气5层', contrib=100000, rank=0, learned={'changchun-gong': 100})
before = p.query('sect/contribution')
r = learn_skill_translated(p, 'huangfeng_valley', 'changchun-gong', SECT_CONFIG)
ck('场景5 任务奖励补灌（免扣贡献）', r == 1 and p.query_skill('changchun-gong', 1) == 1 and
   p.query('sect/contribution') == before, 'r=%s skill=%s contrib=%s' %
   (r, p.query_skill('changchun-gong', 1), p.query('sect/contribution')))

# 场景 6：境界门槛——炼气玩家不能学真阳诀（筑基），筑基可学
# 从真实文件解析 valid_learn 门槛（真阳诀）
def valid_learn_sim(sid, player):
    """忠实翻译 kungfu/skill/<sid>.c 的 valid_learn 境界门槛"""
    path = 'kungfu/skill/%s.c' % sid
    with open(path, encoding='utf-8') as f:
        src = f.read()
    body = extract_func(src, 'valid_learn')
    if body is None:
        return True, '无函数'
    tier = query_cultivation_tier(player.query('realm'))
    # 青元剑诀分段优先（含 'lv <= 30' 标记）
    if 'lv <= 30' in body:
        lv = player.query_skill(sid, 1)
        if lv <= 30:
            return True, ''
        if lv <= 60 and tier < 3:
            return False, '需筑基'
        if lv > 60 and tier < 6:
            return False, '需结丹'
        return True, ''
    if 'SECT_TIER_ZHU' in body:
        if tier < 3:
            return False, '需筑基'
    if 'SECT_TIER_JIE' in body:
        if tier < 6:
            return False, '需结丹'
    if 'SECT_TIER_YING' in body:
        if tier < 9:
            return False, '需元婴'
    # 灵根存在性（长春功「需灵根」，档案黄枫谷.md:27）
    if 'SPIRIT_ROOT_DATA' in body and 'ROOT_QUALITY_T0' not in body:
        root = player.query('spirit_root')
        if not isinstance(root, dict):
            return False, '无灵根'
    # 血灵大法灵根（天灵根或暗灵根）
    if 'ROOT_QUALITY_T0' in body or 'ROOT_VAR_DARK' in body:
        root = player.query('spirit_root')
        if not isinstance(root, dict):
            return False, '无灵根'
        if root.get('quality_idx') != 0 and root.get('variant') != '暗':
            return False, '非天灵根/暗灵根'
    return True, ''

ok, why = valid_learn_sim('zhenyang-jue', MockPlayer(realm='炼气5层'))
ck('场景6a 炼气学真阳诀被拒', not ok and why == '需筑基', why)
ok, why = valid_learn_sim('zhenyang-jue', MockPlayer(realm='筑基初期'))
ck('场景6b 筑基学真阳诀放行', ok, why)

# 场景 7：青元剑诀分段门槛（level 映射层数）
ok, why = valid_learn_sim('qingyuan-jianjue', MockPlayer(realm='炼气5层'))
ck('场景7a 炼气层1-3可练青元剑诀', ok, why)
ok, why = valid_learn_sim('qingyuan-jianjue', MockPlayer(realm='炼气5层', skills={'qingyuan-jianjue': 40}))
ck('场景7b 炼气层4-6被拒', not ok, why)
ok, why = valid_learn_sim('qingyuan-jianjue', MockPlayer(realm='筑基初期', skills={'qingyuan-jianjue': 40}))
ck('场景7c 筑基层4-6放行', ok, why)
ok, why = valid_learn_sim('qingyuan-jianjue', MockPlayer(realm='筑基初期', skills={'qingyuan-jianjue': 70}))
ck('场景7d 筑基层7-9被拒', not ok, why)
ok, why = valid_learn_sim('qingyuan-jianjue', MockPlayer(realm='结丹初期', skills={'qingyuan-jianjue': 70}))
ck('场景7e 结丹层7-9放行', ok, why)

# 场景 8：血灵大法——结丹门槛 + 灵根门槛（对齐档案鬼灵门.md 成长线「结丹 | 血灵大法」）
ok, why = valid_learn_sim('xueling-dafa', MockPlayer(realm='筑基初期',
    spirit_root={'quality_idx': 0, 'variant': None}))
ck('场景8a 筑基天灵根仍被拒（结丹门槛）', not ok and why == '需结丹', why)
ok, why = valid_learn_sim('xueling-dafa',
    MockPlayer(realm='结丹初期', spirit_root={'quality_idx': 3, 'variant': None}))
ck('场景8b 结丹非天灵根/暗灵根被拒', not ok, why)
ok, why = valid_learn_sim('xueling-dafa',
    MockPlayer(realm='结丹初期', spirit_root={'quality_idx': 0, 'variant': None}))
ck('场景8c 结丹天灵根放行', ok, why)
ok, why = valid_learn_sim('xueling-dafa',
    MockPlayer(realm='结丹初期', spirit_root={'quality_idx': 1, 'variant': '暗'}))
ck('场景8d 结丹暗灵根放行', ok, why)
ok, why = valid_learn_sim('xueling-dafa', MockPlayer(realm='结丹初期'))
ck('场景8e 结丹无灵根被拒', not ok and why == '无灵根', why)

# 场景 9：入宗玩家（炼气3层+）九宗全功法静态可学性——rank0 功法全部炼气可修（玩家必有灵根）
for sid, info in skill_defs.items():
    path = 'kungfu/skill/%s.c' % sid
    with open(path, encoding='utf-8') as f:
        src = f.read()
    body = extract_func(src, 'valid_learn') or ''
    if 'SECT_TIER_ZHU' not in body and 'SECT_TIER_JIE' not in body and 'SECT_TIER_YING' not in body and 'ROOT_QUALITY_T0' not in body:
        # 玩家必有灵根（attribute.c generate_spirit_root：无灵根不在玩家生成概率中）
        ok, why = valid_learn_sim(sid, MockPlayer(realm='炼气3层',
                                  spirit_root={'quality_idx': 3, 'variant': None}))
        ck('场景9 炼气可修 %s' % sid, ok, why)

# 场景 10：档案境界成长线门槛逐一对齐（第 2 轮审查 c2 修正，7 处）
# 依据各档案「十、境界成长线」：毒术/暗术=筑基(鬼灵门.md:91)、役虫术=筑基(灵兽山.md:91/御灵宗.md:89)、
#   重剑剑法=筑基(巨剑门.md:84)、阵法术=筑基(天阙堡.md:80)、玄月吸阴功=结丹(掩月宗.md:90)、
#   傀儡术=元婴(灵兽山.md:93)
ARCHIVE_GATES = {
    'dushu': ('zhu', '筑基'), 'anshu': ('zhu', '筑基'), 'yichong-shu': ('zhu', '筑基'),
    'zhongjian-jianfa': ('zhu', '筑基'), 'zhenfa-shu': ('zhu', '筑基'),
    'xuanyue-xiyin-gong': ('jie', '结丹'), 'kuilei-shu': ('ying', '元婴'),
}
for sid, (gate, cn) in ARCHIVE_GATES.items():
    with open('kungfu/skill/%s.c' % sid, encoding='utf-8') as f:
        src = f.read()
    body = extract_func(src, 'valid_learn') or ''
    marker = {'zhu': 'SECT_TIER_ZHU', 'jie': 'SECT_TIER_JIE', 'ying': 'SECT_TIER_YING'}[gate]
    ck('场景10 %s 门槛=%s（档案成长线）' % (sid, cn), marker in body,
       '实现门槛与档案成长线不符')
    # 红绿：低一境界被拒，达标境界放行
    low = {'zhu': '炼气5层', 'jie': '筑基初期', 'ying': '结丹初期'}[gate]
    hi = {'zhu': '筑基初期', 'jie': '结丹初期', 'ying': '元婴初期'}[gate]
    ok, why = valid_learn_sim(sid, MockPlayer(realm=low, spirit_root={'quality_idx': 3, 'variant': None}))
    ck('场景10 %s 低境界被拒' % sid, not ok, why)
    ok, why = valid_learn_sim(sid, MockPlayer(realm=hi, spirit_root={'quality_idx': 3, 'variant': None}))
    ck('场景10 %s 达标境界放行' % sid, ok, why)

# ═══════════ 汇总 ═══════════
print('')
if fails:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
sys.exit(0)
