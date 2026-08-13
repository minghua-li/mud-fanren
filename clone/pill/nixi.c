// clone/pill/nixi.c
// 匿息丹（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("匿息丹", ({ "nixi", "dan" }));
        set_weight(20);
        set("unit", "颗");
        set("value", 250);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 300);
        set("quality", 1);
        set("refine_level", 5);
        set("side_effect", "丹毒轻微。");
        set("long", "服用后可短暂隐匿气息的丹药，逆星盟秘制。\n");
        setup();
}
