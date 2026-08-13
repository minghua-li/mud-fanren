// clone/pill/cao.c
// 止血草（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("止血草", ({ "cao", "yao" }));
        set_weight(20);
        set("unit", "株");
        set("value", 100);
        set("pill_type", "heal");
        set("stage", 0);
        set("effect", 80);
        set("quality", 1);
        set("refine_level", 0);
        set("side_effect", "无。");
        set("long", "一株能快速止血的草药，直接嚼服可回复气血。\n");
        setup();
}
