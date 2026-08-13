// d/yueguo/jiayuan/npc/mofuguanjia.c
// 嘉元城 - 墨府管家
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("墨府管家", ({ "guanjia", "mo guanjia" }) );
        set("title", "墨府管家");
        set("gender", "男性");
        set("age", 45);
        set("long",
                "墨府的管家，衣着整洁，说话滴水不漏。墨府在嘉元城颇有脸面，\n"
                "这位管家迎来送往，把府中事务打理得井井有条。\n");
        set("attitude", "peaceful");
        set("combat_exp", 6000);
        set("max_qi", 400);
        set("eff_qi", 400);
        set("qi", 400);
        set("max_jing", 400);
        set("eff_jing", 400);
        set("jing", 400);
        set("score", 500);
        set("chat_chance", 20);
        set("chat_msg", ({
                "墨府管家拱手道：这位客人，墨府近日不便待客，还请改日再来。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
