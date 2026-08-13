// sect_facility.h
// 门派设施系统 —— 通用设施框架（灵田/丹房/藏经阁/护山大阵/演武场）+ 九宗特色设施
// 设计文档: .knowledge/factions/sects/ 九宗档案「设施」节
//           02-扩充内容/02-区域游戏玩法.md §4 宗门驻地玩法
// 相关 daemon: adm/daemons/sect_facility_d.c
// Created for ticket #60

#ifndef __SECT_FACILITY_H__
#define __SECT_FACILITY_H__

//=============================================================================
// 设施类型（通用框架五类 + 特色）
//=============================================================================
#define SECT_FACILITY_PLANT      1   // 灵田（种植/收获）
#define SECT_FACILITY_ALCHEMY    2   // 丹房（炼丹成功率加成）
#define SECT_FACILITY_LIBRARY    3   // 藏经阁（功法阅读/抄录）
#define SECT_FACILITY_DEFENSE    4   // 护山大阵（防御/抗敌）
#define SECT_FACILITY_TRAINING   5   // 演武场（切磋/修行效率）
#define SECT_FACILITY_SPECIAL    6   // 九宗特色设施

#define SECT_FACILITY_TYPE_NAME ([ \
    SECT_FACILITY_PLANT    : "灵田", \
    SECT_FACILITY_ALCHEMY  : "丹房", \
    SECT_FACILITY_LIBRARY  : "藏经阁", \
    SECT_FACILITY_DEFENSE  : "护山大阵", \
    SECT_FACILITY_TRAINING : "演武场", \
    SECT_FACILITY_SPECIAL  : "特色设施", \
])

//=============================================================================
// 玩家设施数据路径（F_DBASE 路径式访问）
//=============================================================================
#define SECT_FACILITY_PATH       "sect_facility"
#define SECT_FACILITY_PATH_BUFFS "sect_facility/buffs"  // ([ key: ([ "expire": t, "value": v ]) ])
#define SECT_FACILITY_PATH_DAILY "sect_facility/daily"  // ([ key: ([ "day": N, "count": N ]) ])
#define SECT_FACILITY_PATH_PLOTS "sect_facility/plots"  // ([ key: ([ idx: plot ]) ])

//=============================================================================
// 设施效果 buff 类型
//=============================================================================
#define SECT_BUFF_ALCHEMY     "alchemy_success"   // 炼丹成功率加成（丹房）
#define SECT_BUFF_FORGE       "forge_success"     // 炼器成功率加成（化刀坞炼器工坊）
#define SECT_BUFF_TRAIN       "train_efficiency"  // 修行效率加成（演武场）
#define SECT_BUFF_DEFENSE     "defense_power"     // 防御加成（护山大阵/城堡工事）
#define SECT_BUFF_BEAST       "beast_training"    // 御兽驯养效率（兽栏/虫房/万兽园）
#define SECT_BUFF_SWORD       "sword_insight"     // 剑意参悟（巨剑门剑冢）
#define SECT_BUFF_DAO         "dao_insight"       // 道藏参悟（清虚门道观）
#define SECT_BUFF_GHOST       "ghost_refine"      // 鬼道修炼效率（鬼灵门炼尸房）
#define SECT_BUFF_SPIRIT      "spirit_gain"       // 机缘加成（掩月宗天月神舟）

#define SECT_BUFF_NAME ([ \
    SECT_BUFF_ALCHEMY : "炼丹成功率", \
    SECT_BUFF_FORGE   : "炼器成功率", \
    SECT_BUFF_TRAIN   : "修行效率", \
    SECT_BUFF_DEFENSE : "防御", \
    SECT_BUFF_BEAST   : "御兽驯养效率", \
    SECT_BUFF_SWORD   : "剑意参悟", \
    SECT_BUFF_DAO     : "道藏参悟", \
    SECT_BUFF_GHOST   : "鬼道修炼效率", \
    SECT_BUFF_SPIRIT  : "机缘", \
])

//=============================================================================
// 通用消耗/效果数值（02-区域游戏玩法.md §4.4 与 mansion.h 对齐）
//=============================================================================
// 通用设施升级消耗（对齐传功堂/炼丹阁：5000/3000、20000/10000）
#define SECT_UPGRADE_COMMON ([ \
    2 : ({ 5000, 3000 }), \
    3 : ({ 20000, 10000 }), \
])

// 藏经阁升级消耗（§4.4.1 藏经阁建设 50000/20000 映射为 Lv1→2；
// Lv2→3 取 §4.4.1 表中高于藏经阁的最高通用建筑档护山大阵 Lv.2 100000/50000）
#define SECT_UPGRADE_LIBRARY ([ \
    2 : ({ 50000, 20000 }), \
    3 : ({ 100000, 50000 }), \
])

// 防御设施升级消耗（对齐护山大阵 Lv.1/Lv.2/Lv.3：30000/15000、100000/50000、500000/200000）
#define SECT_UPGRADE_DEFENSE ([ \
    2 : ({ 30000, 15000 }), \
    3 : ({ 100000, 50000 }), \
    4 : ({ 500000, 200000 }), \
])

// 坊市升级消耗（对齐 §4.4.1 坊市建设：15000/8000）
#define SECT_UPGRADE_MARKET ([ \
    2 : ({ 15000, 8000 }), \
    3 : ({ 30000, 15000 }), \
])

// 特色设施升级消耗
#define SECT_UPGRADE_SPECIAL ([ \
    2 : ({ 10000, 5000 }), \
    3 : ({ 30000, 15000 }), \
])

// 灵田升级消耗：灵石对齐 mansion.h LAND_UPGRADE_COST（凡土→灵土→灵泉），
// 贡献对齐 §4.4.1 灵药园建设贡献 5000
#define SECT_UPGRADE_PLANT ([ \
    2 : ({ 1000, 5000 }), \
    3 : ({ 10000, 20000 }), \
])

// 灵田地块数（随等级，对齐 LAND_MAX_PLOTS：2/5/8）
#define SECT_PLANT_PLOTS_BASE       2
#define SECT_PLANT_PLOTS_PER_LEVEL  3

// 灵田种植维护费（每次种植，对齐 GARDEN_MAINTENANCE 凡土=10 灵石）
#define SECT_PLANT_MAINTENANCE      10

// 演武场切磋每日奖励基数与每级递增
#define SECT_SPAR_EXP_BASE          500
#define SECT_SPAR_EXP_PER_LEVEL     500
#define SECT_SPAR_CONTRIB           20

// 默认 buff 时长（秒）与使用消耗
#define SECT_BUFF_DURATION_DEFAULT  7200
#define SECT_USE_STONE_DEFAULT      10
#define SECT_USE_CONTRIB_DEFAULT    100

#endif
