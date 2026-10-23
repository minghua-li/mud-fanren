// juyuan.c 巨剑术（青元剑诀 第6-9层 · 筑基~结丹）
// 出处：1F §2.5「巨剑术：多口飞剑合为一柄巨剑 | 汇聚伤害，单次爆发」
// 施展：perform sword.juyuan <目标>（需持剑 + 激发青元剑诀，战斗中对手）
// 数值：内力 250，一剑爆发（临时 apply/damage 加成 40+层×10，上限 120，命中后即刻撤销）
// 飞剑数量体系尚未落地，以层数+筑基修为+内力为门槛

#include "jianjue.h"

inherit F_SSERVER;

int valid_target_arg(string target)
{
    if (target == "version") return 2;
    return target == "null" || target == "other";
}

int perform(object me, object target)
{
    int layer, bonus, cost;
    object weapon;

    if (!target) target = offensive_target(me);
    if (!jianjue_ready(me)) return 0;
    if (!jianjue_target(me, target)) return 0;

    layer = jianjue_layer(me);
    if (layer < 6)
        return notify_fail("巨剑术须青元剑诀第六层以上修为（当前第 " + layer + " 层）。\n");
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("巨剑术须筑基期以上修为方可施展。\n");

    cost = 250;
    if (me->query("neili") < cost)
        return notify_fail("你内力不足，合不出巨剑。\n");

    weapon = me->query_temp("weapon");
    bonus = 40 + layer * 10;
    if (bonus > 120) bonus = 120;

    me->add("neili", -cost);
    me->start_busy(2);
    me->add_temp("apply/damage", bonus);

    message_vision(HIC "\n$N剑诀催动，数道剑光倏地合拢成一柄青色巨剑，挟千钧之势劈向$n！\n" NOR, me, target);
    jianjue_strike(me, target, weapon);

    me->add_temp("apply/damage", -bonus);
    me->delete_temp("attack_val");
    return 1;
}
