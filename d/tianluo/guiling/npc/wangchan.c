// d/tianluo/guiling/npc/wangchan.c
// 鬼灵门 - 王蝉（少门主）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("王蝉", ({ "wang chan", "wangchan" }) );
        set("title", "鬼灵门少门主");
        set("gender", "男性");
        set("age", 25);
        set("long",
                "鬼灵门的少门主，暗灵根资质，是血灵大法双修的最佳人选之一。\n");
        set("attitude", "aggressive");
        set("sect", "guiling_sect");
        set("combat_exp", 1000000);
        set("max_qi", 2000);
        set("eff_qi", 2000);
        set("qi", 2000);
        set("max_jing", 1600);
        set("eff_jing", 1600);
        set("jing", 1600);
        set("max_neili", 2800);
        set("eff_neili", 2800);
        set("neili", 2800);
        set("max_jingli", 1680);
        set("eff_jingli", 1680);
        set("jingli", 1680);
        set("score", 15000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "王",
                "蝉",
                "傲",
                "然",
                "道",
                "：",
                "血",
                "灵",
                "大",
                "法",
                "需",
                "天",
                "灵",
                "根",
                "与",
                "暗",
                "灵",
                "根",
                "双",
                "修",
                "，",
                "我",
                "的",
                "暗",
                "灵",
                "根",
                "，",
                "正",
                "等",
                "着",
                "那",
                "位",
                "天",
                "灵",
                "根",
                "的",
                "道",
                "侣",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
