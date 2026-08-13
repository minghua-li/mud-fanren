// d/yueguo/tainan/obj/jinjing.c
// 金精（坊市炼器材料）
// Created for ticket #74

inherit ITEM;

void create()
{
        set_name("金精", ({ "jinjing", "jing" }) );
        set_weight(300);
        set("unit", "块");
        set("material", "metal");
        set("is_material", 1);
        set("material_id", "jinjing");
        set("value", 1500);
        set("long", "一块金光内敛的金精，质地致密沉重，是锻造下品法宝的主材。\n");
        setup();
}
