// spell.h
// 五行法术体系常量（#76：1F §1.1 五行基础法术落地）
//   设计依据：.knowledge/combat/1F-法术剑诀阵法战斗.md §1.1「法术等级分类 / 各属性法术详情」
//            §1.2 雷法、§1.3 冰焰/异火、§2.2 技能等级系数
//   数据单真源：每个法术 skill 文件用 query_spell_info() 返回自身元数据（威力/法力/冷却/效果），
//            SPELL_D 只做别名索引与施法流程，不在 daemon 里复制数值。
//   法术槽：法术 skill 的 valid_enable 返回 "spells"（与 kungfu/skill/daomen-shufa.c 等既有法术功法一致），
//            施法命令 shifa 直接按技能等级结算，不依赖 enable/prepare。

#ifndef __SPELL_H__
#define __SPELL_H__

#include <element.h>

// ===== 法术品阶（1F §1.1 分级表的最低需求境界）=====
#define SPELL_LOW               1   // 初级下阶：炼气初期
#define SPELL_MID               2   // 初级中阶：炼气中期
#define SPELL_HIGH              3   // 初级上阶：炼气后期~筑基

// 品阶 → 施展/修习所需 tier（与 SECT_D->query_cultivation_tier 同口径：0=炼气初 1=炼气中 2=炼气后）
#define SPELL_TIER_LOW          0
#define SPELL_TIER_MID          1
#define SPELL_TIER_HIGH         2

// ===== 法术类型 =====
#define SPELL_TYPE_ATTACK       1   // 攻击法术（需敌方目标）
#define SPELL_TYPE_CONTROL      2   // 控制法术（需敌方目标，附带负面状态）
#define SPELL_TYPE_DEFEND       3   // 防御/辅助法术（只能对自己）

// ===== 附加效果 =====
#define SPELL_FX_NONE           0   // 无附加效果（纯伤害）
#define SPELL_FX_BURN           1   // 灼烧：持续伤害
#define SPELL_FX_SLOW           2   // 减速：目标身法下降
#define SPELL_FX_BIND           3   // 束缚：目标身法与招架下降 + 短暂迟滞
#define SPELL_FX_GUARD          4   // 护体：自身防御加成
#define SPELL_FX_PIERCE         5   // 破甲：本次伤害额外穿透

// ===== 战斗结算参数 =====
// 技能等级系数（千分比）：500 + 等级×10 → Lv1 0.51、Lv20 0.70、Lv60 1.10、Lv100 1.50、Lv200 2.50
// 对应 1F §2.2「技能等级系数」四档（Lv1-20 0.3-0.8 / 21-60 0.8-1.2 / 61-100 1.2-1.8 / 100+ 1.8-2.5）
#define SPELL_COEF_BASE         500
#define SPELL_COEF_PER_LEVEL    10
#define SPELL_COEF_MAX          2500

// 灼烧：每次跳动伤害占比（千分比）与跳数/间隔（秒）
#define SPELL_BURN_RATIO        250
#define SPELL_BURN_TICKS        3
#define SPELL_BURN_INTERVAL     3

// 减速/束缚的 debuff 量（点）与持续（秒）
#define SPELL_SLOW_DODGE        40
#define SPELL_SLOW_DURATION     12
#define SPELL_BIND_DODGE        30
#define SPELL_BIND_PARRY        30
#define SPELL_BIND_DURATION     10
#define SPELL_GUARD_DEFENSE     60
#define SPELL_GUARD_DURATION    15

// 破甲：额外伤害占比（千分比）
#define SPELL_PIERCE_RATIO      300

// 施法硬直（心跳）与施法后技能熟练度收益（按伤害千分比）
#define SPELL_CAST_BUSY         1
#define SPELL_IMPROVE_RATIO     500

// 法术属性名 → ELE_* 常量（复用 include/element.h 的五行体系）
int spell_element_id(string name)
{
    if (!stringp(name)) return ELE_NONE;
    switch (name)
    {
    case "金": return ELE_GOLD;
    case "木": return ELE_WOOD;
    case "水": return ELE_WATER;
    case "火": return ELE_FIRE;
    case "土": return ELE_EARTH;
    case "雷": return ELE_THUNDER;
    case "冰": return ELE_ICE;
    case "风": return ELE_WIND;
    }
    return ELE_NONE;
}

// 法术品阶 → 所需 tier（0=炼气初期 1=炼气中期 2=炼气后期）
int spell_tier_need(int level)
{
    if (level >= SPELL_HIGH) return SPELL_TIER_HIGH;
    if (level >= SPELL_MID) return SPELL_TIER_MID;
    return SPELL_TIER_LOW;
}

#endif  // __SPELL_H__
