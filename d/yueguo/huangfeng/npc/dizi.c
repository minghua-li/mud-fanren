// d/yueguo/huangfeng/npc/dizi.c
// 黄枫谷 - 弟子
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("黄枫谷弟子", ({ "huangfeng dizi", "dizi" }) );
        set("title", "黄枫谷弟子");
        set("gender", "男性");
        set("age", 22);
        set("long",
                "黄枫谷的年轻弟子，身着黄衫，腰悬符袋，正在谷中往来忙碌。\n");
        set("attitude", "peaceful");
        set("sect", "huangfeng_valley");
        set("combat_exp", 30000);
        set("max_qi", 600);
        set("eff_qi", 600);
        set("qi", 600);
        set("max_jing", 480);
        set("eff_jing", 480);
        set("jing", 480);
        set("max_neili", 900);
        set("eff_neili", 900);
        set("neili", 900);
        set("max_jingli", 540);
        set("eff_jingli", 540);
        set("jingli", 540);
        set("score", 3000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "黄枫谷弟子说道：这位道友可是来寻灵药的？岳麓殿禁制繁多，可莫要乱闯。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
