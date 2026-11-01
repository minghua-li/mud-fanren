// d/xuejin/npc/huwei.c
// 血色禁地 - 核心 护卫妖兽（3 级妖兽，BOSS 护卫）
// Created for ticket #108

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("护卫妖兽", ({ "huwei", "huwei yaoshou" }) );
        set("title", "血色禁地·3级妖兽");
        set("gender", "雄性");
        set("age", 10);
        set("long",
                "一头形如黑豹的妖兽蹲守在水窟洞口，颈后生出两片骨刺，目光死死盯着来者，护卫着巢窟深处的主宰。\n");
        set("attitude", "aggressive");
        set("combat_exp", 600000);
        set("max_qi", 3400);
        set("eff_qi", 3400);
        set("qi", 3400);
        set("max_jing", 2720);
        set("eff_jing", 2720);
        set("jing", 2720);
        set("max_neili", 5100);
        set("eff_neili", 5100);
        set("neili", 5100);
        set("max_jingli", 3060);
        set("eff_jingli", 3060);
        set("jingli", 3060);
        set("score", 6000);
        // 血色禁地试炼挂接（#108）：realm/layer 供守护进程结算，sr_drop 为真实掉落物品表
        set("sr_realm", "blood_forbidden");
        set("sr_layer", 3);
        set("sr_score", 80);
        set("sr_drop", ([
                "/d/yueguo/tainan/obj/huanglongcao": 80,
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
