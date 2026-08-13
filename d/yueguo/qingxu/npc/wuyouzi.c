// d/yueguo/qingxu/npc/wuyouzi.c
// 清虚门 - 无游子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("无游子", ({ "wu youzi", "wuyouzi" }) );
        set("title", "清虚门道人");
        set("gender", "男性");
        set("age", 50);
        set("long",
                "清虚门的代表人物，博学多闻，通晓鬼灵门来历，常为同道解惑。\n");
        set("attitude", "friendly");
        set("sect", "qingxu_sect");
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
        set("score", 50000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "无游子温言道：清净无为，道法自然。我清虚门虽不争强斗胜，却也不惧外敌。\n",
                "无游子低声道：鬼灵门来历诡异，若遇上，千万小心。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
