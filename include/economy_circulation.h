// economy_circulation.h
// P3集成 A：经济循环基础设施 —— 六大循环的消耗/产出常量与接口类型定义
// Created for ticket #48
//
// 参考设计文档：
//   02-经济与资源.md （经济底层模型、消耗通道、全局收支）
//   02-灵根养成与突破.md （灵根修炼消耗）
//   02-战斗机制与平衡.md （阵法维护、装备维修、比武报名）
//   02-区域游戏玩法.md （传送、秘境、洞府、宗门驻地）
//   02-任务链与奖励曲线.md （任务奖励注入、活跃度约束）
//   .knowledge/factions/1D-门派种族声望.md （势力产业税收、声望门槛）

#ifndef __ECONOMY_CIRCULATION_H__
#define __ECONOMY_CIRCULATION_H__

//=============================================================================
// 六大循环标识常量（用于 add_production / add_consumption 的 source/sink 参数）
//=============================================================================

// A1 经济→修炼循环
#define EC_SOURCE_CULTIVATION   "circulation:cultivation"    // 修炼产出（修为转化）
#define EC_SINK_SPIRIT_INFUSE   "circulation:spirit_infuse"  // 灵石灌注消耗
#define EC_SINK_POLY_ARRAY      "circulation:poly_array"     // 聚灵阵消耗

// A2 经济→战斗循环
#define EC_SINK_FORMATION       "circulation:formation"      // 阵法维持消耗
#define EC_SINK_EQUIP_REPAIR    "circulation:equip_repair"   // 装备维修消耗
#define EC_SINK_PVP_DEATH       "circulation:pvp_death"      // PVP死亡惩罚
#define EC_SINK_ARENA_FEE       "circulation:arena_fee"      // 比武报名费

// A3 经济→区域循环
#define EC_SINK_TELEPORT        "circulation:teleport"       // 传送阵费用
#define EC_SINK_SECRET_REALM    "circulation:secret_realm"   // 秘境门票
#define EC_SINK_MANSION_UPKEEP  "circulation:mansion_upkeep" // 洞府维护费
#define EC_SINK_SECT_HQ_UPKEEP  "circulation:sect_hq_upkeep" // 宗门驻地维护

// A4 经济→任务循环
#define EC_SOURCE_QUEST_REWARD  "circulation:quest_reward"   // 任务奖励注入
#define EC_SINK_QUEST_PENALTY   "circulation:quest_penalty"  // 任务失败惩罚
#define EC_SOURCE_ACTIVITY      "circulation:activity_reward"// 活跃度奖励

// A5 经济→声望循环
#define EC_SOURCE_FACTION_TASK  "circulation:faction_task"   // 势力任务奖励
#define EC_SINK_REPUT_SPEND     "circulation:reput_spend"    // 声望消费门槛
#define EC_SOURCE_INDUSTRY_TAX  "circulation:industry_tax"   // 势力产业税收

// A6 经济自身循环
#define EC_SINK_MARKET_TAX      "circulation:market_tax"     // 交易税
#define EC_SINK_AUCTION_FEE     "circulation:auction_fee"    // 拍卖手续费
#define EC_SINK_SYSTEM_BUYBACK  "circulation:system_buyback" // 系统回收
#define EC_SINK_DEATH_PENALTY   "circulation:death_penalty"  // 死亡丢失

//=============================================================================
// A1：经济→修炼 循环常量
//=============================================================================

// 灵石-修为转化率（每灵石兑换的修为值，基准）
#define SPIRIT_TO_EXP_BASE      10      // 1 下品灵石 = 10 修为值

// 聚灵阵基础费用（下品灵石/时辰）
#define POLY_ARRAY_COST_BASE    1       // 炼气基准：1 灵石/时辰
// 聚灵阵境界倍率
// 炼气×1，筑基×3，结丹×10，元婴×30，化神+×80
#define POLY_ARRAY_REALM_MOD  ([ \
    "qige"     : 1,    \
    "zhuji"    : 3,    \
    "jiedan"   : 10,   \
    "yuanying" : 30,   \
    "huashen"  : 80,   \
])

// 聚灵阵修炼速度增益（倍数，相对于基础修炼速度）
#define POLY_ARRAY_SPEED_BOOST  1.5     // 开启聚灵阵后修炼速度×1.5

//=============================================================================
// A2：经济→战斗 循环常量
//=============================================================================

// 阵法维持消耗（下品灵石/回合）
#define FORMATION_COST_PER_ROUND  2     // 结丹基准：2 灵石/回合

// 装备维修费用比例（与购买价的百分比）
#define EQUIP_REPAIR_RATIO_LOW     20   // 法器类：购买价的 20%
#define EQUIP_REPAIR_RATIO_MID     30   // 法宝类：购买价的 30%
#define EQUIP_REPAIR_RATIO_HIGH    50   // 古宝类：购买价的 50%

// PVP死亡惩罚比例（损失携带灵石的百分比）
#define PVP_DEATH_LOSS_MIN      10      // 最低 10%
#define PVP_DEATH_LOSS_MAX      30      // 最高 30%

// 比武大会报名费（下品灵石，按段位）
#define ARENA_FEE_RANK ([ \
    0 : 50,     /* 第1阶：炼气散修     */ \
    1 : 100,    /* 第2阶：筑基修士     */ \
    2 : 200,    /* 第3阶：结丹真人     */ \
    3 : 500,    /* 第4阶：元婴老怪     */ \
    4 : 1000,   /* 第5阶：化神圣君     */ \
    5 : 2000,   /* 第6阶：大乘仙尊     */ \
    6 : 5000,   /* 第7阶：渡劫真仙     */ \
])

//=============================================================================
// A3：经济→区域 循环常量
//=============================================================================

// 传送阵基础费用（下品灵石）
#define TELEPORT_COST_CITY          50      // 城内瞬移
#define TELEPORT_COST_INTERCITY     100     // 城际
#define TELEPORT_COST_CROSSBORDER   1000    // 跨境
#define TELEPORT_COST_CROSSCONT     100000  // 跨大陆

// 传送阵距离系数
#define TELEPORT_DIST_NEARBY        1       // 邻近区域
#define TELEPORT_DIST_SAMELAND      3       // 同大陆
#define TELEPORT_DIST_CROSSLAND     10      // 跨大陆
#define TELEPORT_DIST_CROSSREALM    50      // 跨界

// 秘境门票（下品灵石，按秘境等级）
#define SECRET_REALM_FEE_LOW        50      // Lv.1（炼气）
#define SECRET_REALM_FEE_MID        500     // Lv.2（筑基）
#define SECRET_REALM_FEE_HIGH       2000    // Lv.3（结丹）
#define SECRET_REALM_FEE_TOP        10000   // Lv.4（元婴+）

// 洞府维护费（下品灵石/周，按等级）
#define MANSION_UPKEEP_LV1          100     // 初级洞府
#define MANSION_UPKEEP_LV2          500     // 中级洞府
#define MANSION_UPKEEP_LV3          2000    // 高级洞府
#define MANSION_UPKEEP_LV4          10000   // 顶级洞府

// 宗门驻地维护费（下品灵石/周，按类型）
#define SECT_HQ_UPKEEP_SMALL        1000    // 小型宗门
#define SECT_HQ_UPKEEP_MEDIUM       5000    // 中型宗门
#define SECT_HQ_UPKEEP_LARGE        20000   // 大型宗门
#define SECT_HQ_UPKEEP_LEGENDARY    100000  // 传奇宗门

//=============================================================================
// A4：经济→任务 循环常量
//=============================================================================

// 任务奖励灵石基数（下品灵石，对应设计文档 §3.2 各境界月收入修正）
#define QUEST_REWARD_BASE_QI        5       // 炼气任务基础
#define QUEST_REWARD_BASE_ZHU       20      // 筑基任务基础
#define QUEST_REWARD_BASE_JIE       80      // 结丹任务基础
#define QUEST_REWARD_BASE_YING      300     // 元婴任务基础
#define QUEST_REWARD_BASE_HUA       1000    // 化神+任务基础

// 任务品质倍率
#define QUEST_QUALITY_NORMAL        1.0     // 普通
#define QUEST_QUALITY_GOOD          1.5     // 优秀
#define QUEST_QUALITY_RARE          3.0     // 稀有

// 每日活跃度奖励系数（基于在线分钟数）
// 对应设计文档 §1.1 活跃度修正系数
#define ACTIVITY_MOD_LOW            0.3     // <30分钟
#define ACTIVITY_MOD_MEDIUM         0.8     // 30-120分钟
#define ACTIVITY_MOD_STANDARD       1.0     // 120-240分钟
#define ACTIVITY_MOD_HIGH           1.1     // >240分钟

// 任务失败灵石惩罚（占任务奖励的百分比）
#define QUEST_FAIL_PENALTY_RATIO    20      // 20%

//=============================================================================
// A5：经济→声望 循环常量
//=============================================================================

// 势力任务奖励基数（下品灵石，按声望等级）
#define FACTION_TASK_REWARD_NEUTRAL     10      // 中立
#define FACTION_TASK_REWARD_FRIENDLY    30      // 友善
#define FACTION_TASK_REWARD_TRUST       80      // 信任
#define FACTION_TASK_REWARD_RESPECT     200     // 尊敬
#define FACTION_TASK_REWARD_ADORE       500     // 崇拜
#define FACTION_TASK_REWARD_LEGENDARY   1500    // 传说

// 声望等级消费折扣率（对基础价，与 reputation.h 对齐）
#define REPUT_DISCOUNT_DEADLY     -1.00   // 死敌：无法交易
#define REPUT_DISCOUNT_HOSTILE     3.00   // 敌对：3倍价
#define REPUT_DISCOUNT_COLD        2.00   // 冷淡：2倍价
#define REPUT_DISCOUNT_NEUTRAL     1.00   // 中立：原价
#define REPUT_DISCOUNT_FRIENDLY    0.95   // 友善：95折
#define REPUT_DISCOUNT_TRUST       0.90   // 信任：9折
#define REPUT_DISCOUNT_RESPECT     0.80   // 尊敬：8折
#define REPUT_DISCOUNT_ADORE       0.60   // 崇拜：6折
#define REPUT_DISCOUNT_LEGENDARY   0.50   // 传说：5折

// 势力产业税收比例
#define FACTION_TAX_RATE_BASIC     5       // 坊市交易税 5%
#define FACTION_TAX_RATE_MINE      15      // 矿脉抽成 15%
#define FACTION_TAX_RATE_AUCTION   10      // 拍卖行抽成 10%

//=============================================================================
// A6：经济自身循环 常量
//=============================================================================

// 市场交易税率（千分比，与 inflationd.c 对齐）
#define MARKET_TAX_BASE             50      // 基准税率 5%
#define MARKET_TAX_WARN             80      // 轻度上调 8%
#define MARKET_TAX_ALARM            120     // 中度上调 12%
#define MARKET_TAX_CRISIS           200     // 重度上调 20%

// 拍卖行手续费率（百分比）
#define AUCTION_FEE_RATE            10      // 成交价的 10%

// 系统回收价比例（对市场价的百分比）
#define SYSTEM_BUYBACK_RATIO        50      // 玩家卖给系统 = 市场价的 50%

// 产出/消耗比健康区间（与 inflationd.c 对齐）
#define OUTPUT_CONSUMPTION_MIN      0.95
#define OUTPUT_CONSUMPTION_MAX      1.05

#endif  // __ECONOMY_CIRCULATION_H__
