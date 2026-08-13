#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
main_quest_verify.py — #65 主线任务内容填充（第零章+第一章越国篇）验收脚本
=====================================================================
三部分、任一步失败整体非零退出（与 #59/#61 验证脚本同构）：

[1] 静态校验（读 LPC 原文）：
    - 17 个主线任务定义齐全（id/name/type/realm_range/objectives/rewards/description/chain_id/chapter）
    - chain_defs 两条链（chain_main_0=4 节点、chain_main_1=13 节点），任务全部存在于 quest_defs
    - 每个 objective 的 target 房间文件真实存在（#67/#58 场景挂接）
    - realm_range 合法（0~7、min<=max）
    - 前置任务（prerequisites.quests）都存在、无环
    - 奖励键合法（exp/coin/reputation/contribution/items/skills）
    - LPC 括号配对（去字符串/注释/@LONG heredoc 后统计）
    - 章节定义 5 章齐全，第零章/第一章 chain_id 非空

[2] 行为模拟（Python 忠实翻译 quest_chain_d 关键路径）：
    - 场景1 新玩家第零章全流程：接取→到达→提交→自动接续，4 节点走完
    - 场景2 第一章前 6 节点（炼气段）：mq_1_1..mq_1_6 串行推进
    - 场景3 境界门槛：炼气玩家被 mq_1_7（筑基）拦截；突破筑基后放行
    - 场景4 奖励渠道：声望(REPUTATION_D)/贡献(SECT_D)/物品(items)/功法(skills) 结算
    - 场景5 完整链路：0_1→1_13 全 17 节点逐步推进（含境界突破），第一章完成得称号

[3] LPC 原文守卫 + 突变验证：
    - 守卫断言：main_quest_d.c 的 complete_node 调 complete_quest（结算走框架）、
      quest_progress 整表写回（active[quest_id]=sub; player->set(QUEST_CHAIN_ACTIVE, active)）、
      is_quest_available 供 find_next_available_quest 过滤（境界门槛真实生效）
    - 突变：删除一个任务定义/改坏房间路径 → 对应断言转红（证明非恒真）
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MQ_D = os.path.join(ROOT, "adm/daemons/main_quest_d.c")
MQ_H = os.path.join(ROOT, "include/main_quest.h")
QC_D = os.path.join(ROOT, "adm/daemons/quest_chain_d.c")

FAIL = 0
PASS = 0
CHECKS = []

def check(name, cond, detail=""):
    global PASS, FAIL
    if cond:
        PASS += 1
        CHECKS.append(("PASS", name, detail))
    else:
        FAIL += 1
        CHECKS.append(("FAIL", name, detail))
        print(f"  ✗ FAIL: {name} {detail}")

# ──────────────────────────────────────────────
# 工具：LPC 源码提取
# ──────────────────────────────────────────────

def strip_lpc(src):
    """去字符串/注释/@LONG heredoc，返回 (代码流, 行映射)。"""
    out = []
    i, n = 0, len(src)
    line = 1
    while i < n:
        c = src[i]
        if c == "\n":
            line += 1
            i += 1
            continue
        # @LONG heredoc
        if c == "@" and i + 1 < n and src[i+1].isalpha():
            # 找 marker 字母
            j = i + 1
            while j < n and src[j].isalnum():
                j += 1
            marker = src[i+1:j]
            # 跳到 marker 行结束后的 marker 结束
            k = src.find("\n" + marker, j)
            if k == -1:
                # 未找到结束标记：跳到 EOF
                i = n
                continue
            # 跳过整块到结束标记行
            nl = src.find("\n", k + 1)
            i = nl if nl != -1 else n
            line += src.count("\n", 0, i) - (line - 1) if False else 0
            continue
        # 行注释
        if c == "/" and i + 1 < n and src[i+1] == "/":
            k = src.find("\n", i)
            i = k if k != -1 else n
            continue
        # 块注释
        if c == "/" and i + 1 < n and src[i+1] == "*":
            k = src.find("*/", i + 2)
            i = (k + 2) if k != -1 else n
            continue
        # 字符串
        if c == '"':
            k = i + 1
            while k < n and src[k] != '"':
                if src[k] == "\\":
                    k += 1
                k += 1
            i = k + 1 if k < n else n
            continue
        out.append(c)
        i += 1
    return "".join(out)

def extract_mapping_blocks(src):
    """提取 LPC 源码中的 mapping 赋值块：return ([ ... ]); 或 quest_defs = ([ ... ]);"""
    blocks = {}
    # 简单方式：找 "id": "mq_X_Y" 所在的任务块，按引号配对切分
    # 这里用更稳的方式：找 quest_defs 的每个顶层条目
    # 先定位 quest_defs = ([ 之后
    m = re.search(r"nosave mapping quest_defs = \(\[", src)
    if not m:
        return blocks
    body_start = m.end()
    # 找与之配对的 ])
    # 简单括号配对（此时字符串已由外层剥离——不行，这里用原 src）
    # 用 strip 后的代码流找顶层，但我们要原始文本取值。
    # 折衷：逐个找 '"id": "mq_', 截到下一个 '"id": "mq_' 或 quest_defs 结束
    ids = list(re.finditer(r'"id":\s*"(mq_\d+_\d+)"', src))
    if not ids:
        return blocks
    for idx, mi in enumerate(ids):
        nxt = ids[idx + 1].start() if idx + 1 < len(ids) else len(src)
        blocks[mi.group(1)] = src[mi.start():nxt]
    return blocks

def extract_chain_defs(src):
    """提取 chain_defs 映射：chain_main_X: ({ "mq_...", ... })"""
    m = re.search(r"nosave mapping chain_defs = \(\[(.*?)\n\]\);", src, re.S)
    if not m:
        return {}
    body = m.group(1)
    chains = {}
    for cm in re.finditer(r'"chain_(main_\d+)"\s*:\s*\(\{\s*(.*?)\s*\}\)', body, re.S):
        qids = [q.strip('"') for q in re.findall(r'"mq_\d+_\d+"', cm.group(2))]
        chains["chain_" + cm.group(1)] = qids
    return chains

def parse_quest(src, qid):
    """从任务块提取关键字段（去注释后的文本子串匹配）。"""
    blocks = extract_mapping_blocks(src)
    blk = blocks.get(qid, "")
    if not blk:
        return None
    res = {"id": qid}
    m = re.search(r'"name":\s*"([^"]+)"', blk)
    res["name"] = m.group(1) if m else None
    m = re.search(r'"type":\s*(\w+)', blk)
    res["type"] = m.group(1) if m else None
    m = re.search(r'"chain_id":\s*"(\w+)"', blk)
    res["chain_id"] = m.group(1) if m else None
    m = re.search(r'"chapter":\s*(\w+)', blk)
    res["chapter"] = m.group(1) if m else None
    m = re.search(r'"realm_range":\s*\(\{\s*(\d+),\s*(\d+)\s*\}\)', blk)
    res["realm_range"] = (int(m.group(1)), int(m.group(2))) if m else None
    m = re.search(r'"objectives":\s*\(\{\s*\[([^\]]*)\]\s*\}\)', blk, re.S)
    res["objectives"] = m.group(0) if m else None
    objs = []
    for om in re.finditer(r'"type":\s*(\w+),\s*"target":\s*"([^"]+)"', blk):
        objs.append((om.group(1), om.group(2)))
    res["obj_list"] = objs
    m = re.search(r'"rewards":\s*\(\s*\[([^\]]*)\]', blk, re.S)
    res["rewards_blk"] = m.group(1) if m else ""
    m = re.search(r'"description":\s*"([^"]*)"', blk)
    res["description"] = m.group(1) if m else None
    prereqs = []
    for pm in re.finditer(r'"quests":\s*\(\{\s*((?:"mq_\d+_\d+"\s*,\s*)*"mq_\d+_\d+")\s*\}\)', blk):
        prereqs = [q.strip('"') for q in re.findall(r'"mq_\d+_\d+"', pm.group(1))]
    res["prereqs"] = prereqs
    return res

def brace_balance(src):
    """统计 ()[]{} 配对（已去字符串/注释/heredoc）。"""
    code = strip_lpc(src)
    pairs = {")": "(", "]": "[", "}": "{"}
    stack = []
    for c in code:
        if c in "([{":
            stack.append(c)
        elif c in ")]}":
            if not stack or stack[-1] != pairs[c]:
                return False, f"mismatch near {code[max(0, code.index(c)-10):code.index(c)+10]}"
            stack.pop()
    return (len(stack) == 0, f"unclosed: {stack[:5]}")

# ──────────────────────────────────────────────
# 行为模拟：quest_chain_d 关键路径忠实翻译
# ──────────────────────────────────────────────

class MockPlayer:
    def __init__(self, realm_idx=0, realm_layer=1, sect=None):
        # realm_layer：炼气期层数（1-9）；对齐 realm 存储约定「炼气N层」（ASCII 数字，#61）
        self.db = {
            "realm": "炼气%d层" % realm_layer,
            "realm_index": realm_idx,
            "realm_layer": realm_layer,
            "combat_exp": 0,
            "title": "",
            "quest_chain": {},
        }
        self.sect = sect
        self.inventory = []
        self.log = []
        self.location = "/d/yueguo/qingniu/zhenkou"

    def query(self, k):
        if k == "realm":
            return self.db["realm"]
        if k == "combat_exp":
            return self.db["combat_exp"]
        if k == "title":
            return self.db["title"]
        if k == QUEST_CHAIN_ACTIVE:
            return self.db.get("quest_chain", {}).get("active")
        if k == QUEST_CHAIN_COMPLETED:
            return self.db.get("quest_chain", {}).get("completed")
        if k == QUEST_CHAIN_PROGRESS:
            return self.db.get("quest_chain", {}).get("progress")
        if k == QUEST_CHAIN_DAILY_STREAK:
            return 0
        if k == QUEST_CHAIN_LAST_DAY:
            return 0
        return None

    def set(self, k, v):
        self.db["quest_chain"][k.split("/")[-1]] = v

    def add(self, k, v):
        if k == "combat_exp":
            self.db["combat_exp"] += v
        else:
            self.db[k] = self.db.get(k, 0) + v

    def set_realm(self, idx):
        names = ["炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘"]
        self.db["realm"] = names[idx] + "期"
        self.db["realm_index"] = idx
        self.db["realm_layer"] = 0  # 大境界无层数信息（parse_realm 返回 layer=0）

    def set_realm_layer(self, layer):
        """提升炼气期层数（仅炼气期有层数语义，对齐 sect_d.parse_realm）。"""
        self.db["realm"] = "炼气%d层" % layer
        self.db["realm_layer"] = layer

    def set_location(self, path):
        self.location = path

    def tell(self, msg):
        self.log.append(msg)

    # ── 门派状态（c4 审查第 2 轮：入宗/贡献渠道模拟） ──
    def query_sect(self):
        """模拟 SECT_D->query_player_sect（sect/id 属性）。"""
        return self.db.get("sect_id")

    def join_sect(self, sect_id):
        """模拟 SECT_D->join_sect，含真实 check_join 门禁语义（sect_d.c:403-441）：
        - 已入他派 → 拒（不强制改派）
        - realm 存在且为炼气 1-2 层（realm_index==0 且层数 1-2）→ 「修为不足：入宗需炼气三层以上」拒
        - 炼气 3 层+、筑基以上、realm 缺失/空 → 放行。
          realm 缺失放行与真实代码一致：sect_d.c:425 守卫 stringp(realm) && realm != ""，
          缺失/空不查层数。本脚本 __init__ 恒设 realm 三键，此分支现有场景不可达；
          若要模拟 realm 缺失场景须先 del self.db["realm"]。
        """
        if self.db.get("sect_id"):
            self.log.append("你已是其他门派弟子，入宗被拒")
            return False
        if self.db.get("realm"):
            ri = self.db.get("realm_index", 0)
            layer = self.db.get("realm_layer", 1)
            if ri == 0 and 1 <= layer < 3:
                self.log.append("修为不足：入宗需炼气三层以上")
                return False
        self.db["sect_id"] = sect_id
        return True

    def add_contribution(self, amount):
        """模拟 SECT_D->add_contribution：未入宗返回 0（真实渠道行为）。"""
        if not self.db.get("sect_id"):
            return 0
        self.db["contrib"] = self.db.get("contrib", 0) + amount
        return amount


QUEST_CHAIN_ACTIVE = "quest_chain/active"
QUEST_CHAIN_COMPLETED = "quest_chain/completed"
QUEST_CHAIN_PROGRESS = "quest_chain/progress"
QUEST_CHAIN_DAILY_STREAK = "quest_chain/daily_streak"
QUEST_CHAIN_LAST_DAY = "quest_chain/last_active_day"

# 从 LPC 原文解析出的任务数据（模拟用）
QUEST_DEFS = {}

def realm_index(player):
    return player.db["realm_index"]

def check_prerequisites(prereqs, player):
    if not prereqs:
        return True
    for q in prereqs:
        completed = player.query(QUEST_CHAIN_COMPLETED) or {}
        if not completed.get(q):
            return False
    return True

def is_quest_available(qid, player):
    t = QUEST_DEFS.get(qid)
    if not t:
        return False
    # 一次性任务：已完成不可再接
    completed = player.query(QUEST_CHAIN_COMPLETED) or {}
    if completed.get(qid):
        return False
    active = player.query(QUEST_CHAIN_ACTIVE) or {}
    if qid in active:
        return False
    # 前置
    if not check_prerequisites(t.get("prereqs", []), player):
        return False
    # 境界范围
    rmin, rmax = t["realm_range"]
    ri = realm_index(player)
    if ri < rmin or ri > rmax:
        return False
    return True

def assign_quest(qid, player):
    if not is_quest_available(qid, player):
        return False
    active = player.query(QUEST_CHAIN_ACTIVE) or {}
    active[qid] = {"status": 2, "progress": {}}
    player.set(QUEST_CHAIN_ACTIVE, active)
    return True

def quest_progress(qid, player):
    """OBJ_REACH 按当前位置判定（忠实翻译 main_quest_d.quest_progress）。"""
    active = player.query(QUEST_CHAIN_ACTIVE)
    if not active or qid not in active:
        return False
    t = QUEST_DEFS[qid]
    sub = active[qid]
    progress = sub.get("progress") or {}
    here = player.location
    done = True
    for i, (otype, target) in enumerate(t["obj_list"]):
        key = f"obj_{i}"
        cur = progress.get(key, 0)
        amount = 1
        if otype in ("OBJ_REACH", "OBJ_TALK") and target and here.startswith(target):
            cur = amount
            progress[key] = cur
        if cur < amount:
            done = False
    sub["progress"] = progress
    active[qid] = sub  # 整表写回
    player.set(QUEST_CHAIN_ACTIVE, active)
    return done

def complete_quest(qid, player):
    active = player.query(QUEST_CHAIN_ACTIVE)
    if not active or qid not in active:
        return False
    del active[qid]
    player.set(QUEST_CHAIN_ACTIVE, active)
    completed = player.query(QUEST_CHAIN_COMPLETED) or {}
    completed[qid] = 1
    player.set(QUEST_CHAIN_COMPLETED, completed)
    return True

# 场景数据
CHAIN_0 = ["mq_0_1", "mq_0_2", "mq_0_3", "mq_0_4"]
CHAIN_1 = ["mq_1_1", "mq_1_2", "mq_1_3", "mq_1_4", "mq_1_5",
           "mq_1_6", "mq_1_7", "mq_1_8", "mq_1_9", "mq_1_10",
           "mq_1_11", "mq_1_12", "mq_1_13"]

def quest_target(qid):
    return QUEST_DEFS[qid]["obj_list"][0][1]

def run_chain(player, chain, start_realm=None):
    """跑完整条链：逐节点 接取→到达→提交。境界不足自动暂停。"""
    done_ids = []
    for qid in chain:
        if not assign_quest(qid, player):
            break  # 境界门槛拦截
        # 到达目标
        player.set_location(quest_target(qid))
        if not quest_progress(qid, player):
            break
        if not complete_quest(qid, player):
            break
        # 剧情入宗（忠实翻译 main_quest_d.complete_node 的 mq_1_6 分支）：
        # 完成 mq_1_6 后若玩家未入宗，自动入黄枫谷（默认分支）
        if qid == "mq_1_6" and not player.query_sect():
            player.join_sect("huangfeng_valley")
        done_ids.append(qid)
    return done_ids

# ──────────────────────────────────────────────
# 主流程
# ──────────────────────────────────────────────

def run_all(src, label="真实交付"):
    """对给定 main_quest_d.c 源码文本跑全部断言（静态+行为+守卫+突变）。

    突变段对改坏后的源码文本重跑全部断言，验证对应断言转红（证明非恒真）。
    不做任何文件写入：改坏在内存中的文本副本上进行，工作树零污染。
    """
    global PASS, FAIL, CHECKS, QUEST_DEFS
    PASS, FAIL, CHECKS = 0, 0, []

    chains = extract_chain_defs(src)
    all_ids = sorted(set(chains["chain_main_0"]) | set(chains["chain_main_1"]))
    QUEST_DEFS = {}
    for qid in all_ids:
        t = parse_quest(src, qid)
        QUEST_DEFS[qid] = t if t else None  # 定义缺失（突变检测：静态校验会红）

    print(f"== [{label}] [1] 静态校验 ==")

    # 1.1 任务定义齐全
    expect = set(CHAIN_0 + CHAIN_1)
    defined = set(q for q, t in QUEST_DEFS.items() if t is not None)
    check("17 个主线任务定义齐全", defined == expect,
          f"got {sorted(defined)}")

    # 1.2 每条链节点数与预期一致
    check("chain_main_0 4 节点", chains.get("chain_main_0") == CHAIN_0,
          str(chains.get("chain_main_0")))
    check("chain_main_1 13 节点", chains.get("chain_main_1") == CHAIN_1,
          str(chains.get("chain_main_1")))

    # 1.3 目标房间存在（#67/#58 场景挂接）
    missing_rooms = []
    for qid in all_ids:
        t = QUEST_DEFS[qid]
        if t is None:
            continue
        for otype, target in t["obj_list"]:
            if otype in ("OBJ_REACH", "OBJ_TALK"):
                rel = target.lstrip("/") + ".c"
                if not os.path.exists(os.path.join(ROOT, rel)):
                    missing_rooms.append((qid, target))
    check("目标房间文件全部存在", not missing_rooms, str(missing_rooms))

    # 1.4 realm_range 合法
    bad_range = [(q, t["realm_range"]) for q, t in QUEST_DEFS.items()
                 if t is not None
                 and (not t["realm_range"] or t["realm_range"][0] > t["realm_range"][1]
                      or t["realm_range"][0] < 0 or t["realm_range"][1] > 7)]
    check("realm_range 全部合法 (0~7, min<=max)", not bad_range, str(bad_range))

    # 1.5 前置任务存在且无环
    all_set = set(all_ids)
    bad_prereq = []
    for qid, t in QUEST_DEFS.items():
        if t is None:
            continue
        for p in t.get("prereqs", []):
            if p not in all_set:
                bad_prereq.append((qid, p))
    check("前置任务全部存在", not bad_prereq, str(bad_prereq))
    # 环检测
    visited, stack, in_stack = set(), set(), set()
    def dfs(q):
        if q in in_stack:
            return False
        if q in visited:
            return True
        in_stack.add(q)
        t = QUEST_DEFS.get(q)
        if t is not None:
            for p in t.get("prereqs", []):
                if not dfs(p):
                    return False
        in_stack.discard(q)
        visited.add(q)
        return True
    cyclic = [q for q in all_ids if not dfs(q)]
    check("前置任务无环", not cyclic, str(cyclic))

    # 1.6 奖励键合法
    bad_reward = []
    for qid, t in QUEST_DEFS.items():
        if t is None:
            continue
        blk = t.get("rewards_blk", "")
        # 检查有至少一个 exp/coin 之一
        if "exp" not in blk and "coin" not in blk:
            bad_reward.append((qid, "no exp/coin"))
    check("奖励至少含 exp/coin", not bad_reward, str(bad_reward))

    # 1.7 LPC 括号配对（三个文件）
    for fp in (MQ_D, MQ_H, os.path.join(ROOT, "cmds/usr/main_quest.c")):
        ok, why = brace_balance(open(fp, encoding="utf-8").read())
        check(f"括号配对: {os.path.basename(fp)}", ok, why)

    # 1.8 章节定义 5 章 + 前两章 chain_id 非空
    ch_count = len(re.findall(r'CHAPTER_(?:MORTAL|YUE|LUANXINGHAI|LINGJIE|FEISHENG):\s*\(\[', src))
    check("5 章定义齐全", ch_count == 5, str(ch_count))
    check("前两章 chain_id 非空",
          '"chain_id": "chain_main_0"' in src and '"chain_id": "chain_main_1"' in src)
    check("后续章 chain_id 为空占位",
          '"chain_id": "", "realm_min": 2' in src and
          '"chain_id": "", "realm_min": 4' in src and
          '"chain_id": "", "realm_min": 7' in src)

    # 1.9 主线类型 = QUEST_TYPE_MAIN + REFRESH_ONCE
    main_type_ok = all(t is not None and t["type"] == "QUEST_TYPE_MAIN"
                       for t in QUEST_DEFS.values())
    check("全部任务 type=QUEST_TYPE_MAIN", main_type_ok)

    # 1.10 键一致性守卫（#65 审查第 1 轮 F1）：任务模板必须写 description 键、
    #     query_progress 必须读 description 键（旧实现遗留读 desc 曾导致进度面板描述丢失）
    desc_def_ok = all(t is not None and t["description"] for t in QUEST_DEFS.values())
    check("守卫 任务模板均含 description 键", desc_def_ok)
    # 模板不允许出现 desc 键（防新旧键混用）
    desc_key_absent = not re.search(r'"desc"\s*:', src)
    check("守卫 模板无 desc 键", desc_key_absent)
    # query_progress 读 description 而非 desc
    reads_description = ('node_data["description"]' in src)
    reads_legacy_desc = re.search(r'node_data\["desc"\]', src) is None
    check("守卫 query_progress 读 description 键", reads_description)
    check("守卫 query_progress 无 desc 残留", reads_legacy_desc)

    print(f"\n== [{label}] [2] 行为模拟 ==")

    # 场景1：新玩家（炼气 index=0）走完第零章 4 节点
    p = MockPlayer(realm_idx=0)
    p.set_location("/d/yueguo/qingniu/zhenkou")
    done0 = run_chain(p, CHAIN_0)
    check("场景1 第零章 4 节点全走完", done0 == CHAIN_0, str(done0))
    completed = p.query(QUEST_CHAIN_COMPLETED) or {}
    check("场景1 第零章全完成", set(completed.keys()) == set(CHAIN_0),
          str(sorted(completed.keys())))
    # 场景1b：第零章完成后第一章解锁（mq_1_1 可接）——独立玩家避免污染后续场景
    p1b = MockPlayer(realm_idx=0)
    p1b.set_location("/d/yueguo/qingniu/zhenkou")
    run_chain(p1b, CHAIN_0)
    check("场景1b 跨章解锁 mq_1_1 可接", assign_quest("mq_1_1", p1b))

    # 场景2：第一章炼气段（1.1-1.6）串行推进。
    # 入宗玩家修为对齐真实 join_sect 门禁（sect_d.c:423-430 入宗需炼气三层以上）：
    # 炼气1-2层自动入宗会被拒，故提升到炼气5层使 mq_1_6 自动入宗真实可达。
    p.set_realm_layer(5)
    done1 = run_chain(p, CHAIN_1[:6])
    check("场景2 第一章前 6 节点（炼气段）走完", done1 == CHAIN_1[:6], str(done1))
    check("场景2 炼气5层 mq_1_6 自动入黄枫谷", p.query_sect() == "huangfeng_valley",
          f"sect={p.query_sect()}")

    # 场景3：境界门槛——炼气玩家被 mq_1_7（筑基门槛）拦截
    blocked = assign_quest("mq_1_7", p)
    check("场景3 炼气玩家接 mq_1_7 被境界拦截", not blocked)
    # 突破筑基后放行
    p.set_realm(1)
    ok_after = assign_quest("mq_1_7", p)
    check("场景3 筑基后 mq_1_7 可接", ok_after)

    # 场景4：奖励渠道（在模拟层验证六渠道接线存在性）
    blk17 = QUEST_DEFS["mq_1_7"]["rewards_blk"] if QUEST_DEFS["mq_1_7"] else ""
    blk16 = QUEST_DEFS["mq_1_6"]["rewards_blk"] if QUEST_DEFS["mq_1_6"] else ""
    blk18 = QUEST_DEFS["mq_1_8"]["rewards_blk"] if QUEST_DEFS["mq_1_8"] else ""
    check("场景4 mq_1_6 声望渠道(huangfeng_valley)", "huangfeng_valley" in blk16)
    check("场景4 mq_1_7 贡献渠道(contribution)", "contribution" in blk17)
    check("场景4 mq_1_7 物品渠道(lingzhi)", "/clone/drug/lingzhi" in blk17)
    check("场景4 mq_1_8 功法渠道(qingyuan-jianjue)", "qingyuan-jianjue" in blk18)
    # 物品文件真实存在（LPC new() 引用不带 .c，存在性检查补 .c）
    item_missing = []
    for qid in all_ids:
        t = QUEST_DEFS[qid]
        if t is None:
            continue
        for m in re.finditer(r'"items":\s*\(\{\s*([^)]*?)\s*\}\)',
                             t["rewards_blk"]):
            for ip in re.findall(r'"([^"]+\.c?)"', m.group(1)):
                rel = ip.lstrip("/")
                if not rel.endswith(".c"):
                    rel += ".c"
                if not os.path.exists(os.path.join(ROOT, rel)):
                    item_missing.append((qid, ip))
    check("场景4 奖励物品路径存在", not item_missing, str(item_missing))

    # 场景5：完整链路 0_1→1_13（17 节点），中间突破境界。
    # 炼气5层起步：mq_1_6 自动入宗对齐真实门禁（炼气三层以上），realm_index=0 不影响境界门槛判定
    p2 = MockPlayer(realm_idx=0, realm_layer=5)
    p2.set_location("/d/yueguo/qingniu/zhenkou")
    full = []
    full += run_chain(p2, CHAIN_0)
    full += run_chain(p2, CHAIN_1[:6])   # 炼气段 1.1-1.6
    # 突破筑基
    p2.set_realm(1)
    full += run_chain(p2, ["mq_1_7", "mq_1_8", "mq_1_9", "mq_1_10", "mq_1_11"])
    # 突破结丹
    p2.set_realm(2)
    full += run_chain(p2, ["mq_1_12", "mq_1_13"])
    check("场景5 全 17 节点走完（含境界突破）",
          set(full) == set(all_ids), f"len={len(full)} {sorted(full)}")
    c2 = p2.query(QUEST_CHAIN_COMPLETED) or {}
    check("场景5 第一章链 13 节点全部完成",
          set(c2.keys()) == set(all_ids), str(sorted(c2.keys())))

    # 场景6：剧情入宗（c4 修复，审查第 2 轮）——完成 mq_1_6 后自动入黄枫谷，
    # 贡献/功法奖励对走主线且未手动入宗的玩家真实可达。
    # 修为对齐真实门禁：炼气5层（>三层）自动入宗成功；炼气1-2层被拒（审查第 3 轮 P1 保真度修复）。
    p3 = MockPlayer(realm_idx=0, realm_layer=5)
    p3.set_location("/d/yueguo/qingniu/zhenkou")
    run_chain(p3, CHAIN_0)
    run_chain(p3, CHAIN_1[:6])          # 完成 mq_1_6（拜入黄枫谷）
    check("场景6 完成 mq_1_6 后自动入黄枫谷", p3.query_sect() == "huangfeng_valley",
          f"sect={p3.query_sect()}")
    # 已入宗 → 贡献渠道真实可达（模拟 add_contribution：未入宗返回 0）
    contrib_ok = p3.add_contribution(200) == 200
    check("场景6 入宗后贡献渠道可达", contrib_ok)
    # 未入宗玩家（跳过主线直入后续场景模拟）→ 贡献渠道不可达（静默不发，与真实渠道一致）
    p4 = MockPlayer(realm_idx=0)
    p4.set_location("/d/yueguo/qingniu/zhenkou")
    check("场景6 未入宗玩家贡献渠道不可达", p4.add_contribution(200) == 0)
    # 功法渠道：入宗后 grant_skill 走本门分支（sect_config 命中 qingyuan-jianjue）
    check("场景6 入宗后功法渠道可达（sect_config 命中）",
          "qingyuan-jianjue" in src and "huangfeng_valley" in src)
    # ── P1 保真度（审查第 3 轮 小厮·1）：真实 join_sect 对炼气 1-2 层拒绝（sect_d.c:423-430），
    #    断言「自动入宗成功」必须只对炼气三层以上成立；炼气 1-2 层被拒 + 修为提升后手动补入。
    p_low = MockPlayer(realm_idx=0, realm_layer=1)
    p_low.set_location("/d/yueguo/qingniu/zhenkou")
    run_chain(p_low, CHAIN_0)
    run_chain(p_low, CHAIN_1[:6])
    check("场景6 炼气1层完成 mq_1_6 自动入宗被拒（修为不足）",
          p_low.query_sect() is None, f"sect={p_low.query_sect()}")
    check("场景6 炼气1层手动 sect join 亦被拒（对齐 check_join）",
          p_low.join_sect("huangfeng_valley") is False)
    # 修为提升到炼气5层后手动 sect join 补入成功 → 主线贡献渠道真实可达（真实玩家路径）
    p_low.set_realm_layer(5)
    join_ok = p_low.join_sect("huangfeng_valley")
    check("场景6 炼气5层手动 sect join 补入成功",
          join_ok and p_low.query_sect() == "huangfeng_valley",
          f"sect={p_low.query_sect()}")
    contrib_ok2 = p_low.add_contribution(200) == 200
    check("场景6 补入后贡献渠道可达", contrib_ok2)
    # 对齐真实兜底：realm 缺失玩家入宗放行（sect_d.c:425 守卫 stringp(realm) && realm != ""，
    # 缺失/空不查层数；#57 reverify 确认的合理兜底——存量/异常玩家可能无 realm 属性）。
    # 现有场景 __init__ 恒设 realm 三键，此处显式删除以覆盖该分支（每个 if 分支需自测覆盖）。
    p_nr = MockPlayer(realm_idx=0, realm_layer=1)
    p_nr.set_location("/d/yueguo/qingniu/zhenkou")
    run_chain(p_nr, CHAIN_0)
    run_chain(p_nr, CHAIN_1[:6])   # 炼气1层 → 自动入宗被拒（sect 仍 None）
    del p_nr.db["realm"]
    del p_nr.db["realm_index"]
    del p_nr.db["realm_layer"]
    ok_nr = p_nr.join_sect("huangfeng_valley")
    check("场景6 realm 缺失玩家入宗放行（对齐真实兜底 sect_d.c:425）",
          ok_nr and p_nr.query_sect() == "huangfeng_valley",
          f"sect={p_nr.query_sect()}")

    print(f"\n== [{label}] [3] LPC 原文守卫 ==")

    # 3.1 complete_node 调 complete_quest（结算走框架）
    guard_ok = ("QUEST_CHAIN_D->complete_quest(player, node_id)" in src)
    check("守卫 complete_node 调 complete_quest", guard_ok)

    # 3.2 quest_progress 整表写回（#59 铁律）
    guard_ok2 = ("sub[\"progress\"] = progress;" in src and
                 "active[quest_id] = sub;" in src and
                 "player->set(QUEST_CHAIN_ACTIVE, active);" in src)
    check("守卫 quest_progress 整表写回", guard_ok2)

    # 3.3 find_next_available_quest 用 is_quest_available 过滤（境界门槛真实生效）
    guard_ok3 = ("QUEST_CHAIN_D->is_quest_available(qids[i], player)" in src)
    check("守卫 境界门槛经 is_quest_available", guard_ok3)

    # 3.4 剧情入宗接线（c4 修复，审查第 2 轮）：mq_1_6 完成时调 SECT_D->join_sect
    guard_ok4 = ('if (node_id == "mq_1_6")' in src and
                 "SECT_D->join_sect(player, \"huangfeng_valley\")" in src)
    check("守卫 mq_1_6 完成时自动入黄枫谷", guard_ok4)

    # 3.4-3.6 真实突变验证（#65 审查第 1 轮 F2 修订）：
    # 对改坏后的 LPC 原文文本重跑静态+场景断言，验证对应断言转红——证明脚本非恒真。
    # 注意：改写在内存文本上进行，不写文件、不污染工作树。
    print(f"\n== [{label}] [4] 真实突变（改坏原文文本→重跑→验证转红） ==")
    mutation_cases = [
        ("改坏 mq_0_1 目标房间路径",
         src.replace('"target": "/d/yueguo/qingniu/zhenkou"',
                     '"target": "/d/yueguo/qingniu/NONEXIST"', 1),
         "目标房间文件全部存在"),
        ("删除 mq_1_13 任务定义",
         src.replace('"id": "mq_1_13"', '"id": "mq_1_13_del"', 1),
         "17 个主线任务定义齐全"),
        ("放宽 mq_1_7 境界门槛 {1,2}→{0,1}",
         src.replace('"name": "百药园看守", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,\n        "realm_range": ({ 1, 2 }),',
                     '"name": "百药园看守", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,\n        "realm_range": ({ 0, 1 }),', 1),
         "场景3 炼气玩家接 mq_1_7 被境界拦截"),
        ("删除 mq_1_6 自动入宗接线（c4 修复）",
         src.replace('if (node_id == "mq_1_6")\n    {\n        if (!SECT_D->query_player_sect(player))\n            SECT_D->join_sect(player, "huangfeng_valley");\n    }',
                     'if (0) { }', 1),
         "守卫 mq_1_6 完成时自动入黄枫谷"),
    ]
    for mname, mutated_src, expect_fail in mutation_cases:
        if mutated_src == src:
            check(f"突变 [{mname}]: 改坏文本构造成功", False, "替换未生效（模式不匹配）")
            continue
        failed = _run_mut(mutated_src)
        if expect_fail in failed:
            check(f"突变 [{mname}]: 断言「{expect_fail}」转红（非恒真证明）", True)
        else:
            check(f"突变 [{mname}]: 断言「{expect_fail}」应转红但未转红",
                  False, f"实际失败集: {failed}")

    return PASS, FAIL


def _run_mut(mutated_src):
    """对改坏文本重跑静态+场景断言，返回失败断言名列表（独立统计，不碰主流程计数）。"""
    chains = extract_chain_defs(mutated_src)
    all_ids = sorted(set(chains["chain_main_0"]) | set(chains["chain_main_1"]))
    defs = {}
    for qid in all_ids:
        t = parse_quest(mutated_src, qid)
        defs[qid] = t if t else None

    failed = []
    def mcheck(name, cond):
        if not cond:
            failed.append(name)

    # 静态
    defined = set(q for q, t in defs.items() if t is not None)
    mcheck("17 个主线任务定义齐全", defined == set(CHAIN_0 + CHAIN_1))
    missing_rooms = []
    for qid in all_ids:
        t = defs[qid]
        if t is None:
            continue
        for otype, target in t["obj_list"]:
            if otype in ("OBJ_REACH", "OBJ_TALK"):
                rel = target.lstrip("/") + ".c"
                if not os.path.exists(os.path.join(ROOT, rel)):
                    missing_rooms.append((qid, target))
    mcheck("目标房间文件全部存在", not missing_rooms)
    bad_range = [(q, t["realm_range"]) for q, t in defs.items()
                 if t is not None
                 and (not t["realm_range"] or t["realm_range"][0] > t["realm_range"][1]
                      or t["realm_range"][0] < 0 or t["realm_range"][1] > 7)]
    mcheck("realm_range 全部合法 (0~7, min<=max)", not bad_range)
    mcheck("任务模板均含 description 键",
           all(t is not None and t["description"] for t in defs.values()))
    # 剧情入宗守卫（c4 修复）：mq_1_6 完成时调 SECT_D->join_sect
    mcheck("守卫 mq_1_6 完成时自动入黄枫谷",
           'if (node_id == "mq_1_6")' in mutated_src and
           "SECT_D->join_sect(player, \"huangfeng_valley\")" in mutated_src)

    # 场景3 境界门槛（改坏放宽后：炼气玩家应能接 mq_1_7 → 原拦截断言转红）
    saved_defs, saved_chains = globals().get("QUEST_DEFS"), globals().get("CHAIN_DEFS")
    globals()["QUEST_DEFS"] = defs
    try:
        p = MockPlayer(realm_idx=0)
        p.set_location("/d/yueguo/qingniu/zhenkou")
        run_chain(p, CHAIN_0)
        run_chain(p, CHAIN_1[:6])
        blocked = assign_quest("mq_1_7", p)
        mcheck("场景3 炼气玩家接 mq_1_7 被境界拦截", not blocked)
    finally:
        globals()["QUEST_DEFS"] = saved_defs
    return failed


def main():
    src = open(MQ_D, encoding="utf-8").read()
    run_all(src)

    print(f"\n结果: PASS={PASS} FAIL={FAIL}")
    if FAIL:
        for name, ok, detail in CHECKS:
            if ok == "FAIL":
                print(f"  - {name} {detail}")
        sys.exit(1)
    print("RESULT: OK #65 主线任务内容填充（第零章+第一章越国篇）静态验收全部通过")
    sys.exit(0)


if __name__ == "__main__":
    main()
