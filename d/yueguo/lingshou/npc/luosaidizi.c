// d/yueguo/lingshou/npc/luosaidizi.c
// 灵兽山 - 络腮胡子弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("络腮胡子弟子", ({ "luosai dizi", "luosai" }) );
        set("title", "灵兽山弟子");
        set("gender", "男性");
        set("age", 28);
        set("long",
                "灵兽山的炼气十三层弟子，满脸络腮胡子，血禁试炼中的辣手人物之一。\n");
        set("attitude", "aggressive");
        set("sect", "lingshou_mountain");
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
                "络腮胡子弟子咧嘴一笑：我这灵兽袋里养的宝贝，血禁试炼中不知放翻了多少人。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 100);
        set_skill("dodge", 100);
        set_skill("parry", 100);
        set_skill("unarmed", 100);
        set_skill("yushou-shu", 100);
        set_skill("yichong-shu", 100);
        set_skill("kuilei-shu", 100);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅灵兽山弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "lingshou_mountain")
                return 0;
        return 1;
}
