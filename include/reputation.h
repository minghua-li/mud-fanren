// reputation.h
// 声望与阵营交互系统 - 常量定义
// 对应设计文档 02-扩充内容/02-声望与互动玩法.md

#ifndef __REPUTATION_H__
#define __REPUTATION_H__

// 声望等级（-3 ~ 5）
#define REP_LEVEL_DEADLY      -3   // 死敌
#define REP_LEVEL_HOSTILE     -2   // 敌对
#define REP_LEVEL_COLD        -1   // 冷淡
#define REP_LEVEL_NEUTRAL      0   // 中立
#define REP_LEVEL_FRIENDLY     1   // 友善
#define REP_LEVEL_TRUST        2   // 信任
#define REP_LEVEL_RESPECT      3   // 尊敬
#define REP_LEVEL_ADORE        4   // 崇拜
#define REP_LEVEL_LEGENDARY    5   // 传说

// 声望等级数值阈值
#define REP_VALUE_DEADLY       -10000
#define REP_VALUE_HOSTILE      -2000
#define REP_VALUE_COLD         -101
#define REP_VALUE_NEUTRAL_LOW  -100
#define REP_VALUE_NEUTRAL_HIGH  100
#define REP_VALUE_FRIENDLY      101
#define REP_VALUE_TRUST         2001
#define REP_VALUE_RESPECT       8001
#define REP_VALUE_ADORE         25001
#define REP_VALUE_LEGENDARY     80001

// 声望等级名称
#define REP_NAME_DEADLY       "死敌"
#define REP_NAME_HOSTILE      "敌对"
#define REP_NAME_COLD         "冷淡"
#define REP_NAME_NEUTRAL      "中立"
#define REP_NAME_FRIENDLY     "友善"
#define REP_NAME_TRUST        "信任"
#define REP_NAME_RESPECT      "尊敬"
#define REP_NAME_ADORE        "崇拜"
#define REP_NAME_LEGENDARY    "传说"

// 声望等级颜色
#define REP_COLOR_DEADLY      HIR
#define REP_COLOR_HOSTILE     HIR
#define REP_COLOR_COLD        HIW
#define REP_COLOR_NEUTRAL     NOR
#define REP_COLOR_FRIENDLY    HIG
#define REP_COLOR_TRUST       HIB
#define REP_COLOR_RESPECT     HIM
#define REP_COLOR_ADORE       HIY
#define REP_COLOR_LEGENDARY   HIC

// 势力类型
#define FACTION_TYPE_RIGHTEOUS   1   // 正道阵营
#define FACTION_TYPE_EVIL        2   // 魔道阵营
#define FACTION_TYPE_NEUTRAL     3   // 中立阵营
#define FACTION_TYPE_ORGANIZATION 4  // 独立组织
#define FACTION_TYPE_SECT        5   // 具体门派

// 互斥强度
#define MUTEX_STRONG    0   // 强互斥（系数 0.3）
#define MUTEX_WEAK      1   // 弱互斥（系数 0.1）

// 阵营交互类型
#define FACTION_ACTION_ALLY      1  // 结盟
#define FACTION_ACTION_HOSTILE   2  // 敌对
#define FACTION_ACTION_NEUTRAL   3  // 中立
#define FACTION_ACTION_TRADE     4  // 交易
#define FACTION_ACTION_QUEST     5  // 任务
#define FACTION_ACTION_TRAIN     6  // 修炼

// 商店层级
#define SHOP_TIER_BASIC      0   // 基础商店 - 中立解锁
#define SHOP_TIER_INTERMEDIATE 1 // 中级商店 - 友善解锁
#define SHOP_TIER_ADVANCED   2   // 高级商店 - 信任解锁
#define SHOP_TIER_CORE       3   // 核心宝库 - 尊敬解锁
#define SHOP_TIER_SECRET     4   // 秘密仓库 - 崇拜解锁

// 商店层级名称
#define SHOP_TIER_NAME_BASIC       "基础商店"
#define SHOP_TIER_NAME_INTERMEDIATE "中级商店"
#define SHOP_TIER_NAME_ADVANCED    "高级商店"
#define SHOP_TIER_NAME_CORE        "核心宝库"
#define SHOP_TIER_NAME_SECRET      "秘密仓库"

// 声望维度的存储路径前缀（玩家数据）
#define REP_PATH_FACTION     "reputation/faction"
#define REP_PATH_RACE        "reputation/race"
#define REP_PATH_REGION      "reputation/region"
#define REP_PATH_GLOBAL      "reputation/global"
#define REP_PATH_LAST_INTERACT "reputation/last_interact"
#define REP_PATH_FACTION_REL  "reputation/faction_relation"

// 每日声望获取上限（按境界）
#define REP_DAILY_CAP_QIYIN       500     // 炼气
#define REP_DAILY_CAP_ZHUIJI      2000    // 筑基
#define REP_DAILY_CAP_JIEDAN      6000    // 结丹
#define REP_DAILY_CAP_YUANYING    20000   // 元婴
#define REP_DAILY_CAP_HUASHEN     50000   // 化神
#define REP_DAILY_CAP_LIANXU      100000  // 炼虚+

// 声望折扣率（按等级）
#define REP_DISCOUNT_DEADLY     -1.0    // 死敌→无法交易
#define REP_DISCOUNT_HOSTILE     3.0    // 敌对→3倍价
#define REP_DISCOUNT_COLD        2.0    // 冷淡→2倍价
#define REP_DISCOUNT_NEUTRAL     1.0    // 中立→原价
#define REP_DISCOUNT_FRIENDLY    0.95   // 友善→95折
#define REP_DISCOUNT_TRUST       0.90   // 信任→9折
#define REP_DISCOUNT_RESPECT     0.80   // 尊敬→8折
#define REP_DISCOUNT_ADORE       0.60   // 崇拜→6折
#define REP_DISCOUNT_LEGENDARY   0.50   // 传说→5折

// 势力繁荣度修正
#define PROSPERITY_DECLINE   -2    // 衰败 +20%
#define PROSPERITY_HARD      -1    // 困难 +10%
#define PROSPERITY_PEAK       0    // 鼎盛 无修正
#define PROSPERITY_EXPAND     1    // 扩张 -5%

#define PROSPERITY_MOD_DECLINE  1.20
#define PROSPERITY_MOD_HARD     1.10
#define PROSPERITY_MOD_PEAK     1.00
#define PROSPERITY_MOD_EXPAND   0.95

// 种族初始关系
#define RACE_RELATION_ALLY      1    // 友善
#define RACE_RELATION_NEUTRAL   0    // 中立
#define RACE_RELATION_HOSTILE  -1    // 敌对
#define RACE_RELATION_DEADLY   -2    // 死敌

// 种族初始关系名称
#define RACE_REL_NAME_ALLY     "友善"
#define RACE_REL_NAME_NEUTRAL  "中立"
#define RACE_REL_NAME_HOSTILE  "不友好"
#define RACE_REL_NAME_DEADLY   "敌对"

#endif
