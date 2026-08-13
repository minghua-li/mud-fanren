// clone/pill/zhuji.c
// 筑基丹（丹方产物+shop）
// 丹药实体（#73：#64 子单 丹药炼制链路；丹方产物+shop）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("筑基丹", ({ "zhuji", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 8000);
        set("pill_type", "breakthrough");
        set("stage", 2);
        set("effect", 25);
        set("quality", 2);
        set("refine_level", 8);
        set("side_effect", "药力霸道，突破瓶颈时方能发挥，普通服用无效。");
        set("long", "炼制筑基的关键丹药，服用后冲击筑基瓶颈成功率大增（可叠加最多3颗）。\n");
        setup();
}
