// sect_hq.h
// 宗门驻地系统 —— 类型定义、常量、宏、接口声明
// 对应设计文档 02-扩充内容/02-区域游戏玩法.md §4
// Created for ticket #22

#ifndef __SECT_HQ_H__
#define __SECT_HQ_H__

//=============================================================================
// 驻地类型（对应设计文档 §4.1）
//=============================================================================
#define HQ_TYPE_MOUNTAIN      1   // 山门驻地：初创（≤50人），可迁移
#define HQ_TYPE_STANDARD      2   // 标准驻地：发展中（50~200人），可迁移
#define HQ_TYPE_CORE          3   // 核心驻地：大型（200~1000人），固定
#define HQ_TYPE_SANCTUARY     4   // 圣地：顶级（1000人+），固定

#define HQ_TYPE_NAME ([ \
    HQ_TYPE_MOUNTAIN  : "山门驻地", \
    HQ_TYPE_STANDARD  : "标准驻地", \
    HQ_TYPE_CORE      : "核心驻地", \
    HQ_TYPE_SANCTUARY : "圣地", \
])

//=============================================================================
// 驻地状态
//=============================================================================
#define HQ_STATUS_NORMAL      0   // 正常
#define HQ_STATUS_SIEGE       1   // 攻防战中
#define HQ_STATUS_DAMAGED     2   // 受损
#define HQ_STATUS_DESTROYED   3   // 濒毁

//=============================================================================
// 建筑类型（对应设计文档 §4.2 建筑树）
//=============================================================================
#define HQ_BLDG_CORE          1   // 议事大殿（基础管理）
#define HQ_BLDG_TRAINING      2   // 传功堂（修炼加成）
#define HQ_BLDG_ALCHEMY       3   // 炼丹阁（批量炼丹）
#define HQ_BLDG_FORGE         4   // 炼器坊（批量炼器）
#define HQ_BLDG_MARKET        5   // 坊市（内部交易）
#define HQ_BLDG_HERB          6   // 灵药园（批量种植）
#define HQ_BLDG_LIBRARY       7   // 藏经阁（功法存放）
#define HQ_BLDG_ARRAY_L1      8   // 护山大阵Lv.1
#define HQ_BLDG_ARRAY_L2      9   // 护山大阵Lv.2
#define HQ_BLDG_ARRAY_L3      10  // 护山大阵Lv.3
#define HQ_BLDG_WATCHTOWER    11  // 巡查哨塔
#define HQ_BLDG_TELEPORT      12  // 传送阵

// 特殊建筑（稀有，仅核心/圣地可建）
#define HQ_BLDG_SECRET_PORTAL 13  // 秘境入口（独占小型秘境）
#define HQ_BLDG_AURA_CONVERGE 14  // 灵脉汇聚阵
#define HQ_BLDG_CROSS_PORTAL  15  // 跨界传送阵

#define HQ_BLDG_NAME ([ \
    HQ_BLDG_CORE         : "议事大殿", \
    HQ_BLDG_TRAINING     : "传功堂", \
    HQ_BLDG_ALCHEMY      : "炼丹阁", \
    HQ_BLDG_FORGE        : "炼器坊", \
    HQ_BLDG_MARKET       : "坊市", \
    HQ_BLDG_HERB         : "灵药园", \
    HQ_BLDG_LIBRARY      : "藏经阁", \
    HQ_BLDG_ARRAY_L1     : "护山大阵Lv.1", \
    HQ_BLDG_ARRAY_L2     : "护山大阵Lv.2", \
    HQ_BLDG_ARRAY_L3     : "护山大阵Lv.3", \
    HQ_BLDG_WATCHTOWER   : "巡查哨塔", \
    HQ_BLDG_TELEPORT     : "传送阵", \
    HQ_BLDG_SECRET_PORTAL : "秘境入口", \
    HQ_BLDG_AURA_CONVERGE : "灵脉汇聚阵", \
    HQ_BLDG_CROSS_PORTAL  : "跨界传送阵", \
])

//=============================================================================
// 建筑解锁条件（需要驻地达到的最低类型）
//=============================================================================
#define HQ_BLDG_UNLOCK ([ \
    HQ_BLDG_CORE         : HQ_TYPE_MOUNTAIN, \
    HQ_BLDG_TRAINING     : HQ_TYPE_MOUNTAIN, \
    HQ_BLDG_ALCHEMY      : HQ_TYPE_STANDARD, \
    HQ_BLDG_FORGE        : HQ_TYPE_STANDARD, \
    HQ_BLDG_MARKET       : HQ_TYPE_STANDARD, \
    HQ_BLDG_HERB         : HQ_TYPE_STANDARD, \
    HQ_BLDG_LIBRARY      : HQ_TYPE_CORE, \
    HQ_BLDG_ARRAY_L1     : HQ_TYPE_MOUNTAIN, \
    HQ_BLDG_ARRAY_L2     : HQ_TYPE_STANDARD, \
    HQ_BLDG_ARRAY_L3     : HQ_TYPE_CORE, \
    HQ_BLDG_WATCHTOWER   : HQ_TYPE_MOUNTAIN, \
    HQ_BLDG_TELEPORT     : HQ_TYPE_STANDARD, \
    HQ_BLDG_SECRET_PORTAL : HQ_TYPE_CORE, \
    HQ_BLDG_AURA_CONVERGE : HQ_TYPE_CORE, \
    HQ_BLDG_CROSS_PORTAL  : HQ_TYPE_SANCTUARY, \
])

//=============================================================================
// 建筑建设消耗（对应设计文档 §4.4.1，单位：灵石）
//=============================================================================
#define HQ_BLDG_COST ([ \
    HQ_BLDG_CORE         : 10000, \
    HQ_BLDG_TRAINING     : 5000, \
    HQ_BLDG_ALCHEMY      : 20000, \
    HQ_BLDG_FORGE        : 20000, \
    HQ_BLDG_MARKET       : 15000, \
    HQ_BLDG_HERB         : 10000, \
    HQ_BLDG_LIBRARY      : 50000, \
    HQ_BLDG_ARRAY_L1     : 30000, \
    HQ_BLDG_ARRAY_L2     : 100000, \
    HQ_BLDG_ARRAY_L3     : 500000, \
    HQ_BLDG_WATCHTOWER   : 5000, \
    HQ_BLDG_TELEPORT     : 20000, \
    HQ_BLDG_SECRET_PORTAL : 100000, \
    HQ_BLDG_AURA_CONVERGE : 80000, \
    HQ_BLDG_CROSS_PORTAL  : 500000, \
])

// 建筑建设所需门派贡献
#define HQ_BLDG_CONTRIB ([ \
    HQ_BLDG_CORE         : 5000, \
    HQ_BLDG_TRAINING     : 3000, \
    HQ_BLDG_ALCHEMY      : 10000, \
    HQ_BLDG_FORGE        : 10000, \
    HQ_BLDG_MARKET       : 8000, \
    HQ_BLDG_HERB         : 5000, \
    HQ_BLDG_LIBRARY      : 20000, \
    HQ_BLDG_ARRAY_L1     : 15000, \
    HQ_BLDG_ARRAY_L2     : 50000, \
    HQ_BLDG_ARRAY_L3     : 200000, \
    HQ_BLDG_WATCHTOWER   : 3000, \
    HQ_BLDG_TELEPORT     : 8000, \
    HQ_BLDG_SECRET_PORTAL : 50000, \
    HQ_BLDG_AURA_CONVERGE : 30000, \
    HQ_BLDG_CROSS_PORTAL  : 200000, \
])

// 建筑建设时间（秒，对应设计文档 §4.4.1）
#define HQ_BLDG_BUILD_TIME ([ \
    HQ_BLDG_CORE         : 86400,    // 24h
    HQ_BLDG_TRAINING     : 43200,    // 12h
    HQ_BLDG_ALCHEMY      : 172800,   // 48h
    HQ_BLDG_FORGE        : 172800,   // 48h
    HQ_BLDG_MARKET       : 129600,   // 36h
    HQ_BLDG_HERB         : 86400,    // 24h
    HQ_BLDG_LIBRARY      : 259200,   // 72h
    HQ_BLDG_ARRAY_L1     : 172800,   // 48h
    HQ_BLDG_ARRAY_L2     : 604800,   // 7天
    HQ_BLDG_ARRAY_L3     : 2592000,  // 30天
    HQ_BLDG_WATCHTOWER   : 43200,    // 12h
    HQ_BLDG_TELEPORT     : 86400,    // 24h
    HQ_BLDG_SECRET_PORTAL : 604800,  // 7天
    HQ_BLDG_AURA_CONVERGE : 432000,  // 5天
    HQ_BLDG_CROSS_PORTAL  : 2592000, // 30天
])

//=============================================================================
// 建筑效果参数
//=============================================================================
#define HQ_TRAINING_BONUS       10   // 传功堂：弟子修炼+10%
#define HQ_ALCHEMY_SUCCESS      10   // 炼丹阁：炼丹成功率+10%
#define HQ_ALCHEMY_COST_REDUCE  10   // 炼丹阁：材料消耗-10%
#define HQ_FORGE_SUCCESS        10   // 炼器坊：炼器成功率+10%
#define HQ_FORGE_COST_REDUCE    10   // 炼器坊：材料消耗-10%
#define HQ_MARKET_TAX           5    // 坊市：交易抽税5%
#define HQ_HERB_EFFICIENCY      80   // 灵药园：品质≈个人洞府×0.8

//=============================================================================
// 驻地攻防战（对应设计文档 §4.3）
//=============================================================================

// 宣战条件
#define HQ_SIEGE_MIN_LEVEL_DIFF  -1   // 进攻方等级≥防守方等级-1
#define HQ_SIEGE_MAX_PER_WEEK    1    // 每周最多1次
#define HQ_SIEGE_DURATION        7200 // 攻防战持续2小时

// 备战时间（秒）
#define HQ_SIEGE_PREP_TIME       86400  // 24小时

// 攻防战阶段
#define HQ_SIEGE_PHASE_NONE      0    // 无
#define HQ_SIEGE_PHASE_DECLARE   1    // 宣战
#define HQ_SIEGE_PHASE_PREP      2    // 备战
#define HQ_SIEGE_PHASE_ACTIVE    3    // 激战中
#define HQ_SIEGE_PHASE_SETTLE    4    // 结算

// 攻防战目标点（按顺序突破）
#define HQ_SIEGE_TARGET_GATE     1    // 山门
#define HQ_SIEGE_TARGET_TOWER    2    // 外围哨塔
#define HQ_SIEGE_TARGET_NODE     3    // 护山大阵节点（3个）
#define HQ_SIEGE_TARGET_HALL     4    // 核心大殿

//=============================================================================
// 驻地耐久度与修复（对应设计文档 §4.3.3）
//=============================================================================
#define HQ_DURABILITY_FULL       10000 // 满耐久
#define HQ_DURABILITY_MAX        10000

// 破坏程度
#define HQ_DAMAGE_LIGHT         80    // 轻度：耐久≥80%
#define HQ_DAMAGE_MODERATE      50    // 中度：耐久≥50%
#define HQ_DAMAGE_HEAVY         20    // 重度：耐久≥20%
#define HQ_DAMAGE_CRITICAL      0     // 濒毁：耐久<20%

#define HQ_REPAIR_COST_LIGHT    1000    // 轻度修复成本
#define HQ_REPAIR_COST_MODERATE 5000    // 中度修复成本
#define HQ_REPAIR_COST_HEAVY    20000   // 重度修复成本
#define HQ_REPAIR_COST_CRITICAL 100000  // 濒毁修复成本

#define HQ_REPAIR_TIME_LIGHT    3600    // 1小时
#define HQ_REPAIR_TIME_MODERATE 43200   // 12小时
#define HQ_REPAIR_TIME_HEAVY    259200  // 3天
#define HQ_REPAIR_TIME_CRITICAL 604800  // 7天

//=============================================================================
// 驻地每日收益模型（对应设计文档 §4.4.2）
//=============================================================================
#define HQ_DAILY_TAX_RATE_MIN    0.5  // 最低税率0.5%
#define HQ_DAILY_TAX_RATE_MAX    2.0  // 最高税率2.0%

// 基础日产出
#define HQ_DAILY_OUTPUT ([ \
    HQ_TYPE_MOUNTAIN  : 1000, \
    HQ_TYPE_STANDARD  : 5000, \
    HQ_TYPE_CORE      : 20000, \
    HQ_TYPE_SANCTUARY : 100000, \
])

// 建筑效率系数（各建筑对日产出的贡献倍率）
#define HQ_BLDG_EFFICIENCY ([ \
    HQ_BLDG_CORE         : 1.0, \
    HQ_BLDG_TRAINING     : 0.5, \
    HQ_BLDG_ALCHEMY      : 1.5, \
    HQ_BLDG_FORGE        : 1.5, \
    HQ_BLDG_MARKET       : 2.0, \
    HQ_BLDG_HERB         : 1.0, \
    HQ_BLDG_LIBRARY      : 0.5, \
    HQ_BLDG_ARRAY_L1     : 0.3, \
    HQ_BLDG_ARRAY_L2     : 0.5, \
    HQ_BLDG_ARRAY_L3     : 1.0, \
    HQ_BLDG_WATCHTOWER   : 0.2, \
    HQ_BLDG_TELEPORT     : 0.5, \
    HQ_BLDG_SECRET_PORTAL : 3.0, \
    HQ_BLDG_AURA_CONVERGE : 2.0, \
    HQ_BLDG_CROSS_PORTAL  : 5.0, \
])

//=============================================================================
// 宗门驻地数据字段路径宏
//=============================================================================
#define HQ_FIELD_ID            "id"            // 宗门ID
#define HQ_FIELD_NAME          "name"          // 宗门名称
#define HQ_FIELD_TYPE          "type"          // 驻地类型（HQ_TYPE_*）
#define HQ_FIELD_MASTER        "master"        // 宗主ID
#define HQ_FIELD_MEMBERS       "members"       // 成员ID列表
#define HQ_FIELD_LEVEL         "level"         // 驻地等级
#define HQ_FIELD_STATUS        "status"        // 当前状态
#define HQ_FIELD_CREATED       "created"       // 建立时间
#define HQ_FIELD_DURABILITY    "durability"    // 当前耐久度
#define HQ_FIELD_BUILDINGS     "buildings"     // 已建造建筑
#define HQ_FIELD_CONSTRUCTING  "constructing"  // 在建建筑
#define HQ_FIELD_FUNDS         "funds"         // 宗门资金
#define HQ_FIELD_TAX_RATE      "tax_rate"      // 税率
#define HQ_FIELD_LOCATION      "location"      // 驻地位置
#define HQ_FIELD_SIEGE         "siege"         // 攻防战信息
#define HQ_FIELD_LAST_SIEGE    "last_siege"    // 上次攻防战时间
#define HQ_FIELD_REPUTE        "repute"        // 宗门声望

// 建筑数据结构
// building_id -> ([
//     "built" : time(),    // 建造时间
//     "level" : 1,         // 等级
//     "status" : 1,        // 1=正常, 0=受损暂停
// ])

// 攻防战数据结构
// ([
//     "phase"     : HQ_SIEGE_PHASE_*,
//     "attacker"  : 进攻方宗门ID,
//     "declared"  : time(),
//     "started"   : time(),
//     "progress"  : 突破目标列表,
//     "node_hp"   : ([ 1:hp, 2:hp, 3:hp ]),  // 阵法节点HP
// ])

#endif
