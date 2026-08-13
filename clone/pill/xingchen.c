// clone/pill/xingchen.c
// 星辰丹（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("星辰丹", ({ "xingchen", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 200);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 200);
        set("quality", 1);
        set("refine_level", 5);
        set("side_effect", "丹毒轻微。");
        set("long", "引星辰之力炼制的灵丹，星宫修士日常修炼之用。\n");
        setup();
}
