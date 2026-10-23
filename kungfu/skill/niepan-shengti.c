// niepan-shengti.c 涅槃圣体
// 体修·极限炼体（COMBAT_SYSTEM §三「极限炼体：涅槃圣体，修炼到极致不死不灭」）
// 所属宗门：巨剑门
// 特性：肉身近于不坏，淬体上限进一步提升（BODY_STAGE_ULTIMATE，cap 800）

#include <ansi.h>
#include <sect.h>
#include <globals.h>
#include <body.h>

inherit SKILL;

string type() { return "martial"; }

int valid_enable(string usage)
{
    return usage == "iron-cloth";
}

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_YING)
        return notify_fail("「涅槃圣体」需元婴期以上修为方可修习。\n");
    if ((int)me->query_skill(BODY_SKILL_DUAL, 1) < BODY_PREREQ_SKILL)
        return notify_fail("修习「涅槃圣体」须法体双修有成（梵圣真魔功 " +
                    BODY_PREREQ_SKILL + " 级以上）。\n");
    return 1;
}

mapping *action = ({
([      "action": "$N涅槃圣体自然流转，肌肤泛起淡金光泽，一拳挥出势大力沉",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N举手投足间肉身如宝，硬受$n一击，反手一拳砸回",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N催动涅槃之力，一拳击出，$n只觉如山岳压来",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "瘀伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    if ((int)me->query("qi") < 80)
        return notify_fail("你的气血不足，难以承受涅槃炼体之苦。\n");
    if ((int)me->query("neili") < 40)
        return notify_fail("你的内力不足。\n");

    me->receive_damage("qi", 60);
    me->add("neili", -40);
    return 1;
}
