// clone/pill/ningdan.c
// 凝丹丸（丹方产物+shop）
// 丹药实体（#73：#64 子单 丹药炼制链路；丹方产物+shop）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("凝丹丸", ({ "ningdan", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 25000);
        set("pill_type", "xiuwei");
        set("stage", 2);
        set("effect", 2000);
        set("quality", 2);
        set("refine_level", 12);
        set("side_effect", "药力深厚，筑基期以上服用，丹毒较重。");
        set("long", "供筑基期修士巩固修为的丹药，蕴含浓厚灵力。\n");
        setup();
}
