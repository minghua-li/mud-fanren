// d/yueguo/obj/treasure.c
// 法宝通用基类 —— 1E §4.1 法宝数据结构落地
// 属性（由 FORGE_D 炼制生成时设置）：
//   treasure_type  : 法器/法宝/古宝/通天灵宝/玄天之宝（1E §1.1）
//   level          : 下品/中品/上品/极品（品质，FORGE_QUALITY_*）
//   element        : 金/木/水/火/土/风/雷/空间/时间（1E §1.3）
//   forbidden_count: 禁制层数（禁制越多威能越大）
//   attack/defense : 攻击/防御力加成（品质倍率已折算进数值）
//   special        : 特殊效果描述
//   require_level  : 使用所需境界 tier（SECT_D->query_cultivation_tier 判定）
// 装备接线：继承 EQUIP（ITEM + F_EQUIP）——获得/持有走 F_MOVE（背包），
//           装备走 F_EQUIP wield/wear，交易走 F_DBASE value（既有物品经济接线）。
// Created for ticket #74

#include <ansi.h>
#include <weapon.h>
#include <globals.h>
#include <forge.h>

inherit EQUIP;

void create()
{
    // 属性由 FORGE_D 炼制成功后 set，然后调 setup()
}

void setup()
{
    int atk, def;

    atk = query("attack");
    def = query("defense");

    if (atk > 0)
    {
        // 攻击类法宝：可 wield（御器）
        set("weapon_prop", ([ "attack": atk ]));
        set("flag", EDGED);
        set("skill_type", "sword");
        set("wield_msg", "$N祭起$n，法宝灵光流转，悬于$N身侧。\n");
        set("unwield_msg", "$N收起$n，法宝光华收敛。\n");
    }
    else if (def > 0)
    {
        // 防御类法宝：可 wear（护身）
        set("armor_prop", ([ "defense": def ]));
        set("armor_type", "shield");
        set("wear_msg", "$N将$n祭于身前，灵光化作护盾环绕周身。\n");
        set("unequip_msg", "$N收起$n，护体灵光散去。\n");
    }

    if (stringp(query("element")))
        set("material", "metal");

    ::setup();
}

// 境界限制：未达 require_level 不能装备/御使（接 #61 境界判定 SECT_D->query_cultivation_tier）
private int check_realm(object owner)
{
    int need;

    need = query("require_level");
    if (need <= 0) return 1;
    if (!objectp(owner) || !owner->is_character()) return 1;
    if (SECT_D->query_cultivation_tier(owner) >= need) return 1;
    return 0;
}

int wield()
{
    object owner;

    owner = environment();
    if (!check_realm(owner))
        return notify_fail("你境界不足，无法御使「" + query("name") + "」这件" +
                           query("treasure_type") + "。\n");
    return ::wield();
}

int wear()
{
    object owner;

    owner = environment();
    if (!check_realm(owner))
        return notify_fail("你境界不足，无法御使「" + query("name") + "」这件" +
                           query("treasure_type") + "。\n");
    return ::wear();
}

// 法宝属性展示（F_NAME long() 会追加 extra_long 输出）
string extra_long()
{
    string str;
    int atk, def, need;

    atk = query("attack");
    def = query("defense");
    need = query("require_level");

    str = "\n" HIC "品阶" NOR "：【" + query("treasure_type") + "·" + query("level") + "】\n";
    str += "属性：" + query("element") + "系";
    if (atk > 0)
        str += " · 攻击 +" + atk;
    if (def > 0)
        str += " · 防御 +" + def;
    str += " · 禁制 " + query("forbidden_count") + " 层\n";
    if (need > 0)
        str += "境界需求：" + SECT_D->tier_name(need) + "\n";
    if (stringp(query("special")) && query("special") != "")
        str += "特殊效果：" + query("special") + "\n";
    return str;
}
