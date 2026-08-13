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
                "络",
                "腮",
                "胡",
                "子",
                "弟",
                "子",
                "咧",
                "嘴",
                "一",
                "笑",
                "：",
                "我",
                "这",
                "灵",
                "兽",
                "袋",
                "里",
                "养",
                "的",
                "宝",
                "贝",
                "，",
                "血",
                "禁",
                "试",
                "炼",
                "中",
                "不",
                "知",
                "放",
                "翻",
                "了",
                "多",
                "少",
                "人",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
