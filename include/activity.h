// activity.h
// 活跃度与日常任务系统常量定义

#ifndef __ACTIVITY_H__
#define __ACTIVITY_H__

// 活跃度行为类型
#define ACT_TYPE_HERB         "herb"          // 采集灵药
#define ACT_TYPE_HUNT         "hunt"          // 猎杀妖兽
#define ACT_TYPE_SECT_CHORE   "sect_chore"    // 门派杂务
#define ACT_TYPE_ALCHEMY      "alchemy"       // 炼制任务
#define ACT_TYPE_TRADE        "trade"         // 坊市交易
#define ACT_TYPE_COURIER      "courier"       // 送信任务
#define ACT_TYPE_MINE         "mine"          // 采矿任务
#define ACT_TYPE_PATROL       "patrol"        // 巡山任务

// 活跃度行为基础分值（完成一次所得活跃度）
#define ACT_SCORE_HERB        10
#define ACT_SCORE_HUNT        15
#define ACT_SCORE_SECT_CHORE  5
#define ACT_SCORE_ALCHEMY     20
#define ACT_SCORE_TRADE       8
#define ACT_SCORE_COURIER     10
#define ACT_SCORE_MINE        12
#define ACT_SCORE_PATROL      15

// 每日各行为类型上限次数
#define ACT_DAILY_LIMIT_HERB       3
#define ACT_DAILY_LIMIT_HUNT       3
#define ACT_DAILY_LIMIT_SECT_CHORE 5
#define ACT_DAILY_LIMIT_ALCHEMY    2
#define ACT_DAILY_LIMIT_TRADE      3
#define ACT_DAILY_LIMIT_COURIER    5
#define ACT_DAILY_LIMIT_MINE       3
#define ACT_DAILY_LIMIT_PATROL     2

// 日活跃度阈值奖励
#define ACT_DAILY_THRESHOLD_1   20      // 第一档：20活跃度
#define ACT_DAILY_THRESHOLD_2   50      // 第二档：50活跃度
#define ACT_DAILY_THRESHOLD_3   80      // 第三档：80活跃度
#define ACT_DAILY_THRESHOLD_4   100     // 第四档：100活跃度（满活跃）

// 周活跃度阈值奖励
#define ACT_WEEKLY_THRESHOLD_1  100     // 第一档
#define ACT_WEEKLY_THRESHOLD_2  200     // 第二档
#define ACT_WEEKLY_THRESHOLD_3  350     // 第三档
#define ACT_WEEKLY_THRESHOLD_4  500     // 第四档

// 活跃度每日上限
#define ACT_DAILY_MAX           120
// 活跃度每周上限
#define ACT_WEEKLY_MAX          700

// 活跃度标签名（玩家 dbase 中使用）
#define ACT_DBASE_KEY           "activity"

#endif
