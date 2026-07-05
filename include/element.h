// element.h
// 五行元素系统常量定义与战斗辅助函数
// 基于《凡人修仙传》设定, 面向 LPC MUD 实现

#ifndef __ELEMENT_H__
#define __ELEMENT_H__

#include <ansi.h>

// ===== 元素类型常量 =====
#define ELE_NONE    0

// 基础五行
#define ELE_GOLD    1   // 金
#define ELE_WOOD    2   // 木
#define ELE_WATER   3   // 水
#define ELE_FIRE    4   // 火
#define ELE_EARTH   5   // 土

// 变异属性
#define ELE_THUNDER 6   // 雷 (变异火)
#define ELE_ICE     7   // 冰 (变异水)
#define ELE_WIND    8   // 风 (变异木)

#define ELE_TOTAL_BASIC  5
#define ELE_TOTAL_ALL    8

// ===== 相克系数常量 =====
#define ELEM_COUNTER_ADV   1.5   // 克制时伤害倍率
#define ELEM_COUNTER_DIS   0.7   // 被克时伤害倍率
#define ELEM_VAR_ADV       1.2   // 变异属性对常规属性的基础优势
#define ELEM_VAR_EXTREME   2.0   // 变异属性极端克制 (如雷克鬼道)
#define ELEM_SYNERGY_RATIO 0.10  // 相生时增益系数

// ===== 元素名称查询 =====
string query_element_name(int elem)
{
    switch (elem)
    {
    case ELE_GOLD:   return "金";
    case ELE_WOOD:   return "木";
    case ELE_WATER:  return "水";
    case ELE_FIRE:   return "火";
    case ELE_EARTH:  return "土";
    case ELE_THUNDER:return "雷";
    case ELE_ICE:    return "冰";
    case ELE_WIND:   return "风";
    default:         return "无";
    }
}

// ===== 元素颜色查询 =====
// 遵循现有项目约定 (参考 feature/vein/vein.c)
string query_element_color(int elem)
{
    switch (elem)
    {
    case ELE_GOLD:   return HIW;
    case ELE_WOOD:   return HIG;
    case ELE_WATER:  return HIB;
    case ELE_FIRE:   return HIR;
    case ELE_EARTH:  return YEL;
    case ELE_THUNDER:return HIM;
    case ELE_ICE:    return HIC;
    case ELE_WIND:   return HIW;
    default:         return NOR;
    }
}

// ===== 五行相克系数计算 =====
// 返回攻击方 att_elem 对防御方 def_elem 的伤害倍率
// 克制: 1.5x  被克: 0.7x  无关: 1.0x
float calc_element_modifier(int att_elem, int def_elem)
{
    // 相克表：攻击方 -> (防御方 -> 系数)
    mapping counter = ([
        // 金克木, 火克金 (金被火克)
        ELE_GOLD:   ([ ELE_WOOD: ELEM_COUNTER_ADV, ELE_FIRE: ELEM_COUNTER_DIS ]),
        // 木克土, 金克木 (木被金克)
        ELE_WOOD:   ([ ELE_EARTH: ELEM_COUNTER_ADV, ELE_GOLD: ELEM_COUNTER_DIS ]),
        // 水克火, 土克水 (水被土克)
        ELE_WATER:  ([ ELE_FIRE: ELEM_COUNTER_ADV, ELE_EARTH: ELEM_COUNTER_DIS ]),
        // 火克金, 水克火 (火被水克)
        ELE_FIRE:   ([ ELE_GOLD: ELEM_COUNTER_ADV, ELE_WATER: ELEM_COUNTER_DIS ]),
        // 土克水, 木克土 (土被木克)
        ELE_EARTH:  ([ ELE_WATER: ELEM_COUNTER_ADV, ELE_WOOD: ELEM_COUNTER_DIS ]),
        // 变异 · 雷: 克水/冰/金, 被土制
        ELE_THUNDER:([ ELE_WATER: 1.8, ELE_ICE: 1.5, ELE_GOLD: 1.2, ELE_EARTH: 0.7 ]),
        // 变异 · 冰: 克火/风, 被雷克
        ELE_ICE:    ([ ELE_FIRE: 1.5, ELE_WIND: 1.3, ELE_THUNDER: 0.6 ]),
        // 变异 · 风: 克土/雷, 被冰克
        ELE_WIND:   ([ ELE_EARTH: 1.5, ELE_THUNDER: 1.2, ELE_ICE: 0.7 ]),
    ]);

    if (undefinedp(counter[att_elem]))
        return 1.0;
    if (undefinedp(counter[att_elem][def_elem]))
        return 1.0;
    return counter[att_elem][def_elem];
}

// ===== 相生关系查询 =====
// 返回 elem 所生的元素, 若无不返回 0
int query_element_generates(int elem)
{
    mapping generation = ([
        ELE_GOLD:   ELE_WATER,   // 金生水
        ELE_WATER:  ELE_WOOD,    // 水生木
        ELE_WOOD:   ELE_FIRE,    // 木生火
        ELE_FIRE:   ELE_EARTH,   // 火生土
        ELE_EARTH:  ELE_GOLD,    // 土生金
    ]);
    if (undefinedp(generation[elem]))
        return 0;
    return generation[elem];
}

// ===== 获取战斗用五行元素 =====
// 从角色 dbase 获取五行属性
// 优先级: spirit_root > 直接设置的 element > 随机默认
// 返回元素常量, 如 ELE_GOLD / ELE_NONE
int query_character_element(object ob)
{
    mapping sr;

    // 1. 尝试从 spirit_root (灵根) 获取主属性
    sr = ob->query("spirit_root");
    if (mapp(sr) && stringp(sr["main_element"]))
    {
        string me = sr["main_element"];
        if (me == "金") return ELE_GOLD;
        if (me == "木") return ELE_WOOD;
        if (me == "水") return ELE_WATER;
        if (me == "火") return ELE_FIRE;
        if (me == "土") return ELE_EARTH;
        if (me == "雷") return ELE_THUNDER;
        if (me == "冰") return ELE_ICE;
        if (me == "风") return ELE_WIND;
    }

    // 2. 尝试从 spirit_root_data (另一种 dbase key)
    sr = ob->query("spirit_root_data");
    if (mapp(sr) && stringp(sr["main_element"]))
    {
        string me = sr["main_element"];
        if (me == "金") return ELE_GOLD;
        if (me == "木") return ELE_WOOD;
        if (me == "水") return ELE_WATER;
        if (me == "火") return ELE_FIRE;
        if (me == "土") return ELE_EARTH;
        if (me == "雷") return ELE_THUNDER;
        if (me == "冰") return ELE_ICE;
        if (me == "风") return ELE_WIND;
    }

    // 3. 直接设置的 element 属性 (NPC 用)
    if (intp(ob->query("element")))
        return ob->query("element");

    // 4. NPC 默认随机分配
    // 用 combat_exp 做种子保证同一 NPC 每次分配一致
    if (!userp(ob))
    {
        int exp = ob->query("combat_exp");
        if (exp < 1) exp = 1;
        return 1 + (exp % ELE_TOTAL_BASIC);
    }

    return ELE_NONE;
}

#endif
