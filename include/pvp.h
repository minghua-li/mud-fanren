// pvp.h
// 凡人修仙传 MUD - PVP 系统常量定义
// 段位上限: 第7阶

#ifndef __PVP__
#define __PVP__

// ========== 段位系统（7阶上限） ==========

// 段位数量
#define PVP_RANK_COUNT  7

// 段位定义：名称、所需积分、赛季结算灵石奖励基数
// 第1阶: 炼气散修   (0 pts)
// 第2阶: 筑基修士   (200 pts)
// 第3阶: 结丹真人   (500 pts)
// 第4阶: 元婴老怪   (900 pts)
// 第5阶: 化神圣君   (1400 pts)
// 第6阶: 大乘仙尊   (2000 pts)
// 第7阶: 渡劫真仙   (2800 pts)

#define PVP_RANK_1_NAME         "炼气散修"
#define PVP_RANK_1_SCORE        0
#define PVP_RANK_1_REWARD       50

#define PVP_RANK_2_NAME         "筑基修士"
#define PVP_RANK_2_SCORE        200
#define PVP_RANK_2_REWARD       200

#define PVP_RANK_3_NAME         "结丹真人"
#define PVP_RANK_3_SCORE        500
#define PVP_RANK_3_REWARD       500

#define PVP_RANK_4_NAME         "元婴老怪"
#define PVP_RANK_4_SCORE        900
#define PVP_RANK_4_REWARD       1200

#define PVP_RANK_5_NAME         "化神圣君"
#define PVP_RANK_5_SCORE        1400
#define PVP_RANK_5_REWARD       2500

#define PVP_RANK_6_NAME         "大乘仙尊"
#define PVP_RANK_6_SCORE        2000
#define PVP_RANK_6_REWARD       5000

#define PVP_RANK_7_NAME         "渡劫真仙"
#define PVP_RANK_7_SCORE        2800
#define PVP_RANK_7_REWARD       10000

// 段位阈值数组：索引0为第1阶，索引6为第7阶
#define PVP_RANK_THRESHOLDS  ({ \
    0,    /* 第1阶 */         \
    200,  /* 第2阶 */         \
    500,  /* 第3阶 */         \
    900,  /* 第4阶 */         \
    1400, /* 第5阶 */         \
    2000, /* 第6阶 */         \
    2800  /* 第7阶 */         \
})

#define PVP_RANK_NAMES ({ \
    "炼气散修",           \
    "筑基修士",           \
    "结丹真人",           \
    "元婴老怪",           \
    "化神圣君",           \
    "大乘仙尊",           \
    "渡劫真仙"            \
})

#define PVP_RANK_REWARDS ({ \
    50,    /* 第1阶 */      \
    200,   /* 第2阶 */      \
    500,   /* 第3阶 */      \
    1200,  /* 第4阶 */      \
    2500,  /* 第5阶 */      \
    5000,  /* 第6阶 */      \
    10000  /* 第7阶 */      \
})

// ========== PVP 模式 ==========

#define PVP_MODE_ARENA      1   // 竞技场匹配
#define PVP_MODE_CHALLENGE  2   // 玩家主动挑战
#define PVP_MODE_SPAR       3   // 切磋（不伤和气）

// ========== 匹配参数 ==========

// 匹配队列默认超时（秒）
#define PVP_QUEUE_TIMEOUT       30
// 超时后放宽匹配范围
#define PVP_QUEUE_WIDE_TIMEOUT  60

// 初始匹配范围：同段位 ± 段位差
#define PVP_MATCH_RANGE_NARROW  1
// 超时后放宽范围
#define PVP_MATCH_RANGE_WIDE    2
// 60秒后任意范围
#define PVP_MATCH_RANGE_ANY     99

// ========== 积分增减 ==========

// 基础胜利积分
#define PVP_SCORE_WIN_BASE      15
// 基础失败扣分
#define PVP_SCORE_LOSE_BASE     5
// 胜利额外加分（对手段位更高时，每高一阶+）
#define PVP_SCORE_WIN_HIGHER    5
// 失败额外扣分（对手段位更低时，每低一阶-）
#define PVP_SCORE_LOSE_LOWER    5

// 连胜奖励
#define PVP_STREAK_3_BONUS      5   // 3连胜额外
#define PVP_STREAK_5_BONUS      10  // 5连胜额外
#define PVP_STREAK_10_BONUS     20  // 10连胜额外

// 积分上限/下限保护
#define PVP_SCORE_MIN           0
#define PVP_SCORE_MAX           99999

// 第7阶以上不再降阶保护
#define PVP_RANK_PROTECT        1   // 第1阶不掉阶

// ========== 赛季系统 ==========

// 赛季周期（天）
#define PVP_SEASON_DAYS         30
// 赛季重置保留积分百分比
#define PVP_SEASON_RESERVE_RATIO    50

// ========== 竞技场房间 ==========

#define PVP_ARENA_ROOM          "/biwu/arena_room"
#define PVP_ARENA_PREP_TIME     5    // 准备时间（秒）

// ========== 玩家数据键 ==========

#define PVP_DATA_KEY            "pvp_data"
#define PVP_DATA_SCORE          "score"
#define PVP_DATA_RANK           "rank"
#define PVP_DATA_WINS           "wins"
#define PVP_DATA_LOSSES         "losses"
#define PVP_DATA_STREAK         "streak"
#define PVP_DATA_BEST_RANK      "best_rank"
#define PVP_DATA_SEASON_WINS    "season_wins"
#define PVP_DATA_TOTAL_FIGHTS   "total_fights"
#define PVP_DATA_LAST_FIGHT     "last_fight_time"

// 临时数据键
#define PVP_TEMP_IN_QUEUE       "pvp_in_queue"
#define PVP_TEMP_QUEUE_TIME     "pvp_queue_time"
#define PVP_TEMP_IN_ARENA       "pvp_in_arena"
#define PVP_TEMP_ORIGIN_ROOM    "pvp_origin_room"
#define PVP_TEMP_CHALLENGER     "pvp_challenger"

#endif
