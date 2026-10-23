// santou_liubi.c 三头六臂（梵圣真魔功·法体双修神通）
// 出处：COMBAT_SYSTEM §三「法体双修：三头六臂魔神法相，近战+法术」/ 1F §4.7 skill/秘术/三头六臂.c
// 施展：exert santou_liubi（需激发梵圣真魔功为内功）
// 数值：内力 200，命中 +30、伤害 +40，持续 20 + 每 10 级 +4 秒（上限 60 秒）

#include <ansi.h>
#include <body.h>

inherit F_CLEAN_UP;

void remove_effect(object me, int attack, int damage);

int exert(object me, object target)
{
    int lv, duration, attack, damage;

    if (target != me)
        return notify_fail("「三头六臂」只能对自己施展。\n");

    if (me->query_skill_mapped("force") != BODY_SKILL_DUAL)
        return notify_fail("你须以梵圣真魔功为内功，方能现出三头六臂法相。\n");

    if (me->query_temp(BODY_FAXIANG_BUFF))
        return notify_fail("你的三头六臂法相尚未散去。\n");

    if (me->query("neili") < BODY_FAXIANG_NEILI)
        return notify_fail("你内力不足，凝不出魔神法相。\n");

    lv = me->query_skill(BODY_SKILL_DUAL, 1);
    if (lv < 1)
        return notify_fail("你尚未修习梵圣真魔功。\n");

    duration = BODY_FAXIANG_LEVEL + lv / 10 * BODY_FAXIANG_PER_LEVEL;
    if (duration > BODY_FAXIANG_MAX_TIME) duration = BODY_FAXIANG_MAX_TIME;

    attack = BODY_FAXIANG_ATTACK + lv / 10;
    damage = BODY_FAXIANG_DAMAGE + lv / 5;

    me->add("neili", -BODY_FAXIANG_NEILI);
    me->add_temp("apply/attack", attack);
    me->add_temp("apply/damage", damage);
    me->set_temp(BODY_FAXIANG_BUFF + "/attack", attack);
    me->set_temp(BODY_FAXIANG_BUFF + "/damage", damage);
    me->set_temp(BODY_FAXIANG_BUFF + "/last_time", time() + duration);
    me->start_call_out(__FILE__, "remove_effect", duration, me, attack, damage);
    if (me->is_fighting()) me->start_busy(2);

    message_vision(HIR "\n$N催动梵圣真魔功，身后魔气翻涌，隐隐现出三头六臂的魔神法相！\n" NOR, me);
    tell_object(me, HIG "【三头六臂】命中 +" + attack + "、伤害 +" + damage +
                "，持续 " + duration + " 秒。\n" NOR);
    return 1;
}

void remove_effect(object me, int attack, int damage)
{
    if (!objectp(me)) return;
    if (!me->query_temp(BODY_FAXIANG_BUFF)) return;

    me->add_temp("apply/attack", -attack);
    me->add_temp("apply/damage", -damage);
    me->delete_temp(BODY_FAXIANG_BUFF);
    tell_object(me, HIC "魔神法相渐渐散去，你回到本来面目。\n" NOR);
}
