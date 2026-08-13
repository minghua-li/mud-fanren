// clone/pill/xingjun.c
// 行军丹（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("行军丹", ({ "xingjun", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 200);
        set("pill_type", "heal");
        set("stage", 0);
        set("effect", 120);
        set("quality", 1);
        set("refine_level", 0);
        set("side_effect", "军用丹药，药性平稳。");
        set("long", "九国盟军用丹药，行军途中应急疗伤。\n");
        setup();
}
