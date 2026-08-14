// d/yueguo/jujian/npc/gaoren.c
// 巨剑门 - 巨剑门高人（结丹）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("巨剑门高人", ({ "jujian gaoren", "gaoren" }) );
        set("title", "巨剑门结丹高手");
        set("gender", "男性");
        set("age", 50);
        set("long",
                "巨剑门的结丹期高手，背负无鞘巨剑，血禁试炼中负责以石剑测试禁制强弱。\n");
        set("attitude", "peaceful");
        set("sect", "jujian_gate");
        set("combat_exp", 3000000);
        set("max_qi", 7500);
        set("eff_qi", 7500);
        set("qi", 7500);
        set("max_jing", 6000);
        set("eff_jing", 6000);
        set("jing", 6000);
        set("max_neili", 9500);
        set("eff_neili", 9500);
        set("neili", 9500);
        set("max_jingli", 5700);
        set("eff_jingli", 5700);
        set("jingli", 5700);
        set("score", 45000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "巨剑门高人说道：这禁制强弱，且让我用石剑一试便知。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 200);
        set_skill("dodge", 200);
        set_skill("parry", 200);
        set_skill("sword", 200);
        set_skill("zhongjian-jianfa", 200);
        set_skill("jianxiu-chuancheng", 200);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅巨剑门弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "jujian_gate")
                return 0;
        return 1;
}
