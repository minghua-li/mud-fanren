// clone/pill/yuehua_dan.c
// 月华丹（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("月华丹", ({ "yuehua_dan", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 5000);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 400);
        set("quality", 2);
        set("refine_level", 10);
        set("side_effect", "丹毒较轻。");
        set("long", "掩月宗以月华草秘制的丹药，服用可精进修为。\n");
        setup();
}
