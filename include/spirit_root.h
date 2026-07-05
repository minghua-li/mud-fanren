// spirit_root.h
// 灵根系统常量定义
// 基于《凡人修仙传》设定，面向 LPC MUD 实现
// 

#ifndef __SPIRIT_ROOT__
#define __SPIRIT_ROOT__

// 灵根品质（T0-T5）
#define ROOT_T0          "天灵根"       // 单种纯属性，修炼速度×2.5，自动结丹
#define ROOT_T1          "变异灵根"     // 变异属性（雷/冰/风/暗），修炼速度×2.3
#define ROOT_T2          "真灵根"       // 2-3种属性，修炼速度×1.0
#define ROOT_T3          "假灵根"       // 3-4种属性，修炼速度×0.6
#define ROOT_T4          "伪灵根"       // 4-5种杂属性，修炼速度×0.3
#define ROOT_T5          "无灵根"       // 无属性，修炼速度×0（凡人NPC）

// 灵根品质索引
#define ROOT_QUALITY_T0  0
#define ROOT_QUALITY_T1  1
#define ROOT_QUALITY_T2  2
#define ROOT_QUALITY_T3  3
#define ROOT_QUALITY_T4  4
#define ROOT_QUALITY_T5  5

// 灵根品质名称数组索引
#define ROOT_QUALITY_TOTAL 6

// 五行属性
#define ROOT_METAL      "金"
#define ROOT_WOOD       "木"
#define ROOT_WATER      "水"
#define ROOT_FIRE       "火"
#define ROOT_EARTH      "土"

// 变异灵根子类型
#define ROOT_VAR_THUNDER    "雷"        // 金+水变异，攻击最强
#define ROOT_VAR_ICE        "冰"        // 水+土变异，冰冻控制
#define ROOT_VAR_WIND       "风"        // 风属性变异，速度极快
#define ROOT_VAR_DARK       "暗"        // 特殊变异，隐匿吞噬

// 五行总数
#define ROOT_ELEMENT_TOTAL  5

// 灵根品质对应的修炼速度系数（万分比，避免浮点数）
// 天灵根=25000, 变异灵根=23000, 真灵根=10000, 假灵根=6000, 伪灵根=3000, 无灵根=0
#define ROOT_SPEED_T0       25000
#define ROOT_SPEED_T1       23000
#define ROOT_SPEED_T2       10000
#define ROOT_SPEED_T3       6000
#define ROOT_SPEED_T4       3000
#define ROOT_SPEED_T5       0

// 灵根强度上限（0-100）
#define ROOT_STRENGTH_MAX   100
#define ROOT_STRENGTH_T0_MIN 95
#define ROOT_STRENGTH_T1_MIN 85
#define ROOT_STRENGTH_T2_MIN 70
#define ROOT_STRENGTH_T3_MIN 50
#define ROOT_STRENGTH_T4_MIN 30
#define ROOT_STRENGTH_T5_MIN 0

// 灵根生成概率（基于原文：万人中仅一人有灵根，有灵根者中伪灵根占绝大多数）
// 此处概率以"有灵根者"为分母（排除无灵根），即玩家创建角色自动获得灵根
#define ROOT_PROB_T0        2       // 天灵根 0.2%（按有灵根者计）
#define ROOT_PROB_T1        8       // 变异灵根 0.8%
#define ROOT_PROB_T2        150     // 真灵根 15%
#define ROOT_PROB_T3        200     // 假灵根 20%
#define ROOT_PROB_T4        640     // 伪灵根 64%
// 总计 = 1000（千分比）

// 基础属性加点——初始总点数
#define INIT_ATTR_POINTS    80      // 四项主属性初始可分配总点数
#define INIT_ATTR_BASE      10      // 每项属性基础值
#define INIT_ATTR_MAX       30      // 单项属性最大值

// 灵根在 dbase 中的存储路径
#define SPIRIT_ROOT_DATA    "spirit_root"

// 灵根存储 mapping 的 key
#define SR_QUALITY          "quality"       // 灵根品质名称
#define SR_QUALITY_IDX      "quality_idx"   // 灵根品质索引 (0-5)
#define SR_ELEMENTS         "elements"      // 五行属性数组
#define SR_VARIANT          "variant"       // 变异类型（仅变异灵根有）
#define SR_STRENGTH         "strength"      // 灵根强度 (0-100)
#define SR_PURITY           "purity"        // 灵根精纯度 (0-100)
#define SR_MAIN_ELEMENT     "main_element"  // 主属性

#endif
