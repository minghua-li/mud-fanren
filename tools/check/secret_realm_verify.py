#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#106 血色禁地入口 + 境界门槛静态守卫（可复跑验证）
#108 三层内容落地扩展：房间/怪物/掉落/战斗/主线/状态/命令
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
  六、三层房间结构（#108 c1）：外围/中层/核心/巢窟四房间存在、inherit ROOM、
      create 完整、sr_layer 正确、exits 成环可返回入口、目标路径零悬空、
      各房间 sr_mobs 表指向真实 NPC 文件、注册 LAYERS=3 对齐三层
  七、怪物实体（#108 c2）：1G §3.2.1 怪物表 7 实体（毒蛇/巨蚊/毒蝎群/石傀儡/
      吸血蝠群/护卫妖兽/BOSS墨蛟）文件真实存在、inherit NPC、create 完整、
      sr_realm/sr_layer/sr_score 挂接、墨蛟带 sr_boss；占位生成器
      generate_layer_mobs 已移除
  八、掉落真实非占位（#108 c3）：各 NPC sr_drop 表与注册块奖励物品全部指向
      真实存在的 .c 实体（黄龙草/灵草）；旧悬空路径 condensing_herb /
      blood_forbidden_exit 不再出现
  九、战斗真实（#108 c4）：do_explore 按房间 sr_mobs 生成真实实体并经
      fight_ob 接真实战斗；旧随机数模拟（随机击杀数/随机经验）移除；
      击杀结算挂接 on_mob_defeated 在位、存活标记写入端在位
  十、主线+晋升接线（#108 c6）：mq_1_9 目标=进入 /d/xuejin（OBJ_REACH）+
      试炼存活（OBJ_SURVIVE 读存活标记）；quest_progress 处理 OBJ_SURVIVE；
      sect_d promote 读取存活标记做晋升门槛（写入端/读取端均在位）
  十一、状态转正+进出命令（#108 c7）：注册状态 OPEN 且注册后经既有机制
      set_realm_status 转正；出口路径指向真实房间；cmds/usr/jindi.c 提供
      enter/exit/explore/info 并调用守护进程校验/创建/探索/退出接口
  十二~十六、突变实证（#108）：房间出口改死路径 / sr_mobs 改死路径 /
      删除存活标记写入行 / 状态改回 CLOSED / on_mob_defeated 形参顺序写反
      → 守卫必须红
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


def extract_func_def(src, name):
    """截取函数**定义**体（区别于前向声明）：取第一个「参数列表右括号后、
    分号之前出现 {」的位置，花括号配平截取。前向声明形如 `int f(a, b);`
    （右括号后先遇分号），据此区分。"""
    for m in re.finditer(r'\b%s\s*\(' % re.escape(name), src):
        i = m.start()
        j = src.find(')', i)
        if j == -1:
            continue
        k = src.find('{', j)
        k2 = src.find(';', j)
        if k == -1 or (k2 != -1 and k2 < k):
            continue
        depth = 0
        for t in range(k, len(src)):
            if src[t] == '{':
                depth += 1
            elif src[t] == '}':
                depth -= 1
                if depth == 0:
                    return src[i:t + 1]
    return ''


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

# ═══════════════ #108 三层内容落地扩展 ═══════════════

# 契约常量（#108 交付物：三层房间 + 1G §3.2.1 怪物表 + 真实掉落）
ROOMS = {
    # rel_path: (sr_layer, {exit_dir: target_path}, {sr_mobs 路径: 数量})
    'd/xuejin/waiwei.c': (1,
        {'north': '/d/xuejin/zhongceng', 'south': '/d/xuejin/rukou'},
        {'/d/xuejin/npc/dushe': 2, '/d/xuejin/npc/juwen': 2,
         '/d/xuejin/npc/dushi': 1}),
    'd/xuejin/zhongceng.c': (2,
        {'north': '/d/xuejin/heixin', 'south': '/d/xuejin/waiwei'},
        {'/d/xuejin/npc/shikulei': 2, '/d/xuejin/npc/xixuefu': 2}),
    'd/xuejin/heixin.c': (3,
        {'north': '/d/xuejin/chonguo', 'south': '/d/xuejin/zhongceng'},
        {'/d/xuejin/npc/dushi': 2}),
    'd/xuejin/chonguo.c': (3,
        {'south': '/d/xuejin/heixin'},
        {'/d/xuejin/npc/mojiao': 1, '/d/xuejin/npc/huwei': 2}),
}
# 入口房间（#106 交付）的出口契约：north 通向第 1 层，south 回传送阵
RUKOU_EXITS = {'north': '/d/xuejin/waiwei', 'south': '/d/yueguo/transmit'}
NPC_FILES = {
    # rel_path: (sr_layer, 是否 BOSS)
    'd/xuejin/npc/dushe.c': (1, False),
    'd/xuejin/npc/juwen.c': (1, False),
    'd/xuejin/npc/dushi.c': (1, False),
    'd/xuejin/npc/shikulei.c': (2, False),
    'd/xuejin/npc/xixuefu.c': (2, False),
    'd/xuejin/npc/huwei.c': (3, False),
    'd/xuejin/npc/mojiao.c': (3, True),
}
REAL_DROP_ITEMS = (
    '/d/yueguo/tainan/obj/huanglongcao',
    '/d/yueguo/tainan/obj/lingcao',
)
SURVIVED_FLAG = 'secret_realm/blood_forbidden/survived'
COMMAND_REL = 'cmds/usr/jindi.c'
MAIN_QUEST_REL = 'adm/daemons/main_quest_d.c'
SECT_REL = 'adm/daemons/sect_d.c'
QUEST_HEADER_REL = 'include/quest_chain.h'


def parse_exits(src):
    """解析 create 里 set("exits", ([ "dir" : "/path", ... ])) 的映射"""
    m = re.search(r'set\("exits",\s*\(\[(.*?)\]\)\)', src, re.S)
    if not m:
        return {}
    return {d: p for d, p in re.findall(r'"([a-z]+)"\s*:\s*"(/[^"]+)"', m.group(1))}


def parse_sr_mobs(src):
    """解析 create 里 set("sr_mobs", ([ "/path": N, ... ])) 的映射"""
    m = re.search(r'set\("sr_mobs",\s*\(\[(.*?)\]\)\)', src, re.S)
    if not m:
        return {}
    out = {}
    for p, n in re.findall(r'"(/[^"]+)"\s*:\s*(\d+)', m.group(1)):
        out[p] = int(n)
    return out


def parse_sr_drop(src):
    """解析 set("sr_drop", ([ "/path": percent, ... ])) 的掉落表"""
    m = re.search(r'set\("sr_drop",\s*\(\[(.*?)\]\)\)', src, re.S)
    if not m:
        return {}
    out = {}
    for p, n in re.findall(r'"(/[^"]+)"\s*:\s*(\d+)', m.group(1)):
        out[p] = int(n)
    return out


def room_structure_guard(src, room_rel, expect_layer, expect_exits, expect_mobs):
    """三层房间结构契约（#108 c1/c2/c3）：inherit ROOM + create 完整 +
    sr_layer 正确 + exits 与契约一致且目标文件存在 + sr_mobs 指向真实 NPC
    文件且与契约一致。src 直接传入（突变实证可对改坏后的文本重放，不碰文件）。"""
    bad = []
    if 'inherit ROOM;' not in src:
        bad.append('%s 未 inherit ROOM' % room_rel)
    create = extract_func(src, 'create')
    if not create:
        bad.append('%s 缺 create()' % room_rel)
        return bad
    for key in ('"short"', '"long"', '"exits"'):
        if 'set(%s' % key not in create:
            bad.append('%s create() 缺 set(%s)' % (room_rel, key))
    if 'setup();' not in create:
        bad.append('%s create() 未调 setup()' % room_rel)
    if not re.search(r'set\("sr_layer",\s*%d\b' % expect_layer, create):
        bad.append('%s 缺 set("sr_layer", %d)（层数挂接缺失）' % (room_rel, expect_layer))
    exits = parse_exits(src)
    for d, p in expect_exits.items():
        if exits.get(d) != p:
            bad.append('%s exits.%s 应为 %s（实际 %s）'
                       % (room_rel, d, p, exits.get(d)))
    for d, p in exits.items():
        if not os.path.isfile(p.lstrip('/') + '.c'):
            bad.append('%s exits.%s 指向不存在的房间 %s（悬空）' % (room_rel, d, p))
    mobs = parse_sr_mobs(src)
    for p in expect_mobs:
        if p not in mobs:
            bad.append('%s sr_mobs 缺怪物 %s（1G 怪物表断链）' % (room_rel, p))
    for p in mobs:
        if not os.path.isfile(p.lstrip('/') + '.c'):
            bad.append('%s sr_mobs 指向不存在的 NPC 文件 %s（悬空/占位）' % (room_rel, p))
    return bad


def npc_entity_guard(npc_rel, expect_layer, is_boss):
    """怪物实体契约（#108 c2/c3）：文件存在 + inherit NPC + create 完整 +
    sr_realm/sr_layer/sr_score 挂接 + 掉落表全指向真实物品实体 + 掉落回调
    通知守护进程（击杀结算挂接）。"""
    bad = []
    if not os.path.isfile(npc_rel):
        return ['怪物实体文件 %s 不存在' % npc_rel]
    src = readf(npc_rel)
    if 'inherit NPC;' not in src:
        bad.append('%s 未 inherit NPC' % npc_rel)
    create = extract_func(src, 'create')
    if not create:
        bad.append('%s 缺 create()' % npc_rel)
        return bad
    if 'set_name(' not in create:
        bad.append('%s create() 缺 set_name' % npc_rel)
    if 'setup();' not in create:
        bad.append('%s create() 未调 setup()' % npc_rel)
    if 'set("sr_realm", "blood_forbidden")' not in create:
        bad.append('%s 缺 sr_realm=blood_forbidden 挂接' % npc_rel)
    if not re.search(r'set\("sr_layer",\s*%d\b' % expect_layer, create):
        bad.append('%s 缺 set("sr_layer", %d)' % (npc_rel, expect_layer))
    if not re.search(r'set\("sr_score",\s*\d+\)', create):
        bad.append('%s 缺 sr_score（结算积分挂接）' % npc_rel)
    if is_boss and not re.search(r'set\("sr_boss",\s*1\b', create):
        bad.append('%s 缺 set("sr_boss", 1)（BOSS 存活标记触发条件）' % npc_rel)
    drops = parse_sr_drop(src)
    if not drops:
        bad.append('%s 缺 sr_drop 掉落表' % npc_rel)
    for p in drops:
        if not os.path.isfile(p.lstrip('/') + '.c'):
            bad.append('%s sr_drop 指向不存在的物品 %s（悬空/占位）' % (npc_rel, p))
        elif p not in REAL_DROP_ITEMS:
            bad.append('%s sr_drop 指向非契约真实物品 %s' % (npc_rel, p))
    if 'void winner_reward(object killer, object victim)' not in src:
        bad.append('%s 缺 winner_reward 掉落回调（战斗系统战胜判定挂接）' % npc_rel)
    if 'SECRET_REALM_D->on_mob_defeated(' not in src:
        bad.append('%s 未通知守护进程击杀结算（on_mob_defeated 挂接缺失）' % npc_rel)
    # 调用点实参顺序断言（审查第 1 轮修复）：on_mob_defeated 的实参必须与
    # winner_reward 形参同序（实参 1=killer 玩家侧、实参 2=victim NPC 侧），
    # 与守护进程签名（形参 2 上查 sr_realm）构成完整语义闭环。
    wr_sig = re.search(r'winner_reward\s*\(\s*object\s+(\w+)\s*,\s*object\s+(\w+)\s*\)', src)
    call = re.search(r'SECRET_REALM_D->on_mob_defeated\(\s*(\w+)\s*,\s*(\w+)\s*\)', src)
    if wr_sig and call and (call.group(1) != wr_sig.group(1) or call.group(2) != wr_sig.group(2)):
        bad.append('%s on_mob_defeated 调用实参顺序与 winner_reward 形参顺序不一致'
                   '（killer/victim 颠倒 → 守护进程结算断链）' % npc_rel)
    return bad


def combat_guard(daemon_src):
    """战斗真实契约（#108 c4 + c6 写入端）：do_explore 按房间 sr_mobs 生成
    真实实体并经 fight_ob 接真实战斗，旧随机数模拟移除；击杀结算挂接与
    存活标记写入端在位。"""
    bad = []
    fn = extract_func(daemon_src, 'do_explore')
    if not fn:
        bad.append('do_explore 函数未找到')
    else:
        if 'spawn_layer_mobs(' not in fn:
            bad.append('do_explore 未按房间 sr_mobs 生成真实怪物实体'
                       '（spawn_layer_mobs 缺失）')
        if 'fight_ob(' not in fn:
            bad.append('do_explore 未接真实战斗（F_ATTACK fight_ob 缺失）')
        if 'sr_realm' not in fn:
            bad.append('do_explore 未按 sr_realm 收集本秘境妖兽')
        if 'random(mob_count / 2 + 1)' in fn or 'killed =' in fn:
            bad.append('do_explore 仍含旧随机数模拟战斗（随机击杀数未移除）')
        if 'reward_exp' in fn:
            bad.append('do_explore 仍含旧随机经验模拟（reward_exp 未移除）')
    if 'generate_layer_mobs' in daemon_src:
        bad.append('占位怪物生成器 generate_layer_mobs 仍在位（应已移除）')
    if 'object *spawn_layer_mobs(' not in daemon_src:
        bad.append('spawn_layer_mobs 真实实体生成函数未找到')
    on_mob = extract_func(daemon_src, 'on_mob_defeated')
    if not on_mob:
        bad.append('on_mob_defeated 击杀结算挂接未找到')
    else:
        if 'query_player_instance(' not in on_mob:
            bad.append('on_mob_defeated 未按 killer 查活跃实例')
        if 'sr_boss' not in on_mob:
            bad.append('on_mob_defeated 未处理 BOSS（sr_boss）击杀')
        # 接线语义断言（审查第 1 轮修复）：调用链
        #   reward.h victim->winner_reward(killer, victim)  → NPC 形参
        #   killer=玩家 / victim=NPC → 调用 on_mob_defeated(killer, victim)
        #   → 守护进程形参 1=玩家、形参 2=NPC。
        # 守护进程内 sr_realm 查询必须落在**第二个形参**（NPC 侧）；
        # 若形参顺序写反，玩家对象无 sr_realm，结算/存活写入运行时恒不可达。
        sig = re.search(r'on_mob_defeated\s*\(\s*object\s+(\w+)\s*,\s*object\s+(\w+)\s*\)', on_mob)
        q = re.search(r'(\w+)->query\("sr_realm"\)', on_mob)
        if not sig:
            bad.append('on_mob_defeated 签名非常规 object,object 双形参，无法校验调用顺序')
        elif not q:
            bad.append('on_mob_defeated 未查询 sr_realm（结算挂接缺失）')
        elif q.group(1) != sig.group(2):
            bad.append('on_mob_defeated 在形参 %s（调用点=玩家侧）上查询 sr_realm：'
                       '实参顺序与形参顺序相反，存活写入运行时永不执行（死条件）'
                       % q.group(1))
    mark = extract_func_def(daemon_src, 'mark_trial_survivors')
    if not mark:
        bad.append('mark_trial_survivors 存活标记写入端未找到')
    else:
        if '"/survived"' not in mark or "secret_realm/" not in mark:
            bad.append('mark_trial_survivors 未写入 secret_realm/<id>/survived 标记')
        if 'killer->set(flag, time())' not in mark:
            bad.append('mark_trial_survivors 缺击杀者（BOSS 击杀）存活持久写入')
        if 'inv[i]->set(flag, time())' not in mark:
            bad.append('mark_trial_survivors 缺同室存活玩家存活持久写入')
    return bad


def quest_wiring_guard():
    """主线+晋升接线契约（#108 c6）：mq_1_9=进入+d存活；quest_progress 处理
    OBJ_SURVIVE；sect_d 晋升读取存活标记。"""
    bad = []
    if not os.path.isfile(MAIN_QUEST_REL):
        return ['%s 不存在' % MAIN_QUEST_REL]
    mqd = readf(MAIN_QUEST_REL)
    i = mqd.find('"mq_1_9"')
    j = mqd.find('"mq_1_10"')
    if i == -1 or j == -1 or j <= i:
        bad.append('mq_1_9 节点定义未找到')
        return bad
    blk = mqd[i:j]
    if 'OBJ_REACH' not in blk or '"/d/xuejin"' not in blk:
        bad.append('mq_1_9 缺 OBJ_REACH /d/xuejin（真进副本目标）')
    if 'OBJ_SURVIVE' not in blk or SURVIVED_FLAG not in blk:
        bad.append('mq_1_9 缺 OBJ_SURVIVE %s（试炼存活目标）' % SURVIVED_FLAG)
    if '/d/yueguo/huangfeng/shanmen' in blk:
        bad.append('mq_1_9 仍是 OBJ_REACH 山门旧目标（应改为真进副本）')
    qp = extract_func_def(mqd, 'quest_progress')
    if not qp:
        bad.append('quest_progress 函数定义未找到')
    elif 'OBJ_SURVIVE' not in qp:
        bad.append('quest_progress 未处理 OBJ_SURVIVE 目标类型')
    if not os.path.isfile(QUEST_HEADER_REL):
        bad.append('%s 不存在' % QUEST_HEADER_REL)
    elif '#define OBJ_SURVIVE' not in readf(QUEST_HEADER_REL):
        bad.append('quest_chain.h 缺 OBJ_SURVIVE 常量定义')
    if not os.path.isfile(SECT_REL):
        bad.append('%s 不存在' % SECT_REL)
        return bad
    sd = readf(SECT_REL)
    if 'xuejin_trial_gate' not in sd:
        bad.append('sect_d 缺 xuejin_trial_gate 血禁试炼晋升门槛表')
    promote = extract_func(sd, 'promote')
    if not promote:
        bad.append('sect_d promote 函数未找到')
    elif SURVIVED_FLAG not in promote:
        bad.append('sect_d promote 未读取存活标记（血禁存活不是真实前置）')
    return bad


def status_guard(daemon_src):
    """状态转正+零悬空+层数对齐契约（#108 c5/c7）"""
    bad = []
    block = realm_block(daemon_src, REALM_ID)
    if block is None:
        return ['%s 注册块未找到' % REALM_ID]
    if 'SR_STATUS_OPEN' not in block:
        bad.append('%s 注册状态未转正为 SR_STATUS_OPEN（玩家将被状态拦截）' % REALM_ID)
    if 'set_realm_status("%s", SR_STATUS_OPEN)' % REALM_ID not in daemon_src:
        bad.append('注册后未经既有开放机制 set_realm_status 转正'
                   '（register_realm 会强制置 CLOSED）')
    m = re.search(r'SR_FIELD_EXIT\s*:\s*"(/[^"]*)"', block)
    if not m:
        bad.append('%s 注册缺 SR_FIELD_EXIT' % REALM_ID)
    else:
        exit_path = m.group(1)
        if not os.path.isfile(exit_path.lstrip('/') + '.c'):
            bad.append('%s 注册出口路径 %s 悬空（指向不存在的文件）'
                       % (REALM_ID, exit_path))
    if not re.search(r'SR_FIELD_LAYERS\s*:\s*3\b', block):
        bad.append('%s 注册 LAYERS 未对齐 1G 三层设定（应为 3）' % REALM_ID)
    m = re.search(r'SR_FIELD_REWARDS.*?"item"\s*:\s*"(/[^"]+)"', block, re.S)
    if not m:
        bad.append('%s 注册奖励物品缺失' % REALM_ID)
    else:
        item_path = m.group(1)
        if not os.path.isfile(item_path.lstrip('/') + '.c'):
            bad.append('%s 注册奖励物品 %s 悬空（指向不存在的文件）'
                       % (REALM_ID, item_path))
    # 旧悬空路径不得再出现
    for ghost in ('/d/yue/obj/condensing_herb', '/d/yue/npc/obj/blood_forbidden_exit'):
        if ghost in daemon_src:
            bad.append('旧悬空路径 %s 仍在守护进程中（应已改指真实实体）' % ghost)
    return bad


def command_guard():
    """进出命令契约（#108 c7）：cmds/usr/jindi.c 提供 enter/exit/explore/info
    并调用守护进程真实接口（校验/创建/探索/结算退出）。"""
    bad = []
    if not os.path.isfile(COMMAND_REL):
        return ['玩家命令 %s 不存在（无真实进入/退出命令）' % COMMAND_REL]
    src = readf(COMMAND_REL)
    for token in ('check_entry_condition(', 'create_instance(',
                  'do_explore(', 'do_exit(', 'query_player_instance('):
        if token not in src:
            bad.append('%s 未调用守护进程接口 %s' % (COMMAND_REL, token))
    for sub in ('"enter"', '"exit"', '"explore"', '"info"'):
        if sub not in src:
            bad.append('%s 缺 %s 子命令' % (COMMAND_REL, sub))
    return bad


print('\n═══ 六、三层房间结构（#108 c1） ═══')
room_bad = []
for rel, (layer, exits, mobs) in ROOMS.items():
    if not os.path.isfile(rel):
        room_bad.append('房间文件 %s 不存在' % rel)
        continue
    room_bad += room_structure_guard(readf(rel), rel, layer, exits, mobs)
if os.path.isfile('d/xuejin/rukou.c'):
    rukou_src = readf('d/xuejin/rukou.c')
    r_exits = parse_exits(rukou_src)
    for d, p in RUKOU_EXITS.items():
        if r_exits.get(d) != p:
            room_bad.append('rukou exits.%s 应为 %s（实际 %s）' % (d, p, r_exits.get(d)))
else:
    room_bad.append('入口房间 d/xuejin/rukou.c 不存在')
if not room_bad:
    ok('三层+巢窟四房间真实存在：inherit ROOM、create 完整、sr_layer 正确、'
       'exits 成环可返回入口、目标零悬空、sr_mobs 全指向真实 NPC 实体')
else:
    for b in room_bad:
        err(b)

print('\n═══ 七、怪物实体（#108 c2，1G §3.2.1 怪物表） ═══')
npc_bad = []
for rel, (layer, is_boss) in NPC_FILES.items():
    npc_bad += npc_entity_guard(rel, layer, is_boss)
if not npc_bad:
    ok('怪物表 7 实体（毒蛇/巨蚊/毒蝎群/石傀儡/吸血蝠群/护卫妖兽/BOSS墨蛟）'
       '全部为真实 NPC 文件：inherit NPC、create 完整、sr_realm/sr_layer/'
       'sr_score 挂接、墨蛟带 sr_boss、掉落回调与击杀结算挂接在位')
else:
    for b in npc_bad:
        err(b)

print('\n═══ 八、掉落真实非占位（#108 c3） ═══')
drop_bad = []
for rel in list(NPC_FILES) + [r for r in ROOMS]:
    if os.path.isfile(rel):
        for p in parse_sr_drop(readf(rel)):
            if not os.path.isfile(p.lstrip('/') + '.c'):
                drop_bad.append('%s 掉落物品 %s 悬空' % (rel, p))
sb = status_guard(daemon)
reward_bad = [b for b in sb if '奖励物品' in b]
drop_bad += reward_bad
if not drop_bad:
    ok('副本全部掉落/奖励物品（含注册块奖励）指向真实存在的物品实体'
       '（黄龙草/灵草），无同名占位、零悬空')
else:
    for b in drop_bad:
        err(b)

print('\n═══ 九、战斗真实（#108 c4 + 存活写入端） ═══')
cbad = combat_guard(daemon)
if not cbad:
    ok('do_explore 按房间 sr_mobs 生成真实实体并经 fight_ob 接真实战斗'
       '（旧随机数模拟已移除）；占位生成器已删；on_mob_defeated 结算挂接'
       '与 mark_trial_survivors 存活标记写入端在位')
else:
    for b in cbad:
        err(b)

print('\n═══ 十、主线+晋升接线（#108 c6） ═══')
qbad = quest_wiring_guard()
if not qbad:
    ok('mq_1_9 目标=真进副本（OBJ_REACH /d/xuejin）+试炼存活（OBJ_SURVIVE 读'
       '存活标记）；quest_progress 处理 OBJ_SURVIVE；sect_d promote 读取'
       '存活标记做晋升门槛——写入端与读取端均在位')
else:
    for b in qbad:
        err(b)

print('\n═══ 十一、状态转正+零悬空+进出命令（#108 c5/c7） ═══')
sbad = [b for b in status_guard(daemon) if '奖励物品' not in b]
cbad2 = command_guard()
sbad += cbad2
if not sbad:
    ok('blood_forbidden 状态经既有机制转正为 OPEN；入口/出口路径零悬空；'
       'LAYERS=3 对齐三层；jindi 命令提供 enter/exit/explore/info 并调用'
       '守护进程真实接口')
else:
    for b in sbad:
        err(b)

print('\n═══ 十二、突变实证（房间出口改死路径 → 应红） ═══')
chonguo_raw = readf('d/xuejin/chonguo.c') if os.path.isfile('d/xuejin/chonguo.c') else ''
mut3 = chonguo_raw.replace(
    '"south" : "/d/xuejin/heixin",', '"south" : "/d/xuejin/gone",')
if mut3 == chonguo_raw:
    err('突变注入失败：未找到 chonguo 的 south 出口行')
else:
    m3_bad = room_structure_guard(
        mut3, 'd/xuejin/chonguo.c', 3, {'south': '/d/xuejin/heixin'},
        {'/d/xuejin/npc/mojiao': 1, '/d/xuejin/npc/huwei': 2})
    if m3_bad:
        ok('巢窟出口改死路径 → 守卫红（%d 条失败消息）' % len(m3_bad))
    else:
        err('突变失败：房间出口改死路径后守卫仍绿（守卫恒真）')

print('\n═══ 十三、突变实证（sr_mobs 改死路径 → 应红） ═══')
waiwei_raw = readf('d/xuejin/waiwei.c') if os.path.isfile('d/xuejin/waiwei.c') else ''
mut4 = waiwei_raw.replace('"/d/xuejin/npc/dushe": 2,', '"/d/xuejin/npc/gone": 2,')
if mut4 == waiwei_raw:
    err('突变注入失败：未找到 waiwei 的 sr_mobs dushe 行')
else:
    m4_bad = room_structure_guard(
        mut4, 'd/xuejin/waiwei.c', 1,
        {'north': '/d/yuejin/zhongceng', 'south': '/d/xuejin/rukou'},
        {'/d/xuejin/npc/dushe': 2, '/d/xuejin/npc/juwen': 2,
         '/d/xuejin/npc/dushi': 1})
    if m4_bad:
        ok('房间 sr_mobs 改死路径 → 守卫红（%d 条失败消息）' % len(m4_bad))
    else:
        err('突变失败：sr_mobs 改死路径后守卫仍绿（守卫恒真）')

print('\n═══ 十四、突变实证（删除存活标记写入 → 应红） ═══')
mut5 = daemon_raw.replace('killer->set(flag, time());\n', '')
if mut5 == daemon_raw:
    err('突变注入失败：未找到存活标记持久写入行')
else:
    m5_bad = combat_guard(strip_lpc_comments(mut5))
    if m5_bad:
        ok('删除存活标记持久写入行 → 守卫红（%d 条失败消息）' % len(m5_bad))
    else:
        err('突变失败：删除存活标记写入后守卫仍绿（守卫恒真）')

print('\n═══ 十五、突变实证（状态改回 CLOSED → 应红） ═══')
mut6 = daemon.replace(
    'SR_FIELD_STATUS      : SR_STATUS_OPEN,',
    'SR_FIELD_STATUS      : SR_STATUS_CLOSED,')
if mut6 == daemon:
    err('突变注入失败：未找到 blood_forbidden 的 SR_STATUS_OPEN 注册行')
else:
    m6_bad = status_guard(mut6)
    if m6_bad:
        ok('注册状态改回 CLOSED → 守卫红（%d 条失败消息）' % len(m6_bad))
    else:
        err('突变失败：状态改回 CLOSED 后守卫仍绿（守卫恒真）')

print('\n═══ 十六、突变实证（on_mob_defeated 形参顺序写反 → 应红） ═══')
mut7 = daemon.replace(
    'int on_mob_defeated(object killer, object victim)',
    'int on_mob_defeated(object victim, object killer)')
if mut7 == daemon:
    err('突变注入失败：未找到 on_mob_defeated 签名行')
else:
    m7_bad = combat_guard(mut7)
    if m7_bad:
        ok('on_mob_defeated 形参顺序写反（实参/形参颠倒）→ 守卫红（%d 条失败消息）'
           % len(m7_bad))
    else:
        err('突变失败：形参顺序写反后守卫仍绿（守卫恒真，接线语义断链不可检出）')

print('\n═══════════════════════════════════')
print('断言总数: %d, 失败: %d' % (oks, fails))
if fails:
    print('FAILED: 存在失败项')
    sys.exit(1)
print('RESULT: PASS 全部通过')
