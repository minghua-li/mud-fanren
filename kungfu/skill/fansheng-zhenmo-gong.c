// fansheng-zhenmo-gong.c 梵圣真魔功
// 体修·法体双修（COMBAT_SYSTEM §三「法体双修：梵圣真魔功，三头六臂魔神法相，近战+法术」）
// 所属宗门：巨剑门
// 特性：内功兼炼体，需先有象甲功根基；exert santou_liubi 现出三头六臂法相（近战伤害/命中暴涨）

#include <ansi.h>
#include <sect.h>
#include <globals.h>
#include <body.h>

inherit FORCE;

string type() { return "martial"; }

int valid_enable(string usage)
{
    return usage == "force";
}

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「梵圣真魔功」需筑基期以上修为方可修习。\n");
    if ((int)me->query_skill(BODY_SKILL_BASIC, 1) < BODY_PREREQ_SKILL)
        return notify_fail("修习「梵圣真魔功」须先打好炼体根基（象甲功 " +
                    BODY_PREREQ_SKILL + " 级以上）。\n");
    return 1;
}

mapping *action = ({
([      "action": "$N运起梵圣真魔功，魔气缠身，一拳轰出带着沉重罡气",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N体内魔神之力涌动，抬手一抓，$n只觉筋骨欲裂",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N低喝一声，梵圣真魔功全力催动，一拳砸得空气爆响",
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
    return notify_fail("「梵圣真魔功」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}

// 内功神通：exert santou_liubi（三头六臂法相）
string exert_function_file(string func)
{
    return __DIR__"fansheng-zhenmo-gong/" + func;
}
