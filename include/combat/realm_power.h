// realm_power.h
// 境界压制 / 灵根伤害加成（#78）
//   设计依据：1F §4.1 境界与战斗力关系、02-扩充内容/02-战斗机制与平衡.md §2.1 五行相克数值公式
//            （境界压制系数表）、§2.2 伤害计算模型（灵根加成系数表）
//   境界存储口径：#61 统一约定（root_refine_d.c「境界(realm)属性接线」段落）
//     realm       = 中文境界字符串（"炼气7层" / "筑基初期"）
//     realm_index = 大境界索引（0=凡人 1=炼气 2=筑基 … 8=大乘）
//     realm_sub   = 小阶段（初期/中期/后期/巅峰/大圆满）
//   本文件内的「大境界序号」= realm_index - 1（0=炼气 … 7=大乘），与
//   sect_d.c 的 realm_names 下标、query_cultivation_tier() 的 index*3+stage 同口径。
//
// 关键取舍：任一方境界未声明（如历史 NPC 未设置 realm_index）时系数取中性 1.0，
//   绝不按 combat_exp 猜测境界——避免既有 NPC 的战斗平衡被静默改写。
//   NPC 需要参与境界压制时，显式 set("realm_index", n) + set("realm_sub", "初期")
//   或 set("realm", "筑基初期") 即可。

#ifndef __COMBAT_REALM_POWER__
#define __COMBAT_REALM_POWER__

#include <element.h>

// 大境界名（下标 = realm_index - 1，即 0=炼气 … 7=大乘）
nosave string *realm_power_names = ({
    "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘",
});

// ===== 境界压制系数（千分比，对齐 §2.1 境界压制系数表）=====
#define REALM_SUP_SAME          1000    // 同境界 ×1.0
#define REALM_SUP_STEP_UP       1200    // 每高 1 小阶段 ×1.2
#define REALM_SUP_STEP_DOWN      850    // 每低 1 小阶段 ×0.85
#define REALM_SUP_STEP_CAP      1440    // 同境界内小阶段累计上限 ×1.44（2 小阶段）
#define REALM_SUP_MAJOR_ADV     2000    // 高 1 大境界 ×2.0（表中「压制方系数」上限）
#define REALM_SUP_MAJOR_DIS      500    // 低 1 大境界 ×0.5
#define REALM_SUP_FLOOR          250    // 低 2 大境界及以下 ×0.25（表中「无法命中/可忽略」下限）

// ===== 灵根伤害系数（千分比，对齐 §2.2 灵根加成系数表）=====
// 下标 = spirit_root["quality_idx"]（0=天灵根T0 1=变异T1 2=真T2 3=假T3 4=伪T4 5=无灵根T5）
// 基础倍率：全法术基础倍率（天 1.5 / 变异 1.3 / 真 1.0 / 假 0.8 / 伪 0.6 / 无 0.4）
//   注：§2.2 表只列天/真/伪/变异四档，假灵根（T3）按真(1.0/+15%)与伪(0.6/+5%)之间线性插值，
//       取 0.8 / +10% / -3%；无灵根（T5）不进入玩家生成概率，仅作凡人对齐兜底。
nosave int *realm_root_base_pm = ({ 1500, 1300, 1000, 800, 600, 400 });
// 同系加成：攻击属性与灵根属性（或变异属性）相同时追加（再按灵根强度折算）
nosave int *realm_root_same_pm = ({ 300, 250, 150, 100, 50, 0 });
// 异系惩罚：攻击属性非灵根属性时追加（仅常规五行；变异灵根同系只认变异属性，常规属性 -10%）
nosave int *realm_root_penalty_pm = ({ -150, -100, -50, -30, 0, 0 });

// 大境界序号（0=炼气 … 7=大乘）；未声明境界返回 -1（中性）
int realm_power_index(object ob)
{
    int idx;
    string realm;
    int i;

    if (!objectp(ob)) return -1;

    idx = ob->query("realm_index");
    if (intp(idx) && idx > 0) return idx - 1;

    realm = ob->query("realm");
    if (!stringp(realm) || realm == "") return -1;
    for (i = 0; i < sizeof(realm_power_names); i++)
        if (strsrch(realm, realm_power_names[i]) != -1) return i;

    return -1;
}

// 小阶段序号（0=初期 1=中期 2=后期）；未知返回 -1
int realm_power_stage(object ob)
{
    string sub, realm;

    if (!objectp(ob)) return -1;

    sub = ob->query("realm_sub");
    if (stringp(sub) && sub != "")
    {
        if (strsrch(sub, "初期") != -1) return 0;
        if (strsrch(sub, "中期") != -1) return 1;
        if (strsrch(sub, "后期") != -1) return 2;
        if (strsrch(sub, "巅峰") != -1) return 2;
        if (strsrch(sub, "大圆满") != -1) return 2;
    }

    realm = ob->query("realm");
    if (!stringp(realm)) return -1;
    if (strsrch(realm, "初期") != -1) return 0;
    if (strsrch(realm, "中期") != -1) return 1;
    if (strsrch(realm, "后期") != -1) return 2;

    return -1;
}

// 境界压制系数（千分比）。任一方境界未声明 / 同一方 → 1000（中性）
int realm_power_modifier_permille(object att, object def)
{
    int ai, di, as, ds, big, step, v, i;

    if (!objectp(att) || !objectp(def)) return REALM_SUP_SAME;
    if (att == def) return REALM_SUP_SAME;

    ai = realm_power_index(att);
    di = realm_power_index(def);
    if (ai < 0 || di < 0) return REALM_SUP_SAME;

    as = realm_power_stage(att);
    ds = realm_power_stage(def);
    if (as < 0) as = 1;     // 缺小阶段时按「中期」中性处理
    if (ds < 0) ds = 1;

    big = ai - di;
    if (big == 0)
    {
        step = as - ds;
        if (step == 0) return REALM_SUP_SAME;
        if (step > 0)
        {
            v = REALM_SUP_SAME;
            for (i = 0; i < step; i++)
            {
                v = v * REALM_SUP_STEP_UP / 1000;
                if (v > REALM_SUP_STEP_CAP) return REALM_SUP_STEP_CAP;
            }
            return v;
        }
        step = -step;
        v = REALM_SUP_SAME;
        for (i = 0; i < step; i++)
            v = v * REALM_SUP_STEP_DOWN / 1000;
        return v;
    }

    if (big > 0) return REALM_SUP_MAJOR_ADV;
    if (big == -1) return REALM_SUP_MAJOR_DIS;
    return REALM_SUP_FLOOR;
}

// 境界压制系数（浮点倍率，便于外部调用/展示）
float realm_power_modifier(object att, object def)
{
    return realm_power_modifier_permille(att, def) / 1000.0;
}

// 组合战斗系数（千分比）：境界压制 × 灵根加成（法术）
//   供不走 calc_damage 的伤害来源（#76 法术施法 shifa）复用同一套原语，
//   避免法术自成一套境界/灵根口径而与普攻数值漂移。
int combat_power_permille(object me, object victim, int is_spell, int elem)
{
    int pm;
    mixed *info;

    pm = realm_power_modifier_permille(me, victim);
    if (!is_spell) return pm;

    info = spirit_root_damage_info(me, elem);
    return pm * info[0] / 1000;
}

// 是否法术攻击
//   1) #76 法术 perform 在出手前写入 combat/spell_elem 或 combat/attack_type = "spell"
//   2) 本次攻击的招式槽为 spells（法术槽武功，见 kungfu/skill/daomen-shufa.c 等）
int combat_is_spell_attack(object me)
{
    if (!objectp(me)) return 0;
    if (intp(me->query_temp("combat/spell_elem"))) return 1;
    if (me->query_temp("combat/attack_type") == "spell") return 1;
    return 0;
}

int combat_is_spell_slot(string attack_skill)
{
    return stringp(attack_skill) && attack_skill == "spells";
}

// 本次攻击的五行属性（法术优先用 perform 声明的元素，否则取角色主属性）
int combat_attack_element(object me)
{
    int e;

    if (!objectp(me)) return ELE_NONE;

    e = me->query_temp("combat/spell_elem");
    if (intp(e) && e > ELE_NONE && e <= ELE_TOTAL_ALL) return e;

    return query_character_element(me);
}

// 灵根伤害修正：返回 ({ 千分比系数, 是否同系(0/1), 灵根品质下标 })
//   无灵根数据 → ({ 1000, 0, -1 })（中性）
//   基础倍率对所有法术生效；同系加成按灵根强度折算（强度 100 = 满额）；
//   异系惩罚仅对常规五行法术（金木水火土）生效，变异属性走同系判定。
mixed *spirit_root_damage_info(object ob, int elem)
{
    mapping root;
    string name, variant;
    string *elems;
    int q, strength, pm, same;

    if (!objectp(ob)) return ({ 1000, 0, -1 });

    root = ob->query("spirit_root");
    if (!mapp(root)) return ({ 1000, 0, -1 });

    q = root["quality_idx"];
    if (!intp(q) || q < 0 || q >= 6) return ({ 1000, 0, -1 });

    strength = root["strength"];
    if (!intp(strength) || strength < 0) strength = 0;
    if (strength > 100) strength = 100;

    name = query_element_name(elem);
    if (!stringp(name) || name == "无") return ({ 1000, 0, q });

    same = 0;
    variant = root["variant"];
    elems = root["elements"];
    if (q == 1 && stringp(variant) && variant != "")
    {
        // 变异灵根（T1）：同系只认变异属性（§2.2「变异属性 +25%（常规属性 -10%）」），
        // 构成它的基础五行（如雷=金+水）按常规属性吃 -10% 惩罚。
        if (name == variant) same = 1;
    }
    else
    {
        if (stringp(variant) && name == variant) same = 1;
        else if (arrayp(elems) && member_array(name, elems) >= 0) same = 1;
    }

    pm = realm_root_base_pm[q];
    if (same)
        pm += realm_root_same_pm[q] * strength / 100;
    else if (elem >= ELE_GOLD && elem <= ELE_EARTH)
        pm += realm_root_penalty_pm[q];

    return ({ pm, same, q });
}

#endif
