// d/yueguo/qixuanmen/npc/modafu.c
// 七玄门 - 墨大夫（神手谷）
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("墨大夫", ({ "mo dafu", "mo", "dafu" }) );
        set("title", "神手谷医师");
        set("gender", "男性");
        set("age", 50);
        set("long",
                "七玄门墨大夫，医术高明，素有「神手」之称。面容清癯，眉宇间\n"
                "带着几分沉郁，据说早年有段不为人知的往事。他常在神手谷中\n"
                "研习医道，练功静室常年紧闭。\n");
        set("attitude", "peaceful");
        set("combat_exp", 20000);
        set("max_qi", 800);
        set("eff_qi", 800);
        set("qi", 800);
        set("max_jing", 800);
        set("eff_jing", 800);
        set("jing", 800);
        set("max_neili", 1000);
        set("eff_neili", 1000);
        set("neili", 1000);
        set("score", 2000);
        set("chat_chance", 25);
        set("chat_msg", ({
                "墨大夫头也不抬地翻着药书，淡淡道：药性相生相克，差之毫厘，谬以千里。\n",
                "墨大夫瞥了你一眼：求医？先说说你哪里不舒服。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
