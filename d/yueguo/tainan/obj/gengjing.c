// d/yueguo/tainan/obj/gengjing.c
// 庚精（坊市炼器材料，稀有）
// Created for ticket #74

inherit ITEM;

void create()
{
        set_name("庚精", ({ "gengjing", "jing" }) );
        set_weight(300);
        set("unit", "粒");
        set("material", "metal");
        set("is_material", 1);
        set("material_id", "gengjing");
        set("value", 8000);
        set("long", "一粒米粒大小的庚精，通体泛着白金般的光泽，锋锐之气内敛，是炼制飞剑的极品材料，坊市中难得一见。\n");
        setup();
}
