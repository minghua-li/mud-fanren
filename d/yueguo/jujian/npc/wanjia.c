// d/yueguo/jujian/npc/wanjia.c
// 巨剑门 - 万家先祖
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("万家先祖", ({ "wan jia", "wanjia" }) );
        set("title", "巨剑门前辈");
        set("gender", "男性");
        set("age", 70);
        set("long",
                "巨剑门的前辈，散修出身，入门后使万家在越国立足，立下祖训：不准歧视散修。\n");
        set("attitude", "peaceful");
        set("sect", "jujian_gate");
        set("combat_exp", 5000000);
        set("max_qi", 9000);
        set("eff_qi", 9000);
        set("qi", 9000);
        set("max_jing", 7200);
        set("eff_jing", 7200);
        set("jing", 7200);
        set("max_neili", 12000);
        set("eff_neili", 12000);
        set("neili", 12000);
        set("max_jingli", 7200);
        set("eff_jingli", 7200);
        set("jingli", 7200);
        set("score", 80000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "万",
                "家",
                "先",
                "祖",
                "沉",
                "声",
                "道",
                "：",
                "巨",
                "剑",
                "门",
                "不",
                "准",
                "歧",
                "视",
                "散",
                "修",
                "，",
                "老",
                "夫",
                "当",
                "年",
                "便",
                "是",
                "散",
                "修",
                "出",
                "身",
                "，",
                "今",
                "日",
                "方",
                "有",
                "万",
                "家",
                "立",
                "足",
                "之",
                "地",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
