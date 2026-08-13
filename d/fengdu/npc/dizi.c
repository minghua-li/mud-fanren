// d/fengdu/npc/dizi.c
// 风都国 - 正道盟弟子
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("正道盟弟子", ({ "zhengdao dizi", "dizi" }) );
        set("title", "正道盟弟子");
        set("gender", "男性");
        set("age", 22);
        set("long",
                "正道盟的年轻弟子，白衣佩剑，气度俨然。正道盟以风都国为中心\n"
                "统合天南正派，弟子们皆以正道自居，对魔道中人深恶痛绝。\n");
        set("attitude", "peaceful");
        set("combat_exp", 60000);
        set("max_qi", 700);
        set("eff_qi", 700);
        set("qi", 700);
        set("max_jing", 600);
        set("eff_jing", 600);
        set("jing", 600);
        set("max_neili", 1000);
        set("eff_neili", 1000);
        set("neili", 1000);
        set("score", 2000);
        set("chat_chance", 20);
        set("chat_msg", ({
                "正道盟弟子拱手道：此地乃正道盟总坛，道友若无要事，还请留步。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
