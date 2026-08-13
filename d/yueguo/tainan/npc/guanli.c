// d/yueguo/tainan/npc/guanli.c
// 太南谷 - 坊市管事
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("坊市管事", ({ "fangshi guanli", "guanli" }) );
        set("title", "太南谷坊市管事");
        set("gender", "男性");
        set("age", 40);
        set("long",
                "太南谷坊市的管事，常年守着一座柜台。此人修为不高，但见多识广，\n"
                "对各色材料的成色行情了如指掌，散修们交易前多会先来问价。\n");
        set("attitude", "friendly");
        set("combat_exp", 80000);
        set("max_qi", 900);
        set("eff_qi", 900);
        set("qi", 900);
        set("max_jing", 900);
        set("eff_jing", 900);
        set("jing", 900);
        set("max_neili", 1200);
        set("eff_neili", 1200);
        set("neili", 1200);
        set("score", 4000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "坊市管事招呼道：道友要买卖材料，尽管开口，本坊童叟无欺。\n",
                "坊市管事低声道：太南小会开市时，这里可比现在热闹十倍。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
