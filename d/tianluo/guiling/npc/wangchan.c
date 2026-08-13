// d/tianluo/guiling/npc/wangchan.c
// 鬼灵门 - 王蝉（少门主）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("王蝉", ({ "wang chan", "wangchan" }) );
        set("title", "鬼灵门少门主");
        set("gender", "男性");
        set("age", 25);
        set("long",
                "鬼灵门的少门主，暗灵根资质，是血灵大法双修的最佳人选之一。\n");
        set("attitude", "aggressive");
        set("sect", "guiling_sect");
        set("combat_exp", 1000000);
        set("max_qi", 2000);
        set("eff_qi", 2000);
        set("qi", 2000);
        set("max_jing", 1600);
        set("eff_jing", 1600);
        set("jing", 1600);
        set("max_neili", 2800);
        set("eff_neili", 2800);
        set("neili", 2800);
        set("max_jingli", 1680);
        set("eff_jingli", 1680);
        set("jingli", 1680);
        set("score", 15000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "王蝉傲然道：血灵大法需天灵根与暗灵根双修，我的暗灵根，正等着那位天灵根的道侣。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 150);
        set_skill("dodge", 150);
        set_skill("parry", 150);
        set_skill("unarmed", 150);
        set_skill("guidao-gongfa", 150);
        set_skill("dushu", 150);
        set_skill("anshu", 150);
        set_skill("lianshi-shu", 150);
        set_skill("xueling-dafa", 150);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅鬼灵门弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "guiling_sect")
                return 0;
        return 1;
}
