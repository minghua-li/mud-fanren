// d/tianluo/yuling/npc/quhun.c
// 御灵宗 - 曲魂
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("曲魂", ({ "quhun", "qu hun" }) );
        set("title", "御灵宗修士");
        set("gender", "男性");
        set("age", 40);
        set("long",
                "御灵宗的修士，来自天都国，曾来越国游历，修为大减后在此潜修。\n");
        set("attitude", "aggressive");
        set("sect", "yuling_sect");
        set("combat_exp", 3000000);
        set("max_qi", 7000);
        set("eff_qi", 7000);
        set("qi", 7000);
        set("max_jing", 5600);
        set("eff_jing", 5600);
        set("jing", 5600);
        set("max_neili", 9000);
        set("eff_neili", 9000);
        set("neili", 9000);
        set("max_jingli", 5400);
        set("eff_jingli", 5400);
        set("jingli", 5400);
        set("score", 40000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "曲",
                "魂",
                "说",
                "道",
                "：",
                "弱",
                "肉",
                "强",
                "食",
                "，",
                "这",
                "是",
                "天",
                "地",
                "的",
                "规",
                "矩",
                "。",
                "我",
                "御",
                "灵",
                "宗",
                "驭",
                "兽",
                "役",
                "虫",
                "，",
                "正",
                "合",
                "此",
                "道",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
