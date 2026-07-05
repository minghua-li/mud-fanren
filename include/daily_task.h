// daily_task.h
// 日常任务系统 — 类型定义、常量、数据结构
// Created for #36-A (#39) 日常任务系统

#ifndef __DAILY_TASK_H__
#define __DAILY_TASK_H__

// ──────────────────────────────────────────────
// 任务类型（8种模板）
// ──────────────────────────────────────────────

#define TASK_KILL           1       // 杀怪任务：击杀指定数量/类型的怪物
#define TASK_COLLECT        2       // 采集任务：采集指定数量的资源
#define TASK_VISIT          3       // 拜访任务：拜访指定 NPC 或地点
#define TASK_DELIVER        4       // 送信任务：在 NPC 之间传递信件
#define TASK_ESCORT         5       // 护送任务：护送 NPC 到指定地点
#define TASK_DONATE         6       // 捐献任务：向指定势力捐献资源/灵石
#define TASK_PRACTICE       7       // 修炼任务：完成一定数量的修炼循环
#define TASK_DUNGEON        8       // 副本任务：完成指定副本/挑战

// ──────────────────────────────────────────────
// 任务品质（质量分级）
// ──────────────────────────────────────────────

#define QUALITY_NORMAL      1       // 普通（绿色），基准倍率 1.0
#define QUALITY_GOOD        2       // 优秀（蓝色），基准倍率 1.5
#define QUALITY_RARE        3       // 稀有（紫色），基准倍率 3.0

// ──────────────────────────────────────────────
// 任务状态
// ──────────────────────────────────────────────

#define TASK_STATUS_ACTIVE      1   // 进行中
#define TASK_STATUS_COMPLETED   2   // 已完成
#define TASK_STATUS_FAILED      3   // 失败
#define TASK_STATUS_ABANDONED   4   // 已放弃

// ──────────────────────────────────────────────
// 默认配置常量
// ──────────────────────────────────────────────

#define MAX_DAILY_TASKS         12      // 每日最大可接任务数（最高境界）
#define MIN_DAILY_TASKS         8       // 每日最少可接任务数（最低境界）
#define MAX_ABANDON_PER_DAY     3       // 每日最多放弃次数
#define ABANDON_CD              300     // 放弃后冷却时间（秒，5分钟）
#define STREAK_RESET_DAYS       1       // 连击中断天数（超过此天数未完成即重置）
#define STREAK_BONUS_PER_DAY    5       // 每连续一天加成百分比
#define MAX_STREAK_BONUS        50      // 连击加成上限百分比

// ──────────────────────────────────────────────
// 境界基准值（用于奖励计算）
// ──────────────────────────────────────────────

#define REALM_MORTAL            0       // 凡人
#define REALM_QI_LOW            1       // 炼气前期
#define REALM_QI_MID            2       // 炼气中期
#define REALM_QI_HIGH           3       // 炼气后期
#define REALM_ZHU_LOW           4       // 筑基前期
#define REALM_ZHU_MID           5       // 筑基中期
#define REALM_ZHU_HIGH          6       // 筑基后期
#define REALM_JIE_LOW           7       // 结丹前期
#define REALM_JIE_MID           8       // 结丹中期
#define REALM_JIE_HIGH          9       // 结丹后期
#define REALM_YING_LOW          10      // 元婴前期
#define REALM_YING_MID          11      // 元婴中期
#define REALM_YING_HIGH         12      // 元婴后期
#define REALM_HUA               13      // 化神期
#define REALM_LIAN              14      // 炼虚期
#define REALM_HE                15      // 合体期
#define REALM_DA                16      // 大乘期
#define REALM_MAX               16

// 各境界对应的每日任务上限
#define DAILY_LIMIT_MORTAL      5
#define DAILY_LIMIT_QI          8
#define DAILY_LIMIT_ZHU         9
#define DAILY_LIMIT_JIE         10
#define DAILY_LIMIT_YING        11
#define DAILY_LIMIT_HIGH        12

// 各境界基准修为奖励值
#define BASE_REWARD_MORTAL      20
#define BASE_REWARD_QI          50
#define BASE_REWARD_ZHU         200
#define BASE_REWARD_JIE         1000
#define BASE_REWARD_YING        5000
#define BASE_REWARD_HUA         20000
#define BASE_REWARD_LIAN        80000
#define BASE_REWARD_HE          300000
#define BASE_REWARD_DA          1000000

// 稀有任务概率表（百分比）
#define RARE_CHANCE_QI          5
#define RARE_CHANCE_ZHU         8
#define RARE_CHANCE_JIE         12
#define RARE_CHANCE_YING        15
#define RARE_CHANCE_HIGH        20

// 境界加成系数，每高一个境界层次奖励增加百分比
#define REALM_BONUS_STEP        10

// ──────────────────────────────────────────────
// 玩家数据存储路径（在 player->query() 中使用）
// ──────────────────────────────────────────────

#define DAILY_DATA_PREFIX       "daily_task"

#endif
