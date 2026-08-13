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
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 200);
        set_skill("dodge", 200);
        set_skill("parry", 200);
        set_skill("unarmed", 200);
        set_skill("shuangxiu-zhishu", 200);
        set_skill("xuanyue-xiyin-gong", 200);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅掩月宗弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "yanyue_sect")
                return 0;
        return 1;
}
