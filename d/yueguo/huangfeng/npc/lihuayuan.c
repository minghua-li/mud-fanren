// d/yueguo/huangfeng/npc/lihuayuan.c
// 黄枫谷 - 李化元（长老）
// Created for ticket #58

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("李化元", ({ "li huayuan", "lihuayuan" }) );
        set("title", "黄枫谷长老");
        set("gender", "男性");
        set("age", 40);
        set("long",
                "黄枫谷的长老，三阳之体，修习真阳诀，火属性体质如鱼得水，是韩立的恩师。\n");
        set("attitude", "friendly");
        set("sect", "huangfeng_valley");
        set("combat_exp", 3000000);
        set("max_qi", 7000);
        set("eff_qi", 7000);
        set("qi", 7000);
        set("max_jing", 5600);
        set("eff_jing", 5600);
        set("jing", 5600);
        set("max_neili", 9000);
        set("eff_neili", 9000);
        set("neili", 9000);
        set("max_jingli", 5400);
        set("eff_jingli", 5400);
        set("jingli", 5400);
        set("score", 40000);
        set("chat_chance", 30);
        set("chat_msg", ({
                "李化元说道：三阳之体修真阳诀，火属性体质如鱼得水，你可莫要选错了路。\n",
                "李化元笑道：我黄枫谷弟子，剑法丹药皆有所长，天下大可去得。\n"
        }));
        // —— 传功（#72）：本门功法 + 基本武学，供本门弟子 learn 请教 ——
        set_skill("force", 200);
        set_skill("dodge", 200);
        set_skill("parry", 200);
        set_skill("sword", 200);
        set_skill("changchun-gong", 200);
        set_skill("qingyuan-jianjue", 200);
        set_skill("zhenyang-jue", 200);
        set_skill("xuanbing-jue", 200);
        set_skill("guiyuan-gong", 200);
        set_skill("huanling-jue", 200);
        set_skill("ningyuan-gong", 200);
        setup();
        carry_object("/clone/misc/cloth")->wear();
}

// 传功：仅黄枫谷弟子可请教（learn 命令），外人不得偷学本门功法
int recognize_apprentice(object ob)
{
        if (SECT_D->query_player_sect(ob) != "huangfeng_valley")
                return 0;
        return 1;
}
