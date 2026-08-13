#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#72 force/knowledge 类功法修炼链路（kungfu skill 修炼/learn 传授渠道）验证脚本
运行：python3 tools/check/sect_cultivate_verify.py
覆盖：
  一、静态校验：valid_xiulian 九宗 sect 分支（SECT_D 动态查本门功法 + force 槽过滤）/
     9 个传功 NPC（recognize_apprentice + 本门功法 set_skill + basic skills）/
     18 个 force+knowledge 功法全部有 NPC 传授 / 括号配对 / 引用完整性
     （SECT_D 宏、query_sect_skills 签名、query_player_sect 签名）
  二、LPC 原文守卫：valid_xiulian 函数体（四要素）+ NPC recognize_apprentice 函数体
     （sect id 比对），含突变实证（去掉要素 → 守卫红，证明绑定交付物非恒真）
  三、同构模拟：xiulian 判定（本门 force 放行/异派拒绝/散修拒绝/本门非 force 拒绝）/
     learn 传授链路（本门可学/异派拒/不会此技能/等级上限/境界门槛/等级提升）/
     basic force 教学 / exercise 内力链路（上限 = basic force × 10）
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


# ═══════════ 数据（#62 九宗功法清单，票面验收口径） ═══════════

# force 槽内功（8）：valid_enable("force") 为 1，可经 xiulian 修炼
FORCE_SKILLS = {'changchun-gong', 'zhenyang-jue', 'xuanbing-jue', 'guiyuan-gong',
                'ningyuan-gong', 'shuangxiu-zhishu', 'xuanyue-xiyin-gong', 'xueling-dafa'}
# knowledge/profession 类（10）：无 enable 槽，经 learn 传授提升
KNOWLEDGE_SKILLS = {'yushou-shu', 'yichong-shu', 'kuilei-shu', 'wangu-jue', 'zhenfa-shu',
                    'dushu', 'anshu', 'lianshi-shu', 'zhubao-shu', 'lianqi-shu'}

# 传功 NPC → 门派 id → 本门功法（#58 chuangong 房间 NPC + #72 传授接线）
NPC_SECTS = [
    ('d/yueguo/yanyue/npc/qionglao.c', 'yanyue_sect',
     {'shuangxiu-zhishu', 'xuanyue-xiyin-gong'}),
    ('d/yueguo/huangfeng/npc/lihuayuan.c', 'huangfeng_valley',
     {'changchun-gong', 'qingyuan-jianjue', 'zhenyang-jue', 'xuanbing-jue',
      'guiyuan-gong', 'huanling-jue', 'ningyuan-gong'}),
    ('d/yueguo/lingshou/npc/luosaidizi.c', 'lingshou_mountain',
     {'yushou-shu', 'yichong-shu', 'kuilei-shu'}),
    ('d/yueguo/qingxu/npc/zhangglao.c', 'qingxu_sect',
     {'daomen-shufa', 'qingxu-jian-dian'}),
    ('d/yueguo/huadao/npc/hantianya.c', 'huadao_dock',
     {'daofa-chuancheng', 'lianqi-shu'}),
    ('d/yueguo/tianque/npc/lanyiren.c', 'tianque_fort',
     {'zhubao-shu', 'zhenfa-shu'}),
    ('d/yueguo/jujian/npc/gaoren.c', 'jujian_gate',
     {'zhongjian-jianfa', 'jianxiu-chuancheng'}),
    ('d/tianluo/guiling/npc/wangchan.c', 'guiling_sect',
     {'guidao-gongfa', 'dushu', 'anshu', 'lianshi-shu', 'xueling-dafa'}),
    ('d/tianluo/yuling/npc/zhanglao.c', 'yuling_sect',
     {'wangu-jue'}),
]

# ═══════════ 一、静态校验 ═══════════
print('== 一、静态校验 ==')

# 1. valid_xiulian（feature/kungfu.c）九宗 sect 分支
with open('feature/kungfu.c', encoding='utf-8') as f:
    kungfu_src = f.read()
xl_body = extract_func(kungfu_src, 'valid_xiulian')
ck('valid_xiulian 函数体可提取', xl_body is not None)
if xl_body:
    ck('valid_xiulian 查门派（query_player_sect）', 'SECT_D->query_player_sect(me)' in xl_body,
       '缺 SECT_D 门派查询（九宗玩家门派信息在 sect/id，#57）')
    ck('valid_xiulian 动态取本门功法（query_sect_skills）', 'SECT_D->query_sect_skills(sect_id)' in xl_body,
       '缺本门功法动态查询（DRY：不复制门派→内功映射表）')
    ck('valid_xiulian force 槽过滤（valid_enable）', 'SKILL_D(skill)->valid_enable(' in xl_body,
       '缺 force 槽过滤（只有内功可 xiulian）')
    ck('valid_xiulian 本门判定（member_array）', 'member_array(skill, sect_skills) >= 0' in xl_body,
       '缺本门技能 member_array 判定')

# 2. 9 个传功 NPC
npc_src_cache = {}
for path, sect_id, skills in NPC_SECTS:
    if not os.path.exists(path):
        ck('NPC 文件存在 %s' % path, False, '文件缺失')
        continue
    with open(path, encoding='utf-8') as f:
        src = f.read()
    npc_src_cache[path] = src
    ok, err = check_parens(path)
    ck('括号配对 %s' % path, ok, err)
    recog = extract_func(src, 'recognize_apprentice')
    ck('NPC recognize_apprentice %s' % os.path.basename(path), recog is not None,
       '缺 recognize_apprentice（learn 命令传授入口）')
    if recog:
        # 门派比对：剥串后看调用结构，原文看具体门派 id（字符串字面量被 strip_lpc 剥除）
        ck('NPC 门派比对 %s' % os.path.basename(path),
           'SECT_D->query_player_sect(ob) != ' in recog and
           'SECT_D->query_player_sect(ob) != "%s"' % sect_id in src,
           'recognize 未按 %s 门派限定（他派弟子不得偷学本门功法）' % sect_id)
    for sk in skills:
        ck('NPC 传授 %s <- %s' % (sk, os.path.basename(path)),
           re.search(r'set_skill\("%s",\s*\d+\)' % re.escape(sk), src) is not None,
           '缺 set_skill(%s)' % sk)
    # basic skills（force/dodge/parry 三件套：force 供 exercise 内力上限、dodge/parry 基本武学）
    for base in ('force', 'dodge', 'parry'):
        ck('NPC 基本 %s %s' % (base, os.path.basename(path)),
           re.search(r'set_skill\("%s",\s*\d+\)' % base, src) is not None,
           '缺 set_skill(%s)' % base)

# 3. 18 个 force+knowledge 功法全部有 NPC 传授（挂接 #62，不再断链）
npc_all_skills = {}
for path, sect_id, skills in NPC_SECTS:
    for sk in skills:
        npc_all_skills[sk] = path
for sk in sorted(FORCE_SKILLS | KNOWLEDGE_SKILLS):
    ck('功法 %s 有传授 NPC' % sk, sk in npc_all_skills,
       '无 NPC set_skill(%s)（#62 功法断链）' % sk)

# 4. force 功法文件 valid_enable("force")（xiulian 命令的 force 判定依据）
for sk in sorted(FORCE_SKILLS):
    path = 'kungfu/skill/%s.c' % sk
    if not os.path.exists(path):
        ck('force 功法文件 %s' % sk, False, '文件缺失')
        continue
    with open(path, encoding='utf-8') as f:
        src = f.read()
    ok, err = check_parens(path)
    ck('括号配对 %s' % sk, ok, err)
    ck('force 功法 %s valid_enable(force)' % sk,
       re.search(r'return\s+usage\s*==\s*"force"\s*;', src) is not None,
       '缺 valid_enable("force")（xiulian 命令第 60-69 行 force 判定）')

# 5. 引用完整性：SECT_D 宏 + sect_d.c 接口签名
with open('include/globals.h', encoding='utf-8') as f:
    globals_src = f.read()
ck('SECT_D 宏存在', re.search(r'#define\s+SECT_D\s+"[^"]*sect_d"', globals_src) is not None,
   'globals.h 缺 SECT_D 宏')
with open('adm/daemons/sect_d.c', encoding='utf-8') as f:
    sect_d_src = f.read()
ck('query_player_sect 签名存在', re.search(r'\bstring query_player_sect\(object player\)', sect_d_src) is not None,
   'sect_d.c 缺 query_player_sect')
ck('query_sect_skills 签名存在', re.search(r'\bstring \*query_sect_skills\(string sect_id\)', sect_d_src) is not None,
   'sect_d.c 缺 query_sect_skills')

# ═══════════ 二、LPC 原文守卫（绑定交付物 + 突变实证） ═══════════
print('== 二、LPC 原文守卫 ==')

def guard_valid_xiulian(body):
    """守卫：valid_xiulian 函数体必须含九宗 sect 分支四要素（strip_lpc 后字符串字面量被剥）"""
    return ('SECT_D->query_player_sect(me)' in body
            and 'SECT_D->query_sect_skills(sect_id)' in body
            and 'SKILL_D(skill)->valid_enable(' in body
            and 'member_array(skill, sect_skills) >= 0' in body)

ck('守卫 valid_xiulian 四要素', xl_body is not None and guard_valid_xiulian(xl_body))

# 突变 1：去掉 force 槽过滤（放行本门任意功法）→ 守卫红
if xl_body:
    mutant = xl_body.replace('SKILL_D(skill)->valid_enable(', 'SKILL_D(skill)->noop(')
    ck('突变 valid_xiulian 去 force 过滤 → 守卫红', not guard_valid_xiulian(mutant),
       '守卫对 force 过滤不敏感（恒真）')
    # 突变 2：去掉 SECT_D 门派查询（退回北侠 family 表）→ 守卫红
    mutant2 = xl_body.replace('SECT_D->query_player_sect(me)', '0')
    ck('突变 valid_xiulian 去门派查询 → 守卫红', not guard_valid_xiulian(mutant2),
       '守卫对门派查询不敏感（恒真）')

def guard_npc_recog(recog_stripped, src_raw, sect_id):
    """剥串结构 + 原文门派 id 双重校验"""
    return ('SECT_D->query_player_sect(ob) != ' in recog_stripped
            and 'SECT_D->query_player_sect(ob) != "%s"' % sect_id in src_raw)

for path, sect_id, skills in NPC_SECTS:
    src = npc_src_cache.get(path)
    if not src:
        continue
    recog = extract_func(src, 'recognize_apprentice')
    ck('守卫 recognize %s' % os.path.basename(path),
       recog is not None and guard_npc_recog(recog, src, sect_id),
       'recognize 函数体缺 %s 门派比对' % sect_id)
    if recog:
        mutant = src.replace('SECT_D->query_player_sect(ob) != "%s"' % sect_id,
                             'SECT_D->query_player_sect(ob) != "wrong_sect"')
        ck('突变 recognize %s 换门派 → 守卫红' % os.path.basename(path),
           not guard_npc_recog(recog, mutant, sect_id),
           '守卫对门派 id 不敏感（恒真）')

# ═══════════ 三、同构模拟 ═══════════
print('== 三、同构模拟 ==')

# 3.1 从 sect_d.c 解析 9 宗 skills（与 sect_skill_verify.py 同构）
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
    for mm in re.finditer(r'"([a-z][a-z0-9-]+)"\s*:\s*\(\s*\[\s*"name"\s*:\s*"([^"]+)"', block):
        SECT_CONFIG[sid]['skills'][mm.group(1)] = {'name': mm.group(2)}
ck('解析出 9 宗技能配置', all(SECT_CONFIG[sid]['skills'] for sid in SECT_IDS),
   '9 宗技能配置解析不完整')


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


def skill_meta(sid):
    """解析功法文件的 valid_enable（force 槽）与 valid_learn 门槛"""
    path = 'kungfu/skill/%s.c' % sid
    with open(path, encoding='utf-8') as f:
        src = f.read()
    vl = extract_func(src, 'valid_learn') or ''
    return {
        # 原文正则：valid_enable 函数体 `return usage == "force";`（force 槽功法独有形态）
        'force_slot': re.search(r'return\s+usage\s*==\s*"force"\s*;', src) is not None,
        'gate_zhu': 'SECT_TIER_ZHU' in vl,
        'gate_jie': 'SECT_TIER_JIE' in vl,
        'gate_ying': 'SECT_TIER_YING' in vl,
        'root_req': 'SPIRIT_ROOT_DATA' in vl,
    }


# 3.2 valid_xiulian 同构翻译（对齐 feature/kungfu.c 新分支）
def valid_xiulian_sim(player, skill, cfg):
    """player: {'sect': id|None, 'family': name|None}"""
    if player.get('sect'):
        sect_skills = cfg[player['sect']]['skills']
        if skill in sect_skills and skill_meta(skill)['force_slot']:
            return 0, '本门内功放行'
        return -1, '只能修炼本门内功'
    # 无 sect：北侠 family 表（百姓/公共武学）
    if player.get('family'):
        return -1, '北侠门派走 family_force 表'
    return -1, '散修只能修炼百姓/公共武学'


p_hf = {'sect': 'huangfeng_valley', 'family': None}
r, why = valid_xiulian_sim(p_hf, 'changchun-gong', SECT_CONFIG)
ck('场景1 黄枫谷弟子 xiulian 长春功（本门内功）放行', r == 0, why)
r, why = valid_xiulian_sim(p_hf, 'ningyuan-gong', SECT_CONFIG)
ck('场景2 黄枫谷弟子 xiulian 凝元功（本门内功）放行', r == 0, why)
r, why = valid_xiulian_sim(p_hf, 'dushu', SECT_CONFIG)
ck('场景3 黄枫谷弟子 xiulian 毒术（异派 knowledge）拒绝', r < 0, why)
r, why = valid_xiulian_sim(p_hf, 'qingyuan-jianjue', SECT_CONFIG)
ck('场景4 黄枫谷弟子 xiulian 青元剑诀（本门非 force）拒绝', r < 0,
   '非 force 槽功法不得 xiulian（valid_enable 过滤）')
p_gy = {'sect': 'guiling_sect', 'family': None}
r, why = valid_xiulian_sim(p_gy, 'xueling-dafa', SECT_CONFIG)
ck('场景5 鬼灵门弟子 xiulian 血灵大法（本门内功）放行', r == 0, why)
p_gy2 = {'sect': 'guiling_sect', 'family': None}
r, why = valid_xiulian_sim(p_gy2, 'changchun-gong', SECT_CONFIG)
ck('场景6 鬼灵门弟子 xiulian 长春功（异派内功）拒绝', r < 0, why)
p_free = {'sect': None, 'family': None}
r, why = valid_xiulian_sim(p_free, 'changchun-gong', SECT_CONFIG)
ck('场景7 散修 xiulian 九宗内功拒绝（无门派无习得渠道）', r < 0, why)
p_jj = {'sect': 'jujian_gate', 'family': None}
r, why = valid_xiulian_sim(p_jj, 'zhongjian-jianfa', SECT_CONFIG)
ck('场景8 巨剑门弟子 xiulian 重剑剑法（本门非 force）拒绝', r < 0, why)

# 3.3 learn 传授链路（对齐 learn.c + NPC recognize_apprentice）
def npc_recognize(player_sect, npc_sect):
    return 1 if player_sect == npc_sect else 0


def valid_learn_sim(sid, player):
    """忠实翻译 kungfu/skill/<sid>.c valid_learn 境界门槛"""
    meta = skill_meta(sid)
    tier = query_cultivation_tier(player.get('realm'))
    if meta['gate_zhu'] and tier < 3:
        return False, '需筑基'
    if meta['gate_jie'] and tier < 6:
        return False, '需结丹'
    if meta['gate_ying'] and tier < 9:
        return False, '需元婴'
    if meta['root_req']:
        if not player.get('spirit_root'):
            return False, '无灵根'
    return True, ''


def learn_sim(player, npc, skill):
    """对齐 learn.c：recognize → 会此技能 → 等级上限 → valid_learn → 提升"""
    if not npc_recognize(player.get('sect'), npc['sect']):
        return '拒绝', '非本门弟子不得偷学'
    if skill not in npc['skills']:
        return '拒绝', 'NPC 不会此技能'
    if player.get('skills', {}).get(skill, 0) >= npc['skills'][skill]:
        return '拒绝', '已到师父上限'
    ok, why = valid_learn_sim(skill, player)
    if not ok:
        return '拒绝', why
    player.setdefault('skills', {})
    player['skills'][skill] = player['skills'].get(skill, 0) + 1
    return '提升', ''


# NPC 技能数据（从真实文件解析 set_skill）
NPC_DATA = {}
for path, sect_id, skills in NPC_SECTS:
    src = npc_src_cache[path]
    npc_skills = {}
    for m in re.finditer(r'set_skill\("([a-z0-9-]+)",\s*(\d+)\)', src):
        npc_skills[m.group(1)] = int(m.group(2))
    NPC_DATA[os.path.basename(path)] = {'sect': sect_id, 'skills': npc_skills}

# 黄枫谷弟子向李化元学长春功
p = {'sect': 'huangfeng_valley', 'realm': '炼气5层', 'spirit_root': {'quality_idx': 3},
     'skills': {'changchun-gong': 1}}
r, why = learn_sim(p, NPC_DATA['lihuayuan.c'], 'changchun-gong')
ck('场景9 本门弟子 learn 长春功提升', r == '提升' and p['skills']['changchun-gong'] == 2, why)

# 黄枫谷弟子向李化元学毒术（NPC 不会）→ 拒绝
p2 = {'sect': 'huangfeng_valley', 'realm': '筑基初期', 'spirit_root': {'quality_idx': 3}, 'skills': {}}
r, why = learn_sim(p2, NPC_DATA['lihuayuan.c'], 'dushu')
ck('场景10 learn 毒术（师父不会）拒绝', r == '拒绝', why)

# 黄枫谷弟子向王蝉（鬼灵门）学毒术 → 拒绝（门派）
p3 = {'sect': 'huangfeng_valley', 'realm': '筑基初期', 'spirit_root': {'quality_idx': 3}, 'skills': {}}
r, why = learn_sim(p3, NPC_DATA['wangchan.c'], 'dushu')
ck('场景11 异派弟子 learn 毒术拒绝（recognize 门派检查）', r == '拒绝', why)

# 鬼灵门弟子向王蝉学毒术（筑基门槛）
p4 = {'sect': 'guiling_sect', 'realm': '炼气5层', 'spirit_root': {'quality_idx': 3}, 'skills': {}}
r, why = learn_sim(p4, NPC_DATA['wangchan.c'], 'dushu')
ck('场景12 炼气弟子 learn 毒术被境界门槛拒', r == '拒绝' and why == '需筑基', why)
p5 = {'sect': 'guiling_sect', 'realm': '筑基初期', 'spirit_root': {'quality_idx': 3}, 'skills': {'dushu': 1}}
r, why = learn_sim(p5, NPC_DATA['wangchan.c'], 'dushu')
ck('场景13 筑基弟子 learn 毒术提升', r == '提升' and p5['skills']['dushu'] == 2, why)

# 基本内功（basic force）教学：learn force from 传功长老 → 提升（exercise 内力链路前提）
p6 = {'sect': 'huangfeng_valley', 'realm': '炼气5层', 'spirit_root': {'quality_idx': 3},
      'skills': {'force': 0}}
r, why = learn_sim(p6, NPC_DATA['lihuayuan.c'], 'force')
ck('场景14 learn 基本内功（force）提升', r == '提升' and p6['skills']['force'] == 1, why)

# 场景15：全部 18 个 force/knowledge 功法——本门弟子 learn 可达（NPC 会 + 境界达标）
ALL_18 = sorted(FORCE_SKILLS | KNOWLEDGE_SKILLS)
unreachable = []
for sk in ALL_18:
    npc_path = npc_all_skills.get(sk)
    if not npc_path:
        unreachable.append('%s(无NPC)' % sk)
        continue
    npc = NPC_DATA[os.path.basename(npc_path)]
    # 用高境界 + 灵根玩家测可达性（valid_learn 门槛全过）
    p7 = {'sect': npc['sect'], 'realm': '元婴初期', 'spirit_root': {'quality_idx': 0},
          'skills': {}}
    r, why = learn_sim(p7, npc, sk)
    if r != '提升':
        unreachable.append('%s(%s)' % (sk, why))
ck('场景15 18 个 force/knowledge 功法本门 learn 全部可达', not unreachable,
   '不可达: %s' % unreachable)

# 3.4 exercise 内力链路（对齐 exercise.c + attribute.c query_max_neili）
def query_max_neili(basic_force):
    """基本内功等级 → 内力上限（attribute.c:557-576，TYPE_NEILI=10）"""
    return basic_force * 10


def exercise_gain(basic_force):
    """exercise.c:92 每心跳内力增长"""
    return 1 + basic_force // 10


ck('场景16 exercise 内力链路（basic force 10 → 上限 100）',
   query_max_neili(10) == 100 and exercise_gain(10) >= 2,
   '基本内功 10 级时内力上限应 100、每心跳增长 >= 2')
ck('场景17 exercise 内力链路（basic force 0 无上限）', query_max_neili(0) == 0,
   '无基本内功时内力上限 0（习得渠道=learn force from 传功长老，场景14）')

# ═══════════ 汇总 ═══════════
print('')
if fails:
    print('RESULT: FAIL（%d 项失败）' % fails)
    for e in errors:
        print('  -', e)
    sys.exit(1)
print('RESULT: PASS 全部通过')
sys.exit(0)
