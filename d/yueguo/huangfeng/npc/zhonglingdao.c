// d/yueguo/huangfeng/npc/zhonglingdao.c
// 黄枫谷 - 钟灵道（掌门）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("钟灵道", ({ "zhongling", "zhong lingdao" }) );
        set("title", "黄枫谷掌门");
        set("gender", "男性");
        set("age", 45);
        set("long",
                "黄枫谷掌门，筑基后期修为，生性沉稳、威望极高，执掌谷中日常事务。\n");
        set("attitude", "friendly");
        set("sect", "huangfeng_valley");
        set("combat_exp", 400000);
        set("max_qi", 2200);
        set("eff_qi", 2200);
        set("qi", 2200);
        set("max_jing", 1760);
        set("eff_jing", 1760);
        set("jing", 1760);
        set("max_neili", 3000);
        set("eff_neili", 3000);
        set("neili", 3000);
        set("max_jingli", 1800);
        set("eff_jingli", 1800);
        set("jingli", 1800);
        set("score", 20000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "钟灵道缓缓说道：黄枫谷以丹药符箓立派，若有心向道，可先习长春功打熬根基。\n",
                "钟灵道说道：筑基丹主药出自血色禁地，是谷内最核心的资源。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
