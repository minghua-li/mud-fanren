// spirit_root.h
// 灵根系统常量与宏定义
// 对应设计文档：02-扩充内容/02-灵根养成与突破.md

#ifndef __SPIRIT_ROOT__
#define __SPIRIT_ROOT__

//=============================================================================
// 灵根品质枚举（品质从低到高）
//=============================================================================
#define SPIRIT_ROOT_NONE        0   // 无灵根（凡人）
#define SPIRIT_ROOT_PSEUDO      1   // 伪灵根 T4
#define SPIRIT_ROOT_FAKE        2   // 假灵根 T3
#define SPIRIT_ROOT_TRUE        3   // 真灵根 T2
#define SPIRIT_ROOT_VARIANT     4   // 变异灵根 T1
#define SPIRIT_ROOT_HEAVENLY    5   // 天灵根 T0

// 品质名称映射（配中文显示用）
#define SPIRIT_ROOT_QUALITY_NAME ([ \
    SPIRIT_ROOT_NONE : "无灵根", \
    SPIRIT_ROOT_PSEUDO : "伪灵根", \
    SPIRIT_ROOT_FAKE : "假灵根", \
    SPIRIT_ROOT_TRUE : "真灵根", \
    SPIRIT_ROOT_VARIANT : "变异灵根", \
    SPIRIT_ROOT_HEAVENLY : "天灵根", \
])

// 品质星级显示
#define SPIRIT_ROOT_QUALITY_STARS ([ \
    SPIRIT_ROOT_NONE : "☆☆☆☆☆☆", \
    SPIRIT_ROOT_PSEUDO : "★☆☆☆☆☆", \
    SPIRIT_ROOT_FAKE : "★★☆☆☆☆", \
    SPIRIT_ROOT_TRUE : "★★★☆☆☆", \
    SPIRIT_ROOT_VARIANT : "★★★★☆☆", \
    SPIRIT_ROOT_HEAVENLY : "★★★★★★", \
])

//=============================================================================
// 灵根基础属性表（品质 → 基础数值）
//=============================================================================
// 修炼速度系数
#define SPIRIT_ROOT_SPEED_FACTOR ([ \
    SPIRIT_ROOT_NONE : 0.0, \
    SPIRIT_ROOT_PSEUDO : 0.3, \
    SPIRIT_ROOT_FAKE : 0.6, \
    SPIRIT_ROOT_TRUE : 1.0, \
    SPIRIT_ROOT_VARIANT : 2.3, \
    SPIRIT_ROOT_HEAVENLY : 2.5, \
])

// 品质基准强度（影响功法层数上限和突破判定）
#define SPIRIT_ROOT_BASE_STRENGTH ([ \
    SPIRIT_ROOT_NONE : 0, \
    SPIRIT_ROOT_PSEUDO : 30, \
    SPIRIT_ROOT_FAKE : 50, \
    SPIRIT_ROOT_TRUE : 70, \
    SPIRIT_ROOT_VARIANT : 90, \
    SPIRIT_ROOT_HEAVENLY : 100, \
])

// 品质强度上限（当前品质下灵根强度可达到的最大值）
#define SPIRIT_ROOT_MAX_STRENGTH ([ \
    SPIRIT_ROOT_NONE : 0, \
    SPIRIT_ROOT_PSEUDO : 80, \
    SPIRIT_ROOT_FAKE : 85, \
    SPIRIT_ROOT_TRUE : 95, \
    SPIRIT_ROOT_VARIANT : 100, \
    SPIRIT_ROOT_HEAVENLY : 100, \
])

// 大境界突破品质系数
#define SPIRIT_ROOT_BREAKTHROUGH_QUALITY_FACTOR ([ \
    SPIRIT_ROOT_NONE : 0.0, \
    SPIRIT_ROOT_PSEUDO : 0.3, \
    SPIRIT_ROOT_FAKE : 0.7, \
    SPIRIT_ROOT_TRUE : 1.0, \
    SPIRIT_ROOT_VARIANT : 1.2, \
    SPIRIT_ROOT_HEAVENLY : 5.0, \
])

// 灵力上限修正
#define SPIRIT_ROOT_MANA_LIMIT_FACTOR ([ \
    SPIRIT_ROOT_NONE : 0.0, \
    SPIRIT_ROOT_PSEUDO : 0.8, \
    SPIRIT_ROOT_FAKE : 0.9, \
    SPIRIT_ROOT_TRUE : 1.0, \
    SPIRIT_ROOT_VARIANT : 1.2, \
    SPIRIT_ROOT_HEAVENLY : 1.3, \
])

// 灵力恢复速度修正（每分钟）
#define SPIRIT_ROOT_MANA_REGEN_FACTOR ([ \
    SPIRIT_ROOT_NONE : 0.0, \
    SPIRIT_ROOT_PSEUDO : 0.6, \
    SPIRIT_ROOT_FAKE : 0.8, \
    SPIRIT_ROOT_TRUE : 1.0, \
    SPIRIT_ROOT_VARIANT : 1.3, \
    SPIRIT_ROOT_HEAVENLY : 1.5, \
])

// 后天品质提升上限（最高可提升到真灵根）
#define SPIRIT_ROOT_ACQUIRED_MAX SPIRIT_ROOT_TRUE

//=============================================================================
// 五行属性定义
//=============================================================================
#define SPIRIT_ELEMENT_METAL    "金"
#define SPIRIT_ELEMENT_WOOD     "木"
#define SPIRIT_ELEMENT_WATER    "水"
#define SPIRIT_ELEMENT_FIRE     "火"
#define SPIRIT_ELEMENT_EARTH    "土"

// 所有五行属性列表
#define SPIRIT_ELEMENT_ALL ({ "金", "木", "水", "火", "土" })

// 变异属性列表
#define SPIRIT_ELEMENT_VARIANTS ({ "雷", "冰", "风", "暗" })

// 五行相克表（攻击方 → 被克方）
#define SPIRIT_ELEMENT_COUNTER ([ \
    "金" : "木", \
    "木" : "土", \
    "水" : "火", \
    "火" : "金", \
    "土" : "水", \
])

// 五行相生表
#define SPIRIT_ELEMENT_GENERATE ([ \
    "金" : "水", \
    "水" : "木", \
    "木" : "火", \
    "火" : "土", \
    "土" : "金", \
])

// 变异灵根基础组合
#define SPIRIT_VARIANT_COMBINATION ([ \
    "雷" : ({ "金", "水" }), \
    "冰" : ({ "水", "土" }), \
    "风" : ({ "火", "木" }), \
    "暗" : ({ "水", "火" }), \
])

//=============================================================================
// 灵根洗练系统常量
//=============================================================================

// 洗练方式
#define REFINE_METHOD_NORMAL        1   // 常规洗练（消耗灵石）
#define REFINE_METHOD_DEEPER        2   // 深度洗练（消耗灵石+贡献）
#define REFINE_METHOD_PILL          3   // 丹药洗练（消耗洗髓丹）
#define REFINE_METHOD_HERB          4   // 天材地宝洗练（消耗净灵莲等）

// 洗练结果类型
#define REFINE_RESULT_MINOR         1   // 小幅提升（强度+1~3）
#define REFINE_RESULT_MODERATE      2   // 中等提升（强度+5~10）
#define REFINE_RESULT_MAJOR         3   // 大幅提升（强度+10~20）
#define REFINE_RESULT_NOTHING       4   // 无变化
#define REFINE_RESULT_BOOSTED       5   // 强化提升（特殊事件触发）

// 洗练成本基数
#define REFINE_COST_BASE_NORMAL     5000    // 常规洗练：灵石×5000
#define REFINE_COST_BASE_DEEPER     10000   // 深度洗练：灵石×10000
#define REFINE_CONTRIBUTION_DEEPER  2000    // 深度洗练：门派贡献×2000

// 洗练冷却时间（游戏天数）
#define REFINE_COOLDOWN_NORMAL      30      // 常规洗练冷却
#define REFINE_COOLDOWN_DEEPER      15      // 深度洗练冷却（略短）

//=============================================================================
// 境界定义（简称，便于突破判定）
//=============================================================================
#define REALM_QI_REFINERY       1   // 炼气期
#define REALM_FOUNDATION        2   // 筑基期
#define REALM_CORE_FORMATION    3   // 结丹期
#define REALM_NASCENT_SOUL      4   // 元婴期
#define REALM_SPIRIT_TRANSFORM  5   // 化神期
#define REALM_REFINERY_VOID     6   // 炼虚期
#define REALM_BODY_INTEGRATION  7   // 合体期
#define REALM_MAJOR_ACCEPTANCE  8   // 大乘期
#define REALM_TRIBULATION       9   // 渡劫期

//=============================================================================
// 突破系统常量
//=============================================================================

// 突破方式
#define BREAK_METHOD_NATURAL        1   // 自然突破
#define BREAK_METHOD_PILL_AID       2   // 丹药辅助
#define BREAK_METHOD_SPIRIT_STONE   3   // 灵石灌注
#define BREAK_METHOD_TREASURE       4   // 天材地宝
#define BREAK_METHOD_SECRET_REALM   5   // 秘境突破

// 突破方式基础成功率（百分比）
#define BREAK_METHOD_BASE_RATE ([ \
    BREAK_METHOD_NATURAL : 35, \
    BREAK_METHOD_PILL_AID : 50, \
    BREAK_METHOD_SPIRIT_STONE : 45, \
    BREAK_METHOD_TREASURE : 70, \
    BREAK_METHOD_SECRET_REALM : 60, \
])

// 突破失败灵根强度惩罚
#define BREAK_FAIL_STRENGTH_COST ([ \
    BREAK_METHOD_NATURAL : 5, \
    BREAK_METHOD_PILL_AID : 3, \
    BREAK_METHOD_SPIRIT_STONE : 0, \
    BREAK_METHOD_TREASURE : 10, \
    BREAK_METHOD_SECRET_REALM : 0, \
])

// 突破事件随机分支概率（15%总触发率）
#define BREAK_EVENT_HEART_DEVIL      5   // 心魔入侵
#define BREAK_EVENT_HEAVENLY_BOON    3   // 天降机缘
#define BREAK_EVENT_MANA_RAMPAGE     4   // 灵力暴走
#define BREAK_EVENT_BOTTLE_NECK      2   // 瓶颈感应
#define BREAK_EVENT_SKILL_RESONANCE  1   // 功法共鸣

// 连续失败保底（连续3次失败后下次概率+20%）
#define BREAK_STREAK_FAIL_THRESHOLD  3
#define BREAK_STREAK_FAIL_BONUS      20

// 大境界突破 - 伪灵根特殊上限
#define PSEUDO_MAX_BREAK_RATE        50   // 伪灵根结丹率上限50%

//=============================================================================
// Debuff 常量
//=============================================================================

// 灵根负面效果类型
#define ROOT_DEBUFF_SHOCK        1   // 灵根震荡（修炼效率-30%，24小时）
#define ROOT_DEBUFF_UNSTABLE     2   // 灵根不稳（下次突破概率-10%）
#define ROOT_DEBUFF_DAMAGE       3   // 灵根损伤（强度下降，持续衰减）
#define ROOT_DEBUFF_SEAL         4   // 灵根封闭（效果归零，7天）
#define ROOT_DEBUFF_SIDE_EFFECT  5   // 变异副作用（变异灵根专属）

// 默认持续时间（心跳数，假设 1 心约 ≈ 2 秒，现实时间）
#define ROOT_DEBUFF_DURATION_SHORT    43200   // 24 小时
#define ROOT_DEBUFF_DURATION_LONG     604800  // 7 天

//=============================================================================
// 品质提升路径
//=============================================================================

// 品质提升所需资源（从哪个品质 → 提升到下一级）
// ([ 当前品质 : ({ 条件检查函数, 消耗描述 }) ])
// 具体消耗在 daemon 中实现

// 品质提升档位映射
#define QUALITY_UPGRADE_PATH ([ \
    SPIRIT_ROOT_PSEUDO : SPIRIT_ROOT_FAKE, \
    SPIRIT_ROOT_FAKE : SPIRIT_ROOT_TRUE, \
    SPIRIT_ROOT_TRUE : SPIRIT_ROOT_VARIANT, \
    SPIRIT_ROOT_VARIANT : SPIRIT_ROOT_HEAVENLY, \
])

//=============================================================================
// 属性键路径（用于 dbase set/query）
//=============================================================================
#define ROOT_PROP_QUALITY       "spirit_root/quality"
#define ROOT_PROP_STRENGTH      "spirit_root/strength"
#define ROOT_PROP_PURITY        "spirit_root/purity"
#define ROOT_PROP_ELEMENTS      "spirit_root/elements"
#define ROOT_PROP_MAIN_ELEMENT  "spirit_root/main_element"
#define ROOT_PROP_EXP           "spirit_root/exp"
#define ROOT_PROP_LEVEL         "spirit_root/level"
#define ROOT_PROP_BREAK_COUNT   "spirit_root/break_count"
#define ROOT_PROP_REFINE_COUNT  "spirit_root/refine_count"
#define ROOT_PROP_FAIL_STREAK   "spirit_root/fail_streak"
#define ROOT_PROP_DEBUFF        "spirit_root/debuff"
#define ROOT_PROP_LAST_REFINE   "spirit_root/last_refine_time"
#define ROOT_PROP_LAST_BREAK    "spirit_root/last_break_time"
#define ROOT_PROP_QUALITY_SOURCE "spirit_root/quality_source"  // "innate" | "acquired"

//=============================================================================
// 任务链常量
//=============================================================================
#define BREAK_TASK_CHAIN_QI      "break_task_qi"       // 炼气突破任务链
#define BREAK_TASK_CHAIN_FOUND   "break_task_found"    // 筑基突破任务链
#define BREAK_TASK_CHAIN_CORE    "break_task_core"     // 结丹突破任务链
#define BREAK_TASK_CHAIN_NASCENT "break_task_nascent"  // 元婴突破任务链

#endif  // __SPIRIT_ROOT__
