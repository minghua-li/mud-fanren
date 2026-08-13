// clone/pill/shouwan.c
// 灵兽丸（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("灵兽丸", ({ "shouwan", "wan", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 150);
        set("pill_type", "heal");
        set("stage", 0);
        set("effect", 50);
        set("quality", 1);
        set("refine_level", 0);
        set("side_effect", "兽类丹药，修士服用仅得微效。");
        set("long", "驯兽用灵丹，灵兽山弟子培育灵兽时常用。\n");
        setup();
}
