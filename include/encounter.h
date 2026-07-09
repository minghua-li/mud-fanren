// encounter.h
// 奇遇/隐藏任务系统 — 常量、类型定义
// 基于: 02-扩充内容/02-任务链与奖励曲线.md
//
// 奇遇 = 随机概率触发的稀有任务
// 隐藏任务 = 特定条件解锁的隐藏任务

#ifndef __ENCOUNTER__
#define __ENCOUNTER__

// ─── 奇遇类型 ───
#define ENC_RANDOM_EVENT    1   // 随机事件（野外/副本随机触发）
#define ENC_HIDDEN_QUEST    2   // 隐藏任务（特定行为/条件触发）
#define ENC_SPECIAL_NPC     3   // 特殊NPC（特定地点/时间出现）

// ─── 触发条件类型 ───
#define ENC_COND_REALM      1   // 境界要求
#define ENC_COND_LEVEL      2   // 等级要求
#define ENC_COND_ITEM       3   // 持有物品
#define ENC_COND_TIME       4   // 游戏内时间
#define ENC_COND_WEATHER    5   // 天气
#define ENC_COND_COMBAT     6   // 战斗表现
#define ENC_COND_QUEST      7   // 已完成某任务

// ─── 稀有度级别 ───
#define ENC_RARE_COMMON     1   // 普通奇遇（概率10%，普通奖励）
#define ENC_RARE_UNCOMMON   2   // 优秀奇遇（概率3%，较好奖励）
#define ENC_RARE_RARE       3   // 稀有奇遇（概率1%，稀有奖励）
#define ENC_RARE_LEGENDARY  4   // 传说奇遇（概率0.1%，极品奖励）

// ─── 奇遇状态 ───
#define ENC_STATUS_INACTIVE     0   // 未触发
#define ENC_STATUS_ACTIVE       1   // 已触发/进行中
#define ENC_STATUS_COMPLETED    2   // 已完成
#define ENC_STATUS_EXPIRED      3   // 已过期（限时奇遇超时）

// ─── 概率常量 ───
#define ENC_BASE_PROB_COMMON    10      // 普通奇遇基础概率（万分比）
#define ENC_BASE_PROB_UNCOMMON  3       // 优秀奇遇基础概率
#define ENC_BASE_PROB_RARE      1       // 稀有奇遇概率
#define ENC_BASE_PROB_LEGENDARY 0.1     // 传说奇遇概率

// ─── 玩家数据存储key ───
#define ENCOUNTER_DATA      "encounter"
#define ENCOUNTER_ACTIVE    "encounter/active"
#define ENCOUNTER_COMPLETED "encounter/completed"
#define ENCOUNTER_HISTORY   "encounter/history"

// ─── 奇遇追踪器 ───
#define ENC_HISTORY_MAX     50  // 最多保存50条奇遇记录

// ─── 冷却时间 ───
#define ENC_COOLDOWN_BASE   300     // 奇遇冷却基础时间（秒，5分钟）
#define ENC_COOLDOWN_RARE   3600    // 稀有奇遇冷却（1小时）
#define ENC_COOLDOWN_LEGEND 86400   // 传说奇遇冷却（1天）

#endif // __ENCOUNTER__
