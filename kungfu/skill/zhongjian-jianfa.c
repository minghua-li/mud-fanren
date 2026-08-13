// zhongjian-jianfa.c 重剑剑法
// 所属宗门：巨剑门（九宗档案「巨剑门」功法节）
// 特性：银色巨剑，一劈可破上品法器护罩
// 出处：九宗档案「巨剑门」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 重剑剑法：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "sword";
}

mapping *action = ({
([      "action": "$N双手握住巨剑，高高举起，一剑劈下，势若开山",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N巨剑横扫，罡风凛冽，$n只得举兵相架，虎口发麻",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N重剑缓缓推出，看似笨拙，却蕴含千钧之力，一力降十会",
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
    int lv = me->query_skill("zhongjian-jianfa", 1);

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
