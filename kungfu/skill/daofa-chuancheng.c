// daofa-chuancheng.c 刀法传承
// 所属宗门：化刀坞（九宗档案「化刀坞」功法节）
// 特性：刀意速攻，刀法凶悍
// 出处：九宗档案「化刀坞」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 刀法传承：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "blade";
}

mapping *action = ({
([      "action": "$N刀意迸发，一刀快似一刀地劈向$n，刀光如练",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "割伤"
]),
([      "action": "$N一个箭步抢上，刀锋横扫，速攻如风，$n只得连连后退",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "割伤"
]),
([      "action": "$N大喝一声，凶悍一刀当头劈落，刀风凛冽",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "割伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    int lv = me->query_skill("daofa-chuancheng", 1);

    if ((int)me->query("jing") < 30)
        return notify_fail("你的精神无法集中了，休息一下再练吧。\n");
    if ((int)me->query("qi") < 30)
        return notify_fail("你现在手足酸软，休息一下再练吧。\n");
    if ((int)me->query("neili") < 10)
        return notify_fail("你的内力不够了。\n");

    me->receive_damage("qi", 30);
    me->add("neili", -10);
    return 1;
}
