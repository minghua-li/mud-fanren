// d/yueguo/qixuanmen/npc/dizi.c
// 七玄门 - 弟子
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("七玄门弟子", ({ "dizi", "menren" }) );
        set("title", "七玄门弟子");
        set("gender", "男性");
        set("age", 20);
        set("long",
                "七玄门的年轻弟子，身着青色劲装，腰挎钢刀，神情精悍。七玄门\n"
                "虽已衰落，门中弟子仍以能打敢拼著称，镜州江湖上无人敢轻慢。\n");
        set("attitude", "peaceful");
        set("combat_exp", 5000);
        set("max_qi", 400);
        set("eff_qi", 400);
        set("qi", 400);
        set("max_jing", 320);
        set("eff_jing", 320);
        set("jing", 320);
        set("max_neili", 400);
        set("eff_neili", 400);
        set("neili", 400);
        set("score", 800);
        set("chat_chance", 25);
        set("chat_msg", ({
                "七玄门弟子抱刀而立：此乃七玄门重地，无事莫要逗留。\n",
                "七玄门弟子咧嘴一笑：想入我七玄门？先去炼骨崖过一关再说。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
