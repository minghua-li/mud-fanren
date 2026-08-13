// d/yueguo/yanyue/npc/qionglao.c
// 掩月宗 - 穹老怪（结丹长老）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("穹老怪", ({ "qiong lao", "qionglao" }) );
        set("title", "掩月宗长老");
        set("gender", "男性");
        set("age", 60);
        set("long",
                "掩月宗的结丹期前辈，性格古怪，法力深厚，说话总带着三分冷硬。\n");
        set("attitude", "peaceful");
        set("sect", "yanyue_sect");
        set("combat_exp", 3000000);
        set("max_qi", 6500);
        set("eff_qi", 6500);
        set("qi", 6500);
        set("max_jing", 5200);
        set("eff_jing", 5200);
        set("jing", 5200);
        set("max_neili", 8500);
        set("eff_neili", 8500);
        set("neili", 8500);
        set("max_jingli", 5100);
        set("eff_jingli", 5100);
        set("jingli", 5100);
        set("score", 30000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "穹老怪哼了一声：小娃娃，来我掩月宗传功阁，须得静下心来，莫要浮躁。\n",
                "穹老怪怪笑道：玄月吸阴功乃阴系双修秘术，岂是人人修得的？\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
