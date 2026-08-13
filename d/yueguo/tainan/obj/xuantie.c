// d/yueguo/tainan/obj/xuantie.c
// 玄铁（坊市炼器材料）
// Created for ticket #74

inherit ITEM;

void create()
{
        set_name("玄铁", ({ "xuantie", "tie" }) );
        set_weight(500);
        set("unit", "块");
        set("material", "metal");
        set("is_material", 1);
        set("material_id", "xuantie");
        set("value", 3000);
        set("long", "一块乌黑发亮的玄铁，极沉极坚，寻常火焰难以熔炼，是锻造飞剑的通用良材。\n");
        setup();
}
