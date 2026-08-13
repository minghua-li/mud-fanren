// d/yueguo/huadao/npc/xiaoer.c
// 化刀坞 - 萧二（弟子）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("萧二", ({ "xiaoer", "xiao er" }) );
        set("title", "化刀坞弟子");
        set("gender", "男性");
        set("age", 25);
        set("long",
                "化刀坞的弟子，刀法凶悍，血禁试炼中曾与清虚门弟子厮杀。\n");
        set("attitude", "aggressive");
        set("sect", "huadao_dock");
        set("combat_exp", 40000);
        set("max_qi", 700);
        set("eff_qi", 700);
        set("qi", 700);
        set("max_jing", 560);
        set("eff_jing", 560);
        set("jing", 560);
        set("max_neili", 1000);
        set("eff_neili", 1000);
        set("neili", 1000);
        set("max_jingli", 600);
        set("eff_jingli", 600);
        set("jingli", 600);
        set("score", 4000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "萧",
                "二",
                "笑",
                "道",
                "：",
                "师",
                "兄",
                "教",
                "我",
                "，",
                "刀",
                "法",
                "要",
                "快",
                "，",
                "快",
                "到",
                "对",
                "手",
                "来",
                "不",
                "及",
                "反",
                "应",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
