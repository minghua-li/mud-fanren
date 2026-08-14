// clone/pill/junxu.c
// 军需丹（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("军需丹", ({ "junxu", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 300);
        set("pill_type", "heal");
        set("stage", 0);
        set("effect", 150);
        set("quality", 1);
        set("refine_level", 0);
        set("side_effect", "军用丹药，药性平稳。");
        set("long", "天渊城制式军用丹药，战场疗伤必备。\n");
        setup();
}
