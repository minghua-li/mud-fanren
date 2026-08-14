// d/yueguo/lingshou/npc/yunlu.c
// 灵兽山 - 云露（结丹）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("云露", ({ "yunlu", "yun lu" }) );
        set("title", "灵兽山长老");
        set("gender", "男性");
        set("age", 40);
        set("long",
                "灵兽山的结丹期修士，门派代表人物，腰间灵兽袋鼓鼓囊囊，隐隐有活物鼓动。\n");
        set("attitude", "peaceful");
        set("sect", "lingshou_mountain");
        set("combat_exp", 3000000);
        set("max_qi", 6800);
        set("eff_qi", 6800);
        set("qi", 6800);
        set("max_jing", 5440);
        set("eff_jing", 5440);
        set("jing", 5440);
        set("max_neili", 8500);
        set("eff_neili", 8500);
        set("neili", 8500);
        set("max_jingli", 5100);
        set("eff_jingli", 5100);
        set("jingli", 5100);
        set("score", 40000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "云露说道：灵兽山驭兽之术冠绝越国，只是……有些事情，未必如表面那般简单。\n",
                "云露低声道：金翅蚕吐丝可炼宝衣，噬金虫吞噬万物，这都是我灵兽山的根基。\n"
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
