#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
#60 门派设施系统 静态验收（LPC 状态机/引用完整性/接口签名/设施配置↔房间一致性）
与 tools/facility/path_verify.py（端到端路径）互补：本脚本只做静态断言，不改任何文件。
用法：python3 tools/facility/static_check.py  （exit 0 = 全部断言通过）
"""
#!/usr/bin/env python3
# #60 门派设施系统 静态验收（LPC 状态机/引用完整性/接口签名/设施配置一致性）
import os, re, sys

import os as _os
BASE = _os.path.normpath(_os.path.join(_os.path.dirname(_os.path.abspath(__file__)), "..", ".."))
errors = []
checks = 0

def fail(msg):
    errors.append(msg)

def lpc_tokens(src):
    """LPC 词法扫描：跳过 行注释//、块注释/* */、字符串 "..."、@LONG heredoc；
    返回去除这些后的代码流（保留换行）与字符串字面量列表。"""
    i, n = 0, len(src)
    out = []
    strings = []
    while i < n:
        c = src[i]
        if c == '/' and i+1 < n and src[i+1] == '/':
            while i < n and src[i] != '\n': i += 1
            continue
        if c == '/' and i+1 < n and src[i+1] == '*':
            i += 2
            while i+1 < n and not (src[i] == '*' and src[i+1] == '/'):
                i += 1
            i += 2
            continue
        if c == '"':
            i += 1
            s = []
            while i < n:
                if src[i] == '\\': s.append(src[i]); s.append(src[i+1] if i+1<n else ''); i += 2; continue
                if src[i] == '"': break
                s.append(src[i]); i += 1
            i += 1
            strings.append(''.join(s))
            out.append('""')
            continue
        if c == '@' and i+1 < n and src[i+1] in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
            # heredoc：@LONG ... LONG
            m = re.match(r'@([A-Z]+)', src[i:])
            marker = m.group(1)
            endmarker = '\n' + marker
            j = src.find(endmarker, i)
            if j == -1:
                fail("heredoc %s 无结束标记" % marker)
                i += 1
                continue
            i = j + len(endmarker)
            out.append('""')
            continue
        out.append(c)
        i += 1
    return ''.join(out), strings

def paren_ok(path, src):
    global checks
    checks += 1
    code, strings = lpc_tokens(src)
    # 去掉单引号字符字面量影响：LPC 无 char 字面量，忽略
    stack = []
    pairs = {')': '(', ']': '[', '}': '{'}
    opens = set('([{')
    for ch in code:
        if ch in opens:
            stack.append(ch)
        elif ch in pairs:
            if not stack or stack[-1] != pairs[ch]:
                fail("%s 括号不配对：遇到 %s 栈顶 %s" % (path, ch, stack[-1] if stack else '空'))
                return False
            stack.pop()
    if stack:
        fail("%s 括号未闭合：%s" % (path, ''.join(stack)))
        return False
    return True

def string_truncation_ok(path, src):
    """检查字符串内是否有未转义的半角引号截断（#67 教训）"""
    global checks
    checks += 1
    code, strings = lpc_tokens(src)
    # 字符串已被 lpc_tokens 正常配对；这里再查：src 中 " 的数量必须为偶数
    q = src.count('"')
    if q % 2 != 0:
        fail("%s 半角引号数量为奇数（%d），存在字符串截断" % (path, q))
        return False
    return True

def extract_exits(src):
    m = re.search(r'set\("exits",\s*\(\[(.*?)\]\)', src, re.S)
    if not m: return {}
    body = m.group(1)
    exits = {}
    for dm in re.finditer(r'"([a-z]+)"\s*:\s*"([^"]+)"', body):
        exits[dm.group(1)] = dm.group(2)
    return exits

def extract_objects(src):
    m = re.search(r'set\("objects",\s*\(\[(.*?)\]\)', src, re.S)
    if not m: return []
    body = m.group(1)
    objs = []
    for om in re.finditer(r'([A-Za-z_][A-Za-z0-9_./]*)', body):
        objs.append(om.group(1))
    return objs

def extract_sect_facility(src):
    m = re.search(r'set\("sect_facility",\s*"([^"]+)"\)', src)
    return m.group(1) if m else None

# ---------- 1. 收集本票全部 LPC 文件 ----------
new_files = []
for root, dirs, files in os.walk(BASE):
    if '.git' in root or '.huddle' in root or 'document/knowledge' in root or '.knowledge' in root:
        continue
    for f in files:
        if f.endswith('.c'):
            p = os.path.join(root, f)
            rel = os.path.relpath(p, BASE)
            if '/fac/' in rel or rel in ('adm/daemons/sect_facility_d.c','cmds/usr/facility.c','d/yueguo/obj/spirit_herb.c'):
                new_files.append(rel)
            elif rel in ('d/yueguo/yanyue/dadian.c','d/yueguo/huangfeng/dadian.c','d/yueguo/huangfeng/chuangong.c',
                         'd/yueguo/huangfeng/shanmen.c','d/yueguo/huangfeng/yuexudian.c',
                         'd/yueguo/lingshou/dadian.c','d/yueguo/qingxu/dadian.c','d/yueguo/huadao/dadian.c',
                         'd/yueguo/tianque/dadian.c','d/yueguo/jujian/dadian.c',
                         'd/tianluo/guiling/dadian.c','d/tianluo/yuling/dadian.c'):
                new_files.append(rel)
new_files = sorted(set(new_files))
print("校验文件数：%d" % len(new_files))

# ---------- 2. 括号配对 + 字符串截断 ----------
for rel in new_files:
    full = os.path.join(BASE, rel)
    with open(full) as f:
        src = f.read()
    paren_ok(rel, src)
    string_truncation_ok(rel, src)

# ---------- 3. exits 双向互逆（本票新增/修改房间） ----------
exits_map = {}
for rel in new_files:
    full = os.path.join(BASE, rel)
    with open(full) as f:
        src = f.read()
    ex = extract_exits(src)
    if ex:
        exits_map[rel] = ex

REV = {}
for a, b in [('north','south'),('east','west'),('up','down'),('northeast','southwest'),
             ('northwest','southeast'),('southeast','northwest'),('southwest','northeast')]:
    REV[a] = b; REV[b] = a

checks += 1
for rel, ex in exits_map.items():
    for d, target in ex.items():
        target_rel = target[1:] + '.c'  # 去前导 /
        if not os.path.exists(os.path.join(BASE, target_rel)):
            fail("%s 出口 %s→%s 目标文件不存在" % (rel, d, target))
            continue
        with open(os.path.join(BASE, target_rel)) as f:
            tsrc = f.read()
        tex = extract_exits(tsrc)
        back = REV.get(d)
        if back is None:
            fail("%s 出口方向 %s 无反向映射" % (rel, d))
            continue
        expected_back = "/" + rel[:-2]  # 本房间路径（去 .c）
        if tex.get(back) != expected_back:
            fail("%s 出口 %s→%s 反向缺失（%s 的 %s 应指向 %s，实际 %s）" % (rel, d, target, target_rel, back, expected_back, tex.get(back)))
print("exits 双向检查完成")

# ---------- 4. 设施配置 ↔ 房间一致性 ----------
daemon_src = open(os.path.join(BASE, 'adm/daemons/sect_facility_d.c')).read()
# 提取 facility_config 中的 key/room 对（粗解析）
room_of = {}
for m in re.finditer(r'"([a-z_]+)":\s*\(\[\s*"sect":\s*"([a-z_]+)"[^}]*?"room":\s*"([^"]+)"', daemon_src, re.S):
    room_of[m.group(1)] = m.group(3)
print("设施配置条目：%d" % len(room_of))
checks += 1
for key, room in room_of.items():
    full = os.path.join(BASE, room[1:] + '.c')
    if not os.path.exists(full):
        fail("设施 %s 房间 %s 文件不存在" % (key, room))
    else:
        with open(full) as f:
            rsrc = f.read()
        sf = extract_sect_facility(rsrc)
        if sf != key:
            fail("设施房间 %s 的 sect_facility=%s 与配置 key=%s 不一致" % (room, sf, key))

# 9 宗各至少一个设施
sect_fac = {}
for m in re.finditer(r'"([a-z_]+)":\s*\(\[\s*"sect":\s*"([a-z_]+)"', daemon_src):
    sect_fac.setdefault(m.group(2), []).append(m.group(1))
checks += 1
nine = ['yanyue_sect','huangfeng_valley','lingshou_mountain','qingxu_sect','huadao_dock',
        'tianque_fort','jujian_gate','guiling_sect','yuling_sect']
for s in nine:
    if s not in sect_fac:
        fail("门派 %s 无设施配置" % s)
    else:
        print("  %s: %d 设施" % (s, len(sect_fac[s])))

# 通用五类都有落地
types = {}
for m in re.finditer(r'"([a-z_]+)":\s*\(\[\s*"sect":\s*"([a-z_]+)"[^}]*?"type":\s*(SECT_FACILITY_[A-Z]+)', daemon_src, re.S):
    types.setdefault(m.group(3), []).append(m.group(1))
checks += 1
for t in ['SECT_FACILITY_PLANT','SECT_FACILITY_ALCHEMY','SECT_FACILITY_LIBRARY','SECT_FACILITY_DEFENSE','SECT_FACILITY_TRAINING']:
    if t not in types:
        fail("通用设施类型 %s 未落地" % t)
    else:
        print("  %s: %s" % (t, ','.join(types[t])))

# ---------- 5. 接口签名存在性 ----------
sect_d = open(os.path.join(BASE, 'adm/daemons/sect_d.c')).read()
moneyd = open(os.path.join(BASE, 'adm/daemons/moneyd.c')).read()
facility_c = open(os.path.join(BASE, 'cmds/usr/facility.c')).read()
checks += 1
for fn in ['add_contribution','query_contribution','query_rank','query_sect_skill_info',
           'query_sect_config','query_sect_ranks','query_sect_name','query_player_sect',
           'query_cultivation_tier','learn_skill']:
    if re.search(r'^(?:string \*|string|mixed \*|mixed|mapping|int|void)\s*%s\s*\(' % fn, sect_d, re.M):
        pass
    else:
        fail("sect_d.c 缺接口定义：%s" % fn)
for fn in ['player_pay']:
    if not re.search(r'^(int|void|varargs int)\s+%s\s*\(' % fn, moneyd, re.M):
        fail("moneyd.c 缺接口定义：%s" % fn)
# facility.c 调用的 SECT_FACILITY_D 接口必须都在 daemon 中定义
d_callers = set(re.findall(r'SECT_FACILITY_D->([a-z_]+)\(', facility_c))
d_defs = set(m.group(1) for m in re.finditer(r'^(?:string \*|string|mixed \*|mixed|mapping|int|void)\s*([a-z_]+)\s*\(', daemon_src, re.M))
missing = d_callers - d_defs
if missing:
    fail("facility.c 调用但 sect_facility_d.c 未定义：%s" % sorted(missing))
print("接口签名检查完成：facility.c 调用 %d 个接口，全在" % len(d_callers))

# ---------- 6. 宏/路径引用 ----------
globals_src = open(os.path.join(BASE, 'include/globals.h')).read()
checks += 1
if 'SECT_FACILITY_D' not in globals_src:
    fail("globals.h 缺 SECT_FACILITY_D 宏")
# daemon 依赖 SECT_D / MONEY_D 宏在 globals.h
for macro in ['SECT_D', 'MONEY_D']:
    if not re.search(r'#define\s+%s\b' % macro, globals_src):
        fail("globals.h 缺 %s 宏" % macro)
# include 完整性
for inc in ['sect_facility.h', 'sect.h', 'globals.h', 'mansion.h']:
    if not os.path.exists(os.path.join(BASE, 'include', inc)):
        fail("include/%s 不存在" % inc)
    if inc == 'mansion.h':
        if not re.search(r'PLOT_GROWING|PLOT_MATURE|PLOT_FALLOW|PLOT_EMPTY|GROWTH_BASE_SHORT', open(os.path.join(BASE,'include',inc)).read()):
            fail("mansion.h 缺 PLOT_* / GROWTH_* 常量")

# ---------- 7. 种子配置与命令引用一致 ----------
seed_ids = set(re.findall(r'"([a-z]+)":\s*\(\[\s*"name":', daemon_src))
checks += 1
if not {'lingcao','huanglongcao','zidanshen'} <= seed_ids:
    fail("种子配置缺项：%s" % seed_ids)

# ---------- 汇总 ----------
print("\n===== 静态验收汇总 =====")
print("断言/检查项：%d" % checks)
if errors:
    print("FAIL（%d 项）：" % len(errors))
    for e in errors:
        print("  - " + e)
    sys.exit(1)
print("OK: #60 门派设施系统静态验收全部通过（%d 文件括号/引号、exits 双向、设施配置↔房间、接口签名、宏/常量引用）" % len(new_files))
sys.exit(0)
