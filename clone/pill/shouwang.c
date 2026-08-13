// clone/pill/shouwang.c
// 兽王丹（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("兽王丹", ({ "shouwang", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 50000);
        set("pill_type", "xiuwei");
        set("stage", 2);
        set("effect", 3000);
        set("quality", 2);
        set("refine_level", 10);
        set("side_effect", "丹毒较重，需以强大肉身承受药力。");
        set("long", "以妖兽精血炼制的丹药，大幅提升服用者修为战力。\n");
        setup();
}
