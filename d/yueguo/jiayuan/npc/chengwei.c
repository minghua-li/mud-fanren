// d/yueguo/jiayuan/npc/chengwei.c
// 嘉元城 - 城门卫
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("城门卫", ({ "chengmen wei", "wei" }) );
        set("title", "嘉元城守门兵丁");
        set("gender", "男性");
        set("age", 25);
        set("long",
                "嘉元城城门的守门兵丁，身着皂衣，挎着腰刀，目光警惕地打量着\n"
                "进出城门的行人。\n");
        set("attitude", "peaceful");
        set("combat_exp", 3000);
        set("max_qi", 350);
        set("eff_qi", 350);
        set("qi", 350);
        set("max_jing", 300);
        set("eff_jing", 300);
        set("jing", 300);
        set("score", 300);
        set("chat_chance", 20);
        set("chat_msg", ({
                "城门卫喝道：进城莫要生事，惹了惊蛟会可没人保你。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
