// d/tianluo/guiling/npc/wangtiangu.c
// 鬼灵门 - 王天古（元婴主事）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("王天古", ({ "wangtiangu", "wang tiangu" }) );
        set("title", "鬼灵门主事长老");
        set("gender", "男性");
        set("age", 60);
        set("long",
                "鬼灵门的元婴期长老，门中主事之人，心机深沉，手段狠辣。\n");
        set("attitude", "aggressive");
        set("sect", "guiling_sect");
        set("combat_exp", 10000000);
        set("max_qi", 20000);
        set("eff_qi", 20000);
        set("qi", 20000);
        set("max_jing", 16000);
        set("eff_jing", 16000);
        set("jing", 16000);
        set("max_neili", 26000);
        set("eff_neili", 26000);
        set("neili", 26000);
        set("max_jingli", 15600);
        set("eff_jingli", 15600);
        set("jingli", 15600);
        set("score", 200000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "王天古阴声道：魔道之中，实力为尊。我鬼灵门虽列六宗之末，灭你越国七派却绰绰有余。\n",
                "王天古沉声道：燕家已入我门中，血灵大法双修在即，魔道大兴指日可待。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
