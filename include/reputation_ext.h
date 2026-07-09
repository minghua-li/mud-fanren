// reputation_ext.h
// 声望互动扩展系统 - 常量定义
// 对应设计文档 02-扩充内容/02-声望与互动玩法.md 第4~8章
// 好友亲密等级、小队、交易、战争、外交

#ifndef __REPUTATION_EXT_H__
#define __REPUTATION_EXT_H__

// ===================== 好友亲密等级 =====================

#define FRIEND_INTIMATE_ACQUAINTANCE  0    // 点头之交 (0)
#define FRIEND_INTIMATE_OFTEN         1    // 常来常往 (200)
#define FRIEND_INTIMATE_DAOYOU        2    // 道友之交 (800)
#define FRIEND_INTIMATE_LIFEDEATH     3    // 生死之交 (3000)
#define FRIEND_INTIMATE_CLOSE         4    // 莫逆之交 (10000)
#define FRIEND_INTIMATE_COUPLE        5    // 道侣/结义 (30000)

#define FRIEND_INTIMATE_THRESHOLD_1    200
#define FRIEND_INTIMATE_THRESHOLD_2    800
#define FRIEND_INTIMATE_THRESHOLD_3    3000
#define FRIEND_INTIMATE_THRESHOLD_4    10000
#define FRIEND_INTIMATE_THRESHOLD_5    30000

#define FRIEND_INTIMATE_NAME_0  "点头之交"
#define FRIEND_INTIMATE_NAME_1  "常来常往"
#define FRIEND_INTIMATE_NAME_2  "道友之交"
#define FRIEND_INTIMATE_NAME_3  "生死之交"
#define FRIEND_INTIMATE_NAME_4  "莫逆之交"
#define FRIEND_INTIMATE_NAME_5  "道侣/结义"

// 好友上限
#define FRIEND_MAX_DEFAULT      30
#define FRIEND_MAX_ZHUIJI       50
#define FRIEND_MAX_JIEDAN       80
#define FRIEND_MAX_YUANYING     120

// 亲密值获取
#define INTIMATE_TEAM_TASK      20    // 组队完成副本/任务 +10~+30
#define INTIMATE_GIFT           10    // 赠送礼物 +1~+20
#define INTIMATE_DUAL_CULT      75    // 双修 +50~+100
#define INTIMATE_RESCUE         50    // 互相救援 +50
#define INTIMATE_TEACH          20    // 传功指导 +20/小时
#define INTIMATE_MAIL            1    // 邮件 +1/封

// 每日亲密值上限(同一队友)
#define INTIMATE_DAILY_CAP      100

// 好友在线状态
#define FRIEND_STATUS_OFFLINE   0
#define FRIEND_STATUS_ONLINE    1
#define FRIEND_STATUS_BUSY      2    // 战斗中
#define FRIEND_STATUS_CLOSED    3    // 闭关中
#define FRIEND_STATUS_IDLE      4    // 空闲

#define FRIEND_STATUS_NAME_0    "离线"
#define FRIEND_STATUS_NAME_1    "在线"
#define FRIEND_STATUS_NAME_2    "战斗中"
#define FRIEND_STATUS_NAME_3    "闭关中"
#define FRIEND_STATUS_NAME_4    "空闲"

// 存储路径
#define FRIEND_PATH             "friend/list"
#define FRIEND_INTIMATE_PATH    "friend/intimate"
#define FRIEND_BLACKLIST_PATH   "friend/blacklist"
#define FRIEND_GIFT_PATH        "friend/gift_today"

// ===================== 小队系统 =====================

#define SQUAD_MAX_MEMBERS       5
#define SQUAD_CREATE_REALM      "筑基"
#define SQUAD_MIN_INTIMATE      FRIEND_INTIMATE_DAOYOU  // 道友之交以上
#define SQUAD_NAME_MAX_LEN      12    // 4个汉字 * 3字节

// 小队福利（与同小队≥3人组队时生效）
#define SQUAD_BONUS_EXP         10    // 额外+10%经验
#define SQUAD_BONUS_REP         5     // 额外+5%声望
#define SQUAD_BONUS_DROP        10    // 掉落率+10%

#define SQUAD_DAYS_FOR_LINGMAI  7     // 持续活动7天可租灵脉
#define SQUAD_DAYS_FOR_BASE     30    // 持续活动30天+全员结丹=专属洞府
#define SQUAD_BASE_REALM        "结丹"

// 存储路径
#define SQUAD_PATH              "squad"
#define SQUAD_MEMBER_PATH       "squad/members"

// ===================== 交易系统 =====================

// 交易类型
#define TRADE_TYPE_FACE         1     // 面对面交易
#define TRADE_TYPE_MAIL         2     // 邮件寄送
#define TRADE_TYPE_AUCTION      3     // 拍卖行
#define TRADE_TYPE_STALL        4     // 坊市摆摊
#define TRADE_TYPE_BLACK        5     // 黑市交易
#define TRADE_TYPE_CONTRIBUTE   6     // 宗门贡献兑换

#define TRADE_NAME_FACE         "面对面交易"
#define TRADE_NAME_MAIL         "邮件寄送"
#define TRADE_NAME_AUCTION      "拍卖行"
#define TRADE_NAME_STALL        "坊市摆摊"
#define TRADE_NAME_BLACK        "黑市交易"
#define TRADE_NAME_CONTRIBUTE   "宗门贡献兑换"

// 交易税率
#define TRADE_TAX_FACE          0.00
#define TRADE_TAX_MAIL          0.05
#define TRADE_TAX_AUCTION       0.10
#define TRADE_TAX_STALL         0.05
#define TRADE_TAX_BLACK         0.00

// 每日交易上限(灵石)
#define TRADE_DAILY_LIMIT_QIYIN    5000
#define TRADE_DAILY_LIMIT_ZHUIJI   50000
#define TRADE_DAILY_LIMIT_JIEDAN   500000
#define TRADE_DAILY_LIMIT_YUANYING 5000000

// 交易日志保留天数
#define TRADE_LOG_RETAIN_DAYS   7

// 存储路径
#define TRADE_PATH              "trade"
#define TRADE_DAILY_PATH        "trade/daily"
#define STALL_PATH              "trade/stall"

// ===================== 战争系统 =====================

// 战争类型
#define WAR_TYPE_CLAN_BATTLE    1     // 门派战
#define WAR_TYPE_RIGHTEOUS_EVIL 2     // 正魔之战
#define WAR_TYPE_RANDOM_SEA     3     // 乱星海争夺战
#define WAR_TYPE_INTERFACE      4     // 界面战争

#define WAR_NAME_CLAN_BATTLE    "门派战"
#define WAR_NAME_RIGHTEOUS_EVIL "正魔之战"
#define WAR_NAME_RANDOM_SEA     "乱星海争夺战"
#define WAR_NAME_INTERFACE      "界面战争"

// 战争阶段
#define WAR_PHASE_DECLARE       0     // 宣战阶段(准备期)
#define WAR_PHASE_BATTLE        1     // 战斗阶段
#define WAR_PHASE_SETTLE        2     // 结算阶段

// 战争准备时间(秒)
#define WAR_PREPARE_TIME        86400    // 24小时
#define WAR_BATTLE_TIME         7200     // 2小时

// 战功等级
#define WAR_MERIT_RECRUIT       1     // 新兵 (0)
#define WAR_MERIT_SCOUT         2     // 哨兵 (5000)
#define WAR_MERIT_SOLDIER       3     // 战士 (20000)
#define WAR_MERIT_VANGUARD      4     // 先锋 (80000)
#define WAR_MERIT_CAPTAIN       5     // 统领 (300000)
#define WAR_MERIT_GENERAL       6     // 将军 (1000000)
#define WAR_MERIT_MARSHAL       7     // 元帅 (5000000)

#define WAR_MERIT_THRESHOLD_1   0
#define WAR_MERIT_THRESHOLD_2   5000
#define WAR_MERIT_THRESHOLD_3   20000
#define WAR_MERIT_THRESHOLD_4   80000
#define WAR_MERIT_THRESHOLD_5   300000
#define WAR_MERIT_THRESHOLD_6   1000000
#define WAR_MERIT_THRESHOLD_7   5000000

// 战争奖励声望
#define WAR_REP_WIN             500    // 胜方全体+500
#define WAR_REP_LOSE            -200   // 败方-200
#define WAR_REP_KILL            20     // 击杀敌方+10~+30
#define WAR_REP_DESTROY         1000   // 摧毁据点+500~+2000
#define WAR_REP_MVP             2000   // MVP额外+2000

// 战争存储路径
#define WAR_PATH                "war"
#define WAR_MERIT_PATH          "war/merit"

// ===================== 外交事件系统 =====================

// 外交事件类型
#define DIPLOMACY_BORDER        1     // 边界冲突
#define DIPLOMACY_DISPUTE       2     // 种族纠纷
#define DIPLOMACY_RESOURCE      3     // 资源发现
#define DIPLOMACY_ALLIANCE      4     // 联盟提议
#define DIPLOMACY_DISASTER      5     // 天灾事件
#define DIPLOMACY_INVASION      6     // 外族入侵
#define DIPLOMACY_AUCTION       7     // 跨族拍卖会

// 外交立场
#define DIPLOMACY_STANCE_HELP       1.0    // 直接援助
#define DIPLOMACY_STANCE_SUPPORT    0.5    // 间接支持
#define DIPLOMACY_STANCE_NEUTRAL    0.0    // 保持中立
#define DIPLOMACY_STANCE_OPPOSE    -0.5    // 反对
#define DIPLOMACY_STANCE_ATTACK    -1.5    // 直接敌对

// 势力重要性系数
#define DIPLOMACY_IMPORT_CORE       1.5    // 事件核心方
#define DIPLOMACY_IMPORT_RELATED    1.0    // 事件相关方
#define DIPLOMACY_IMPORT_SPECTATOR  0.3    // 事件旁观方

// 外交事件频率(天)
#define DIPLOMACY_FREQ_BORDER       7
#define DIPLOMACY_FREQ_DISPUTE      14
#define DIPLOMACY_FREQ_RESOURCE     30
#define DIPLOMACY_FREQ_ALLIANCE     90
#define DIPLOMACY_FREQ_DISASTER     90
#define DIPLOMACY_FREQ_INVASION     180
#define DIPLOMACY_FREQ_AUCTION      365

// 基础声望变动
#define DIPLOMACY_REP_BASE          1000
#define DIPLOMACY_REP_MINOR         200

#endif
