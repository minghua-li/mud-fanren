// qingxu-jian-dian.c 清虚剑典
// 所属宗门：清虚门（九宗档案「清虚门」功法节）
// 特性：剑符双修，道剑路线
// 出处：九宗档案「清虚门」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「清虚剑典」需筑基期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "sword";
}

mapping *action = ({
([      "action": "$N长剑出鞘，道剑合一，一道清光斩向$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "刺伤"
]),
([      "action": "$N剑尖一挑，符箓化作飞剑破空而出，直取$n$l",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "刺伤"
]),
([      "action": "$N剑符合一，剑光符影交织成网，将$n困在当中",
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
    int lv = me->query_skill("qingxu-jian-dian", 1);

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
