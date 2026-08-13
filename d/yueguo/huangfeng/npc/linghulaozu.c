// d/yueguo/huangfeng/npc/linghulaozu.c
// 黄枫谷 - 令狐老祖（太上长老）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("令狐老祖", ({ "linghulaozu", "linghu laozu" }) );
        set("title", "黄枫谷太上长老");
        set("gender", "男性");
        set("age", 900);
        set("long",
                "黄枫谷的元婴期太上长老，九百余岁，常年云游在外，偶尔回谷坐镇。\n");
        set("attitude", "friendly");
        set("sect", "huangfeng_valley");
        set("combat_exp", 10000000);
        set("max_qi", 20000);
        set("eff_qi", 20000);
        set("qi", 20000);
        set("max_jing", 16000);
        set("eff_jing", 16000);
        set("jing", 16000);
        set("max_neili", 25000);
        set("eff_neili", 25000);
        set("neili", 25000);
        set("max_jingli", 15000);
        set("eff_jingli", 15000);
        set("jingli", 15000);
        set("score", 200000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "令狐老祖捋须说道：老夫九百余岁，看尽世事，你且记住——修仙一途，机缘与心性缺一不可。\n",
                "令狐老祖叹道：魔道若犯越国，老夫少不得要行一次诱饵之计了。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
