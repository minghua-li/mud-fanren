// d/yueguo/tianque/npc/lanyiren.c
// 天阙堡 - 蓝衣人（弟子）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("蓝衣人", ({ "lan yi ren", "lanyiren" }) );
        set("title", "天阙堡弟子");
        set("gender", "男性");
        set("age", 30);
        set("long",
                "天阙堡的弟子，一身蓝衣，身家丰富，曾在血禁试炼中历练。\n");
        set("attitude", "peaceful");
        set("sect", "tianque_fort");
        set("combat_exp", 40000);
        set("max_qi", 700);
        set("eff_qi", 700);
        set("qi", 700);
        set("max_jing", 560);
        set("eff_jing", 560);
        set("jing", 560);
        set("max_neili", 1000);
        set("eff_neili", 1000);
        set("neili", 1000);
        set("max_jingli", 600);
        set("eff_jingli", 600);
        set("jingli", 600);
        set("score", 4000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "蓝衣人叹道：血禁试炼凶险异常，我那一身家当，险些尽数折在禁地之中。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 100);
        set_skill("dodge", 100);
        set_skill("parry", 100);
        set_skill("unarmed", 100);
        set_skill("zhubao-shu", 100);
        set_skill("zhenfa-shu", 100);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅天阙堡弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "tianque_fort")
                return 0;
        return 1;
}
