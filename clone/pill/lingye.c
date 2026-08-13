// clone/pill/lingye.c
// 法士灵液（shop 修复）
// 丹药实体（#73：#64 子单 丹药炼制链路；shop 修复）
// 属性对齐 1E §4.1：pill_type/stage/effect/quality/side_effect/refine_level

#include <pill.h>

inherit DAN_BASE;

void create()
{
        set_name("法士灵液", ({ "lingye", "ye" }));
        set_weight(20);
        set("unit", "瓶");
        set("value", 300);
        set("pill_type", "xiuwei");
        set("stage", 1);
        set("effect", 500);
        set("quality", 1);
        set("refine_level", 5);
        set("side_effect", "灵液温和，丹毒极微。");
        set("long", "慕兰法士秘制的灵液，服用后灵力流转加快，修为精进。\n");
        setup();
}
