// achievement.h
// 成就系统常量定义

#ifndef __ACHIEVEMENT_H__
#define __ACHIEVEMENT_H__

// 成就分类
#define ACH_CAT_CULTIVATION    "cultivation"     // 修炼成就
#define ACH_CAT_TASK           "task"            // 任务成就
#define ACH_CAT_COMBAT         "combat"          // 战斗成就
#define ACH_CAT_COLLECTION     "collection"      // 收集成就
#define ACH_CAT_EXPLORATION    "exploration"     // 探索成就
#define ACH_CAT_SOCIAL         "social"          // 社交成就
#define ACH_CAT_LIFE           "life"            // 生活成就
#define ACH_CAT_HIDDEN         "hidden"          // 隐藏成就

// 成就分类中文名
#define ACH_CAT_CN_CULTIVATION    "修炼成就"
#define ACH_CAT_CN_TASK           "任务成就"
#define ACH_CAT_CN_COMBAT         "战斗成就"
#define ACH_CAT_CN_COLLECTION     "收集成就"
#define ACH_CAT_CN_EXPLORATION    "探索成就"
#define ACH_CAT_CN_SOCIAL         "社交成就"
#define ACH_CAT_CN_LIFE           "生活成就"
#define ACH_CAT_CN_HIDDEN         "隐藏成就"

// 成就基础分值
#define ACH_SCORE_BASIC       10
#define ACH_SCORE_CULTIVATION 20
#define ACH_SCORE_COMBAT      15
#define ACH_SCORE_COLLECTION  10
#define ACH_SCORE_HIDDEN      30
#define ACH_SCORE_SOCIAL      10
#define ACH_SCORE_LIFE        10

// 段位奖励阈值
#define ACH_TIER_1      100       // 成就新星
#define ACH_TIER_2      300       // 成就达人
#define ACH_TIER_3      500       // 成就宗师
#define ACH_TIER_4      800       // 成就传说
#define ACH_TIER_5      1000      // 全境圆满

// 奖励类型
#define ACH_REWARD_TITLE      "title"        // 称号
#define ACH_REWARD_ATTRIBUTE  "attribute"    // 属性加成
#define ACH_REWARD_ITEM       "item"         // 道具
#define ACH_REWARD_EXP        "exp"          // 经验

// 成就状态
#define ACH_STATUS_LOCKED      0    // 未解锁
#define ACH_STATUS_UNLOCKED    1    // 已解锁
#define ACH_STATUS_NOTIFIED    2    // 已通知

#endif
