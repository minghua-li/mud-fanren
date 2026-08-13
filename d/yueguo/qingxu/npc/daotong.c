// d/yueguo/qingxu/npc/daotong.c
// 清虚门 - 守山道童
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("守山道童", ({ "daotong", "dao tong" }) );
        set("title", "清虚门弟子");
        set("gender", "男性");
        set("age", 16);
        set("long",
                "清虚门的守山道童，一身素净道袍，眉目清秀。\n");
        set("attitude", "peaceful");
        set("sect", "qingxu_sect");
        set("combat_exp", 8000);
        set("max_qi", 300);
        set("eff_qi", 300);
        set("qi", 300);
        set("max_jing", 240);
        set("eff_jing", 240);
        set("jing", 240);
        set("max_neili", 500);
        set("eff_neili", 500);
        set("neili", 500);
        set("max_jingli", 300);
        set("eff_jingli", 300);
        set("jingli", 300);
        set("score", 1000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "守山道童轻声道：道长请留步，清虚山乃清净之地，入山请低声细语。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
