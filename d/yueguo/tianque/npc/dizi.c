// d/yueguo/tianque/npc/dizi.c
// 天阙堡 - 弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("天阙堡弟子", ({ "tianque dizi", "dizi" }) );
        set("title", "天阙堡弟子");
        set("gender", "男性");
        set("age", 25);
        set("long",
                "天阙堡的年轻弟子，黄衣上绣着黄色巨印，正在堡中巡查。\n");
        set("attitude", "peaceful");
        set("sect", "tianque_fort");
        set("combat_exp", 30000);
        set("max_qi", 600);
        set("eff_qi", 600);
        set("qi", 600);
        set("max_jing", 480);
        set("eff_jing", 480);
        set("jing", 480);
        set("max_neili", 900);
        set("eff_neili", 900);
        set("neili", 900);
        set("max_jingli", 540);
        set("eff_jingli", 540);
        set("jingli", 540);
        set("score", 3000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "天",
                "阙",
                "堡",
                "弟",
                "子",
                "说",
                "道",
                "：",
                "堡",
                "内",
                "城",
                "防",
                "森",
                "严",
                "，",
                "外",
                "人",
                "不",
                "得",
                "擅",
                "入",
                "，",
                "道",
                "友",
                "请",
                "出",
                "示",
                "身",
                "份",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
