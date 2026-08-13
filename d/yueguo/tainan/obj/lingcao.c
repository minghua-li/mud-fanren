// d/yueguo/tainan/obj/lingcao.c
// 灵草（坊市炼丹材料）
// Created for ticket #67

inherit ITEM;

void create()
{
        set_name("灵草", ({ "lingcao", "cao" }) );
        set_weight(50);
        set("unit", "株");
        set("material", "herb");
        set("is_material", 1);
        set("material_id", "lingcao");
        set("value", 100);
        set("long", "一株常见的灵草，叶片泛着淡淡灵光，是炼制炼气期丹药的基础材料。\n");
        setup();
}
