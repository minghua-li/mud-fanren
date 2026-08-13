// d/yueguo/lingshou/npc/shaonvdizi.c
// 灵兽山 - 少女弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("少女弟子", ({ "shaonv dizi", "shaonv" }) );
        set("title", "灵兽山弟子");
        set("gender", "女性");
        set("age", 18);
        set("long",
                "灵兽山的少女弟子，原在太南会卖金竺笔，后来入了灵兽山。\n");
        set("attitude", "peaceful");
        set("sect", "lingshou_mountain");
        set("combat_exp", 15000);
        set("max_qi", 400);
        set("eff_qi", 400);
        set("qi", 400);
        set("max_jing", 320);
        set("eff_jing", 320);
        set("jing", 320);
        set("max_neili", 700);
        set("eff_neili", 700);
        set("neili", 700);
        set("max_jingli", 420);
        set("eff_jingli", 420);
        set("jingli", 420);
        set("score", 2000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "少",
                "女",
                "弟",
                "子",
                "笑",
                "道",
                "：",
                "这",
                "位",
                "道",
                "友",
                "，",
                "可",
                "要",
                "看",
                "看",
                "我",
                "这",
                "金",
                "竺",
                "笔",
                "？",
                "当",
                "初",
                "在",
                "太",
                "南",
                "会",
                "可",
                "是",
                "抢",
                "手",
                "得",
                "很",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
