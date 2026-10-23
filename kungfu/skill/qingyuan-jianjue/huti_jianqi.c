// huti_jianqi.c 护体剑气（青元剑诀 第1-4层 · 炼气期）
// 出处：1F §2.2「第1-4层 炼气期：基础剑芒、护体剑气」——飞剑剑气遍布周身凝成护盾
// 施展：perform sword.huti_jianqi <目标>（战斗中运转，效果作用于自身）
// 数值：内力 100，减伤随层数成长（百分比 10+层、固定 30+层×10），持续 20+层×5 秒（上限 60 秒）

#include "jianjue.h"

inherit F_SSERVER;

#define HUTI_BUFF "active_buffer/qingyuan-jianjue.huti_jianqi"

void remove_effect(object me);

int valid_target_arg(string target)
{
    if (target == "version") return 2;
    return target == "null" || target == "other";
}

int perform(object me, object target)
{
    int layer, duration, week, flat, cost;

    if (!target) target = offensive_target(me);
    if (!jianjue_ready(me)) return 0;
    if (!jianjue_target(me, target)) return 0;

    if (me->query_temp(HUTI_BUFF))
        return notify_fail("你的护体剑气已在运转。\n");

    layer = jianjue_layer(me);
    if (layer < 1)
        return notify_fail("你尚未修习青元剑诀，运不起护体剑气。\n");

    cost = 100;
    if (me->query("neili") < cost)
        return notify_fail("你内力不足，护体剑气凝而不实。\n");

    week = 10 + layer;
    if (week > 20) week = 20;
    flat = 30 + layer * 10;
    if (flat > 120) flat = 120;
    duration = 20 + layer * 5;
    if (duration > 60) duration = 60;

    me->add("neili", -cost);
    me->add_temp("apply/week_injure", week);
    me->add_temp("apply/defense_attack", flat);
    me->set_temp(HUTI_BUFF + "/week", week);
    me->set_temp(HUTI_BUFF + "/flat", flat);
    me->set_temp(HUTI_BUFF + "/last_time", time() + duration);
    me->set_temp(HUTI_BUFF + "/name", "护体剑气");
    me->start_call_out(__FILE__, "remove_effect", duration, me);
    me->start_busy(1 + random(2));

    message_vision(HIC "\n$N默运青元剑诀，周身剑气萦绕，凝成一层青蒙蒙的光罩护住全身。\n" NOR, me);
    tell_object(me, HIG "【护体剑气】减伤 " + week + "% 及 " + flat +
                " 点，持续 " + duration + " 秒。\n" NOR);
    return 1;
}

void remove_effect(object me)
{
    int week, flat;

    if (!objectp(me)) return;
    if (!me->query_temp(HUTI_BUFF)) return;

    week = me->query_temp(HUTI_BUFF + "/week");
    flat = me->query_temp(HUTI_BUFF + "/flat");
    if (intp(week) && week > 0) me->add_temp("apply/week_injure", -week);
    if (intp(flat) && flat > 0) me->add_temp("apply/defense_attack", -flat);
    me->delete_temp(HUTI_BUFF);
    tell_object(me, HIC "剑气光罩渐渐散去。\n" NOR);
}
