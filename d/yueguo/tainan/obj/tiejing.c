// d/yueguo/tainan/obj/tiejing.c
// 铁精（坊市炼器材料）
// Created for ticket #67

inherit ITEM;

void create()
{
        set_name("铁精", ({ "tiejing", "jing" }) );
        set_weight(300);
        set("unit", "块");
        set("material", "metal");
        set("is_material", 1);
        set("material_id", "tiejing");
        set("value", 200);
        set("long", "一块拇指大的铁精，色泽青黑，入手沉重，是炼制基础法器常用的材料。\n");
        setup();
}
