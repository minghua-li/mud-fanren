// d/yueguo/yanyue/npc/nannanwan.c
// 掩月宗 - 南宫婉（元婴长老）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("南宫婉", ({ "nangong", "nangong wan" }) );
        set("title", "掩月宗元婴长老");
        set("gender", "女性");
        set("age", 30);
        set("long",
                "掩月宗的元婴期长老，容貌绝美，天灵根资质，是掩月宗仅有的两位元婴修士之一。\n");
        set("attitude", "friendly");
        set("sect", "yanyue_sect");
        set("combat_exp", 10000000);
        set("max_qi", 18000);
        set("eff_qi", 18000);
        set("qi", 18000);
        set("max_jing", 14400);
        set("eff_jing", 14400);
        set("jing", 14400);
        set("max_neili", 20000);
        set("eff_neili", 20000);
        set("neili", 20000);
        set("max_jingli", 12000);
        set("eff_jingli", 12000);
        set("jingli", 12000);
        set("score", 100000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "南宫婉淡淡说道：修仙之路漫长，唯有道心坚定者方能走得长远。\n",
                "南宫婉说道：我掩月宗灵根测试最为严谨，天灵根一出，震动越国。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
