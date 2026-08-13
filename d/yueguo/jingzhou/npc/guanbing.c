// d/yueguo/jingzhou/npc/guanbing.c
// 镜州 - 官兵
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("镜州官兵", ({ "guanbing", "bing" }) );
        set("title", "镜州城巡丁");
        set("gender", "男性");
        set("age", 28);
        set("long",
                "一名镜州城的官兵，身着皂袍，持矛巡街。镜州城如今由官府管辖，\n"
                "兵丁们对江湖帮派向来没什么好脸色。\n");
        set("attitude", "peaceful");
        set("combat_exp", 2500);
        set("max_qi", 320);
        set("eff_qi", 320);
        set("qi", 320);
        set("max_jing", 280);
        set("eff_jing", 280);
        set("jing", 280);
        set("score", 250);
        set("chat_chance", 15);
        set("chat_msg", ({
                "官兵哼了一声：城里少惹事，彩霞山那帮人如今可管不到镜州城。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
