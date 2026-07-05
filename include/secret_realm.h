// secret_realm.h
// 秘境副本系统 —— 类型定义、常量、宏、接口声明
// Created for ticket #31

#ifndef __SECRET_REALM_H__
#define __SECRET_REALM_H__

// 秘境类型（对应 ticket #31 四种类型）
#define SR_TYPE_STORY        1    // 剧情秘境：主线触发，一次性
#define SR_TYPE_TIMED        2    // 限时秘境：全局定时开启，多人
#define SR_TYPE_CHALLENGE    3    // 多人秘境：消耗凭证，小队
#define SR_TYPE_FORTUNE      4    // 随机秘境：概率触发，单人

// 秘境状态
#define SR_STATUS_CLOSED      0   // 关闭（未开放）
#define SR_STATUS_OPENING     1   // 开启预告
#define SR_STATUS_OPEN        2   // 入口开放
#define SR_STATUS_ACTIVE      3   // 探索进行中
#define SR_STATUS_SETTLING    4   // 结算中
#define SR_STATUS_COOLDOWN    5   // 冷却中

// 难度等级
#define SR_DIFFICULTY_EASY    1   // 简单
#define SR_DIFFICULTY_NORMAL  2   // 普通
#define SR_DIFFICULTY_HARD    3   // 困难
#define SR_DIFFICULTY_HELL    4   // 地狱

// 秘境重置周期
#define SR_RESET_DAILY        1   // 每日重置
#define SR_RESET_WEEKLY       2   // 每周重置
#define SR_RESET_MONTHLY      3   // 每月重置
#define SR_RESET_ONCE         4   // 一次性（不重置）

// 奖励类型
#define SR_REWARD_EXP         1   // 经验
#define SR_REWARD_POTENTIAL   2   // 潜能
#define SR_REWARD_SCORE       3   // 声望
#define SR_REWARD_ITEM        4   // 物品
#define SR_REWARD_SKILL       5   // 技能
#define SR_REWARD_POINT       6   // 秘境积分

// 默认冷却时间（秒）
#define SR_CD_DAILY          86400    // 每日秘境冷却
#define SR_CD_WEEKLY         604800   // 每周秘境冷却
#define SR_CD_CHALLENGE      7200     // 挑战秘境基础冷却（2小时）
#define SR_CD_FORTUNE        0        // 随机秘境无冷却（随机触发）

// 默认队伍人数限制
#define SR_TEAM_MIN          1        // 最小队伍人数
#define SR_TEAM_STORY_MAX    4        // 剧情秘境最大队伍
#define SR_TEAM_TIMED_MAX    30       // 限时秘境最大人数
#define SR_TEAM_CHALLENGE_MAX 5       // 多人秘境最大队伍
#define SR_TEAM_FORTUNE_MAX  1        // 随机秘境单人

// 秘境入口结构字段（供 secret_realm_d 内部注册表使用）
#define SR_FIELD_ID          "id"            // 秘境唯一标识
#define SR_FIELD_NAME        "name"          // 秘境名称
#define SR_FIELD_TYPE        "type"          // 秘境类型（SR_TYPE_*）
#define SR_FIELD_DIFFICULTY  "difficulty"    // 难度等级
#define SR_FIELD_RESET       "reset"         // 重置周期
#define SR_FIELD_STATUS      "status"        // 当前状态
#define SR_FIELD_ENTRY       "entry"         // 入口房间路径
#define SR_FIELD_EXIT        "exit"          // 出口房间路径
#define SR_FIELD_MIN_LEVEL   "min_level"     // 最低境界要求
#define SR_FIELD_MAX_LEVEL   "max_level"     // 最高境界限制
#define SR_FIELD_ITEM_REQ    "item_req"      // 所需物品（"物品路径:数量"）
#define SR_FIELD_TEAM_REQ    "team_req"      // 组队要求
#define SR_FIELD_DURATION    "duration"      // 持续时间（秒）
#define SR_FIELD_CD          "cooldown"      // 冷却时间（秒）
#define SR_FIELD_REWARDS     "rewards"       // 奖励表
#define SR_FIELD_LAYERS      "layers"        // 秘境层数

// 怪物/BOSS 生成配置字段
#define SR_MOB_ID            "id"
#define SR_MOB_NAME          "name"
#define SR_MOB_LEVEL         "level"
#define SR_MOB_HP            "hp"
#define SR_MOB_SKILLS        "skills"
#define SR_MOB_DROP          "drop_table"

#endif
