// d/yueguo/tainan/obj/huanglongcao.c
// 黄龙草（坊市炼丹材料）
// Created for ticket #67

inherit ITEM;

void create()
{
        set_name("黄龙草", ({ "huanglongcao", "cao" }) );
        set_weight(60);
        set("unit", "株");
        set("material", "herb");
        set("is_material", 1);
        set("material_id", "huanglongcao");
        set("herb_year", 80);
        set("value", 500);
        set("long", "一株叶片泛黄的灵草，是炼制黄龙丹的主药，比普通灵草要稀罕些。\n");
        setup();
}
