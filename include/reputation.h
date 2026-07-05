// reputation.h
// 声望系统常量与数据结构定义

#ifndef __REPUTATION__
#define __REPUTATION__

// 六大势力定义（声望维度）
// 每个势力有独立声望值，保存在玩家 query("reputation/<faction_id>")
#define FACTION_RIGHTEOUS       "righteous"         // 正道联盟（越国七派等）
#define FACTION_EVIL            "evil"              // 魔道六宗
#define FACTION_STAR_PALACE     "star_palace"       // 星宫（乱星海）
#define FACTION_REBEL_ALLIANCE  "rebel_alliance"    // 逆星盟（乱星海）
#define FACTION_DEMON_RACE      "demon_race"        // 灵界妖族
#define FACTION_JIAOCHI         "jiaochi"           // 角蚩族

// 声望等级：-3 ~ 5
#define REP_LEVEL_ARCHENEMY     -3   // 死敌
#define REP_LEVEL_HOSTILE       -2   // 敌对
#define REP_LEVEL_COLD          -1   // 冷淡
#define REP_LEVEL_NEUTRAL       0    // 中立
#define REP_LEVEL_FRIENDLY      1    // 友善
#define REP_LEVEL_TRUST         2    // 信任
#define REP_LEVEL_RESPECT       3    // 尊敬
#define REP_LEVEL_ADORE         4    // 崇拜
#define REP_LEVEL_LEGEND        5    // 传说

// 声望等级阈值（达到该值即升级）
#define REP_THRESHOLD_ARCHENEMY  -10000  // <= 死敌
#define REP_THRESHOLD_HOSTILE    -2000   // <= 敌对
#define REP_THRESHOLD_COLD       -101    // <= 冷淡
#define REP_THRESHOLD_NEUTRAL    100     // < 友善
#define REP_THRESHOLD_FRIENDLY   2000    // < 信任
#define REP_THRESHOLD_TRUST      8000    // < 尊敬
#define REP_THRESHOLD_RESPECT    25000   // < 崇拜
#define REP_THRESHOLD_ADORE      80000   // < 传说
// >= REP_THRESHOLD_ADORE = 传说

// 声望值范围
#define REP_MIN_VALUE            -100000
#define REP_MAX_VALUE            100000

// 每日声望获取上限（单势力），按境界
#define REP_DAILY_CAP_QI_LIAN		   500     // 炼气
#define REP_DAILY_CAP_ZHU_JI		  2000     // 筑基
#define REP_DAILY_CAP_JIE_DAN		  6000     // 结丹
#define REP_DAILY_CAP_YUAN_YING	 20000     // 元婴
#define REP_DAILY_CAP_HUA_SHEN	 50000     // 化神
#define REP_DAILY_CAP_LIAN_XU	  100000     // 炼虚+

// 每日总声望上限（全势力）
#define REP_DAILY_TOTAL_QI_LIAN		  2000
#define REP_DAILY_TOTAL_ZHU_JI		  8000
#define REP_DAILY_TOTAL_JIE_DAN	 25000
#define REP_DAILY_TOTAL_YUAN_YING	 80000
#define REP_DAILY_TOTAL_HUA_SHEN	200000
#define REP_DAILY_TOTAL_LIAN_XU	  500000

// 互斥关系强度
#define REP_MUTEX_STRONG   0.30   // 强互斥：扣减30%
#define REP_MUTEX_WEAK     0.10   // 弱互斥：扣减10%

// 声望衰减常量
#define REP_DECAY_DAYS_TRIGGER    7    // 7天无互动开始触发衰减
#define REP_DECAY_LEGEND         10    // 传说级：每日-10
#define REP_DECAY_ADORE          10    // 崇拜级：每日-10
#define REP_DECAY_RESPECT         5    // 尊敬级：每日-5
#define REP_DECAY_TRUST           3    // 信任级：每日-3
#define REP_DECAY_FRIENDLY        1    // 友善级：每日-1
#define REP_DECAY_NEGATIVE       -2    // 负声望：每日回升+2（向中立靠拢）

// 重复惩罚
#define REP_REPEAT_PENALTY_1ST   1.00  // 第1次：正常
#define REP_REPEAT_PENALTY_2ND   0.90  // 第2次：90%
#define REP_REPEAT_PENALTY_3RD   0.70  // 第3次：70%
#define REP_REPEAT_PENALTY_4TH   0.50  // 第4次：50%
#define REP_REPEAT_PENALTY_5TH   0.30  // 第5次+：30%

// 效率修正（以友善→信任阶段为基准1.0）
#define REP_EFF_NEWBIE           2.0   // 中立→友善：快速入门
#define REP_EFF_NORMAL           1.0   // 友善→信任：平稳阶段
#define REP_EFF_SLOW             0.6   // 信任→尊敬：减速区
#define REP_EFF_DEEP             0.3   // 尊敬→崇拜：深度区
#define REP_EFF_BOTTLENECK       0.1   // 崇拜以上：瓶颈区

#endif
