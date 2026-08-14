// d/yueguo/tainan/obj/yinjing.c
// 银精（坊市炼器材料）
// Created for ticket #74

inherit ITEM;

void create()
{
        set_name("银精", ({ "yinjing", "jing" }) );
        set_weight(300);
        set("unit", "块");
        set("material", "metal");
        set("is_material", 1);
        set("material_id", "yinjing");
        set("value", 500);
        set("long", "一块银光灿灿的银精，入手微凉，杂质已炼去大半，是炼制中品法器的常用材料。\n");
        setup();
}
