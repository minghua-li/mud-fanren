// d/tianluo/yuling/npc/dizi.c
// 御灵宗 - 弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("御灵宗弟子", ({ "yuling dizi", "dizi" }) );
        set("title", "御灵宗弟子");
        set("gender", "男性");
        set("age", 28);
        set("long",
                "御灵宗的弟子，身上挂满灵兽袋，脚下趴着一头低阶灵兽。\n");
        set("attitude", "aggressive");
        set("sect", "yuling_sect");
        set("combat_exp", 35000);
        set("max_qi", 650);
        set("eff_qi", 650);
        set("qi", 650);
        set("max_jing", 520);
        set("eff_jing", 520);
        set("jing", 520);
        set("max_neili", 950);
        set("eff_neili", 950);
        set("neili", 950);
        set("max_jingli", 570);
        set("eff_jingli", 570);
        set("jingli", 570);
        set("score", 3500);
        set("chat_chance", 30);
        set("chat_msg", ({
                "御灵宗弟子喝道：兽潮一起，越国七派顷刻覆灭，识相的就乖乖退下！\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
