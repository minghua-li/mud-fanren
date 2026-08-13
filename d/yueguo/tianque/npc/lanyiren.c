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
                "蓝",
                "衣",
                "人",
                "叹",
                "道",
                "：",
                "血",
                "禁",
                "试",
                "炼",
                "凶",
                "险",
                "异",
                "常",
                "，",
                "我",
                "那",
                "一",
                "身",
                "家",
                "当",
                "，",
                "险",
                "些",
                "尽",
                "数",
                "折",
                "在",
                "禁",
                "地",
                "之",
                "中",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
