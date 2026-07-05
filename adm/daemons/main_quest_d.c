// main_quest_d.c
// 主线任务守护进程 — 节点管理、进度追踪、奖励发放
// Based on: 02-扩充内容/02-任务链与奖励曲线.md
//
// 职责：
//   1. 管理 5 章主线框架定义（节点+条件+奖励）
//   2. 玩家进度追踪（当前章节/节点/已完成列表）
//   3. 串行链自动推进
//   4. 章节完成检测与里程碑奖励发放
//   5. 境界条件检查（防止跳章节）

#include <ansi.h>
#include <main_quest.h>

inherit F_DBASE;

// 节点数据模型：
// ([
//   "node_0_1": ([
//     "id":       "node_0_1",
//     "chapter":  0,
//     "index":    1,
//     "name":     "七玄门入门",
//     "desc":     "前往七玄门，拜墨大夫为师...",
//     "chapter_name": "凡人篇",
//     "chapter_idx":  0,
//     "prereq_node":  null,
//     "realm_min":    0,
//     "rewards":      ([ "exp": 100, "coin": 10 ]),
//   ]),
// ])

// 章节数据模型：
// ([
//   "chapter": 0,
//   "name": "凡人篇",
//   "node_ids": ({ "node_0_1", "node_0_2", ... }),
//   "realm_min": 0,
//   "prereq_chapter": null,
//   "chapter_reward": ([ "exp": 300, "coin": 30, "item": "...", "title": "初入修仙" ]),
// ])

protected mapping quest_chapters = ([]);   // chapter_index → chapter_data
protected mapping quest_nodes   = ([]);    // node_id → node_data

// ── 公开接口声明 ──────────────────────────────────
int    start_quest(object player);
int    accept_node(object player, string node_id);
int    complete_node(object player, string node_id);
string query_progress(object player);
int    query_current_chapter(object player);
string query_current_node_id(object player);
int    get_chapter_node_count(int chapter);
string *get_chapter_node_ids(int chapter);
int    is_node_completed(object player, string node_id);
int    is_chapter_completed(object player, int chapter);
int    is_chapter_unlocked(object player, int chapter);
int    award_chapter_reward(object player, int chapter);
int    award_node_reward(object player, string node_id);
mixed  query_chapter_info(int chapter);
mixed  query_node_info(string node_id);
int    get_player_realm_index(object player);

// ── 初始化 ──────────────────────────────────────────
void create()
{
    seteuid(getuid());
    set("channel_id", "主线任务精灵");
    set("name", "主线任务系统");

    initialize_chapters();
    initialize_nodes();

    CHANNEL_D->do_channel(this_object(), "sys",
        "主线任务系统启动完毕。");
}

// ── 章节定义 ────────────────────────────────────────
void initialize_chapters()
{
    // 凡人篇（第零章）
    quest_chapters[CHAPTER_MORTAL] = ([
        "chapter":       CHAPTER_MORTAL,
        "name":          CHAPTER_0_NAME,
        "node_ids":      ({ "node_0_1", "node_0_2", "node_0_3", "node_0_4" }),
        "realm_min":     CHAPTER_0_MIN_REALM,
        "prereq_chapter": -1,
        "chapter_reward": ([
            "exp":      CHAPTER_0_BASE * CHAPTER_MULTIPLIER,
            "coin":     CHAPTER_0_BASE / 2,
            "title":    "初入修仙",
            "item":     "/clone/pill/huang_long_dan",
        ]),
    ]);

    // 越国篇（第一章）
    quest_chapters[CHAPTER_YUE] = ([
        "chapter":       CHAPTER_YUE,
        "name":          CHAPTER_1_NAME,
        "node_ids":      ({ "node_1_1", "node_1_2", "node_1_3", "node_1_4" }),
        "realm_min":     CHAPTER_1_MIN_REALM,
        "prereq_chapter": CHAPTER_MORTAL,
        "chapter_reward": ([
            "exp":      CHAPTER_1_BASE * CHAPTER_MULTIPLIER,
            "coin":     CHAPTER_1_BASE / 2,
            "title":    "越国风云",
            "item":     "/clone/pill/zhu_ji_dan",
        ]),
    ]);

    // 乱星海篇（第二章）
    quest_chapters[CHAPTER_LUANXINGHAI] = ([
        "chapter":       CHAPTER_LUANXINGHAI,
        "name":          CHAPTER_2_NAME,
        "node_ids":      ({ "node_2_1", "node_2_2", "node_2_3" }),
        "realm_min":     CHAPTER_2_MIN_REALM,
        "prereq_chapter": CHAPTER_YUE,
        "chapter_reward": ([
            "exp":      CHAPTER_2_BASE * CHAPTER_MULTIPLIER,
            "coin":     CHAPTER_2_BASE / 2,
            "title":    "星海霸主",
            "item":     "/clone/pill/bu_tian_dan",
        ]),
    ]);

    // 灵界篇（第三章）
    quest_chapters[CHAPTER_LINGJIE] = ([
        "chapter":       CHAPTER_LINGJIE,
        "name":          CHAPTER_3_NAME,
        "node_ids":      ({ "node_3_1", "node_3_2", "node_3_3" }),
        "realm_min":     CHAPTER_3_MIN_REALM,
        "prereq_chapter": CHAPTER_LUANXINGHAI,
        "chapter_reward": ([
            "exp":      CHAPTER_3_BASE * CHAPTER_MULTIPLIER,
            "coin":     CHAPTER_3_BASE / 2,
            "title":    "灵界传奇",
            "item":     "/clone/pill/tong_tian_ling_bao",
        ]),
    ]);

    // 飞升篇（终章）
    quest_chapters[CHAPTER_FEISHENG] = ([
        "chapter":       CHAPTER_FEISHENG,
        "name":          CHAPTER_4_NAME,
        "node_ids":      ({ "node_4_1", "node_4_2" }),
        "realm_min":     CHAPTER_4_MIN_REALM,
        "prereq_chapter": CHAPTER_LINGJIE,
        "chapter_reward": ([
            "exp":      CHAPTER_4_BASE * CHAPTER_MULTIPLIER,
            "coin":     CHAPTER_4_BASE / 2,
            "title":    "飞升上界",
        ]),
    ]);
}

// ── 节点定义 ────────────────────────────────────────
void initialize_nodes()
{
    // ═══════════════════════════════════════════════
    // 凡人篇（第零章）
    // ═══════════════════════════════════════════════

    // node_0_1: 离开家乡
    quest_nodes["node_0_1"] = ([
        "id":           "node_0_1",
        "chapter":      CHAPTER_MORTAL,
        "index":        1,
        "name":         "离开家乡",
        "desc":         "你出生于越国镜州青牛镇的一个小山村。家中贫困，不足以支撑生计。听闻江湖上有仙人踪迹，你决定离开家乡，前往七玄门拜师学艺。",
        "chapter_name": CHAPTER_0_NAME,
        "prereq_node":  0,            // 无前置
        "realm_min":    REALM_MORTAL,
        "rewards":      ([ "exp": 100, "coin": 10 ]),
    ]);

    // node_0_2: 拜入七玄门
    quest_nodes["node_0_2"] = ([
        "id":           "node_0_2",
        "chapter":      CHAPTER_MORTAL,
        "index":        2,
        "name":         "拜入七玄门",
        "desc":         "经过一路跋涉，你来到七玄门。得知七玄门是越国七大修仙门派之一，门主墨大夫法力高深。你需通过入门考核，方可成为七玄门弟子。",
        "chapter_name": CHAPTER_0_NAME,
        "prereq_node":  "node_0_1",
        "realm_min":    REALM_MORTAL,
        "rewards":      ([ "exp": 200, "coin": 20 ]),
    ]);

    // node_0_3: 练气入门
    quest_nodes["node_0_3"] = ([
        "id":           "node_0_3",
        "chapter":      CHAPTER_MORTAL,
        "index":        3,
        "name":         "练气入门",
        "desc":         "拜入七玄门后，墨大夫传授你《长春功》心法。你需要勤加修炼，感悟天地灵气，达到炼气入门境界。",
        "chapter_name": CHAPTER_0_NAME,
        "prereq_node":  "node_0_2",
        "realm_min":    REALM_QI_INIT,
        "rewards":      ([ "exp": 300, "coin": 30 ]),
    ]);

    // node_0_4: 离别七玄门
    quest_nodes["node_0_4"] = ([
        "id":           "node_0_4",
        "chapter":      CHAPTER_MORTAL,
        "index":        4,
        "name":         "离别七玄门",
        "desc":         "在七玄门修行已有小成。墨大夫告知你，修行之路漫漫，需走出山门，前往越国各大修仙门派历练。临行前，墨大夫赠你一本秘籍和些许盘缠。",
        "chapter_name": CHAPTER_0_NAME,
        "prereq_node":  "node_0_3",
        "realm_min":    REALM_QI_INIT,
        "rewards":      ([ "exp": 500, "coin": 50 ]),
    ]);

    // ═══════════════════════════════════════════════
    // 越国篇（第一章）
    // ═══════════════════════════════════════════════

    // node_1_1: 拜入宗门
    quest_nodes["node_1_1"] = ([
        "id":           "node_1_1",
        "chapter":      CHAPTER_YUE,
        "index":        1,
        "name":         "拜入宗门",
        "desc":         "你来到越国修仙界，需要在黄枫谷、掩月宗、灵兽山等七派中选择一个宗门拜入。不同的门派有不同的功法和特色，选择将影响你的修仙之路。",
        "chapter_name": CHAPTER_1_NAME,
        "prereq_node":  "node_0_4",
        "realm_min":    CHAPTER_1_MIN_REALM,
        "rewards":      ([ "exp": 1000, "coin": 100 ]),
    ]);

    // node_1_2: 血色禁地
    quest_nodes["node_1_2"] = ([
        "id":           "node_1_2",
        "chapter":      CHAPTER_YUE,
        "index":        2,
        "name":         "血色禁地",
        "desc":         "越国修仙界每六十年开启一次的血色禁地即将开启。各大门派的弟子将进入禁地争夺资源。你需要进入血色禁地，寻找筑基机缘。",
        "chapter_name": CHAPTER_1_NAME,
        "prereq_node":  "node_1_1",
        "realm_min":    CHAPTER_1_MIN_REALM,
        "rewards":      ([ "exp": 3000, "coin": 300 ]),
    ]);

    // node_1_3: 筑基试炼
    quest_nodes["node_1_3"] = ([
        "id":           "node_1_3",
        "chapter":      CHAPTER_YUE,
        "index":        3,
        "name":         "筑基试炼",
        "desc":         "在血色禁地中获得筑基丹后，你需要寻找一处灵气充沛之地，闭关冲击筑基期。筑基成功才算真正踏入修仙门槛。",
        "chapter_name": CHAPTER_1_NAME,
        "prereq_node":  "node_1_2",
        "realm_min":    REALM_QI_7,
        "rewards":      ([ "exp": 5000, "coin": 500 ]),
    ]);

    // node_1_4: 越国终章
    quest_nodes["node_1_4"] = ([
        "id":           "node_1_4",
        "chapter":      CHAPTER_YUE,
        "index":        4,
        "name":         "越国终章",
        "desc":         "筑基有成后，你发现越国修仙界只是冰山一角。传闻乱星海有更广阔的天地，那里有更多的天材地宝和更强的对手。你决定前往乱星海。",
        "chapter_name": CHAPTER_1_NAME,
        "prereq_node":  "node_1_3",
        "realm_min":    REALM_QI_7,
        "rewards":      ([ "exp": 8000, "coin": 800 ]),
    ]);

    // ═══════════════════════════════════════════════
    // 乱星海篇（第二章）
    // ═══════════════════════════════════════════════

    // node_2_1: 初临星海
    quest_nodes["node_2_1"] = ([
        "id":           "node_2_1",
        "chapter":      CHAPTER_LUANXINGHAI,
        "index":        1,
        "name":         "初临星海",
        "desc":         "通过跨界传送阵，你来到乱星海。这里岛屿星罗棋布，修仙势力错综复杂。你需要在天星城落脚，了解这里的规则。",
        "chapter_name": CHAPTER_2_NAME,
        "prereq_node":  "node_1_4",
        "realm_min":    CHAPTER_2_MIN_REALM,
        "rewards":      ([ "exp": 10000, "coin": 1000 ]),
    ]);

    // node_2_2: 虚天殿探秘
    quest_nodes["node_2_2"] = ([
        "id":           "node_2_2",
        "chapter":      CHAPTER_LUANXINGHAI,
        "index":        2,
        "name":         "虚天殿探秘",
        "desc":         "乱星海传说中的虚天殿开启，据说殿中有无数珍宝和突破元婴的机缘。你需要凑齐虚天残图，进入虚天殿寻找机缘。",
        "chapter_name": CHAPTER_2_NAME,
        "prereq_node":  "node_2_1",
        "realm_min":    CHAPTER_2_MIN_REALM + 5,
        "rewards":      ([ "exp": 30000, "coin": 3000 ]),
    ]);

    // node_2_3: 乱星海终章
    quest_nodes["node_2_3"] = ([
        "id":           "node_2_3",
        "chapter":      CHAPTER_LUANXINGHAI,
        "index":        3,
        "name":         "乱星海终章",
        "desc":         "元婴大成后，你感到乱星海已经无法满足你的修行需求。传闻灵界才是更高层次修士的舞台，你决定寻找前往灵界的途径。",
        "chapter_name": CHAPTER_2_NAME,
        "prereq_node":  "node_2_2",
        "realm_min":    CHAPTER_2_MIN_REALM + 10,
        "rewards":      ([ "exp": 80000, "coin": 8000 ]),
    ]);

    // ═══════════════════════════════════════════════
    // 灵界篇（第三章）
    // ═══════════════════════════════════════════════

    // node_3_1: 灵界初探
    quest_nodes["node_3_1"] = ([
        "id":           "node_3_1",
        "chapter":      CHAPTER_LINGJIE,
        "index":        1,
        "name":         "灵界初探",
        "desc":         "你以化神之姿降临灵界。灵界广袤无垠，种族林立。你需要在这片强者为尊的世界中找到立足之地。",
        "chapter_name": CHAPTER_3_NAME,
        "prereq_node":  "node_2_3",
        "realm_min":    CHAPTER_3_MIN_REALM,
        "rewards":      ([ "exp": 500000, "coin": 50000 ]),
    ]);

    // node_3_2: 地渊历练
    quest_nodes["node_3_2"] = ([
        "id":           "node_3_2",
        "chapter":      CHAPTER_LINGJIE,
        "index":        2,
        "name":         "地渊历练",
        "desc":         "灵界有三处秘境：地渊、广寒界、魔金山脉。你需要深入地渊，寻找大乘突破的契机。",
        "chapter_name": CHAPTER_3_NAME,
        "prereq_node":  "node_3_1",
        "realm_min":    CHAPTER_3_MIN_REALM + 5,
        "rewards":      ([ "exp": 2000000, "coin": 100000 ]),
    ]);

    // node_3_3: 灵界终章
    quest_nodes["node_3_3"] = ([
        "id":           "node_3_3",
        "chapter":      CHAPTER_LINGJIE,
        "index":        3,
        "name":         "灵界终章",
        "desc":         "大乘圆满后，你感知到天劫将至。渡过天劫便可飞升真仙界。你需要准备渡劫法宝和丹药，迎接飞升天劫。",
        "chapter_name": CHAPTER_3_NAME,
        "prereq_node":  "node_3_2",
        "realm_min":    REALM_DACHENG,
        "rewards":      ([ "exp": 5000000, "coin": 200000 ]),
    ]);

    // ═══════════════════════════════════════════════
    // 飞升篇（终章）
    // ═══════════════════════════════════════════════

    // node_4_1: 渡飞升劫
    quest_nodes["node_4_1"] = ([
        "id":           "node_4_1",
        "chapter":      CHAPTER_FEISHENG,
        "index":        1,
        "name":         "渡飞升劫",
        "desc":         "大乘圆满之后，飞升天劫降临。你需要以毕生修为对抗天劫，渡过三灾九难，方可飞升真仙界。",
        "chapter_name": CHAPTER_4_NAME,
        "prereq_node":  "node_3_3",
        "realm_min":    CHAPTER_4_MIN_REALM,
        "rewards":      ([ "exp": 10000000, "coin": 1000000 ]),
    ]);

    // node_4_2: 飞升上界
    quest_nodes["node_4_2"] = ([
        "id":           "node_4_2",
        "chapter":      CHAPTER_FEISHENG,
        "index":        2,
        "name":         "飞升上界",
        "desc":         "成功渡过天劫，你破碎虚空，飞升真仙界。北寒仙域的入口已在眼前，一段全新的旅程即将开始。恭喜你完成了凡人到真仙的蜕变！",
        "chapter_name": CHAPTER_4_NAME,
        "prereq_node":  "node_4_1",
        "realm_min":    CHAPTER_4_MIN_REALM,
        "rewards":      ([ "exp": 50000000, "coin": 5000000 ]),
        "is_final":     1,
    ]);
}

// ═══════════════════════════════════════════════════
// 核心 API
// ═══════════════════════════════════════════════════

// 开始（或继续）主线任务
// 检查玩家当前进度，自动推进到下一个可接取节点
// 返回：1=新节点已激活  0=无可用新节点  -1=参数错误
int start_quest(object player)
{
    int chapter, realm;
    string node_id;

    if (!player) return -1;

    // 检查玩家是否已全部完成
    if (player->query(MQ_KEY_STATUS) == MQ_COMPLETED)
        return 0;

    // 初始化状态
    if (player->query(MQ_KEY_STATUS) == MQ_INACTIVE)
    {
        player->set(MQ_KEY_STATUS, MQ_ACTIVE);
        player->set(MQ_KEY_CHAPTER, CHAPTER_MORTAL);
        player->set(MQ_KEY_COMP_NODES, ({ }));
        player->set(MQ_KEY_COMP_CHAPTERS, ({ }));
    }

    chapter = player->query(MQ_KEY_CHAPTER);
    node_id = player->query(MQ_KEY_NODE);

    // 已有活跃节点
    if (stringp(node_id) && node_id != "")
        return 0;

    // 查找下一个可接取节点
    node_id = find_next_available_node(player, chapter);
    if (node_id && node_id != "")
    {
        player->set(MQ_KEY_NODE, node_id);
        return 1;
    }

    // 检查是否所有章节都完成
    if (chapter >= CHAPTER_COUNT - 1)
    {
        player->set(MQ_KEY_STATUS, MQ_COMPLETED);
        return 0;
    }

    return 0;
}

// 接取指定节点
// 返回：1=成功  0=不可接取  -1=参数错误
int accept_node(object player, string node_id)
{
    mapping node, chapter_data;
    int realm;

    if (!player || !stringp(node_id) || node_id == "")
        return -1;

    if (undefinedp(quest_nodes[node_id]))
        return 0;

    node = quest_nodes[node_id];

    // 检查是否已接取或已完成
    if (player->query(MQ_KEY_NODE) == node_id)
        return 0;

    if (is_node_completed(player, node_id))
        return 0;

    // 检查前置节点
    if (node["prereq_node"] && node["prereq_node"] != 0)
    {
        if (!is_node_completed(player, node["prereq_node"]))
            return 0;
    }

    // 检查境界要求
    realm = get_player_realm_index(player);
    if (realm < node["realm_min"])
        return 0;

    // 检查章节解锁
    chapter_data = quest_chapters[node["chapter"]];
    if (!chapter_data)
        return 0;

    if (!is_chapter_unlocked(player, node["chapter"]))
        return 0;

    // 设置当前节点
    player->set(MQ_KEY_CHAPTER, node["chapter"]);
    player->set(MQ_KEY_NODE, node_id);
    if (player->query(MQ_KEY_STATUS) == MQ_INACTIVE)
        player->set(MQ_KEY_STATUS, MQ_ACTIVE);

    return 1;
}

// 完成指定节点
// 检查节点状态，发放奖励，自动推送到下一节点
// 返回：1=完成成功（含下一节点已激活） 2=完成且章节完成
//       3=全部主线完成  0=条件不满足  -1=参数错误
int complete_node(object player, string node_id)
{
    mapping node;
    string next_id;
    int chapter, next_chapter, result, realm;

    if (!player || !stringp(node_id) || node_id == "")
        return -1;

    if (undefinedp(quest_nodes[node_id]))
        return 0;

    node = quest_nodes[node_id];

    // 检查是否已激活
    if (player->query(MQ_KEY_NODE) != node_id)
        return 0;

    // 检查境界
    realm = get_player_realm_index(player);
    if (realm < node["realm_min"])
        return 0;

    // 发放节点奖励
    award_node_reward(player, node_id);

    // 标记节点完成
    add_completed_node(player, node_id);
    player->set(MQ_KEY_NODE, "");

    // 检查章节是否完成
    chapter = node["chapter"];
    if (check_chapter_complete(player, chapter))
    {
        // 发放章节奖励
        award_chapter_reward(player, chapter);
        add_completed_chapter(player, chapter);

        // 检查是否可以进入下一章
        next_chapter = chapter + 1;
        if (next_chapter < CHAPTER_COUNT &&
            is_chapter_unlocked(player, next_chapter))
        {
            player->set(MQ_KEY_CHAPTER, next_chapter);
        }
        else if (next_chapter >= CHAPTER_COUNT)
        {
            // 所有章节完成
            player->set(MQ_KEY_STATUS, MQ_COMPLETED);
            return 3;
        }

        result = 2;
    }
    else
    {
        result = 1;
    }

    // 自动激活下一节点
    next_id = find_next_in_chapter(player, chapter);
    if (next_id && next_id != "")
    {
        player->set(MQ_KEY_NODE, next_id);
    }

    return result;
}

// 查询玩家主线进度摘要
// 返回格式化的进度字符串
string query_progress(object player)
{
    string output, node_id, status_str;
    int chapter, status;
    mapping node_data, chapter_data;
    string *completed_nodes;
    int *completed_chapters;

    if (!player) return "";

    status = player->query(MQ_KEY_STATUS);
    if (status == MQ_INACTIVE)
        return "你还未开始主线任务。\n";

    output = HIC "╔══════════════════════════════════╗\n" NOR;
    output += HIC "║       主 线 任 务 进 度         ║\n" NOR;
    output += HIC "╚══════════════════════════════════╝\n" NOR;

    completed_chapters = player->query(MQ_KEY_COMP_CHAPTERS);
    if (!pointerp(completed_chapters))
        completed_chapters = ({});

    for (int ch = 0; ch < CHAPTER_COUNT; ch++)
    {
        chapter_data = quest_chapters[ch];
        if (!chapter_data) continue;

        if (member_array(ch, completed_chapters) != -1)
        {
            output += sprintf(" " HIG "■" NOR " %s " HIG "(已完成)" NOR "\n",
                       chapter_data["name"]);
        }
        else if (is_chapter_unlocked(player, ch))
        {
            output += sprintf(" " HIY "▶" NOR " %s " HIY "(进行中)" NOR "\n",
                       chapter_data["name"]);
        }
        else
        {
            output += sprintf(" " HIB "□" NOR " %s " HIB "(未解锁)" NOR "\n",
                       chapter_data["name"]);
        }
    }

    output += "\n";
    node_id = player->query(MQ_KEY_NODE);
    if (stringp(node_id) && node_id != "" && !undefinedp(quest_nodes[node_id]))
    {
        node_data = quest_nodes[node_id];
        output += sprintf("当前任务：" HIW "%s" NOR "\n", node_data["name"]);
        output += sprintf("任务说明：%s\n", node_data["desc"]);
        output += sprintf("任务奖励：经验 %d  灵石 %d\n",
                   node_data["rewards"]["exp"],
                   node_data["rewards"]["coin"]);
    }
    else
    {
        // 找下一个可用节点
        node_id = find_next_available_node(player, player->query(MQ_KEY_CHAPTER));
        if (stringp(node_id) && node_id != "" && !undefinedp(quest_nodes[node_id]))
        {
            node_data = quest_nodes[node_id];
            output += sprintf("可接任务：" HIW "%s" NOR "\n", node_data["name"]);
            output += "输入 " HIG "main_quest accept" NOR " 接取。\n";
        }
        else
        {
            if (status == MQ_COMPLETED)
                output += HIG "全部主线任务已完成！恭喜你飞升上界！" NOR "\n";
            else
                output += "当前无可接取的主线任务。请提升境界后重试。\n";
        }
    }

    return output;
}

// ═══════════════════════════════════════════════════
// 辅助方法
// ═══════════════════════════════════════════════════

// 获取玩家境界索引（简化实现）
// 实际实现应查询 player->query("level") 或等效字段
// TODO: 对接实际境界系统后替换此方法
int get_player_realm_index(object player)
{
    int level;

    if (!player) return 0;

    // 尝试从 player 对象获取境界/等级
    // 兼容不同属性名
    level = player->query("level");
    if (level <= 0)
        level = player->query("combat_exp") / 10000;

    if (level < 0) level = 0;
    if (level > 100) level = 100;

    return level;
}

// 检查章节是否已解锁
int is_chapter_unlocked(object player, int chapter)
{
    mapping ch_data;
    int *completed_chapters;

    if (chapter < 0 || chapter >= CHAPTER_COUNT)
        return 0;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;

    // 检查境界
    if (get_player_realm_index(player) < ch_data["realm_min"])
        return 0;

    // 检查前置章节
    if (ch_data["prereq_chapter"] < 0)
        return 1;  // 第一章无条件

    completed_chapters = player->query(MQ_KEY_COMP_CHAPTERS);
    if (!pointerp(completed_chapters))
        return 0;

    return (member_array(ch_data["prereq_chapter"], completed_chapters) != -1);
}

// 检查章节内所有节点是否完成
int check_chapter_complete(object player, int chapter)
{
    string *node_ids;
    mapping ch_data;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;

    node_ids = ch_data["node_ids"];
    foreach (string nid in node_ids)
    {
        if (!is_node_completed(player, nid))
            return 0;
    }

    return 1;
}

// 检查节点是否已完成
int is_node_completed(object player, string node_id)
{
    string *completed;

    completed = player->query(MQ_KEY_COMP_NODES);
    if (!pointerp(completed)) return 0;

    return (member_array(node_id, completed) != -1);
}

// 检查章节是否已完成
int is_chapter_completed(object player, int chapter)
{
    int *completed;

    completed = player->query(MQ_KEY_COMP_CHAPTERS);
    if (!pointerp(completed)) return 0;

    return (member_array(chapter, completed) != -1);
}

// 在指定章节中找下一个未完成的节点
string find_next_in_chapter(object player, int chapter)
{
    mapping ch_data;
    string *node_ids;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return "";

    node_ids = ch_data["node_ids"];
    foreach (string nid in node_ids)
    {
        if (!is_node_completed(player, nid))
        {
            // 检查前置
            mapping nd = quest_nodes[nid];
            if (nd && nd["prereq_node"] && nd["prereq_node"] != 0)
            {
                if (!is_node_completed(player, nd["prereq_node"]))
                    continue;
            }
            return nid;
        }
    }

    return "";
}

// 查找下一个可用节点（跨章节）
string find_next_available_node(object player, int start_chapter)
{
    mapping ch_data;
    string *node_ids;

    for (int ch = start_chapter; ch < CHAPTER_COUNT; ch++)
    {
        if (!is_chapter_unlocked(player, ch))
            continue;

        ch_data = quest_chapters[ch];
        if (!ch_data) continue;

        node_ids = ch_data["node_ids"];
        foreach (string nid in node_ids)
        {
            if (is_node_completed(player, nid))
                continue;

            mapping nd = quest_nodes[nid];
            if (!nd) continue;

            // 检查前置
            if (nd["prereq_node"] && nd["prereq_node"] != 0)
            {
                if (!is_node_completed(player, nd["prereq_node"]))
                    continue;
            }

            // 检查境界
            if (get_player_realm_index(player) < nd["realm_min"])
                continue;

            return nid;
        }
    }

    return "";
}

// 将节点 ID 加入已完成列表
void add_completed_node(object player, string node_id)
{
    string *completed;

    completed = player->query(MQ_KEY_COMP_NODES);
    if (!pointerp(completed))
        completed = ({});

    if (member_array(node_id, completed) == -1)
        completed += ({ node_id });

    player->set(MQ_KEY_COMP_NODES, completed);
}

// 将章节加入已完成列表
void add_completed_chapter(object player, int chapter)
{
    int *completed;

    completed = player->query(MQ_KEY_COMP_CHAPTERS);
    if (!pointerp(completed))
        completed = ({});

    if (member_array(chapter, completed) == -1)
        completed += ({ chapter });

    player->set(MQ_KEY_COMP_CHAPTERS, completed);
}

// 发放节点奖励
int award_node_reward(object player, string node_id)
{
    mapping node, rewards;

    if (undefinedp(quest_nodes[node_id]))
        return 0;

    node = quest_nodes[node_id];
    rewards = node["rewards"];

    // 发放经验
    if (rewards["exp"] > 0)
        player->add("combat_exp", rewards["exp"]);

    // 发放灵石
    if (rewards["coin"] > 0)
    {
        // 通过经济系统发放（简化：直接给钱）
        object coin;
        coin = new ("/clone/money/coin");
        if (coin)
        {
            coin->set_amount(rewards["coin"]);
            coin->move(player);
        }
    }

    tell_object(player, sprintf(
        HIG "主线任务【%s】完成！获得经验 %d，灵石 %d。\n" NOR,
        node["name"], rewards["exp"], rewards["coin"]));

    return 1;
}

// 发放章节完成奖励
int award_chapter_reward(object player, int chapter)
{
    mapping ch_data, reward;
    string title_name;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;

    reward = ch_data["chapter_reward"];

    // 发放经验奖励
    if (reward["exp"] > 0)
        player->add("combat_exp", reward["exp"]);

    // 发放灵石奖励
    if (reward["coin"] > 0)
    {
        object coin;
        coin = new ("/clone/money/coin");
        if (coin)
        {
            coin->set_amount(reward["coin"]);
            coin->move(player);
        }
    }

    // 发放称号
    title_name = reward["title"];
    if (stringp(title_name) && title_name != "")
    {
        player->set("title", title_name);
    }

    // 发放物品
    if (stringp(reward["item"]) && reward["item"] != "")
    {
        object item;
        item = new (reward["item"]);
        if (item)
        {
            item->move(player);
            tell_object(player, sprintf(
                "获得特殊物品：%s。\n", item->query("name")));
        }
    }

    tell_object(player, sprintf(
        HIC "\n═══════════════════════════════\n" NOR
        HIC "★ 章节完成！%s ★\n" NOR
        "  获得称号：%s\n"
        "  经验 +%d，灵石 +%d\n"
        HIC "═══════════════════════════════\n" NOR,
        ch_data["name"], title_name,
        reward["exp"], reward["coin"]));

    return 1;
}

// 查询章节信息
mixed query_chapter_info(int chapter)
{
    if (undefinedp(quest_chapters[chapter]))
        return 0;

    return quest_chapters[chapter];
}

// 查询节点信息
mixed query_node_info(string node_id)
{
    if (undefinedp(quest_nodes[node_id]))
        return 0;

    return quest_nodes[node_id];
}

// 获取当前章节
int query_current_chapter(object player)
{
    if (!player) return -1;
    return player->query(MQ_KEY_CHAPTER);
}

// 获取当前节点 ID
string query_current_node_id(object player)
{
    if (!player) return "";
    return player->query(MQ_KEY_NODE);
}

// 获取章节节点数量
int get_chapter_node_count(int chapter)
{
    mapping ch_data;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;

    return sizeof(ch_data["node_ids"]);
}

// 获取章节节点 ID 列表
string *get_chapter_node_ids(int chapter)
{
    mapping ch_data;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return ({});

    return ch_data["node_ids"];
}

// 防止主动 destruct
int clean_up()
{
    return 1;
}
