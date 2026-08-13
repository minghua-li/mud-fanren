// d/yueguo/huangfeng/npc/hongfu.c
// 黄枫谷 - 红拂（结丹女修）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("红拂", ({ "hongfu", "hong fu" }) );
        set("title", "黄枫谷长老");
        set("gender", "女性");
        set("age", 35);
        set("long",
                "黄枫谷唯一的女结丹修士，为人严肃，极为护短。\n");
        set("attitude", "friendly");
        set("sect", "huangfeng_valley");
        set("combat_exp", 3000000);
        set("max_qi", 6500);
        set("eff_qi", 6500);
        set("qi", 6500);
        set("max_jing", 5200);
        set("eff_jing", 5200);
        set("jing", 5200);
        set("max_neili", 8000);
        set("eff_neili", 8000);
        set("neili", 8000);
        set("max_jingli", 4800);
        set("eff_jingli", 4800);
        set("jingli", 4800);
        set("score", 40000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "红",
                "拂",
                "冷",
                "冷",
                "说",
                "道",
                "：",
                "谷",
                "内",
                "弟",
                "子",
                "皆",
                "是",
                "我",
                "黄",
                "枫",
                "谷",
                "之",
                "人",
                "，",
                "谁",
                "敢",
                "欺",
                "负",
                "我",
                "谷",
                "中",
                "弟",
                "子",
                "，",
                "莫",
                "怪",
                "我",
                "出",
                "手",
                "。",
                "\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
