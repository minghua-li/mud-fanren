// d/yueguo/huadao/npc/hantianya.c
// 化刀坞 - 寒天涯（弟子）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("寒天涯", ({ "han tianya", "hantianya" }) );
        set("title", "化刀坞弟子");
        set("gender", "男性");
        set("age", 30);
        set("long",
                "化刀坞的弟子，人称『人妖』，行为乖戾，行事出人意表。\n");
        set("attitude", "aggressive");
        set("sect", "huadao_dock");
        set("combat_exp", 50000);
        set("max_qi", 800);
        set("eff_qi", 800);
        set("qi", 800);
        set("max_jing", 640);
        set("eff_jing", 640);
        set("jing", 640);
        set("max_neili", 1200);
        set("eff_neili", 1200);
        set("neili", 1200);
        set("max_jingli", 720);
        set("eff_jingli", 720);
        set("jingli", 720);
        set("score", 5000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "寒天涯怪笑道：嘿，你这小辈，可敢与我比划比划？我行事向来乖戾，可莫要惹我。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 100);
        set_skill("dodge", 100);
        set_skill("parry", 100);
        set_skill("blade", 100);
        set_skill("daofa-chuancheng", 100);
        set_skill("lianqi-shu", 100);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅化刀坞弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "huadao_dock")
                return 0;
        return 1;
}
