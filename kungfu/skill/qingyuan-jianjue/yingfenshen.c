// yingfenshen.c 剑影分光术（青元剑诀 第6-9层 · 筑基~结丹）
// 出处：1F §2.2「第6-9层 筑基~结丹：青竹蜂云剑操控、剑影分光术」/ §2.5 剑影分光术
// 施展：perform sword.yingfenshen <目标>（需持剑 + 激发青元剑诀，战斗中对手）
// 数值：内力 200，一口飞剑幻化多道剑光（3 道起，每多一层 +1，上限 6 道）
// 飞剑数量体系（36/72 口）尚未落地，此处以层数+筑基修为+内力为门槛

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
    if (layer < 6)
        return notify_fail("剑影分光术须青元剑诀第六层以上修为（当前第 " + layer + " 层）。\n");
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("剑影分光术须筑基期以上修为方可施展。\n");

    cost = 200;
    if (me->query("neili") < cost)
        return notify_fail("你内力不足，分不出剑影。\n");

    weapon = me->query_temp("weapon");
    hits = 3 + (layer - 6);
    if (hits > 6) hits = 6;

    me->add("neili", -cost);
    me->start_busy(2);

    message_vision(HIC "\n$N手中长剑一抖，剑光分化成 " + chinese_number(hits) +
                   " 道剑影，虚实难辨地罩向$n！\n" NOR, me, target);

    for (i = 0; i < hits; i++)
    {
        if (!objectp(target) || target->query("qi") < 0) break;
        jianjue_strike(me, target, weapon);
    }

    me->delete_temp("attack_val");
    return 1;
}
