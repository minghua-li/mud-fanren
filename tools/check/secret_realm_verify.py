#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#106 血色禁地入口 + 境界门槛静态守卫（可复跑验证）
运行：python3 tools/check/secret_realm_verify.py
覆盖：
  一、入口房间：d/xuejin/rukou.c 真实存在且可加载（inherit ROOM、create 里
      set("short"/"long"/"exits") 且 setup()）
  二、注册一致性：secret_realm_d.c 里 blood_forbidden 注册的 entry 路径与
      真实入口房间路径一致，且该路径指向真实存在的 .c 文件（零悬空）
  三、境界门槛（#61 境界体系）：blood_forbidden 注册带 min_realm_index=2
      （筑基）+ min_realm_sub=1（中期起），不再带旧 min_level/max_level
      数值字段；check_entry_condition 存在 undefinedp 存在性判断 +
      ROOT_REFINE_D->query_player_realm_index / query_player_sub_stage
      新分支（缺键在 LPC 中取 0，intp(0) 为真，必须用 undefinedp 判存在，
      否则全部秘境误走 0=凡人档、门槛被放开）
  四、突变实证：把注册入口路径改回死路径 → 守卫必须红
  五、突变实证：把境界门槛字段删回旧写法（min_level 数值）→ 守卫必须红
退出码：0 全绿，1 有失败
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

oks = 0
fails = 0

# 契约常量（本票 #106 交付物，守卫绑定的是「契约本身」，与 shop_verify 同法）
DAEMON = 'adm/daemons/secret_realm_d.c'
HEADER = 'include/secret_realm.h'
ROOM_REL = 'd/xuejin/rukou.c'
ROOM_PATH = '/d/xuejin/rukou'
REALM_ID = 'blood_forbidden'


def ok(msg):
    global oks
    oks += 1
    print('  ok   %s' % msg)


def err(msg):
    global fails
    fails += 1
    print('  FAIL %s' % msg)


def readf(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


def strip_lpc_comments(src):
    """去 // 行注释与 /* */ 块注释，但保留字符串字面量内容。
    字符串存在断言必须查本函数产物而非原文：目标行被注释掉（非删除）时
    原文恒绿（#64 复审实测洞，见 .knowledge/tech/tools-check-guard-convention）。"""
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if c == '"':
            j = i + 1
            while j < n:
                if src[j] == '\\':
                    j += 2
                    continue
                if src[j] == '"':
                    break
                j += 1
            end = j + 1 if j < n else n
            out.append(src[i:end])
            i = end
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '/':
            j = src.find('\n', i)
            i = n if j == -1 else j
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '*':
            j = src.find('*/', i + 2)
            i = n if j == -1 else j + 2
            continue
        out.append(c)
        i += 1
    return ''.join(out)


def extract_func(src, name):
    """粗略截取函数体：从首次 `name(` 起、花括号配平到第一个闭括号"""
    i = src.find('%s(' % name)
    if i == -1:
        return ''
    j = src.find('{', i)
    if j == -1:
        return ''
    depth = 0
    for k in range(j, len(src)):
        if src[k] == '{':
            depth += 1
        elif src[k] == '}':
            depth -= 1
            if depth == 0:
                return src[i:k + 1]
    return src[i:]


def realm_block(src, realm_id):
    """截取某秘境的 register_realm((([ ... ])) 注册块（自 ID 行回溯到
    register_realm( 起，到块内首个 ])); 止）"""
    i = src.find('"%s"' % realm_id)
    if i == -1:
        return None
    start = src.rfind('register_realm((', 0, i)
    end = src.find(']));', i)
    if start == -1 or end == -1:
        return None
    return src[start:end + 4]


def entry_path(block):
    """从注册块提取 SR_FIELD_ENTRY 的路径字面量"""
    m = re.search(r'SR_FIELD_ENTRY\s*:\s*"(/[^"]*)"', block)
    return m.group(1) if m else None


def room_guard(room_src):
    """入口房间可加载性契约：inherit ROOM + create 里 short/long/exits + setup"""
    bad = []
    if 'inherit ROOM;' not in room_src:
        bad.append('入口房间未 inherit ROOM')
    create = extract_func(room_src, 'create')
    if not create:
        bad.append('入口房间缺少 create()')
        return bad
    for key in ('"short"', '"long"', '"exits"'):
        if 'set(%s' % key not in create:
            bad.append('入口房间 create() 缺 set(%s)' % key)
    if 'setup();' not in create:
        bad.append('入口房间 create() 未调 setup()')
    return bad


def entry_guard(daemon_src):
    """注册一致性契约：blood_forbidden 的 entry 路径与真实房间一致且文件存在。
    对任意 secret_realm_d.c 原文可重跑。"""
    bad = []
    block = realm_block(daemon_src, REALM_ID)
    if block is None:
        bad.append('%s 注册块未找到' % REALM_ID)
        return bad
    path = entry_path(block)
    if path is None:
        bad.append('%s 注册块缺 SR_FIELD_ENTRY 路径' % REALM_ID)
        return bad
    if path != ROOM_PATH:
        bad.append('%s 注册入口路径为 %s，与真实入口房间 %s 不一致'
                  % (REALM_ID, path, ROOM_PATH))
    if not os.path.isfile(ROOM_REL):
        bad.append('入口房间文件 %s 不存在' % ROOM_REL)
    elif not os.path.isfile(path.lstrip('/') + '.c'):
        bad.append('注册入口路径 %s 指向不存在的文件（悬空路径）' % path)
    return bad


def gate_guard(daemon_src):
    """境界门槛契约（#61 境界体系，筑基中期起）：
    注册块带 min_realm_index=2 + min_realm_sub=1、无旧 min_level/max_level；
    check_entry_condition 带 undefinedp 存在性判断并委托 check_realm_gate，
    后者统一走 ROOT_REFINE_D 读 realm_index/子阶段。
    对任意 secret_realm_d.c 原文可重跑。"""
    bad = []
    block = realm_block(daemon_src, REALM_ID)
    if block is None:
        bad.append('%s 注册块未找到' % REALM_ID)
        return bad
    if not re.search(r'SR_FIELD_MIN_REALM_INDEX\s*:\s*2\b', block):
        bad.append('%s 注册缺 min_realm_index=2（筑基）' % REALM_ID)
    if not re.search(r'SR_FIELD_MIN_REALM_SUB\s*:\s*1\b', block):
        bad.append('%s 注册缺 min_realm_sub=1（中期起）' % REALM_ID)
    if 'SR_FIELD_MIN_LEVEL' in block:
        bad.append('%s 注册仍带旧 min_level 数值字段（应改走 #61 境界体系）'
                  % REALM_ID)
    if 'SR_FIELD_MAX_LEVEL' in block:
        bad.append('%s 注册仍带旧 max_level 数值字段（应改走 #61 境界体系）'
                  % REALM_ID)
    fn = extract_func(daemon_src, 'check_entry_condition')
    if not fn:
        bad.append('check_entry_condition 函数未找到')
        return bad
    if 'undefinedp(realm[SR_FIELD_MIN_REALM_INDEX])' not in fn:
        bad.append('check_entry_condition 缺 undefinedp(realm[SR_FIELD_MIN_REALM_INDEX])'
                  ' 存在性判断（缺键取 0、intp(0) 为真，intp 判断会放开全部门槛）')
    if 'check_realm_gate(' not in fn:
        bad.append('check_entry_condition 未委托 check_realm_gate 做 #61 境界门槛校验')
    gate = extract_func(daemon_src, 'check_realm_gate')
    if not gate:
        bad.append('check_realm_gate 函数未找到（#61 境界门槛校验缺失）')
    else:
        if 'ROOT_REFINE_D->query_player_realm_index(' not in gate:
            bad.append('check_realm_gate 未按 #61 体系读 realm_index'
                       '（ROOT_REFINE_D->query_player_realm_index 缺失）')
        if 'ROOT_REFINE_D->query_player_sub_stage(' not in gate:
            bad.append('check_realm_gate 未按 #61 体系读子阶段'
                       '（ROOT_REFINE_D->query_player_sub_stage 缺失）')
        if 'idx > min_idx' not in gate or 'idx < min_idx' not in gate:
            bad.append('check_realm_gate 缺大境界比较（idx > min_idx / idx < min_idx），'
                       '高境界放行契约不成立')
    return bad


print('═══ 一、入口房间（d/xuejin/rukou.c） ═══')
if not os.path.isfile(ROOM_REL):
    err('入口房间文件 %s 不存在' % ROOM_REL)
else:
    rbad = room_guard(readf(ROOM_REL))
    if not rbad:
        ok('入口房间文件存在且可加载（inherit ROOM + short/long/exits + setup）')
    else:
        for b in rbad:
            err(b)

print('\n═══ 二、注册一致性（守护进程 entry 路径 ↔ 真实房间） ═══')
daemon_raw = readf(DAEMON)
daemon = strip_lpc_comments(daemon_raw)
bad = entry_guard(daemon)
if not bad:
    ok('blood_forbidden 注册入口路径与真实房间 %s 一致，路径零悬空' % ROOM_PATH)
else:
    for b in bad:
        err(b)

print('\n═══ 三、境界门槛（#61 境界体系：筑基中期起） ═══')
bad2 = gate_guard(daemon)
if not bad2:
    ok('blood_forbidden 注册 min_realm_index=2 + min_realm_sub=1（无旧 level 字段）；'
       'check_entry_condition 带 undefinedp 存在性判断并委托 check_realm_gate'
       '（ROOT_REFINE_D 读 realm_index/子阶段，大境界比较放行）')
else:
    for b in bad2:
        err(b)

print('\n═══ 四、突变实证（入口路径改错 → 应红） ═══')
# 突变：把注册入口路径改回 #31 旧死路径 /d/yue/npc/obj/blood_forbidden_entry
mut1 = daemon.replace('"%s"' % ROOM_PATH, '"/d/yue/npc/obj/blood_forbidden_entry"')
if mut1 == daemon:
    err('突变注入失败：未找到可替换的入口路径 %s' % ROOM_PATH)
else:
    bad3 = entry_guard(mut1)
    if bad3:
        ok('入口路径改回死路径 → 守卫红（%d 条失败消息）' % len(bad3))
    else:
        err('突变失败：入口路径改回死路径后守卫仍绿（守卫恒真）')

print('\n═══ 五、突变实证（境界门槛删回旧数值 → 应红） ═══')
# 突变：在原文上删掉 #61 境界体系两字段、恢复旧 min_level/max_level 数值写法
mut2 = daemon_raw.replace(
    '        SR_FIELD_MIN_REALM_INDEX : 2,   // 筑基（#61 境界体系）\n'
    '        SR_FIELD_MIN_REALM_SUB   : 1,   // 中期起\n',
    '        SR_FIELD_MIN_LEVEL   : 5,    // 炼气期\n'
    '        SR_FIELD_MAX_LEVEL   : 15,   // 筑基以下\n')
if mut2 == daemon_raw:
    err('突变注入失败：未找到可替换的 min_realm_index/min_realm_sub 注册行')
else:
    bad4 = gate_guard(strip_lpc_comments(mut2))
    if bad4:
        ok('境界门槛删回旧 level 数值 → 守卫红（%d 条失败消息）' % len(bad4))
    else:
        err('突变失败：境界门槛删回旧 level 数值后守卫仍绿（守卫恒真）')

print('\n═══════════════════════════════════')
print('断言总数: %d, 失败: %d' % (oks, fails))
if fails:
    print('FAILED: 存在失败项')
    sys.exit(1)
print('RESULT: PASS 全部通过')
