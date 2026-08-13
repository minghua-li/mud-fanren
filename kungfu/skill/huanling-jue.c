// huanling-jue.c 幻灵决
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：幻术功法，幻影分身
// 出处：九宗档案「黄枫谷」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_JIE)
        return notify_fail("「幻灵决」需结丹期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "spells";
}

mapping *action = ({
([      "action": "$N掐诀施法，幻影分身从$P身侧浮现，真假难辨地攻向$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "精神"
]),
([      "action": "$N布下迷魂幻阵，$n眼前景象骤变，心神恍惚，攻势散乱",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "精神"
]),
([      "action": "$N幻象丛生，$n只觉四面皆是$N的身影，不知该防何处",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "精神"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「幻灵决」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
