// clone/pill/lianqisan.c
// 炼气散（丹方产物）
// 丹药实体（#73：#64 子单 丹药炼制链路；丹方产物）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("炼气散", ({ "lianqisan", "san", "dan" }));
        set_weight(20);
        set("unit", "瓶");
        set("value", 1000);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 300);
        set("quality", 1);
        set("refine_level", 5);
        set("side_effect", "炼制炼气散的材料多为寻常灵草，长期服用丹毒微量。");
        set("long", "一瓶以灵草为主药炼制的散剂，供炼气期修士精进修为。\n");
        setup();
}
