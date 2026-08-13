// d/yueguo/qingniu/npc/xiaofan.c
// 青牛镇 - 小贩
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("青牛镇小贩", ({ "xiaofan", "fan" }) );
        set("title", "货郎");
        set("gender", "男性");
        set("age", 30);
        set("long",
                "一个挑着货担的青牛镇小贩，担子里装着针线、粗布、盐巴之类\n"
                "的日用杂货，也捎带着收些山货草药。\n");
        set("attitude", "friendly");
        set("combat_exp", 300);
        set("max_qi", 200);
        set("eff_qi", 200);
        set("qi", 200);
        set("max_jing", 200);
        set("eff_jing", 200);
        set("jing", 200);
        set("score", 100);
        set("chat_chance", 20);
        set("chat_msg", ({
                "小贩吆喝道：针头线脑，粗布盐巴，走过路过别错过喽！\n",
                "小贩凑过来低声道：往北走就是彩霞山，七玄门招弟子时，镇上可热闹了。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
