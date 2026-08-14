// jianxiu-chuancheng.c 剑修传承
// 所属宗门：巨剑门（九宗档案「巨剑门」功法节）
// 特性：剑修立派，体剑双修
// 出处：九宗档案「巨剑门」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 剑修传承：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "sword";
}

mapping *action = ({
([      "action": "$N心剑合一，一道凝练剑意透体而出，直指$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "刺伤"
]),
([      "action": "$N体剑双修，肉身与剑光融为一体，$n竟难觅破绽",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "刺伤"
]),
([      "action": "$N剑气护体，剑意纵横，将$n的攻势一一格开",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "刺伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    int lv = me->query_skill("jianxiu-chuancheng", 1);

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
