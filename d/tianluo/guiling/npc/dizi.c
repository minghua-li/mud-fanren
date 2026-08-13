// d/tianluo/guiling/npc/dizi.c
// 鬼灵门 - 弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("鬼灵门弟子", ({ "guiling dizi", "dizi" }) );
        set("title", "鬼灵门弟子");
        set("gender", "男性");
        set("age", 30);
        set("long",
                "鬼灵门的弟子，面色阴冷，腰间挂着驭鬼法器，周身隐隐有鬼气缠绕。\n");
        set("attitude", "aggressive");
        set("sect", "guiling_sect");
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
                "鬼",
                "灵",
                "门",
                "弟",
                "子",
                "阴",
                "森",
                "说",
                "道",
                "：",
                "擅",
                "闯",
                "鬼",
                "灵",
                "门",
                "者",
                "，",
                "魂",
                "魄",
                "都",
                "别",
                "想",
                "留",
                "下",
                "！",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
