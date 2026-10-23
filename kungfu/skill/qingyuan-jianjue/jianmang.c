// jianmang.c 剑芒（青元剑诀 第1-4层 · 炼气期）
// 出处：1F §2.2「第1-4层 炼气期：基础剑芒、护体剑气」「第5层 筑基初期：剑芒强化」
// 施展：perform sword.jianmang <目标>（需持剑 + 激发青元剑诀，战斗中对手）
// 数值：内力 60，2 道剑芒；第5层起强化为 3 道（层数见 jianjue.h）

#include "jianjue.h"

inherit F_SSERVER;

int valid_target_arg(string target)
{
    if (target == "version") return 2;
    return target == "null" || target == "other";
}

int perform(object me, object target)
{
    int layer, hits, i, cost;
    object weapon;

    if (!target) target = offensive_target(me);
    if (!jianjue_ready(me)) return 0;
    if (!jianjue_target(me, target)) return 0;

    layer = jianjue_layer(me);
    if (layer < 1)
        return notify_fail("你尚未修习青元剑诀，施展不出剑芒。\n");

    cost = 60;
    if (me->query("neili") < cost)
        return notify_fail("你内力不足，凝不出剑芒。\n");

    weapon = me->query_temp("weapon");
    hits = (layer >= 5) ? 3 : 2;   // 第5层剑芒强化（1F §2.2）

    me->add("neili", -cost);
    me->start_busy(1 + random(2));

    message_vision(HIC "\n$N剑诀一引，一道青色剑芒破空而出，直取$n！\n" NOR, me, target);

    for (i = 0; i < hits; i++)
    {
        if (!objectp(target) || target->query("qi") < 0) break;
        jianjue_strike(me, target, weapon);
    }

    me->delete_temp("attack_val");
    return 1;
}
