// side_quest.h
// 支线任务系统 — 常量、类型定义、数据结构
// 基于: 02-扩充内容/02-任务链与奖励曲线.md 第三章
//
// 支线任务类型：角色支线 / 场景支线 / 收集支线 / 隐藏支线 / 门派支线

#ifndef __SIDE_QUEST__
#define __SIDE_QUEST__

// ─── 支线任务类型 ───
#define SIDE_CHARACTER      1   // 角色支线：与特定NPC相关的剧情
#define SIDE_SCENE          2   // 场景支线：特定区域触发的任务
#define SIDE_COLLECTION     3   // 收集支线：收集特定物品/套装
#define SIDE_HIDDEN         4   // 隐藏支线：特定行为/极低概率触发
#define SIDE_FACTION        5   // 门派支线：门派贡献达标后解锁

// ─── 触发方式 ───
#define TRIGGER_NPC_DIALOG  1   // NPC对话触发
#define TRIGGER_AREA_ENTER  2   // 进入区域触发
#define TRIGGER_ITEM_HOLD   3   // 持有物品触发
#define TRIGGER_AUTO        4   // 条件满足自动触发
#define TRIGGER_REPUTATION  5   // 声望达标触发
#define TRIGGER_QUEST_CHAIN 6   // 其他任务完成触发

// ─── 支线状态 ───
#define SIDE_STATUS_LOCKED      0   // 未解锁
#define SIDE_STATUS_AVAILABLE   1   // 可接取
#define SIDE_STATUS_ACTIVE      2   // 进行中
#define SIDE_STATUS_COMPLETED   3   // 已完成
#define SIDE_STATUS_FAILED      4   // 已失败

// ─── 奖励衰减 ───
// 超过建议境界上限后奖励衰减
// 实际奖励 = 基准奖励 × max(0.1, 1 - (玩家境界 - 建议上限) / 5级)
#define SIDE_REWARD_DECAY_MIN   0.1 // 最低衰减到10%
#define SIDE_REWARD_DECAY_STEP  5   // 每超过建议境界5级衰减一级

// ─── 支线奖励范围（按建议境界）───
// 炼气期
#define SIDE_EXP_QI         200
#define SIDE_COIN_QI        20
#define SIDE_REP_QI         10
#define SIDE_SPECIAL_QI     5   // 特殊产出概率%

// 筑基期
#define SIDE_EXP_ZHU        1000
#define SIDE_COIN_ZHU       100
#define SIDE_REP_ZHU        20
#define SIDE_SPECIAL_ZHU    10

// 结丹期
#define SIDE_EXP_JIE        10000
#define SIDE_COIN_JIE       500
#define SIDE_REP_JIE        30
#define SIDE_SPECIAL_JIE    15

// 元婴期
#define SIDE_EXP_YING       60000
#define SIDE_COIN_YING      5000
#define SIDE_REP_YING       50
#define SIDE_SPECIAL_YING   20

// 化神期
#define SIDE_EXP_HUA        300000
#define SIDE_COIN_HUA       20000
#define SIDE_REP_HUA        60
#define SIDE_SPECIAL_HUA    25

// ─── 玩家数据存储key ───
#define SIDE_QUEST_DATA     "side_quest"
#define SIDE_QUEST_ACTIVE   "side_quest/active"
#define SIDE_QUEST_COMPLETED "side_quest/completed"

// ─── 同时持有上限 ───
#define SIDE_CONCURRENT_MAX     10  // 支线同时持有上限

#endif // __SIDE_QUEST__
