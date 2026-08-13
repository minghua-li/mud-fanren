// d/yueguo/qixuanmen/npc/zhanglao.c
// 七玄门 - 长老（泛称）
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("七玄门长老", ({ "zhanglao", "lao" }) );
        set("title", "七玄门长老");
        set("gender", "男性");
        set("age", 60);
        set("long",
                "七玄门的长老，白发苍苍，精神矍铄。七玄门设长老会，与门主\n"
                "并驾齐驱，共掌门中大事。这位长老常驻落日峰，打理门中事务。\n");
        set("attitude", "peaceful");
        set("combat_exp", 30000);
        set("max_qi", 1000);
        set("eff_qi", 1000);
        set("qi", 1000);
        set("max_jing", 900);
        set("eff_jing", 900);
        set("jing", 900);
        set("max_neili", 1500);
        set("eff_neili", 1500);
        set("neili", 1500);
        set("score", 3000);
        set("chat_chance", 20);
        set("chat_msg", ({
                "长老抚须道：我七玄门鼎盛时雄霸镜州数十载，如今虽衰，傲骨犹存。\n",
                "长老打量着你：江湖凶险，若无去处，不妨考虑入我七玄门磨炼一番。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
