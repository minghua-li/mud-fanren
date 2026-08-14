// qingyuan-jianjue.c 青元剑诀
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：剑修功法，残本9层，每三层一门神通（剑芒/剑盾/剑影分光）
// 出处：九宗档案「黄枫谷」功法节 / 1C §3.3

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    int lv = me->query_skill("qingyuan-jianjue", 1);

    // 残本九层境界分段（1C §3.3）：层1-3炼气期、层4-6筑基期、层7-9结丹期
    if (lv <= 30)
    {
        if (SECT_D->query_cultivation_tier(me) < 0)
            return notify_fail("「青元剑诀」前三层需炼气期修为方可修习。\n");
    }
    else if (lv <= 60)
    {
        if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
            return notify_fail("「青元剑诀」中三层需筑基期修为方可修习。\n");
    }
    else
    {
        if (SECT_D->query_cultivation_tier(me) < SECT_TIER_JIE)
            return notify_fail("「青元剑诀」后三层需结丹期修为方可修习。\n");
    }
    return 1;
}

int valid_enable(string usage)
{
    return usage == "sword";
}

mapping *action = ({
([      "action": "$N剑诀一引，一道青色剑芒破空而出，直刺$n$l，凌厉无匹",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "刺伤"
]),
([      "action": "$N剑光回旋，在身周凝成护体剑盾，剑芒暗藏，伺机反击",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "刺伤"
]),
([      "action": "$N手中长剑一化为三，三道剑影分光斩向$n，虚实难辨",
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
    int lv = me->query_skill("qingyuan-jianjue", 1);

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
