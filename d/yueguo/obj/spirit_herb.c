// d/yueguo/obj/spirit_herb.c
// 灵田收获/坊市售出的灵材药材（参数化模板，克隆后由设施系统填充）
// Created for ticket #60

#include <ansi.h>

inherit ITEM;

// 由门派设施系统调用：name/id/unit/value/desc
void setup_herb(string name, string id, string unit, int value, string desc)
{
    set_name(name, ({ id }));
    set("unit", unit);
    set("long", desc + "\n");
    set("value", value);
    set_weight(10);
    set("herb", 1);
}

void create()
{
    set_name(HIG "灵草" NOR, ({ "lingcao" }));
    set_weight(10);
    if (clonep())
        set_default_object(__FILE__);
    else
    {
        set("unit", "株");
        set("long", "一株灵气氤氲的灵草，炼丹炼药的基础材料。\n");
        set("value", 200);
        set("herb", 1);
    }
    setup();
}
