// d/yueguo/qingxu/npc/zhangglao.c
// 清虚门 - 传功长老
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("传功长老", ({ "chuangong zhanglao", "zhanglao" }) );
        set("title", "清虚门长老");
        set("gender", "男性");
        set("age", 55);
        set("long",
                "清虚门的传功长老，道士装束，以道门术法见长，负责传授门中弟子。\n");
        set("attitude", "peaceful");
        set("sect", "qingxu_sect");
        set("combat_exp", 3000000);
        set("max_qi", 6500);
        set("eff_qi", 6500);
        set("qi", 6500);
        set("max_jing", 5200);
        set("eff_jing", 5200);
        set("jing", 5200);
        set("max_neili", 8500);
        set("eff_neili", 8500);
        set("neili", 8500);
        set("max_jingli", 5100);
        set("eff_jingli", 5100);
        set("jingli", 5100);
        set("score", 40000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "传功长老说道：道门术法以清虚为本，你且随我静心修习。\n",
                "传功长老说道：清虚剑典剑符双修，道剑一出，剑意清正。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
