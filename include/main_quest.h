// main_quest.h
// 主线任务框架数据类型与常量定义
// Based on: 02-扩充内容/02-任务链与奖励曲线.md 与 .knowledge/quests/1G-任务副本奇遇.md
// 5 章主线框架，串行链式推进，奖励里程碑
//
// 说明（#65 重写）：
//   - 主线任务注册到 QUEST_CHAIN_D（#59 任务链框架），本头文件只保留章节/奖励常量，
//     境界门槛由 quest_chain_d 的 realm_range（统一境界索引 0=炼气 1=筑基 2=结丹 ...）承担。
//   - 玩家进度数据由 QUEST_CHAIN_D 统一存储（quest_chain/*），本文件不再定义玩家存储 key。

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

// ── 奖励系数 ─────────────────────────────────────
#define CHAPTER_0_BASE       100   // 凡人篇基础奖励
#define CHAPTER_1_BASE       1000  // 越国篇基础奖励
#define CHAPTER_2_BASE       10000 // 乱星海篇基础奖励
#define CHAPTER_3_BASE       500000  // 灵界篇基础奖励
#define CHAPTER_4_BASE       10000000 // 飞升篇基础奖励

#define CHAPTER_MULTIPLIER   3     // 章节完成奖励倍数（里程碑奖励 = 章节基础 × 倍数）

#endif
