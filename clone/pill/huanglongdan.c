// clone/pill/huanglongdan.c
// 黄龙丹（丹方产物）
// 丹药实体（#73：#64 子单 丹药炼制链路；丹方产物）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("黄龙丹", ({ "huanglongdan", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 3000);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 800);
        set("quality", 1);
        set("refine_level", 5);
        set("side_effect", "黄龙草药性偏烈，久服丹毒累积稍快。");
        set("long", "以黄龙草炼制的丹药，可大幅提升炼气期修为。\n");
        setup();
}
