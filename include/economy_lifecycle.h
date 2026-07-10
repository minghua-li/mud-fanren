// economy_lifecycle.h
// 经济循环生命周期系统 — 常量定义
// 对应设计文档 02-扩充内容/02-经济与资源.md §5-§7
// 创建于 #48 P3 集成 A4-A6
//
// 职责:
//   为 quest_economy_d.c / faction_economy_d.c / economyd.c 提供共享常量
//   确保经济→任务、经济→声望、经济自身循环三组接口使用统一的数值框架

#ifndef __ECONOMY_LIFECYCLE_H__
#define __ECONOMY_LIFECYCLE_H__

//=============================================================================
// 一、任务奖励经济约束 (A4 — 经济→任务循环)
//=============================================================================

// 任务灵石产出占全服总产出的比例上限（按设计文档 §6.3）
#define QUEST_COIN_PCT_CAP   0.45    // 任务灵石占总产出 ≤45%

// 每日预算分配比例（按境界）
#define BUDGET_ALLOC_QIGE      0.25   // 炼气期占 25%
#define BUDGET_ALLOC_ZHUJI     0.30   // 筑基期占 30%
#define BUDGET_ALLOC_JIEDAN    0.25   // 结丹期占 25%
#define BUDGET_ALLOC_YUANYING  0.15   // 元婴期占 15%
#define BUDGET_ALLOC_HUASHEN   0.05   // 化神期+占 5%

// 各渠道任务奖励占比（相对任务灵石总量）
#define QUEST_DAILY_COIN_PCT   0.40   // 日常任务 ≤40%
#define QUEST_MAIN_COIN_PCT    0.20   // 主线任务 ≤20%
#define QUEST_SIDE_COIN_PCT    0.15   // 支线任务 ≤15%
#define QUEST_WEEKLY_COIN_PCT  0.15   // 周常任务 ≤15%
#define QUEST_OTHER_COIN_PCT   0.10   // 其他 ≤10%

// 每日任务灵石预算系数（按境界，下品灵石/天）
// 基于设计文档 §6.3: 日常灵石产出上限 = 境界基准×系数
#define QUEST_DAILY_BUDGET_QIGE      50     // 炼气期 50/天
#define QUEST_DAILY_BUDGET_ZHUJI     200    // 筑基期 200/天
#define QUEST_DAILY_BUDGET_JIEDAN    1000   // 结丹期 1000/天
#define QUEST_DAILY_BUDGET_YUANYING  5000   // 元婴期 5000/天
#define QUEST_DAILY_BUDGET_HUASHEN   20000  // 化神期 20000/天

// 玩家每日任务灵石获取上限（防刷）
#define QUEST_DAILY_PLAYER_CAP_QIGE      20    // 炼气
#define QUEST_DAILY_PLAYER_CAP_ZHUJI     80    // 筑基
#define QUEST_DAILY_PLAYER_CAP_JIEDAN    400   // 结丹
#define QUEST_DAILY_PLAYER_CAP_YUANYING  2000  // 元婴
#define QUEST_DAILY_PLAYER_CAP_HUASHEN   8000  // 化神+

// 活跃度修正系数（与设计文档 §1.1 一致）
#define ACTIVITY_MOD_MIN      0.3    // < 30分钟
#define ACTIVITY_MOD_LOW      0.8    // 30-120分钟
#define ACTIVITY_MOD_NORMAL   1.0    // 120-240分钟
#define ACTIVITY_MOD_HIGH     1.1    // > 240分钟（边际递减）

// 任务失败惩罚比例
#define QUEST_FAIL_COIN_PENALTY    0.50   // 灵石惩罚 = 奖励的 50%
#define QUEST_FAIL_REP_PENALTY     0.30   // 声望惩罚 = 奖励的 30%

//=============================================================================
// 二、声望经济系统 (A5 — 经济→声望循环)
//=============================================================================

// 势力任务灵石注入比例（任务奖励中灵石占声望价值的比例）
#define FACTION_TASK_COIN_RATIO     0.60   // 势力任务灵石占总值 60%
#define FACTION_TASK_REP_RATIO      0.40   // 声望占总值 40%

// 声望等级消费阈值（需达到指定等级才能购买对应层级商品）
// 对应 reputation.h 中的 SHOP_TIER_* 定义
#define REP_THRESHOLD_BUY_BASIC     0      // 基础购买： 中立(0)
#define REP_THRESHOLD_BUY_MID       1      // 中级购买： 友善(1)
#define REP_THRESHOLD_BUY_ADV       2      // 高级购买： 信任(2)
#define REP_THRESHOLD_BUY_CORE      3      // 核心购买： 尊敬(3)
#define REP_THRESHOLD_BUY_SECRET    4      // 秘密购买： 崇拜(4)

// 声望等级解锁的经济特权
#define REP_SPENDING_DISCOUNT_NEUTRAL   1.00  // 中立：无折扣
#define REP_SPENDING_DISCOUNT_FRIENDLY  0.95  // 友善：95折
#define REP_SPENDING_DISCOUNT_TRUST     0.90  // 信任：9折
#define REP_SPENDING_DISCOUNT_RESPECT   0.80  // 尊敬：8折
#define REP_SPENDING_DISCOUNT_ADORE     0.60  // 崇拜：6折

//=============================================================================
// 三、势力产业税收 (A5 — 势力产业)
//=============================================================================

// 势力产业类型
#define FACTION_INDUSTRY_MINE      "mine"       // 矿脉
#define FACTION_INDUSTRY_MARKET    "market"     // 坊市
#define FACTION_INDUSTRY_AUCTION   "auction"    // 拍卖行
#define FACTION_INDUSTRY_LINGMAI   "lingmai"    // 灵脉
#define FACTION_INDUSTRY_HALL      "hall"       // 驻地大厅

// 基础税率（千分比）
#define TAX_MARKET_BASE           50    // 坊市交易税 5%
#define TAX_AUCTION_BASE          100   // 拍卖行手续费 10%
#define TAX_MINE_BASE             20    // 矿脉开采税 2%
#define TAX_LINGMAI_BASE          30    // 灵脉使用税 3%

// 税率动态调节范围（基于通胀状态）
#define TAX_MOD_HEALTHY           1.0   // 健康：基准税率
#define TAX_MOD_WARNING           1.3   // 预警：税率 ×1.3
#define TAX_MOD_CRITICAL          1.8   // 危机：税率 ×1.8

// 势力产业收入分配比例
#define FACTION_INCOME_FACTION    0.40   // 40% 入势力金库
#define FACTION_INCOME_CONTRIBUTE 0.30   // 30% 按贡献分配给成员
#define FACTION_INCOME_MAINTENANCE 0.20  // 20% 用于驻地维护
#define FACTION_INCOME_SYSTEM     0.10   // 10% 系统回收（防通胀）

//=============================================================================
// 四、经济自身循环指标 (A6 — 经济自身循环跑通)
//=============================================================================

// 经济健康指标阈值（全服人均灵石，下品灵石）
// 与 inflationd.c 保持一致
#define ECON_HEALTHY_MAX         300    // 健康上限
#define ECON_WARNING_MAX         500    // 预警上限
#define ECON_ALARM_MAX           800    // 危险上限

// 产出/消耗比健康区间
#define ECON_RATIO_MIN           0.95   // 产出不足下限
#define ECON_RATIO_MAX           1.05   // 产出过剩上限
#define ECON_RATIO_CRISIS_LOW    0.85   // 严重不足阈值
#define ECON_RATIO_CRISIS_HIGH   1.15   // 严重过剩阈值

// 灵石回收通道权重（数值越大,该通道回收比例越高）
#define SINK_TAX_WEIGHT          30     // 交易税 30%
#define SINK_AUCTION_WEIGHT      15     // 拍卖行 15%
#define SINK_REPAIR_WEIGHT       15     // 装备维修 15%
#define SINK_TRANSPORT_WEIGHT    10     // 传送费 10%
#define SINK_TRAINING_WEIGHT     20     // 修炼消耗 20%
#define SINK_DEATH_PENALTY       5      // 死亡惩罚 5%
#define SINK_CRAFT_FAIL          5      // 制作失败 5%

// 系统回收/产出比目标（设计文档 §7.4）
#define RECOVERY_TARGET_MIN      0.40   // 最低 40%
#define RECOVERY_TARGET_MAX      0.60   // 最高 60%

// 通货膨胀自动调节系数
#define AUTO_ADJUST_PRODUCTION   0.05   // 每次调节调整产出 ±5%
#define AUTO_ADJUST_CONSUMPTION  0.05   // 每次调节调整消耗 ±5%
#define AUTO_ADJUST_INTERVAL     3600   // 每小时检查一次

// 经济事件冷却期（秒）
#define EVENT_COOLDOWN_CRISIS    604800 // 7天
#define EVENT_COOLDOWN_WARNING   604800 // 7天

// 产出增益系数（供任务奖励系统参考）
#define OUTPUT_BOOST_SHORTAGE    1.20   // 产出不足时 +20%
#define OUTPUT_BOOST_SURPLUS     0.85   // 产出过剩时 -15%
#define OUTPUT_BOOST_NORMAL      1.00   // 正常

//=============================================================================
// 五、境界索引工具宏
//=============================================================================

// 根据境界索引获取每日任务灵石预算
#define QUEST_DAILY_BUDGET(realm_idx) \
  (realm_idx <= 0 ? QUEST_DAILY_BUDGET_QIGE : \
   realm_idx == 1 ? QUEST_DAILY_BUDGET_ZHUJI : \
   realm_idx == 2 ? QUEST_DAILY_BUDGET_JIEDAN : \
   realm_idx == 3 ? QUEST_DAILY_BUDGET_YUANYING : \
   QUEST_DAILY_BUDGET_HUASHEN)

// 根据境界索引获取玩家每日灵石上限
#define QUEST_DAILY_PLAYER_CAP(realm_idx) \
  (realm_idx <= 0 ? QUEST_DAILY_PLAYER_CAP_QIGE : \
   realm_idx == 1 ? QUEST_DAILY_PLAYER_CAP_ZHUJI : \
   realm_idx == 2 ? QUEST_DAILY_PLAYER_CAP_JIEDAN : \
   realm_idx == 3 ? QUEST_DAILY_PLAYER_CAP_YUANYING : \
   QUEST_DAILY_PLAYER_CAP_HUASHEN)

#endif // __ECONOMY_LIFECYCLE_H__
