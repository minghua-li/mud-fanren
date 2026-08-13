// clone/pill/jiejindan.c
// 结金丹（丹方产物）
// 丹药实体（#73：#64 子单 丹药炼制链路；丹方产物）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("结金丹", ({ "jiejindan", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 100000);
        set("pill_type", "breakthrough");
        set("stage", 3);
        set("effect", 15);
        set("quality", 3);
        set("refine_level", 15);
        set("side_effect", "逆天丹药，药力暴烈，冲击结丹瓶颈时方显神效。");
        set("long", "辅助结丹的逆天丹药，服用后冲击结丹瓶颈成功率提升（可叠加最多3颗）。\n");
        setup();
}
