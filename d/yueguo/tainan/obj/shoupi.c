// d/yueguo/tainan/obj/shoupi.c
// 兽皮（坊市炼器材料）
// Created for ticket #67

inherit ITEM;

void create()
{
        set_name("兽皮", ({ "shoupi", "pi" }) );
        set_weight(200);
        set("unit", "张");
        set("material", "hide");
        set("is_material", 1);
        set("material_id", "shoupi");
        set("value", 100);
        set("long", "一张鞣制过的兽皮，质地坚韧，可用于缝制皮甲或炼制法器。\n");
        setup();
}
