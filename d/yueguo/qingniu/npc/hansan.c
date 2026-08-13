// d/yueguo/qingniu/npc/hansan.c
// 青牛镇 - 韩三叔（春香酒楼掌柜）
// Created for ticket #67

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("韩三叔", ({ "han sanshu", "han", "sanshu" }) );
        set("title", "春香酒楼掌柜");
        set("gender", "男性");
        set("age", 45);
        set("long",
                "春香酒楼的掌柜韩三叔，圆脸微胖，常年笑呵呵的。他在青牛镇\n"
                "开了十几年酒楼，与镇上各色人等都有交情，消息灵通，待人\n"
                "也热络。\n");
        set("attitude", "friendly");
        set("combat_exp", 800);
        set("max_qi", 300);
        set("eff_qi", 300);
        set("qi", 300);
        set("max_jing", 300);
        set("eff_jing", 300);
        set("jing", 300);
        set("score", 300);
        set("chat_chance", 30);
        set("chat_msg", ({
                "韩三叔抹着柜台笑道：客官是要打尖还是住店？本店的女儿红是镜州一绝。\n",
                "韩三叔压低声音道：听说彩霞山上的七玄门又要招收弟子了，镇上不少后生都动了心思。\n",
                "韩三叔叹道：野狼帮那帮人又在镇外转悠，这几年越发不安分了。\n",
        }));
        setup();
        carry_object("/clone/misc/cloth")->wear();
}
