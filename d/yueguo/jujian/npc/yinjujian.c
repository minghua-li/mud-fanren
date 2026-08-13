// d/yueguo/jujian/npc/yinjujian.c
// 巨剑门 - 银巨剑修士（弟子）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("银巨剑修士", ({ "yin jujian", "yinjujian" }) );
        set("title", "巨剑门弟子");
        set("gender", "男性");
        set("age", 30);
        set("long",
                "巨剑门的弟子，使一柄银色巨剑，血禁试炼中所向披靡，连杀数名化刀坞弟子。\n");
        set("attitude", "aggressive");
        set("sect", "jujian_gate");
        set("combat_exp", 60000);
        set("max_qi", 900);
        set("eff_qi", 900);
        set("qi", 900);
        set("max_jing", 720);
        set("eff_jing", 720);
        set("jing", 720);
        set("max_neili", 1300);
        set("eff_neili", 1300);
        set("neili", 1300);
        set("max_jingli", 780);
        set("eff_jingli", 780);
        set("jingli", 780);
        set("score", 6000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "银巨剑修士冷声道：一剑破万法，我这银巨剑下，尚无挡得住的一击。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
