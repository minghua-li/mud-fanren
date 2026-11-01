// d/xuejin/npc/mojiao.c
// 血色禁地 - 核心 BOSS 墨蛟（3 级妖兽）
// Created for ticket #108

#include <ansi.h>

inherit NPC;

void create()
{
        set_name("墨蛟", ({ "mojiao", "mo jiao" }) );
        set("title", "血色禁地·3级妖兽·巢穴主宰");
        set("gender", "雄性");
        set("age", 200);
        set("long",
                "黑水翻涌，一条巨蛟自水底缓缓浮起，鳞甲如墨玉般漆黑，双目血光闪烁。"
                "它盘踞在这巢窟已逾百年，周身血雾凝而不散——这就是血色禁地深处的主宰。\n");
        set("attitude", "aggressive");
        set("combat_exp", 1500000);
        set("max_qi", 6000);
        set("eff_qi", 6000);
        set("qi", 6000);
        set("max_jing", 4800);
        set("eff_jing", 4800);
        set("jing", 4800);
        set("max_neili", 9000);
        set("eff_neili", 9000);
        set("neili", 9000);
        set("max_jingli", 5400);
        set("eff_jingli", 5400);
        set("jingli", 5400);
        set("score", 15000);
        // 血色禁地试炼挂接（#108）：realm/layer/score 供守护进程结算；
        // sr_boss=1 标记 BOSS——击杀即「试炼存活」，由守护进程写入存活标记
        set("sr_realm", "blood_forbidden");
        set("sr_layer", 3);
        set("sr_score", 200);
        set("sr_boss", 1);
        set("sr_drop", ([
                "/d/yueguo/tainan/obj/huanglongcao": 100,
        ]));
        setup();
}

// 真实掉落（#108 c3）：BOSS 必掉筑基丹主药黄龙草（主线 mq_1_9 口径：禁地为主产地）。
// 掉落物品为已合入主干的真实物品实体，不造同名占位。
// 触发链路：战斗系统通用 NPC 战胜判定 COMBAT_D->winner_reward（include/combat/reward.h）
// 在受害者（本 NPC）倒下时回调 victim->winner_reward(killer, victim)；
// 守护进程 SECRET_REALM_D->on_mob_defeated 在同链路中写入试炼存活标记。
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
                tell_object(killer, "巨蛟倒地，黑水翻腾，它的身上掉落了" +
                            ob->name() + "——筑基丹的主药。\n");
        }

        // 试炼结算挂接（#108）：通知守护进程击杀（积分 + BOSS 存活标记）
        SECRET_REALM_D->on_mob_defeated(killer, victim);
}
