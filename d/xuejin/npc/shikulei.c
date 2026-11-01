// d/xuejin/npc/shikulei.c
// 血色禁地 - 中层 石傀儡（2 级妖兽）
// Created for ticket #108

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("石傀儡", ({ "shikulei", "kulei" }) );
        set("title", "血色禁地·2级妖兽");
        set("gender", "男性");
        set("age", 80);
        set("long",
                "一具丈二高的石像活了过来，石皮缝隙里透出暗红色的血光，缓步从遗迹深处走出。\n");
        set("attitude", "aggressive");
        set("combat_exp", 300000);
        set("max_qi", 2600);
        set("eff_qi", 2600);
        set("qi", 2600);
        set("max_jing", 2080);
        set("eff_jing", 2080);
        set("jing", 2080);
        set("max_neili", 3900);
        set("eff_neili", 3900);
        set("neili", 3900);
        set("max_jingli", 2340);
        set("eff_jingli", 2340);
        set("jingli", 2340);
        set("score", 3000);
        // 血色禁地试炼挂接（#108）：realm/layer 供守护进程结算，sr_drop 为真实掉落物品表
        set("sr_realm", "blood_forbidden");
        set("sr_layer", 2);
        set("sr_score", 50);
        set("sr_drop", ([
                "/d/yueguo/tainan/obj/huanglongcao": 60,
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
                tell_object(killer, "那只" + victim->name() + "的残骸旁掉落了" +
                            ob->name() + "。\n");
        }

        // 试炼结算挂接（#108）：通知守护进程击杀（积分 + BOSS 存活标记）
        SECRET_REALM_D->on_mob_defeated(killer, victim);
}
