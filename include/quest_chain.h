// quest_chain.h
// 任务链系统常量与类型定义
// 接 quests/1G-任务副本奇遇 与 02-扩充内容/02-任务链与奖励曲线.md

#ifndef __QUEST_CHAIN__
#define __QUEST_CHAIN__

// ─── 任务类型（7种） ───
#define QUEST_TYPE_MAIN         1   // 主线任务（一次性，剧情驱动，不可回流）
#define QUEST_TYPE_SIDE         2   // 支线任务（部分可重做，手动触发/剧情分支）
#define QUEST_TYPE_DAILY        3   // 日常任务（每日刷新）
#define QUEST_TYPE_WEEKLY       4   // 周常任务（每周刷新）
#define QUEST_TYPE_ACHIEVEMENT  5   // 成就任务（一次性，达成条件自动完成）
#define QUEST_TYPE_ENCOUNTER    6   // 奇遇任务（一次性，随机触发/条件解锁）
#define QUEST_TYPE_FACTION      7   // 势力任务（可重复有限制，按声望等级解锁）

// ─── 任务链类型（5种） ───
#define CHAIN_SERIAL            1   // 串行链：前置完成后自动接续
#define CHAIN_BRANCH            2   // 分支链：同一节点可选不同方向
#define CHAIN_PARALLEL          3   // 并行链：多条任务链同时推进
#define CHAIN_CONDITIONAL       4   // 条件链：满足声望/境界/物品条件解锁后续
#define CHAIN_TIMED             5   // 限时链：有游戏内时间限制，超时失败

// ─── 任务状态 ───
#define QUEST_STATUS_LOCKED     0   // 未解锁（不可见）
#define QUEST_STATUS_AVAILABLE  1   // 可接取
#define QUEST_STATUS_ACTIVE     2   // 进行中
#define QUEST_STATUS_COMPLETED  3   // 已完成
#define QUEST_STATUS_FAILED     4   // 已失败
#define QUEST_STATUS_EXPIRED    5   // 已过期

// ─── 任务目标类型 ───
#define OBJ_COLLECT             1   // 采集/收集 N 个物品
#define OBJ_KILL                2   // 击杀 N 个目标
#define OBJ_DELIVER             3   // 交付指定物品
#define OBJ_REACH               4   // 到达指定区域
#define OBJ_TALK                5   // 与指定 NPC 对话
#define OBJ_ESCORT              6   // 护送目标
#define OBJ_CRAFT               7   // 制造/炼制指定物品
#define OBJ_USE                 8   // 使用指定物品
#define OBJ_EXPLORE             9   // 探索指定区域数量

// ─── 刷新类型 ───
#define REFRESH_ONCE            0   // 一次性
#define REFRESH_DAILY           1   // 每日刷新
#define REFRESH_WEEKLY          2   // 每周刷新
#define REFRESH_MONTHLY         3   // 每月刷新

// ─── 任务质量（日常稀有度） ───
#define QUEST_QUALITY_NORMAL    1   // 普通（绿色）
#define QUEST_QUALITY_GOOD      2   // 优秀（蓝色），奖励×1.5
#define QUEST_QUALITY_RARE      3   // 稀有（紫色），奖励×3.0

// ─── 任务分组（用于日常最大接取数控制） ───
#define QUEST_GROUP_DAILY_LINGYAO   "daily_lingyao"
#define QUEST_GROUP_DAILY_SHOULE    "daily_shoulei"
#define QUEST_GROUP_DAILY_SCHOOL    "daily_school"
#define QUEST_GROUP_DAILY_LIANZHI   "daily_lianzhi"
#define QUEST_GROUP_DAILY_FANGSHI   "daily_fangshi"
#define QUEST_GROUP_DAILY_SONGXIN   "daily_songxin"
#define QUEST_GROUP_DAILY_CAIKUANG  "daily_caikuang"
#define QUEST_GROUP_DAILY_XUNSHAN   "daily_xunshan"

// ─── 日常质量权重 ───
#define DAILY_RARE_CHANCE_QI    5   // 炼气期稀有日常概率(%)
#define DAILY_RARE_CHANCE_ZHU   8   // 筑基期
#define DAILY_RARE_CHANCE_JIE   12  // 结丹期
#define DAILY_RARE_CHANCE_YING  15  // 元婴期
#define DAILY_RARE_CHANCE_HUA   20  // 化神期+

// ─── 每日可接日常数（按境界） ───
#define DAILY_MAX_QI            8   // 炼气期
#define DAILY_MAX_ZHU           9   // 筑基期
#define DAILY_MAX_JIE           10  // 结丹期
#define DAILY_MAX_YING          11  // 元婴期
#define DAILY_MAX_HUA           12  // 化神期+

// ─── 日常上限控制 ───
#define DAILY_SAME_TYPE_MAX     3   // 每日同类日常最多完成数
#define DAILY_ABANDON_MAX       3   // 每日最多放弃次数
#define DAILY_ABANDON_CD        300 // 放弃后冷却时间（秒，5分钟）
#define DAILY_CONCURRENT_MAX    20  // 同时持有任务上限

// ─── 奖励缩放系数 ───
#define REWARD_SAME_REALM       1.0 // 同境界：100%
#define REWARD_LOWER_REALM      0.4 // 低境界做高难：40%
#define REWARD_HIGHER_REALM_MIN 0.3 // 高境界做低日常：最低30%
#define REWARD_HIGHER_REALM_DECAY 0.7 // 高境界衰减系数

// ─── 连续奖励加成 ───
#define DAILY_STREAK_BONUS      0.05 // 每连续一天加成5%
#define DAILY_STREAK_MAX_DAYS   7    // 最大连续加成天数
#define DAILY_STREAK_VIP_BONUS  1.5  // 7天后奖励×1.5

// ─── 奖励品质系数 ───
#define QUALITY_COEFF_NORMAL    1.0
#define QUALITY_COEFF_GOOD      1.5
#define QUALITY_COEFF_RARE      3.0

// ─── 灵石占修为比例 ───
#define COIN_RATIO_MIN          20  // 最低20%
#define COIN_RATIO_MAX          30  // 最高30%
#define COIN_FLOOR              10  // 最低10灵石

// ─── 境界基准值（用于日常奖励计算） ───
// 境界索引顺序：0=炼气,1=筑基,2=结丹,3=元婴,4=化神,5=炼虚,6=合体,7=大乘
#define REALM_BASE_QI           50
#define REALM_BASE_ZHU          200
#define REALM_BASE_JIE          1000
#define REALM_BASE_YING         5000
#define REALM_BASE_HUA          20000
#define REALM_BASE_LIANXU       80000
#define REALM_BASE_HETI         300000
#define REALM_BASE_DACHENG      1000000

// ─── 境界名称列表 ───
#define REALM_NAMES             ({ "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘" })

// ─── 日常/周常每周占比约束 ───
#define DAILY_EXP_SHARE         35  // 日常修为占35%
#define DAILY_COIN_SHARE        25  // 日常灵石占25%
#define DAILY_GONGXIAN_SHARE    40  // 日常门派贡献占40%
#define WEEKLY_EXP_SHARE        20  // 周常修为占20%
#define WEEKLY_COIN_SHARE       15  // 周常灵石占15%

// ─── 组队奖励加成 ───
#define TEAM_BONUS_MIN          10  // 组队最低加成10%
#define TEAM_BONUS_MAX          20  // 组队最高加成20%

// ─── 属性路径宏（用于玩家数据存取） ───
#define QUEST_CHAIN_DATA         "quest_chain"
#define QUEST_CHAIN_PROGRESS     "quest_chain/progress"
#define QUEST_CHAIN_COMPLETED    "quest_chain/completed"
#define QUEST_CHAIN_ACTIVE       "quest_chain/active"
#define QUEST_CHAIN_DAILY_RESET  "quest_chain/daily_reset"
#define QUEST_CHAIN_WEEKLY_RESET "quest_chain/weekly_reset"
#define QUEST_CHAIN_DAILY_COUNT  "quest_chain/daily_count"
#define QUEST_CHAIN_WEEKLY_COUNT "quest_chain/weekly_count"
#define QUEST_CHAIN_DAILY_STREAK "quest_chain/daily_streak"
#define QUEST_CHAIN_LAST_DAY     "quest_chain/last_active_day"

// ─── 奖励键扩展（宗门任务/事件用） ───
// 基础键：exp（修为经验）/ coin（灵石）/ reputation（声望数组）/ items（物品路径数组）
// 扩展键（#59 宗门任务链）：
//   "contribution": N   → 门派贡献，结算走 SECT_D->add_contribution
//   "skills": ({ id })  → 功法发放，见 sect_quest.h 说明

#endif // __QUEST_CHAIN__
