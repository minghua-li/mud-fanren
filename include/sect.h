// sect.h
// 门派系统常量 —— 九宗入宗/晋升/功法
// 设计文档: .knowledge/factions/sects/ 九宗档案、1D-门派种族声望.md §三
// 相关 daemon: adm/daemons/sect_d.c

// -------- 玩家门派数据路径 --------
#define SECT_PATH              "sect"
#define SECT_PATH_ID           "sect/id"            // 所属门派 ID
#define SECT_PATH_RANK         "sect/rank"          // 阶位 0-N
#define SECT_PATH_CONTRIB      "sect/contribution"  // 门派贡献
#define SECT_PATH_JOIN_TIME    "sect/join_time"     // 入门时间戳
#define SECT_PATH_BETRAYED     "sect/betrayed"      // 叛门记录 ([ sect_id: time ])
#define SECT_PATH_LEARNED      "sect/learned"       // 已学功法 ([ skill_id: time ])

// -------- 通用阶位（按 1D §三；各宗在 sect_d.c 自定义称号数组） --------
#define SECT_RANK_OUTER        0   // 外门弟子
#define SECT_RANK_INNER        1   // 内门弟子
#define SECT_RANK_TRUE         2   // 真传弟子
#define SECT_RANK_ELDER        3   // 长老（各宗最高阶位的门槛按档案）
#define SECT_RANK_DEPUTY       4   // 副宗主/副门主（部分宗门）
#define SECT_RANK_LEADER       5   // 宗主/门主/掌门/太上长老（部分宗门）

// -------- 入宗/退宗/叛门声望变动 --------
#define SECT_JOIN_REP_GAIN     1000    // 入宗时本门声望增益
#define SECT_LEAVE_REP_PENALTY -5000   // 正常退出门派声望惩罚（1D §三）
#define SECT_BETRAY_REP_PENALTY -20000 // 叛门声望惩罚（1D §三）

// -------- 境界要求（tier = 境界索引*3 + 小阶段 0初/1中/2后） --------
// 境界索引对齐 quest_chain.h REALM_NAMES: 0炼气 1筑基 2结丹 3元婴 4化神
#define SECT_TIER_QI_LATE      2       // 炼气后期
#define SECT_TIER_ZHU          3       // 筑基
#define SECT_TIER_ZHU_LATE     5       // 筑基后期
#define SECT_TIER_JIE          6       // 结丹
#define SECT_TIER_JIE_LATE     8       // 结丹后期
#define SECT_TIER_YING         9       // 元婴

// -------- 晋升贡献门槛（默认值，可调） --------
#define SECT_CONTRIB_INNER     1000
#define SECT_CONTRIB_TRUE      5000
#define SECT_CONTRIB_ELDER     20000
#define SECT_CONTRIB_DEPUTY    50000
#define SECT_CONTRIB_LEADER    100000

// -------- 功法学习贡献消耗 --------
#define SECT_SKILL_COST_BASIC  500     // 外门功法
#define SECT_SKILL_COST_INNER  2000    // 内门功法
#define SECT_SKILL_COST_ELITE  8000    // 真传/长老功法
#define SECT_SKILL_COST_APEX   30000   // 镇派绝学（如血灵大法）
