// main_quest_d.c
// 主线任务守护进程 —— 5 章主线框架 + 第零章凡人篇 + 第一章越国篇落地
//
// 职责（#65 重写，路径②直接实施）：
//   1. 定义主线任务模板（第零章 4 节点 + 第一章越国篇 13 节点，按 1G §二 取材）
//   2. 注册到 QUEST_CHAIN_D（#59 任务链框架）：任务模板 + 串行链
//   3. 玩家侧接口：接取 / 目标推进 / 完成 / 进度查询（main_quest.c 调用）
//
// 设计依据:
//   .knowledge/quests/1G-任务副本奇遇.md（§二 主线任务链：第零章/第一章节点表）
//   02-扩充内容/02-任务链与奖励曲线.md（§2.1 章节结构 / §2.2 跨章境界门槛 / §2.3 奖励曲线）
//
// 奖励接入（c4，复用 #59/#57 接线，走 grant_quest_rewards 六渠道）：
//   修为经验 → combat_exp（主线难度系数 2.5，实际发放 = 模板值 × 2.5 × 链长加成）
//   灵石     → MONEY_D->pay_player（1 灵石 = 100 文）
//   声望     → REPUTATION_D->add_reputation（黄枫谷/掩月宗声望）
//   门派贡献 → SECT_D->add_contribution（需已入宗）
//   物品     → new() + move(player)（真实物品路径：baicao-dan/lingzhi）
//   功法     → 本门功法写 sect/learned 免贡献解锁（grant_skill，黄枫谷青元剑诀）
//
// 境界门槛（c3，复用 quest_chain_d 统一境界索引）：
//   0=炼气 1=筑基 2=结丹 3=元婴 4=化神 5=炼虚 6=合体 7=大乘
//   任务 realm_range 即境界门槛；跨章解锁（第零章→第一章）由章节 realm_min 承担
//   注：索引粒度只到大境界（炼气7层/13层细节在 desc 提示），与 #57 exp_to_tier 同源边界
//
// 场景挂接（c3）：OBJ_REACH 目标用 #67 越国世俗区 + #58 九宗驻地真实房间路径

#include <ansi.h>
#include <quest_chain.h>
#include <main_quest.h>
#include <globals.h>

inherit F_DBASE;

// ─── 主线任务链定义 ───
// 字段对齐 quest_chain_d.register_quest 模板格式；type=QUEST_TYPE_MAIN 一次性不可回流
// chain_id: chain_main_0（第零章）/ chain_main_1（第一章）
// realm_range: 统一境界索引 ({min_idx, max_idx})
// objectives: OBJ_REACH 到达真实房间（OBJ_TALK 与 OBJ_REACH 同按位置判定，见 quest_progress）
nosave mapping quest_defs = ([
    // ═══════ 第零章·凡人篇（1G §二 0.1-0.11 主干压缩为 4 节点） ═══════
    "mq_0_1": ([
        "id": "mq_0_1", "chain_id": "chain_main_0",
        "chapter":     CHAPTER_MORTAL,
        "name": "离开家乡", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qingniu/zhenkou", "amount": 1 ]) }),
        "rewards": ([ "exp": 100, "coin": 10 ]),
        "description": "你出生于越国镜州青牛镇的一个小山村。家中贫困，听闻江湖上有仙人踪迹，你决心离开家乡，前往七玄门拜师学艺。前往青牛镇镇口，踏上修仙之路。",
    ]),
    "mq_0_2": ([
        "id": "mq_0_2", "chain_id": "chain_main_0",
        "chapter":     CHAPTER_MORTAL,
        "name": "拜入七玄门", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_0_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qixuanmen/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 150, "coin": 15 ]),
        "description": "你来到七玄门彩霞山。七玄门是越国世俗武林大派，门主与神手谷谷主墨大夫皆是当世高手。通过入门考核，方可成为七玄门弟子。",
    ]),
    "mq_0_3": ([
        "id": "mq_0_3", "chain_id": "chain_main_0",
        "chapter":     CHAPTER_MORTAL,
        "name": "练气入门", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_0_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qixuanmen/shenshougu", "amount": 1 ]) }),
        "rewards": ([ "exp": 200, "coin": 20 ]),
        "description": "墨大夫收你为记名弟子，传授你《长春功》心法。前往神手谷墨大夫静室，勤加修炼，感悟天地灵气，踏入炼气期。",
    ]),
    "mq_0_4": ([
        "id": "mq_0_4", "chain_id": "chain_main_0",
        "chapter":     CHAPTER_MORTAL,
        "name": "离别七玄门", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_0_3" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qixuanmen/caixiashan", "amount": 1 ]) }),
        "rewards": ([ "exp": 300, "coin": 30 ]),
        "description": "在七玄门修行已有小成。墨大夫告知你修行之路漫漫，需走出山门前往越国修仙界历练。临行前他赠你些许盘缠，你从彩霞山下山，踏上新的旅程。",
    ]),

    // ═══════ 第一章·越国篇（1G §二 1.1-1.13 完整 13 节点） ═══════
    "mq_1_1": ([
        "id": "mq_1_1", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "嘉元城初到", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_0_4" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/jiayuan/dajie", "amount": 1 ]) }),
        "rewards": ([ "exp": 600, "coin": 100 ]),
        "description": "你抵达岚州嘉元城（炼气七层以上方宜前往）。这座世俗大城人流如织，墨家府邸门庭若市，城中也偶有修士出没。先在大街上打探修仙界消息，寻找太南谷坊市的线索。",
    ]),
    "mq_1_2": ([
        "id": "mq_1_2", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "墨府恩怨", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_1_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/jiayuan/mofu", "amount": 1 ]) }),
        "rewards": ([ "exp": 800, "coin": 120 ]),
        "description": "墨府是嘉元城第一修仙家族，家主墨凤舞修为高深。你应约前往墨府，助其清理府中叛徒，了结一场恩怨。墨府玉佩与萦香丸配方将是你在此城立足的依仗。",
    ]),
    "mq_1_3": ([
        "id": "mq_1_3", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "太南小会", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_1_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/tainan/fangshi", "amount": 1 ]) }),
        "rewards": ([ "exp": 1000, "coin": 150 ]),
        "description": "太南谷是越国修仙者的交易重地，每逢集会，散修云集（炼气九层以上方宜前往）。前往太南谷坊市，参加修仙者交易小会，用灵石兑换丹药法器。",
    ]),
    "mq_1_4": ([
        "id": "mq_1_4", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "灵根鉴定", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_1_3" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/tainan/tainansi", "amount": 1 ]) }),
        "rewards": ([ "exp": 1000, "coin": 150 ]),
        "description": "太南寺中住着一位精通灵根鉴定的青颜真人。请其为你鉴定灵根资质——灵根决定你的修仙潜力与拜师方向，务必认真对待。",
    ]),
    "mq_1_5": ([
        "id": "mq_1_5", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "升仙大会", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_1_4" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 1200, "coin": 200 ]),
        "description": "越国七派联合升仙大会在黄枫谷山门召开（炼气顶峰方有资格参与）。各派长老亲临选拔弟子，这是散修拜入名门正派的最佳机会。",
    ]),
    "mq_1_6": ([
        "id": "mq_1_6", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "拜入黄枫谷", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "mq_1_5" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 1500, "coin": 250,
                      "reputation": ({ ([ "faction": "huangfeng_valley", "value": 50 ]) }) ]),
        "description": "升仙大会上你表现出众，选择拜入黄枫谷（也可选择掩月宗、灵兽山等其余六派，主线以黄枫谷为默认分支）。前往黄枫谷大殿行拜师礼，成为黄枫谷弟子，正式踏入修仙门派。",
    ]),
    "mq_1_7": ([
        "id": "mq_1_7", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "百药园看守", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "mq_1_6" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/fac/baiyaoyuan", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 300, "contribution": 200,
                      "items": ({ "/clone/drug/lingzhi" }) ]),
        "description": "筑基初期的你被派往黄枫谷百药园看守灵药。在灵田劳作中学习灵药知识，获赐灵芝灵药。好好干，修仙路漫漫，根基要牢。",
    ]),
    "mq_1_8": ([
        "id": "mq_1_8", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "岳麓殿之行", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "mq_1_7" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/yuexudian", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 300,
                      "skills": ({ "qingyuan-jianjue" }) ]),
        "description": "岳麓殿藏经阁收藏黄枫谷历代功法与丹药配方，青元剑诀残本亦藏于此。你前往岳麓殿，获授青元剑诀残本（入黄枫谷后由本门功法渠道习得）。",
    ]),
    "mq_1_9": ([
        "id": "mq_1_9", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "血色禁地试炼", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "mq_1_8" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 3000, "coin": 400,
                      "items": ({ "/clone/drug/baicao-dan" }) ]),
        "description": "筑基中期，你获得进入血色禁地的资格。禁地每六十年开启一次，内部灵药遍地、妖兽成群，是筑基丹材料的主产地。前往黄枫谷山门集结，随队进入禁地采药历练。",
    ]),
    "mq_1_10": ([
        "id": "mq_1_10", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "正魔之战前夕", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "mq_1_9" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 3000, "coin": 400, "contribution": 300 ]),
        "description": "筑基后期，魔道六宗大举入侵越国，黄枫谷上下备战。你前往大殿听候掌门调遣，领取守山职责，为即将到来的大战积蓄力量。",
    ]),
    "mq_1_11": ([
        "id": "mq_1_11", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "乌龙潭之战", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "mq_1_10" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 3200, "coin": 500, "contribution": 500,
                      "reputation": ({ ([ "faction": "huangfeng_valley", "value": 80 ]) }) ]),
        "description": "正魔大战在乌龙潭爆发。你随黄枫谷弟子出征，在血战中斩敌立功，为宗门赢得战功，也为自己挣得赫赫声望。",
    ]),
    "mq_1_12": ([
        "id": "mq_1_12", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "掩月宗之行", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "mq_1_11" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/yanyue/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 3200, "coin": 500,
                      "reputation": ({ ([ "faction": "yanyue_sect", "value": 30 ]) }) ]),
        "description": "结丹初期的你前往掩月宗大殿，与故人南宫婉重逢。掩月宗乃越国第一大宗，你在此探得乱星海与天月神舟的情报，为远行做准备。",
    ]),
    "mq_1_13": ([
        "id": "mq_1_13", "chain_id": "chain_main_1",
        "chapter":     CHAPTER_YUE,
        "name": "越国终章", "type": QUEST_TYPE_MAIN, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "mq_1_12" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/transmit", "amount": 1 ]) }),
        "rewards": ([ "exp": 3200, "coin": 600 ]),
        "description": "越国正魔大战收尾，你也已成结丹修士。乱星海有更广阔的天地，传闻那里天材地宝无数。前往越国传送阵，决定前往乱星海，开启新的篇章。（可选择留越国做支线，随时可触发离去）",
    ]),
]);

// ─── 主线串行链（第零章 4 节点 / 第一章 13 节点） ───
nosave mapping chain_defs = ([
    "chain_main_0": ({ "mq_0_1", "mq_0_2", "mq_0_3", "mq_0_4" }),
    "chain_main_1": ({ "mq_1_1", "mq_1_2", "mq_1_3", "mq_1_4", "mq_1_5",
                       "mq_1_6", "mq_1_7", "mq_1_8", "mq_1_9", "mq_1_10",
                       "mq_1_11", "mq_1_12", "mq_1_13" }),
]);

// ─── 章节定义（5 章框架；第零章/第一章已落地，后续章另开子票） ───
// realm_min 用 quest_chain 统一境界索引（0=炼气 1=筑基 2=结丹 3=元婴 4=化神 5=炼虚 6=合体 7=大乘）
// 第零章→第一章跨章门槛：炼气≥7 层（索引 0，7 层细节在 desc）；第一章→第二章：结丹初期（索引 2）
nosave mapping quest_chapters = ([
    CHAPTER_MORTAL: ([
        "chapter": CHAPTER_MORTAL, "name": CHAPTER_0_NAME,
        "chain_id": "chain_main_0",
        "realm_min": 0, "prereq_chapter": -1,
        "chapter_reward": ([ "exp": CHAPTER_0_BASE * CHAPTER_MULTIPLIER,
                             "coin": CHAPTER_0_BASE / 2,
                             "title": "初入修仙",
                             "item": "/clone/drug/baicao-dan" ]),
    ]),
    CHAPTER_YUE: ([
        "chapter": CHAPTER_YUE, "name": CHAPTER_1_NAME,
        "chain_id": "chain_main_1",
        "realm_min": 0, "prereq_chapter": CHAPTER_MORTAL,
        "chapter_reward": ([ "exp": CHAPTER_1_BASE * CHAPTER_MULTIPLIER,
                             "coin": CHAPTER_1_BASE / 2,
                             "title": "越国风云",
                             "item": "/clone/drug/lingzhi" ]),
    ]),
    // 后续章框架占位（乱星海/灵界/飞升）——内容按 1G 分阶段另开子 ticket
    CHAPTER_LUANXINGHAI: ([
        "chapter": CHAPTER_LUANXINGHAI, "name": CHAPTER_2_NAME,
        "chain_id": "", "realm_min": 2, "prereq_chapter": CHAPTER_YUE,
        "chapter_reward": ([ ]),
    ]),
    CHAPTER_LINGJIE: ([
        "chapter": CHAPTER_LINGJIE, "name": CHAPTER_3_NAME,
        "chain_id": "", "realm_min": 4, "prereq_chapter": CHAPTER_LUANXINGHAI,
        "chapter_reward": ([ ]),
    ]),
    CHAPTER_FEISHENG: ([
        "chapter": CHAPTER_FEISHENG, "name": CHAPTER_4_NAME,
        "chain_id": "", "realm_min": 7, "prereq_chapter": CHAPTER_LINGJIE,
        "chapter_reward": ([ ]),
    ]),
]);

// ── 公开接口声明 ──────────────────────────────────
int    start_quest(object player);
int    accept_node(object player, string node_id);
int    complete_node(object player, string node_id);
string query_progress(object player);
string query_current_node_id(object player);
int    quest_progress(object player, string quest_id);
int    get_player_realm_index(object player);
int    is_chapter_completed(object player, int chapter);
int    is_chapter_unlocked(object player, int chapter);
mixed  query_chapter_info(int chapter);
mixed  query_node_info(string node_id);
string *get_chapter_node_ids(int chapter);

// ── 初始化 ──────────────────────────────────────────
void create()
{
    seteuid(ROOT_UID);
    set("channel_id", "主线任务精灵");
    set("name", "主线任务系统");

    register_all_quests();

    CHANNEL_D->do_channel(this_object(), "sys",
        "主线任务系统启动完毕。");
}

int clean_up()
{
    return 1;
}

// 注册主线任务到 QUEST_CHAIN_D（任务模板 + 串行链）
void register_all_quests()
{
    string *qids = keys(quest_defs);
    string *cids = keys(chain_defs);
    int i;

    for (i = 0; i < sizeof(qids); i++)
        QUEST_CHAIN_D->register_quest(quest_defs[qids[i]]);

    for (i = 0; i < sizeof(cids); i++)
        QUEST_CHAIN_D->register_chain(cids[i], CHAIN_SERIAL, chain_defs[cids[i]], ([]));
}

// ═══════════════════════════════════════════
// 核心 API
// ═══════════════════════════════════════════

// 开始（或继续）主线任务：接取下一个可用主线节点
// 返回：1=新节点已激活  0=无可用新节点  -1=参数错误
int start_quest(object player)
{
    string next;

    if (!player) return -1;

    next = find_next_available_quest(player);
    if (stringp(next) && next != "")
    {
        if (QUEST_CHAIN_D->assign_quest(player, next))
            return 1;
    }
    return 0;
}

// 接取指定主线节点（显式接取；自动接续失败时玩家手动 accept 当前节点）
// 返回：1=成功  0=不可接取  -1=参数错误
int accept_node(object player, string node_id)
{
    if (!player || !stringp(node_id) || node_id == "")
        return -1;

    if (undefinedp(quest_defs[node_id]))
        return 0;

    if (QUEST_CHAIN_D->assign_quest(player, node_id))
        return 1;

    return 0;
}

// 完成指定主线节点（目标达成检查 + 结算 + 章节完成检测）
// 返回：1=完成成功（含下一节点已激活） 2=完成且章节完成
//       3=全部主线完成  0=条件不满足  -1=参数错误
int complete_node(object player, string node_id)
{
    mapping ch_data;
    int chapter;

    if (!player || !stringp(node_id) || node_id == "")
        return -1;

    if (undefinedp(quest_defs[node_id]))
        return 0;

    // 目标推进检查（OBJ_REACH 按当前位置判定）
    if (!quest_progress(player, node_id))
        return 0;

    if (!QUEST_CHAIN_D->complete_quest(player, node_id))
        return 0;

    // 剧情入宗（c4 修复，审查第 2 轮）：mq_1_6「拜入黄枫谷」完成时自动入宗
    // ——默认分支（黄枫谷）剧情落地。仅当玩家未入宗时尝试；join_sect 自带
    // 条件校验（炼气三层/已入他派/叛门记录），不满足时拒绝并提示，不强行写入。
    // 入宗后贡献/功法奖励（mq_1_7/1_8/1_10/1_11）经 SECT_D 渠道真实可达。
    if (node_id == "mq_1_6")
    {
        if (!SECT_D->query_player_sect(player))
            SECT_D->join_sect(player, "huangfeng_valley");
    }

    // 章节完成检测
    chapter = quest_defs[node_id]["chapter"];
    if (is_chapter_completed(player, chapter))
    {
        // 发放章节奖励
        award_chapter_reward(player, chapter);

        // 章节完成：终章完成=全部完成；否则章节完成（后续章未落地则等待）
        if (chapter >= CHAPTER_COUNT - 1)
            return 3;
        return 2;
    }

    return 1;
}

// 查询玩家主线进度摘要
string query_progress(object player)
{
    string output, node_id;
    int chapter, status;
    mapping node_data, chapter_data;
    int i;

    if (!player) return "";

    output = HIC "╔══════════════════════════════════╗\n" NOR;
    output += HIC "║       主 线 任 务 进 度         ║\n" NOR;
    output += HIC "╚══════════════════════════════════╝\n" NOR;

    for (i = 0; i < CHAPTER_COUNT; i++)
    {
        chapter_data = quest_chapters[i];
        if (!chapter_data) continue;

        if (is_chapter_completed(player, i))
            output += sprintf(" " HIG "■" NOR " %s " HIG "(已完成)" NOR "\n",
                       chapter_data["name"]);
        else if (is_chapter_unlocked(player, i))
            output += sprintf(" " HIY "▶" NOR " %s " HIY "(进行中)" NOR "\n",
                       chapter_data["name"]);
        else
            output += sprintf(" " HIB "□" NOR " %s " HIB "(未解锁)" NOR "\n",
                       chapter_data["name"]);
    }

    output += "\n";
    node_id = query_current_node_id(player);
    if (stringp(node_id) && node_id != "" && !undefinedp(quest_defs[node_id]))
    {
        node_data = quest_defs[node_id];
        output += sprintf("当前任务：" HIW "%s" NOR "\n", node_data["name"]);
        output += sprintf("任务说明：%s\n", node_data["description"]);
        output += sprintf("任务奖励：经验 %d  灵石 %d\n",
                   node_data["rewards"]["exp"],
                   node_data["rewards"]["coin"]);
        output += sprintf("目标：" HIW "%s" NOR "\n", obj_target_text(node_data["objectives"]));
        output += "输入 " HIG "main_quest submit" NOR " 提交（需到达目标地点）。\n";
    }
    else
    {
        string next = find_next_available_quest(player);
        if (stringp(next) && next != "" && !undefinedp(quest_defs[next]))
        {
            node_data = quest_defs[next];
            output += sprintf("可接任务：" HIW "%s" NOR "\n", node_data["name"]);
            output += sprintf("任务说明：%s\n", node_data["description"]);
            output += sprintf("奖励：经验 %d  灵石 %d\n",
                       node_data["rewards"]["exp"],
                       node_data["rewards"]["coin"]);
            output += "输入 " HIG "main_quest accept" NOR " 接取。\n";
        }
        else
        {
            output += "当前无可接取的主线任务。请提升境界后重试。\n";
        }
    }

    return output;
}

// 获取当前活跃主线节点 ID
string query_current_node_id(object player)
{
    mapping active;
    string *ids;
    int i;

    if (!player) return "";

    active = QUEST_CHAIN_D->get_player_quests(player);
    if (!mapp(active)) return "";

    ids = keys(active);
    for (i = 0; i < sizeof(ids); i++)
    {
        if (quest_defs[ids[i]])
            return ids[i];
    }
    return "";
}

// 推进主线任务目标进度（OBJ_REACH/OBJ_TALK 按当前位置判定）
// 返回目标是否全部达成
// 注意：必须取整张活跃任务表 active 修改子表再整表写回（#59 铁律）
int quest_progress(object player, string quest_id)
{
    mapping active;
    mapping sub;
    mapping template;
    mapping objectives;
    mapping progress;
    string here;
    int i, done;

    if (!objectp(player)) return 0;

    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active)) return 0;
    sub = active[quest_id];
    if (!mapp(sub)) return 0;

    template = quest_defs[quest_id];
    if (!mapp(template)) return 0;

    objectives = template["objectives"];
    progress = sub["progress"];
    if (!mapp(progress)) progress = ([]);

    here = base_name(environment(player));
    done = 1;

    for (i = 0; i < sizeof(objectives); i++)
    {
        mapping obj = objectives[i];
        string key = "obj_" + i;
        int cur = progress[key];
        int amount = obj["amount"];
        string target = obj["target"];

        if (!amount) amount = 1;
        if (!cur) cur = 0;

        // 到达/对话类目标：所在房间路径前缀匹配目标即达成
        if ((obj["type"] == OBJ_REACH || obj["type"] == OBJ_TALK) &&
            stringp(target) && target != "" && stringp(here) &&
            strsrch(here, target) == 0)
        {
            cur = amount;
            progress[key] = cur;
        }

        if (cur < amount)
            done = 0;
    }

    sub["progress"] = progress;
    active[quest_id] = sub;
    player->set(QUEST_CHAIN_ACTIVE, active);
    QUEST_CHAIN_D->save_player_quest_state(player);

    return done;
}

// ═══════════════════════════════════════════
// 辅助方法
// ═══════════════════════════════════════════

// 获取玩家境界索引（委托 quest_chain_d，统一语义 0=炼气 1=筑基 ...）
int get_player_realm_index(object player)
{
    return QUEST_CHAIN_D->get_player_realm_index(player);
}

// 章节是否已解锁（境界 + 前置章节）
int is_chapter_unlocked(object player, int chapter)
{
    mapping ch_data;

    if (!player) return 0;
    if (chapter < 0 || chapter >= CHAPTER_COUNT)
        return 0;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;

    // 未落地章节（chain_id 为空）视为未解锁
    if (!stringp(ch_data["chain_id"]) || ch_data["chain_id"] == "")
        return 0;

    // 检查境界
    if (get_player_realm_index(player) < ch_data["realm_min"])
        return 0;

    // 检查前置章节
    if (ch_data["prereq_chapter"] < 0)
        return 1;

    return is_chapter_completed(player, ch_data["prereq_chapter"]);
}

// 章节是否已完成（链内全部节点完成）
int is_chapter_completed(object player, int chapter)
{
    mapping ch_data;
    mapping completed;
    string *qids;
    int i;

    if (!player) return 0;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;

    qids = chain_defs[ch_data["chain_id"]];
    if (!arrayp(qids) || sizeof(qids) == 0) return 0;

    completed = player->query(QUEST_CHAIN_COMPLETED);
    if (!mapp(completed)) return 0;

    for (i = 0; i < sizeof(qids); i++)
    {
        if (!completed[qids[i]])
            return 0;
    }
    return 1;
}

// 查找下一个可接取的主线节点（跨章节：从当前进度向后找）
string find_next_available_quest(object player)
{
    mapping ch_data;
    string *qids;
    int i, ch;

    if (!player) return "";

    for (ch = 0; ch < CHAPTER_COUNT; ch++)
    {
        ch_data = quest_chapters[ch];
        if (!ch_data) continue;
        if (!is_chapter_unlocked(player, ch)) continue;

        qids = chain_defs[ch_data["chain_id"]];
        if (!arrayp(qids)) continue;

        for (i = 0; i < sizeof(qids); i++)
        {
            if (QUEST_CHAIN_D->is_quest_available(qids[i], player))
                return qids[i];
        }
    }
    return "";
}

// 发放章节完成奖励（title + item + 里程碑经验/灵石）
int award_chapter_reward(object player, int chapter)
{
    mapping ch_data, reward;
    string title_name;
    string item_path;

    if (!player) return 0;

    ch_data = quest_chapters[chapter];
    if (!ch_data) return 0;
    reward = ch_data["chapter_reward"];
    if (!mapp(reward)) return 0;

    // 经验 / 灵石（里程碑奖励）
    if (reward["exp"] > 0)
        player->add("combat_exp", reward["exp"]);
    if (reward["coin"] > 0)
        MONEY_D->pay_player(player, reward["coin"] * 100);

    // 称号
    title_name = reward["title"];
    if (stringp(title_name) && title_name != "")
        player->set("title", title_name);

    // 物品
    item_path = reward["item"];
    if (stringp(item_path) && item_path != "")
    {
        object item = new(item_path);
        if (item)
        {
            item->move(player);
            tell_object(player, sprintf("获得特殊物品：%s。\n", item->query("name")));
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

// 目标文本（进度面板用）
string obj_target_text(mixed objectives)
{
    if (!arrayp(objectives) || sizeof(objectives) == 0)
        return "无";
    return objectives[0]["target"];
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
    if (undefinedp(quest_defs[node_id]))
        return 0;
    return quest_defs[node_id];
}

// 获取章节节点 ID 列表
string *get_chapter_node_ids(int chapter)
{
    mapping ch_data;

    if (chapter < 0 || chapter >= CHAPTER_COUNT)
        return ({});

    ch_data = quest_chapters[chapter];
    if (!ch_data) return ({});

    return chain_defs[ch_data["chain_id"]];
}
