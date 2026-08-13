// xuanyue-xiyin-gong.c 玄月吸阴功
// 所属宗门：掩月宗（九宗档案「掩月宗」功法节）
// 特性：阴系双修秘术，与合欢宗同源
// 出处：九宗档案「掩月宗」功法节 / 1C §3.3

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「玄月吸阴功」需筑基期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N施展玄月吸阴功，一股阴寒吸力缠向$n，攫取对方真元",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "内伤"
]),
([      "action": "$N周身阴气森然，噬体阴劲无声无息地渗入$n经脉",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "内伤"
]),
([      "action": "$N引动月华之力，阴柔劲力如丝如缕缠向$n",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "内伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「玄月吸阴功」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
