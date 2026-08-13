// clone/pill/juqi.c
// 聚气丹（丹方产物+shop）
// 丹药实体（#73：#64 子单 丹药炼制链路；丹方产物+shop）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("聚气丹", ({ "juqi", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 800);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 200);
        set("quality", 1);
        set("refine_level", 5);
        set("side_effect", "常见丹药，丹毒轻微。");
        set("long", "聚集天地灵气的丹药，服用后可小幅增加修为。\n");
        setup();
}
