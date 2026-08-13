// zhenyang-jue.c 真阳诀
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：火系顶阶功法，三阳之体如鱼得水
// 出处：九宗档案「黄枫谷」功法节 / 1C §3.3

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「真阳诀」需筑基期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N运起真阳诀，一股炽热真气轰然爆发，火浪滚滚扑向$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "灼伤"
]),
([      "action": "$N双掌连挥，三阳火弹连环射出，炸裂在$n周身",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "灼伤"
]),
([      "action": "$N周身燃起赤红阳炎，护体真火灼得空气扭曲",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "灼伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「真阳诀」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
