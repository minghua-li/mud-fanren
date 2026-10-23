// wandu-hunyuan-shen.c 万毒混元身
// 体修·毒体修炼（COMBAT_SYSTEM §三「毒体修炼：万毒混元身，毒抗+毒攻」）
// 所属宗门：鬼灵门（魔道毒功）
// 特性：以万毒淬体，升高淬体上限（BODY_STAGE_POISON，cap 1200）；
//       毒体小成后空手出手自带「五毒神掌毒」，并对毒性伤害有额外抗性

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
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_JIE)
        return notify_fail("「万毒混元身」需结丹期以上修为方可修习。\n");
    if ((int)me->query_skill(BODY_SKILL_ULTIMATE, 1) < BODY_PREREQ_SKILL)
        return notify_fail("修习「万毒混元身」须极限炼体有成（涅槃圣体 " +
                    BODY_PREREQ_SKILL + " 级以上）。\n");
    return 1;
}

mapping *action = ({
([      "action": "$N掌缘泛出青紫毒气，一掌拍向$n，空气中隐隐有腥味",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "毒伤"
]),
([      "action": "$N五指如钩，毒气缠指，直插$n要害",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "毒伤"
]),
([      "action": "$N万毒混元身催至极致，周身毒雾翻涌，一掌罩向$n",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "毒伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

// 毒体淬炼：以自身气血试毒（practice 需有相当气血）
int practice_skill(object me)
{
    if ((int)me->query("qi") < 100)
        return notify_fail("你气血不足，压制不住体内毒性。\n");
    if ((int)me->query("neili") < 50)
        return notify_fail("你的内力不足，收不住毒气。\n");

    me->receive_damage("qi", 80);
    me->add("neili", -50);
    return 1;
}
