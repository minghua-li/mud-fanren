// d/yueguo/huadao/npc/hantianya.c
// 化刀坞 - 寒天涯（弟子）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("寒天涯", ({ "han tianya", "hantianya" }) );
        set("title", "化刀坞弟子");
        set("gender", "男性");
        set("age", 30);
        set("long",
                "化刀坞的弟子，人称『人妖』，行为乖戾，行事出人意表。\n");
        set("attitude", "aggressive");
        set("sect", "huadao_dock");
        set("combat_exp", 50000);
        set("max_qi", 800);
        set("eff_qi", 800);
        set("qi", 800);
        set("max_jing", 640);
        set("eff_jing", 640);
        set("jing", 640);
        set("max_neili", 1200);
        set("eff_neili", 1200);
        set("neili", 1200);
        set("max_jingli", 720);
        set("eff_jingli", 720);
        set("jingli", 720);
        set("score", 5000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "寒",
                "天",
                "涯",
                "怪",
                "笑",
                "道",
                "：",
                "嘿",
                "，",
                "你",
                "这",
                "小",
                "辈",
                "，",
                "可",
                "敢",
                "与",
                "我",
                "比",
                "划",
                "比",
                "划",
                "？",
                "我",
                "行",
                "事",
                "向",
                "来",
                "乖",
                "戾",
                "，",
                "可",
                "莫",
                "要",
                "惹",
                "我",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
