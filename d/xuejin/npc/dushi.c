// d/xuejin/npc/dushi.c
// 血色禁地 - 外围/核心 毒蝎群（1-2 级妖兽）
// Created for ticket #108

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("毒蝎群", ({ "dushi", "xie" }) );
        set("title", "血色禁地·1-2级妖兽");
        set("gender", "雄性");
        set("age", 3);
        set("long",
                "成群的毒蝎从腐木下涌出，尾钩高翘，滴落的毒液在地面蚀出一个个小坑。\n");
        set("attitude", "aggressive");
        set("combat_exp", 110000);
        set("max_qi", 1500);
        set("eff_qi", 1500);
        set("qi", 1500);
        set("max_jing", 1200);
        set("eff_jing", 1200);
        set("jing", 1200);
        set("max_neili", 2250);
        set("eff_neili", 2250);
        set("neili", 2250);
        set("max_jingli", 1350);
        set("eff_jingli", 1350);
        set("jingli", 1350);
        set("score", 1100);
        // 血色禁地试炼挂接（#108）：realm/layer 供守护进程结算，sr_drop 为真实掉落物品表
        set("sr_realm", "blood_forbidden");
        set("sr_layer", 1);
        set("sr_score", 30);
        set("sr_drop", ([
                "/d/yueguo/tainan/obj/lingcao": 80,
                "/d/yueguo/tainan/obj/huanglongcao": 40,
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
