// d/yueguo/jujian/npc/dizi.c
// 巨剑门 - 黑衣弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("黑衣弟子", ({ "heyi dizi", "dizi" }) );
        set("title", "巨剑门弟子");
        set("gender", "男性");
        set("age", 26);
        set("long",
                "巨剑门的弟子，一身黑衣，后背一把一人高的无鞘巨剑，神色冷酷。\n");
        set("attitude", "peaceful");
        set("sect", "jujian_gate");
        set("combat_exp", 30000);
        set("max_qi", 650);
        set("eff_qi", 650);
        set("qi", 650);
        set("max_jing", 520);
        set("eff_jing", 520);
        set("jing", 520);
        set("max_neili", 950);
        set("eff_neili", 950);
        set("neili", 950);
        set("max_jingli", 570);
        set("eff_jingli", 570);
        set("jingli", 570);
        set("score", 3000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "黑",
                "衣",
                "弟",
                "子",
                "喝",
                "道",
                "：",
                "黑",
                "衣",
                "巨",
                "剑",
                "，",
                "生",
                "死",
                "不",
                "惧",
                "。",
                "来",
                "者",
                "何",
                "人",
                "，",
                "报",
                "上",
                "名",
                "来",
                "！",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
