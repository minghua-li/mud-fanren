// d/yueguo/huadao/npc/dizi.c
// 化刀坞 - 守坞弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("守坞弟子", ({ "huadao dizi", "dizi" }) );
        set("title", "化刀坞弟子");
        set("gender", "男性");
        set("age", 24);
        set("long",
                "化刀坞的守坞弟子，腰挎长刀，目光锐利。\n");
        set("attitude", "peaceful");
        set("sect", "huadao_dock");
        set("combat_exp", 25000);
        set("max_qi", 550);
        set("eff_qi", 550);
        set("qi", 550);
        set("max_jing", 440);
        set("eff_jing", 440);
        set("jing", 440);
        set("max_neili", 850);
        set("eff_neili", 850);
        set("neili", 850);
        set("max_jingli", 510);
        set("eff_jingli", 510);
        set("jingli", 510);
        set("score", 2500);
        set("chat_chance", 30);
        set("chat_msg", ({
                "守",
                "坞",
                "弟",
                "子",
                "沉",
                "声",
                "道",
                "：",
                "来",
                "者",
                "何",
                "人",
                "？",
                "化",
                "刀",
                "坞",
                "乃",
                "刀",
                "修",
                "之",
                "地",
                "，",
                "外",
                "人",
                "莫",
                "要",
                "随",
                "意",
                "擅",
                "入",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
