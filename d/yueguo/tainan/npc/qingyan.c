// d/yueguo/tainan/npc/qingyan.c
// 太南谷 - 青颜真人（散修）
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("青颜真人", ({ "qingyan zhenren", "qingyan" }) );
        set("title", "散修");
        set("gender", "男性");
        set("age", 50);
        set("long",
                "青颜真人是一位散修，常年在太南谷一带走动。他面容清瘦，谈吐\n"
                "随和，太南小会上总能看到他的身影，与各路修士都有些交情。\n");
        set("attitude", "friendly");
        set("combat_exp", 300000);
        set("max_qi", 1600);
        set("eff_qi", 1600);
        set("qi", 1600);
        set("max_jing", 1500);
        set("eff_jing", 1500);
        set("jing", 1500);
        set("max_neili", 2000);
        set("eff_neili", 2000);
        set("neili", 2000);
        set("score", 8000);
        set("chat_chance", 25);
        set("chat_msg", ({
                "青颜真人抚须笑道：道友面生，可是初来太南谷？此地买卖公道，多住些时日便熟了。\n",
                "青颜真人叹道：升仙大会十年一次，炼气期的道友莫要错过，那可是入七大派的好机缘。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
