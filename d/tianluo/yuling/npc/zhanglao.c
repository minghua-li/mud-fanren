// d/tianluo/yuling/npc/zhanglao.c
// 御灵宗 - 长老
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("御灵宗长老", ({ "zhanglao", "yuling zhanglao" }) );
        set("title", "御灵宗长老");
        set("gender", "男性");
        set("age", 55);
        set("long",
                "御灵宗的结丹期长老，擅长万蛊诀，养蛊役虫之术出神入化。\n");
        set("attitude", "aggressive");
        set("sect", "yuling_sect");
        set("combat_exp", 3000000);
        set("max_qi", 6800);
        set("eff_qi", 6800);
        set("qi", 6800);
        set("max_jing", 5440);
        set("eff_jing", 5440);
        set("jing", 5440);
        set("max_neili", 8800);
        set("eff_neili", 8800);
        set("neili", 8800);
        set("max_jingli", 5280);
        set("eff_jingli", 5280);
        set("jingli", 5280);
        set("score", 45000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "御灵宗长老说道：万蛊诀养蛊役虫，我御灵宗虫兽双修，天南魔道谁人不知。\n",
                "御灵宗长老冷冷道：灵兽山本就是我御灵宗的分支，数千年暗桩，终有一日要回归。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 200);
        set_skill("dodge", 200);
        set_skill("parry", 200);
        set_skill("unarmed", 200);
        set_skill("wangu-jue", 200);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅御灵宗弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "yuling_sect")
                return 0;
        return 1;
}
