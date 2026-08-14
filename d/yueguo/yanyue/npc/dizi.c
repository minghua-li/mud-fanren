// d/yueguo/yanyue/npc/dizi.c
// 掩月宗 - 女弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("掩月宗女弟子", ({ "yanyue dizi", "dizi" }) );
        set("title", "掩月宗弟子");
        set("gender", "女性");
        set("age", 20);
        set("long",
                "掩月宗的年轻女弟子，容貌秀丽，衣袂飘飘，是掩月宗新收的弟子。\n");
        set("attitude", "peaceful");
        set("sect", "yanyue_sect");
        set("combat_exp", 20000);
        set("max_qi", 500);
        set("eff_qi", 500);
        set("qi", 500);
        set("max_jing", 400);
        set("eff_jing", 400);
        set("jing", 400);
        set("max_neili", 800);
        set("eff_neili", 800);
        set("neili", 800);
        set("max_jingli", 480);
        set("eff_jingli", 480);
        set("jingli", 480);
        set("score", 2000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "掩月宗女弟子低声说道：这位道友请留步，掩月宗山门重地，莫要乱闯。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
