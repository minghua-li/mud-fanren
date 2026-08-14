// d/yueguo/yanyue/npc/nishang.c
// 掩月宗 - 霓裳仙子（结丹领队）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("霓裳仙子", ({ "nishang", "nishang xianzi" }) );
        set("title", "掩月宗长老");
        set("gender", "女性");
        set("age", 28);
        set("long",
                "掩月宗的结丹期女修，容貌秀美的少妇，气质雍容，是掩月宗带队之人。\n");
        set("attitude", "friendly");
        set("sect", "yanyue_sect");
        set("combat_exp", 3000000);
        set("max_qi", 6000);
        set("eff_qi", 6000);
        set("qi", 6000);
        set("max_jing", 4800);
        set("eff_jing", 4800);
        set("jing", 4800);
        set("max_neili", 8000);
        set("eff_neili", 8000);
        set("neili", 8000);
        set("max_jingli", 4800);
        set("eff_jingli", 4800);
        set("jingli", 4800);
        set("score", 30000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "霓裳仙子轻声说道：掩月宗乃越国七派之首，能入我宗者，无不是资质上乘之人。\n",
                "霓裳仙子说道：双修之术阴阳相济，可精进法力，这是我掩月宗的立派之基。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
