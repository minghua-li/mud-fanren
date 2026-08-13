// teleport.h
// 传送网络系统 —— 类型定义、常量、宏、接口声明
// Created for ticket #33

#ifndef __TELEPORT_H__
#define __TELEPORT_H__

//=============================================================================
// 传送等级（对应设计文档 §7.1）
//=============================================================================
#define TP_LV0_CITY          0   // 城内瞬移：同一城市内部，50灵石
#define TP_LV1_REGION        1   // 区域传送：同国家/海域，100~500灵石
#define TP_LV2_CONTINENT     2   // 大陆传送：跨国/海域，中品灵石+大挪移令
#define TP_LV3_CROSS         3   // 跨大陆传送：灵界各大陆间，极品灵石+大乘权限
#define TP_LV4_REALM         4   // 跨界传送：人界↔灵界，化神境界突破

// 传送等级名称
#define TP_LEVEL_NAME ([ \
    TP_LV0_CITY     : "城内瞬移", \
    TP_LV1_REGION   : "区域传送", \
    TP_LV2_CONTINENT : "大陆传送", \
    TP_LV3_CROSS    : "跨大陆传送", \
    TP_LV4_REALM    : "跨界传送", \
])

//=============================================================================
// 传送阵状态
//=============================================================================
#define TP_STATUS_ACTIVE      1   // 激活（可用）
#define TP_STATUS_INACTIVE    0   // 未激活（不可用）
#define TP_STATUS_MAINT       2   // 维护中（暂时不可用）

//=============================================================================
// 传送费用常量（设计文档 §5.3.3 传送收费模型）
// 单位：文铜板（1灵石=100文）
//=============================================================================

// 基础费（灵石，1灵石=100文铜板）
#define TP_BASE_CITY          50      // 城内瞬移：50灵石
#define TP_BASE_INTERCITY     100     // 城际：100灵石
#define TP_BASE_CROSSBORDER   1000    // 跨境：1,000灵石
#define TP_BASE_CROSSCONT     100000  // 跨大陆：100,000灵石

// 距离系数
#define TP_DIST_NEARBY        1       // 邻近区域
#define TP_DIST_SAMELAND      3       // 同大陆
#define TP_DIST_CROSSLAND     10      // 跨大陆
#define TP_DIST_CROSSREALM    50      // 跨界

// 境界系数（设计文档 §5.3.3）
// 炼气×0.5，筑基×1，结丹×2，元婴×5，化神×10，炼虚×20
#define TP_REALM_MORTAL       0       // 凡人
#define TP_REALM_QI           1       // 炼气
#define TP_REALM_BASE         2       // 筑基
#define TP_REALM_CORE         3       // 结丹
#define TP_REALM_NASCENT      4       // 元婴
#define TP_REALM_TRANSFORM    5       // 化神
#define TP_REALM_VOID         6       // 炼虚

#define TP_REALM_COEFF ([ \
    TP_REALM_MORTAL   : 0, \
    TP_REALM_QI       : 50,   /* ×0.5 = 50/100 */ \
    TP_REALM_BASE     : 100,  /* ×1.0 = 100/100 */ \
    TP_REALM_CORE     : 200,  /* ×2.0 = 200/100 */ \
    TP_REALM_NASCENT  : 500,  /* ×5.0 = 500/100 */ \
    TP_REALM_TRANSFORM : 1000, /* ×10.0 = 1000/100 */ \
    TP_REALM_VOID     : 2000, /* ×20.0 = 2000/100 */ \
])

// 声望折扣等级（设计文档 §5.4.1）
#define TP_REPUT_STRANGER     0       // 陌路：无折扣
#define TP_REPUT_ACQUAINT     100     // 相识：无折扣
#define TP_REPUT_FRIENDLY     500     // 友善：9折
#define TP_REPUT_RESPECT      2000    // 尊敬：8折
#define TP_REPUT_REVERE       5000    // 崇敬：7折
#define TP_REPUT_WORSHIP      10000   // 崇拜：5折

#define TP_REPUT_DISCOUNT ([ \
    TP_REPUT_STRANGER  : 100,  /* 100% */ \
    TP_REPUT_ACQUAINT  : 100,  /* 100% */ \
    TP_REPUT_FRIENDLY  : 90,   /* 90%  */ \
    TP_REPUT_RESPECT   : 80,   /* 80%  */ \
    TP_REPUT_REVERE    : 70,   /* 70%  */ \
    TP_REPUT_WORSHIP   : 50,   /* 50%  */ \
])

//=============================================================================
// 传送冷却时间（秒）
//=============================================================================
#define TP_CD_CITY            5       // 城内瞬移：5秒
#define TP_CD_REGION          30      // 区域传送：30秒
#define TP_CD_CONTINENT       300     // 大陆传送：5分钟
#define TP_CD_CROSS           3600    // 跨大陆传送：1小时
#define TP_CD_REALM           86400   // 跨界传送：24小时

//=============================================================================
// 传送节点结构字段（供 teleport_d 内部注册表使用）
//=============================================================================
#define TP_FIELD_ID            "id"              // 节点唯一标识
#define TP_FIELD_NAME          "name"            // 节点名称
#define TP_FIELD_LEVEL         "level"           // 传送等级（TP_LV*）
#define TP_FIELD_ROOM          "room"            // 所在房间路径
#define TP_FIELD_STATUS        "status"          // 激活状态
#define TP_FIELD_COST_BASE     "cost_base"       // 基础费用（灵石）
#define TP_FIELD_DIST_COEFF    "dist_coeff"      // 距离系数（TP_DIST_*）
#define TP_FIELD_REALM_MIN     "realm_min"       // 最低境界要求
#define TP_FIELD_UNLOCK_QUEST  "unlock_quest"    // 解锁任务（可选）
#define TP_FIELD_UNLOCK_ITEM   "unlock_item"     // 解锁物品（"路径:数量"，可选）
#define TP_FIELD_UNLOCK_REPUT  "unlock_reput"    // 解锁声望（"势力:声望值"，可选）
#define TP_FIELD_DEST          "dests"           // 可达目标节点ID列表
#define TP_FIELD_GROUP         "group"           // 所属传送群组（如"越国"、"天星城"）
#define TP_FIELD_DESC          "desc"            // 传送阵描述

//=============================================================================
// 传送网络预定义节点宏（对应设计文档 §7.3 传送阵网络图）
//=============================================================================

// 人界 → 越国区域
#define TP_NODE_MIRROR_LAKE       "mirror_lake"     // 镜州江湖
#define TP_NODE_YUE_SECTS         "yue_sects"       // 越国各派（越国七派传送阵，d/yueguo/transmit）
#define TP_NODE_TAI_NAN           "tai_nan"         // 太南谷
#define TP_NODE_HUANGFENG         "huangfeng"       // 黄枫谷
#define TP_NODE_JIA_YUAN          "jia_yuan"        // 岚州嘉元城
#define TP_NODE_QING_NIU          "qing_niu"        // 青牛镇（镜州出生地）
#define TP_NODE_TIANLUO_SECTS     "tianluo_sects"   // 天罗国魔道两宗（鬼灵门/御灵宗）

// 人界 → 乱星海
#define TP_NODE_ANCIENT_PORTAL    "ancient_portal"  // 古传送阵（越国↔乱星海）
#define TP_NODE_KUI_XING          "kui_xing"        // 魁星岛
#define TP_NODE_TIAN_XING         "tian_xing"       // 天星城（枢纽）
#define TP_NODE_INNER_ISLANDS     "inner_islands"   // 内星海各岛
#define TP_NODE_OUTER_ISLANDS     "outer_islands"   // 外星海·妖兽岛

// 灵界 → 天渊城区域
#define TP_NODE_TIAN_YUAN         "tian_yuan"       // 天渊城（灵界枢纽）
#define TP_NODE_SAN_HUANG         "san_huang"       // 三皇领地
#define TP_NODE_BARBARIAN         "barbarian"       // 蛮荒世界
#define TP_NODE_CROSS_PORTAL      "cross_portal"    // 跨大陆传送阵

// 灵界 → 雷鸣大陆
#define TP_NODE_FU_JIAO           "fu_jiao"         // 伏蛟城（雷鸣大陆）
#define TP_NODE_YUN_CHENG         "yun_cheng"       // 云城（天云十三族）

//=============================================================================
// 默认传送网络覆盖范围
//=============================================================================
#define TP_NETWORK_HUMAN          "human_realm"     // 人界传送网
#define TP_NETWORK_SPIRIT         "spirit_realm"    // 灵界传送网
#define TP_NETWORK_CROSS          "cross_realm"     // 跨界传送网

#endif
