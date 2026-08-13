// d/yueguo/tianque/npc/gaoshou.c
// 天阙堡 - 天阙堡高手（结丹）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("天阙堡高手", ({ "tianque gaoshou", "gaoshou" }) );
        set("title", "天阙堡结丹高手");
        set("gender", "男性");
        set("age", 45);
        set("long",
                "天阙堡的结丹期高手，持黄色大印法宝，声势最为浩大。\n");
        set("attitude", "peaceful");
        set("sect", "tianque_fort");
        set("combat_exp", 3000000);
        set("max_qi", 7000);
        set("eff_qi", 7000);
        set("qi", 7000);
        set("max_jing", 5600);
        set("eff_jing", 5600);
        set("jing", 5600);
        set("max_neili", 9000);
        set("eff_neili", 9000);
        set("neili", 9000);
        set("max_jingli", 5400);
        set("eff_jingli", 5400);
        set("jingli", 5400);
        set("score", 45000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "天阙堡高手朗声道：我这黄色大印一出，声如风雷、势如小山，你且看好了！\n",
                "天阙堡高手说道：我天阙堡筑堡建州，阵法冠绝越国，便是魔道来犯，也休想破堡。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
