// d/yueguo/huadao/npc/zhishilao.c
// 化刀坞 - 执事长老
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("执事长老", ({ "zhishi zhanglao", "zhishilao" }) );
        set("title", "化刀坞长老");
        set("gender", "男性");
        set("age", 50);
        set("long",
                "化刀坞的执事长老，刀意深沉，负责坞中刀法传承与炼器事务。\n");
        set("attitude", "peaceful");
        set("sect", "huadao_dock");
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
        set("score", 40000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "执事长老说道：刀修一途，贵在刀意。我化刀坞的刀，快、狠、准，血禁试炼中谁人不知。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
