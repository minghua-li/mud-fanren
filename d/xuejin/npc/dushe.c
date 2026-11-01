// d/xuejin/npc/dushe.c
// 血色禁地 - 外围 毒蛇（1 级妖兽）
// Created for ticket #108

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("毒蛇", ({ "dushe", "she" }) );
        set("title", "血色禁地·1级妖兽");
        set("gender", "雄性");
        set("age", 2);
        set("long",
                "一条足有丈许的青黑色毒蛇盘在腐叶间，信子嘶嘶作响，竖瞳里泛着幽绿的光。\n");
        set("attitude", "aggressive");
        set("combat_exp", 90000);
        set("max_qi", 1100);
        set("eff_qi", 1100);
        set("qi", 1100);
        set("max_jing", 880);
        set("eff_jing", 880);
        set("jing", 880);
        set("max_neili", 1650);
        set("eff_neili", 1650);
        set("neili", 1650);
        set("max_jingli", 990);
        set("eff_jingli", 990);
        set("jingli", 990);
        set("score", 900);
        // 血色禁地试炼挂接（#108）：realm/layer 供守护进程结算，sr_drop 为真实掉落物品表
        set("sr_realm", "blood_forbidden");
        set("sr_layer", 1);
        set("sr_score", 20);
        set("sr_drop", ([
                "/d/yueguo/tainan/obj/lingcao": 80,
        ]));
        setup();
}

// 真实掉落（#108 c3）：掉落物品为已合入主干的真实物品实体，不造同名占位。
// 触发链路：战斗系统通用 NPC 战胜判定 COMBAT_D->winner_reward（include/combat/reward.h）
// 在受害者（本 NPC）倒下时回调 victim->winner_reward(killer, victim)。
void winner_reward(object killer, object victim)
{
        mapping drops;
        string *paths;
        string path;
        object room, ob;
        int i;

        if (!objectp(killer) || !userp(killer))
                return;

        room = environment(victim);
        if (!objectp(room))
                return;

        drops = query("sr_drop");
        if (!mapp(drops))
                return;

        paths = keys(drops);
        for (i = 0; i < sizeof(paths); i++)
        {
                path = paths[i];
                if (random(100) >= drops[path])
                        continue;
                ob = new(path);
                if (!objectp(ob))
                        continue;
                ob->move(room);
                tell_object(killer, "那只" + victim->name() + "的尸体旁掉落了" +
                            ob->name() + "。\n");
        }

        // 试炼结算挂接（#108）：通知守护进程击杀（积分 + BOSS 存活标记）
        SECRET_REALM_D->on_mob_defeated(killer, victim);
}
