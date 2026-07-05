// teleport_d.c
// 传送网络系统守护进程 —— 传送节点管理/费用计算/解锁校验/传送执行/冷却管理
// Created for ticket #33

#include <ansi.h>
#include <teleport.h>

inherit F_DBASE;

// -------- 全局状态 -------------------------------------------------

// 传送节点注册表
// node_id -> ([
//     TP_FIELD_ID       : node_id,
//     TP_FIELD_NAME     : "节点名",
//     TP_FIELD_LEVEL    : TP_LV*,
//     TP_FIELD_ROOM     : "房间路径",
//     TP_FIELD_STATUS   : TP_STATUS_*,
//     TP_FIELD_COST_BASE : 基础费用(灵石),
//     TP_FIELD_DIST_COEFF : TP_DIST_*,
//     TP_FIELD_REALM_MIN : TP_REALM_*,
//     TP_FIELD_UNLOCK_QUEST  : "任务标识" / 0,
//     TP_FIELD_UNLOCK_ITEM   : "路径:数量" / 0,
//     TP_FIELD_UNLOCK_REPUT  : ([ "势力":声望值 ]) / 0,
//     TP_FIELD_DEST    : ({ "dest_id1", "dest_id2", ... }),
//     TP_FIELD_GROUP   : "群组名",
//     TP_FIELD_DESC    : "描述",
// ])
nosave mapping teleport_nodes = ([]);

// 玩家传送冷却记录
// player_id -> ([ node_id : time() + cooldown_seconds, ... ])
nosave mapping player_cooldowns = ([]);

// 传送阵入口注册表：room_path -> node_id
nosave mapping teleport_entries = ([]);

// -------- 初始化与心跳 ---------------------------------------------

void create()
{
    seteuid(getuid());
    set("channel_id", "传送精灵");
    set("name", "传送网络系统");

    // 注册预定义传送节点
    init_teleport_nodes();

    CHANNEL_D->do_channel(this_object(), "sys", "传送网络系统已启动。");
    set_heart_beat(600);  // 每10分钟清理过期冷却
}

void heart_beat()
{
    string *pids, *nids;
    int i, j, now;

    now = time();

    // 清理过期的冷却记录
    pids = keys(player_cooldowns);
    for (i = 0; i < sizeof(pids); i++)
    {
        nids = keys(player_cooldowns[pids[i]]);
        for (j = 0; j < sizeof(nids); j++)
        {
            if (player_cooldowns[pids[i]][nids[j]] <= now)
                map_delete(player_cooldowns[pids[i]], nids[j]);
        }
        if (sizeof(player_cooldowns[pids[i]]) == 0)
            map_delete(player_cooldowns, pids[i]);
    }
}

// ======== 预定义传送节点注册 =======================================

void init_teleport_nodes()
{
    // -------- 人界传送网络（设计文档 §7.3） --------

    // 镜州江湖（起点，无门槛）
    register_node(([
        TP_FIELD_ID        : TP_NODE_MIRROR_LAKE,
        TP_FIELD_NAME      : "镜州江湖传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/city/kedian",       // 示例：客栈附近
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_MORTAL,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_YUE_SECTS, TP_NODE_TAI_NAN, TP_NODE_JIA_YUAN }),
        TP_FIELD_GROUP     : "越国",
        TP_FIELD_DESC      : "镜州江湖的传送阵，连接越国各主要区域。",
    ]));

    // 越国各派
    register_node(([
        TP_FIELD_ID        : TP_NODE_YUE_SECTS,
        TP_FIELD_NAME      : "越国七派传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/lingxiao/room1",     // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_MIRROR_LAKE, TP_NODE_TAI_NAN, TP_NODE_HUANGFENG, TP_NODE_JIA_YUAN }),
        TP_FIELD_GROUP     : "越国",
        TP_FIELD_DESC      : "越国七派共建的传送网络，各派弟子可便捷往来。",
    ]));

    // 太南谷
    register_node(([
        TP_FIELD_ID        : TP_NODE_TAI_NAN,
        TP_FIELD_NAME      : "太南谷传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/wudang/wdroad9",     // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_MIRROR_LAKE, TP_NODE_YUE_SECTS, TP_NODE_HUANGFENG }),
        TP_FIELD_GROUP     : "越国",
        TP_FIELD_DESC      : "太南谷坊市的传送阵，散修聚集之地，四通八达。",
    ]));

    // 黄枫谷
    register_node(([
        TP_FIELD_ID        : TP_NODE_HUANGFENG,
        TP_FIELD_NAME      : "黄枫谷传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/shaolin/shanmen",    // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_YUE_SECTS, TP_NODE_TAI_NAN, TP_NODE_ANCIENT_PORTAL }),
        TP_FIELD_GROUP     : "越国",
        TP_FIELD_DESC      : "黄枫谷门派的内部传送阵，可通往越国各地及古传送阵。",
    ]));

    // 岚州嘉元城
    register_node(([
        TP_FIELD_ID        : TP_NODE_JIA_YUAN,
        TP_FIELD_NAME      : "嘉元城传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/xinyang/kezhan",     // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_MIRROR_LAKE, TP_NODE_YUE_SECTS }),
        TP_FIELD_GROUP     : "越国",
        TP_FIELD_DESC      : "嘉元城的传送阵，连接镜州江湖与越国各派。",
    ]));

    // -------- 乱星海传送网络 --------

    // 古传送阵（越国↔乱星海）
    register_node(([
        TP_FIELD_ID        : TP_NODE_ANCIENT_PORTAL,
        TP_FIELD_NAME      : "古传送阵",
        TP_FIELD_LEVEL     : TP_LV2_CONTINENT,
        TP_FIELD_ROOM      : "/d/lingxiao/room2",     // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_CROSSBORDER,
        TP_FIELD_DIST_COEFF : TP_DIST_CROSSLAND,
        TP_FIELD_REALM_MIN : TP_REALM_QI,              // 炼气后期
        TP_FIELD_UNLOCK_QUEST : "quest_ancient_portal",
        TP_FIELD_UNLOCK_ITEM  : "/obj/relic/portal_token:1",
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_KUI_XING }),
        TP_FIELD_GROUP     : "跨海",
        TP_FIELD_DESC      : "上古遗留的跨国传送阵，需要大挪移令才能启动，通往乱星海魁星岛。",
    ]));

    // 魁星岛
    register_node(([
        TP_FIELD_ID        : TP_NODE_KUI_XING,
        TP_FIELD_NAME      : "魁星岛传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/taohuadao/jieyin",   // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : 200,
        TP_FIELD_DIST_COEFF : TP_DIST_SAMELAND,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_ANCIENT_PORTAL, TP_NODE_TIAN_XING, TP_NODE_INNER_ISLANDS }),
        TP_FIELD_GROUP     : "乱星海",
        TP_FIELD_DESC      : "魁星岛的传送阵，乱星海外围岛屿的交通枢纽。",
    ]));

    // 天星城（枢纽）
    register_node(([
        TP_FIELD_ID        : TP_NODE_TIAN_XING,
        TP_FIELD_NAME      : "天星城传送殿",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/changan/kedian",     // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_SAMELAND,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : ([ "星宫" : TP_REPUT_ACQUAINT ]),
        TP_FIELD_DEST      : ({
            TP_NODE_KUI_XING, TP_NODE_INNER_ISLANDS,
            TP_NODE_OUTER_ISLANDS, TP_NODE_ANCIENT_PORTAL
        }),
        TP_FIELD_GROUP     : "乱星海",
        TP_FIELD_DESC      : "天星城的核心传送殿，乱星海最大的传送枢纽，可传送至内星海各岛和妖兽猎场。",
    ]));

    // 内星海各岛
    register_node(([
        TP_FIELD_ID        : TP_NODE_INNER_ISLANDS,
        TP_FIELD_NAME      : "内星海传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/taohuadao/jieyin",   // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : 150,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_QI,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_TIAN_XING, TP_NODE_KUI_XING }),
        TP_FIELD_GROUP     : "乱星海",
        TP_FIELD_DESC      : "连接内星海各岛屿的传送网络。",
    ]));

    // 外星海·妖兽岛（猎妖传送）
    register_node(([
        TP_FIELD_ID        : TP_NODE_OUTER_ISLANDS,
        TP_FIELD_NAME      : "妖兽岛传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/taohuadao/haitan",   // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : 200,
        TP_FIELD_DIST_COEFF : TP_DIST_SAMELAND,
        TP_FIELD_REALM_MIN : TP_REALM_BASE,            // 筑基期
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_TIAN_XING }),
        TP_FIELD_GROUP     : "乱星海",
        TP_FIELD_DESC      : "从天星城传送殿可直达妖兽岛（200灵石/次），是猎妖者的首选传送点。",
    ]));

    // -------- 灵界传送网络 --------

    // 天渊城（灵界枢纽）
    register_node(([
        TP_FIELD_ID        : TP_NODE_TIAN_YUAN,
        TP_FIELD_NAME      : "天渊城传送殿",
        TP_FIELD_LEVEL     : TP_LV2_CONTINENT,
        TP_FIELD_ROOM      : "/d/guiyunzhuang/taihu", // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : 500,
        TP_FIELD_DIST_COEFF : TP_DIST_SAMELAND,
        TP_FIELD_REALM_MIN : TP_REALM_TRANSFORM,       // 化神期
        TP_FIELD_UNLOCK_QUEST : "quest_flyrise",
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : ([ "天渊城" : TP_REPUT_FRIENDLY ]),
        TP_FIELD_DEST      : ({ TP_NODE_SAN_HUANG, TP_NODE_BARBARIAN, TP_NODE_CROSS_PORTAL }),
        TP_FIELD_GROUP     : "灵界",
        TP_FIELD_DESC      : "天渊城的核心传送殿，灵界交通枢纽，连接三皇领地、蛮荒世界和跨大陆传送阵。",
    ]));

    // 三皇领地
    register_node(([
        TP_FIELD_ID        : TP_NODE_SAN_HUANG,
        TP_FIELD_NAME      : "三皇领地传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/riyuejiao/meizhuang", // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_TRANSFORM,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_TIAN_YUAN }),
        TP_FIELD_GROUP     : "灵界",
        TP_FIELD_DESC      : "连接三皇领地与天渊城的传送阵，灵界日常出行的主要通道。",
    ]));

    // 蛮荒世界
    register_node(([
        TP_FIELD_ID        : TP_NODE_BARBARIAN,
        TP_FIELD_NAME      : "蛮荒世界传送阵",
        TP_FIELD_LEVEL     : TP_LV2_CONTINENT,
        TP_FIELD_ROOM      : "/d/xingxiu/silk1",      // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_CROSSBORDER,
        TP_FIELD_DIST_COEFF : TP_DIST_SAMELAND,
        TP_FIELD_REALM_MIN : TP_REALM_TRANSFORM,
        TP_FIELD_UNLOCK_QUEST : "quest_barbarian",
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_TIAN_YUAN }),
        TP_FIELD_GROUP     : "灵界",
        TP_FIELD_DESC      : "通往蛮荒世界危险区域的传送阵，需要天渊城任务许可方可使用。",
    ]));

    // 跨大陆传送阵
    register_node(([
        TP_FIELD_ID        : TP_NODE_CROSS_PORTAL,
        TP_FIELD_NAME      : "跨大陆超级传送阵",
        TP_FIELD_LEVEL     : TP_LV3_CROSS,
        TP_FIELD_ROOM      : "/d/mingjiao/wuchang",   // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_CROSSCONT,
        TP_FIELD_DIST_COEFF : TP_DIST_CROSSLAND,
        TP_FIELD_REALM_MIN : TP_REALM_VOID,            // 炼虚期
        TP_FIELD_UNLOCK_QUEST : "quest_cross_portal",
        TP_FIELD_UNLOCK_ITEM  : "/obj/relic/ultimate_portal_token:1",
        TP_FIELD_UNLOCK_REPUT : ([ "天云十三族" : TP_REPUT_FRIENDLY ]),
        TP_FIELD_DEST      : ({ TP_NODE_FU_JIAO }),
        TP_FIELD_GROUP     : "灵界",
        TP_FIELD_DESC      : "灵界顶级的跨大陆传送阵，消耗极大，需要炼虚期修为和天云十三族的许可。",
    ]));

    // -------- 雷鸣大陆传送网络 --------

    // 伏蛟城
    register_node(([
        TP_FIELD_ID        : TP_NODE_FU_JIAO,
        TP_FIELD_NAME      : "伏蛟城传送阵",
        TP_FIELD_LEVEL     : TP_LV2_CONTINENT,
        TP_FIELD_ROOM      : "/d/dalicheng/tianlong", // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_CROSSBORDER,
        TP_FIELD_DIST_COEFF : TP_DIST_SAMELAND,
        TP_FIELD_REALM_MIN : TP_REALM_VOID,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_CROSS_PORTAL, TP_NODE_YUN_CHENG }),
        TP_FIELD_GROUP     : "雷鸣大陆",
        TP_FIELD_DESC      : "伏蛟城的传送阵，雷鸣大陆的交通枢纽。",
    ]));

    // 云城（天云十三族）
    register_node(([
        TP_FIELD_ID        : TP_NODE_YUN_CHENG,
        TP_FIELD_NAME      : "云城传送阵",
        TP_FIELD_LEVEL     : TP_LV1_REGION,
        TP_FIELD_ROOM      : "/d/emei/houshan",       // 示例路径
        TP_FIELD_STATUS    : TP_STATUS_ACTIVE,
        TP_FIELD_COST_BASE : TP_BASE_INTERCITY,
        TP_FIELD_DIST_COEFF : TP_DIST_NEARBY,
        TP_FIELD_REALM_MIN : TP_REALM_VOID,
        TP_FIELD_UNLOCK_QUEST : 0,
        TP_FIELD_UNLOCK_ITEM  : 0,
        TP_FIELD_UNLOCK_REPUT : 0,
        TP_FIELD_DEST      : ({ TP_NODE_FU_JIAO }),
        TP_FIELD_GROUP     : "雷鸣大陆",
        TP_FIELD_DESC      : "云城的传送阵，天云十三族的领地内部传送。",
    ]));
}

// ======== 节点注册与查询 ===========================================

// 注册一个传送节点
// 参数：节点配置 mapping（含所有 TP_FIELD_* 字段）
// 返回 1=成功，0=失败
int register_node(mapping node)
{
    string id;

    if (!mapp(node))
        return 0;

    id = node[TP_FIELD_ID];
    if (!stringp(id))
        return 0;

    teleport_nodes[id] = node;

    // 注册入口
    if (stringp(node[TP_FIELD_ROOM]))
        teleport_entries[node[TP_FIELD_ROOM]] = id;

    return 1;
}

// 获取所有传送节点 ID
string *query_all_nodes()
{
    return keys(teleport_nodes);
}

// 根据节点 ID 获取节点信息
mapping query_node(string node_id)
{
    if (undefinedp(teleport_nodes[node_id]))
        return 0;

    return teleport_nodes[node_id];
}

// 根据房间路径查找节点 ID
string find_node_by_room(string room_path)
{
    if (undefinedp(teleport_entries[room_path]))
        return 0;

    return teleport_entries[room_path];
}

// 获取某节点可达的所有目标节点
// 返回 mapping 数组，每个含 id/name/level/group
mapping *query_node_dests(string node_id)
{
    mapping node;
    string *dests;
    mapping *result;
    mapping dest_info;
    int i;

    node = teleport_nodes[node_id];
    if (!mapp(node))
        return 0;

    dests = node[TP_FIELD_DEST];
    if (!arrayp(dests))
        return 0;

    result = ({});
    for (i = 0; i < sizeof(dests); i++)
    {
        dest_info = teleport_nodes[dests[i]];
        if (mapp(dest_info))
        {
            result += ({ ([
                "id"    : dest_info[TP_FIELD_ID],
                "name"  : dest_info[TP_FIELD_NAME],
                "level" : dest_info[TP_FIELD_LEVEL],
                "group" : dest_info[TP_FIELD_GROUP],
            ]) });
        }
    }

    return result;
}

// 按群组获取节点列表
string *query_nodes_by_group(string group)
{
    string *ids, *result;
    int i;

    ids = keys(teleport_nodes);
    result = ({});
    for (i = 0; i < sizeof(ids); i++)
    {
        if (teleport_nodes[ids[i]][TP_FIELD_GROUP] == group)
            result += ({ ids[i] });
    }
    return result;
}

// 获取所有群组名
string *query_all_groups()
{
    string *ids, *result;
    int i;
    string group;

    ids = keys(teleport_nodes);
    result = ({});
    for (i = 0; i < sizeof(ids); i++)
    {
        group = teleport_nodes[ids[i]][TP_FIELD_GROUP];
        if (member_array(group, result) == -1)
            result += ({ group });
    }
    return result;
}

// ======== 解锁校验 ================================================

// 检查玩家是否满足境界要求
// 返回 1=满足，0=不满足
// 由于本 MUD 尚无标准境界系统，用 combat_exp 近似判断
int check_realm_requirement(object me, int min_realm)
{
    int exp;

    if (!objectp(me))
        return 0;

    if (min_realm <= TP_REALM_MORTAL)
        return 1;

    exp = me->query("combat_exp");

    // 用经验值近似映射到境界
    // 凡人：< 60000
    // 炼气：60000 ~ 500000
    // 筑基：500000 ~ 3000000
    // 结丹：3000000 ~ 10000000
    // 元婴：10000000 ~ 50000000
    // 化神：50000000 ~ 200000000
    // 炼虚：>= 200000000
    switch (min_realm)
    {
    case TP_REALM_QI:
        return (exp >= 60000);
    case TP_REALM_BASE:
        return (exp >= 500000);
    case TP_REALM_CORE:
        return (exp >= 3000000);
    case TP_REALM_NASCENT:
        return (exp >= 10000000);
    case TP_REALM_TRANSFORM:
        return (exp >= 50000000);
    case TP_REALM_VOID:
        return (exp >= 200000000);
    default:
        return 0;
    }
}

// 获取玩家当前境界等级（近似）
int query_player_realm(object me)
{
    int exp;

    if (!objectp(me))
        return TP_REALM_MORTAL;

    exp = me->query("combat_exp");

    if (exp < 60000)         return TP_REALM_MORTAL;
    else if (exp < 500000)   return TP_REALM_QI;
    else if (exp < 3000000)  return TP_REALM_BASE;
    else if (exp < 10000000) return TP_REALM_CORE;
    else if (exp < 50000000) return TP_REALM_NASCENT;
    else if (exp < 200000000) return TP_REALM_TRANSFORM;
    else                     return TP_REALM_VOID;
}

// 检查玩家是否满足特定节点的解锁条件
// 返回 1=已解锁，0=未解锁
int is_node_unlocked(object me, string node_id)
{
    mapping node;
    string *unlocked;

    if (!objectp(me) || !stringp(node_id))
        return 0;

    node = teleport_nodes[node_id];
    if (!mapp(node))
        return 0;

    // 状态检查
    if (node[TP_FIELD_STATUS] != TP_STATUS_ACTIVE)
        return 0;

    // 检查境界
    if (!check_realm_requirement(me, node[TP_FIELD_REALM_MIN]))
        return 0;

    // 检查玩家个人解锁记录（存储在玩家对象上，自动持久化）
    unlocked = me->query("teleport/unlocked");
    if (arrayp(unlocked) && member_array(node_id, unlocked) != -1)
        return 1;  // 已个人解锁

    // 以下条件只在首次使用时检查（无需个人解锁记录）
    // 如果是解锁任务/物品类节点，需要个人解锁记录
    // 如果无特殊解锁条件（quest/item/reput均为0），视为自动解锁
    if (node[TP_FIELD_UNLOCK_QUEST] == 0 &&
        node[TP_FIELD_UNLOCK_ITEM] == 0 &&
        node[TP_FIELD_UNLOCK_REPUT] == 0)
    {
        return 1;  // 自动解锁
    }

    return 0;
}

// 尝试为玩家解锁传送节点
// 返回 1=解锁成功, 0=解锁失败, -1=条件不满足
int try_unlock_node(object me, string node_id)
{
    mapping node;
    string *unlocked;

    if (!objectp(me) || !stringp(node_id))
        return 0;

    node = teleport_nodes[node_id];
    if (!mapp(node))
        return 0;

    // 境界检查
    if (!check_realm_requirement(me, node[TP_FIELD_REALM_MIN]))
        return -1;

    // 检查任务要求
    if (stringp(node[TP_FIELD_UNLOCK_QUEST]))
    {
        if (!me->query("quest/" + node[TP_FIELD_UNLOCK_QUEST]))
            return -1;
    }

    // 检查物品要求
    if (stringp(node[TP_FIELD_UNLOCK_ITEM]))
    {
        string *parts;
        string item_path;
        int item_count;
        object item;

        parts = explode(node[TP_FIELD_UNLOCK_ITEM], ":");
        if (sizeof(parts) >= 1)
            item_path = parts[0];
        if (sizeof(parts) >= 2)
            item_count = atoi(parts[1]);
        else
            item_count = 1;

        item = present(item_path, me);
        if (!objectp(item) || (int)item->query_amount() < item_count)
            return -1;
    }

    // 检查声望要求
    if (mapp(node[TP_FIELD_UNLOCK_REPUT]))
    {
        string *factions;
        int i, required;

        factions = keys(node[TP_FIELD_UNLOCK_REPUT]);
        for (i = 0; i < sizeof(factions); i++)
        {
            required = node[TP_FIELD_UNLOCK_REPUT][factions[i]];
            if ((int)me->query("reputation/" + factions[i]) < required)
                return -1;
        }
    }

    // 条件全部满足，记录解锁（存储在玩家对象上，自动持久化）
    unlocked = me->query("teleport/unlocked");
    if (!arrayp(unlocked))
        unlocked = ({});

    if (member_array(node_id, unlocked) == -1)
        unlocked += ({ node_id });

    me->set("teleport/unlocked", unlocked);

    return 1;
}

// ======== 费用计算 ================================================

// 计算从源节点到目标节点的传送费用（单位：文铜板）
// 公式：费用 = (基础费 + 距离系数 × 境界系数) × 声望折扣
// 设计文档 §5.3.3
int calculate_cost(object me, string src_id, string dest_id)
{
    mapping src_node, dest_node;
    int base_cost, dist_coeff, realm_coeff;
    int total_cost, discount;
    int player_realm;

    if (!objectp(me))
        return 0;

    src_node = teleport_nodes[src_id];
    dest_node = teleport_nodes[dest_id];

    if (!mapp(src_node) || !mapp(dest_node))
        return 999999;  // 不可达

    // 基础费：以源节点为准
    base_cost = src_node[TP_FIELD_COST_BASE];

    // 距离系数：取两节点中较大的（以距离最远的为准）
    dist_coeff = src_node[TP_FIELD_DIST_COEFF];
    if ((int)dest_node[TP_FIELD_DIST_COEFF] > dist_coeff)
        dist_coeff = dest_node[TP_FIELD_DIST_COEFF];

    // 境界系数
    player_realm = query_player_realm(me);
    realm_coeff = TP_REALM_COEFF[player_realm];
    if (realm_coeff < 50)
        realm_coeff = 50;  // 最低 ×0.5

    // 计算总费用（以灵石为单位，再转为文铜板）
    // 费用 = 基础费 + (距离系数 × 境界系数 × 基础费 / 1000)
    total_cost = (base_cost + (dist_coeff * realm_coeff * base_cost) / 1000) * 100;

    // 根据目的节点的声望要求计算折扣
    discount = 100;
    if (mapp(dest_node[TP_FIELD_UNLOCK_REPUT]))
    {
        string *factions;
        int i, reput, reput_tier;

        factions = keys(dest_node[TP_FIELD_UNLOCK_REPUT]);
        for (i = 0; i < sizeof(factions); i++)
        {
            reput = me->query("reputation/" + factions[i]);

            // 按声望等级找折扣
            if (reput >= TP_REPUT_WORSHIP)
                reput_tier = TP_REPUT_WORSHIP;
            else if (reput >= TP_REPUT_REVERE)
                reput_tier = TP_REPUT_REVERE;
            else if (reput >= TP_REPUT_RESPECT)
                reput_tier = TP_REPUT_RESPECT;
            else if (reput >= TP_REPUT_FRIENDLY)
                reput_tier = TP_REPUT_FRIENDLY;
            else
                reput_tier = 0;

            if (reput_tier > 0)
            {
                int tier_discount = TP_REPUT_DISCOUNT[reput_tier];
                if (tier_discount < discount)
                    discount = tier_discount;
            }
        }
    }

    total_cost = total_cost * discount / 100;

    // 最低消费：1文
    if (total_cost < 1)
        total_cost = 1;

    return total_cost;
}

// 获取费用的人类可读描述（以灵石为单位）
string describe_cost(object me, string src_id, string dest_id)
{
    int cost_in_wen;

    cost_in_wen = calculate_cost(me, src_id, dest_id);

    if (cost_in_wen >= 999999)
        return "不可达";

    return MONEY_D->money_str(cost_in_wen);
}

// ======== 冷却管理 ================================================

// 检查玩家是否在冷却中
// 返回剩余冷却秒数，0=无冷却
int query_cooldown(object me, string node_id)
{
    string pid;
    mapping cooldowns;

    if (!objectp(me))
        return 0;

    pid = me->query("id");
    if (!mapp(player_cooldowns))
        return 0;

    cooldowns = player_cooldowns[pid];
    if (!mapp(cooldowns))
        return 0;

    if (undefinedp(cooldowns[node_id]))
        return 0;

    if (cooldowns[node_id] <= time())
    {
        map_delete(cooldowns, node_id);
        return 0;
    }

    return cooldowns[node_id] - time();
}

// 设置冷却
// 返回 1=成功
int set_cooldown(object me, string node_id)
{
    mapping node;
    string pid;
    int cd_seconds;

    if (!objectp(me))
        return 0;

    node = teleport_nodes[node_id];
    if (!mapp(node))
        return 0;

    // 根据传送等级确定冷却时间
    switch ((int)node[TP_FIELD_LEVEL])
    {
    case TP_LV0_CITY:
        cd_seconds = TP_CD_CITY;
        break;
    case TP_LV1_REGION:
        cd_seconds = TP_CD_REGION;
        break;
    case TP_LV2_CONTINENT:
        cd_seconds = TP_CD_CONTINENT;
        break;
    case TP_LV3_CROSS:
        cd_seconds = TP_CD_CROSS;
        break;
    case TP_LV4_REALM:
        cd_seconds = TP_CD_REALM;
        break;
    default:
        cd_seconds = TP_CD_REGION;
    }

    pid = me->query("id");

    if (!mapp(player_cooldowns))
        player_cooldowns = ([]);

    if (!mapp(player_cooldowns[pid]))
        player_cooldowns[pid] = ([]);

    player_cooldowns[pid][node_id] = time() + cd_seconds;

    return 1;
}

// ======== 传送执行 ================================================

// 执行传送
// 参数：玩家对象，目标节点 ID
// 返回：1=成功，0=失败，-1=灵石不足，-2=未解锁，-3=冷却中，-4=目标无效
int do_teleport(object me, string dest_id)
{
    mapping dest_node;
    object env;
    string src_id, src_room;
    int cost, remaining;

    if (!objectp(me) || !userp(me))
        return 0;

    // 从当前房间查找源节点
    env = environment(me);
    if (!objectp(env))
        return 0;

    src_room = base_name(env);
    src_id = find_node_by_room(src_room);

    if (!stringp(src_id))
        return 0;

    dest_node = teleport_nodes[dest_id];
    if (!mapp(dest_node))
        return -4;

    // 检查目标是否可达（在源节点的目标列表中）
    if (member_array(dest_id, teleport_nodes[src_id][TP_FIELD_DEST]) == -1)
        return -4;

    // 检查解锁状态
    if (!is_node_unlocked(me, dest_id))
        return -2;

    // 检查冷却
    remaining = query_cooldown(me, src_id);
    if (remaining > 0)
        return -3;

    // 计算并扣除费用
    cost = calculate_cost(me, src_id, dest_id);
    if (cost < 0 || cost >= 999999)
        return -4;

    if (!MONEY_D->player_pay(me, cost))
        return -1;

    // 设置冷却
    set_cooldown(me, src_id);

    // 执行传送
    tell_object(me, HIC "传送阵发出耀眼的灵光，空间波动将你包裹...\n" NOR);
    message("vision", me->name() + "的身影在灵光中逐渐消失。\n",
            environment(me), ({ me }));

    if (!me->move(dest_node[TP_FIELD_ROOM]))
    {
        // 传送失败，退回费用
        MONEY_D->pay_player(me, cost);
        tell_object(me, HIY "传送被中断，灵石已退还。\n" NOR);
        return 0;
    }

    message("vision", me->name() + "的身影在灵光中凭空出现。\n",
            environment(me), ({ me }));
    tell_object(me, HIC "你感到一阵天旋地转，已抵达" + dest_node[TP_FIELD_NAME] + "。\n" NOR);

    return 1;
}

// 传送失败消息
string teleport_error_msg(int code)
{
    switch (code)
    {
    case -1:
        return HIR "你的灵石不足以支付传送费用！\n" NOR;
    case -2:
        return HIY "你还未解锁目标传送阵，无法传送！\n" NOR;
    case -3:
        return HIM "传送阵还在冷却中，请稍后再试！\n" NOR;
    case -4:
        return HIY "该传送阵无法到达目标地点！\n" NOR;
    default:
        return HIR "传送失败！\n" NOR;
    }
}

// ======== 传送阵信息查询 ==========================================

// 获取玩家当前所在位置的传送阵信息
// 返回 mapping 或 0
mapping query_current_node(object me)
{
    object env;
    string room_path, node_id;

    if (!objectp(me))
        return 0;

    env = environment(me);
    if (!objectp(env))
        return 0;

    room_path = base_name(env);
    node_id = find_node_by_room(room_path);

    if (!stringp(node_id))
        return 0;

    return teleport_nodes[node_id];
}

// 获取玩家可用的传送目标列表（含费用和状态）
// 返回 mapping 数组
mapping *query_available_dests(object me)
{
    mapping node, dest_info;
    string *dests;
    mapping *result;
    int i, cost, remaining;

    node = query_current_node(me);
    if (!mapp(node))
        return 0;

    dests = node[TP_FIELD_DEST];
    if (!arrayp(dests))
        return 0;

    result = ({});
    for (i = 0; i < sizeof(dests); i++)
    {
        dest_info = teleport_nodes[dests[i]];
        if (!mapp(dest_info))
            continue;

        cost = calculate_cost(me, node[TP_FIELD_ID], dests[i]);
        remaining = query_cooldown(me, node[TP_FIELD_ID]);

        result += ({ ([
            "id"        : dests[i],
            "name"      : dest_info[TP_FIELD_NAME],
            "level"     : dest_info[TP_FIELD_LEVEL],
            "group"     : dest_info[TP_FIELD_GROUP],
            "cost"      : cost,
            "cost_str"  : MONEY_D->money_str(cost),
            "unlocked"  : is_node_unlocked(me, dests[i]),
            "cooldown"  : remaining,
            "realm_min" : dest_info[TP_FIELD_REALM_MIN],
            "desc"      : dest_info[TP_FIELD_DESC],
        ]) });
    }

    return result;
}

// ======== 状态查询 ================================================

// 获取传送网络系统总览
string describe_network()
{
    string *groups, *nodes;
    string output;
    int i, j, active, total;

    output = HIC "╔══════════════════════════════════════════╗\n" NOR;
    output += HIC "║           传送网络系统总览               ║\n" NOR;
    output += HIC "╚══════════════════════════════════════════╝\n" NOR;

    groups = query_all_groups();
    for (i = 0; i < sizeof(groups); i++)
    {
        output += sprintf("\n" HIW "【%s】" NOR "\n", groups[i]);
        nodes = query_nodes_by_group(groups[i]);
        for (j = 0; j < sizeof(nodes); j++)
        {
            mapping n = teleport_nodes[nodes[j]];
            if (!mapp(n))
                continue;

            output += sprintf("  %-12s  Lv.%d  %s  %s\n",
                n[TP_FIELD_NAME],
                n[TP_FIELD_LEVEL],
                (n[TP_FIELD_STATUS] == TP_STATUS_ACTIVE) ? HIG "●激活" NOR : HIR "●关闭" NOR,
                n[TP_FIELD_DESC]);
        }
    }

    return output;
}

// 获取保存数据（供 save/restore 持久化使用）
// 传送节点配置为预定义，不需要持久化；玩家解锁记录存储在玩家对象上
mapping query_save_data()
{
    return ([]);
}

// 恢复保存数据
void restore_save_data(mapping data)
{
    // 玩家解锁记录存储在玩家对象上，自动持久化
}
