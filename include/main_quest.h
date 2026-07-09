// main_quest.h
// 主线任务框架数据类型与常量定义
// Based on: 02-扩充内容/02-任务链与奖励曲线.md
// 5 章主线框架，串行链式推进，奖励里程碑

#ifndef __MAIN_QUEST__
#define __MAIN_QUEST__

// ── 章节 ID ─────────────────────────────────────────
#define CHAPTER_MORTAL       0     // 凡人篇（第零章）
#define CHAPTER_YUE          1     // 越国篇（第一章）
#define CHAPTER_LUANXINGHAI  2     // 乱星海篇（第二章）
#define CHAPTER_LINGJIE      3     // 灵界篇（第三章）
#define CHAPTER_FEISHENG     4     // 飞升篇（终章）
#define CHAPTER_COUNT        5     // 总章节数

// ── 章节名称 ───────────────────────────────────────
#define CHAPTER_0_NAME       "凡人篇"
#define CHAPTER_1_NAME       "越国篇"
#define CHAPTER_2_NAME       "乱星海篇"
#define CHAPTER_3_NAME       "灵界篇"
#define CHAPTER_4_NAME       "飞升篇"

// ── 节点状态 ───────────────────────────────────────
#define NODE_LOCKED          0     // 未解锁（前置未完成）
#define NODE_AVAILABLE       1     // 可接取
#define NODE_ACTIVE          2     // 进行中
#define NODE_COMPLETED       3     // 已完成

// ── 玩家主线状态 ──────────────────────────────────
#define MQ_INACTIVE          0     // 未开始
#define MQ_ACTIVE            1     // 进行中
#define MQ_COMPLETED         2     // 全部完成

// ── 章节解锁境界要求（简化境界索引）─────────────
#define REALM_MORTAL         0     // 凡人
#define REALM_QI_INIT        3     // 炼气初期
#define REALM_QI_7           7     // 炼气 7 层
#define REALM_JIEDAN_INIT    20    // 结丹初期
#define REALM_HUASHEN_INIT   40    // 化神期
#define REALM_DACHENG        50    // 大乘期

#define CHAPTER_0_MIN_REALM  REALM_MORTAL
#define CHAPTER_1_MIN_REALM  REALM_QI_7
#define CHAPTER_2_MIN_REALM  REALM_JIEDAN_INIT
#define CHAPTER_3_MIN_REALM  REALM_HUASHEN_INIT
#define CHAPTER_4_MIN_REALM  REALM_DACHENG

// ── 每章节节点数量 ───────────────────────────────
#define CHAPTER_0_NODES      11    // 凡人篇节点数
#define CHAPTER_1_NODES      13    // 越国篇节点数
#define CHAPTER_2_NODES      10    // 乱星海篇节点数
#define CHAPTER_3_NODES      10    // 灵界篇节点数
#define CHAPTER_4_NODES      6     // 飞升篇节点数

// ── 奖励系数 ─────────────────────────────────────
#define CHAPTER_0_BASE       100   // 凡人篇基础奖励
#define CHAPTER_1_BASE       1000  // 越国篇基础奖励
#define CHAPTER_2_BASE       10000 // 乱星海篇基础奖励
#define CHAPTER_3_BASE       500000  // 灵界篇基础奖励
#define CHAPTER_4_BASE       10000000 // 飞升篇基础奖励

#define CHAPTER_MULTIPLIER   3     // 章节完成奖励倍数（里程碑奖励 = 章节基础 × 倍数）

// ── 玩家数据存储 key ────────────────────────────
#define MQ_KEY_CHAPTER       "main_quest/chapter"       // 当前章节
#define MQ_KEY_NODE          "main_quest/node"          // 当前活跃节点
#define MQ_KEY_STATUS        "main_quest/status"        // 主线整体状态
#define MQ_KEY_COMP_NODES    "main_quest/completed_nodes"   // 已完成节点 ({ "node_0_1", ... })
#define MQ_KEY_COMP_CHAPTERS "main_quest/completed_chapters" // 已完成章节 ({ 0, 1, ... })

#endif
