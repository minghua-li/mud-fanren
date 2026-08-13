// xuanbing-jue.c 玄冰诀
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：水属性辅助功法，修炼容易瓶颈易破
// 出处：九宗档案「黄枫谷」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「玄冰诀」需筑基期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N催动玄冰诀，寒气自掌心涌出，凝成冰锥射向$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "冻伤"
]),
([      "action": "$N双手一合，玄冰寒气弥漫开来，$n只觉四肢僵直难动",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "冻伤"
]),
([      "action": "$N周身水汽凝冰，化作冰甲护住全身",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "冻伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「玄冰诀」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
