// daomen-shufa.c 道门术法
// 所属宗门：清虚门（九宗档案「清虚门」功法节）
// 特性：清净无为，术法为主
// 出处：九宗档案「清虚门」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 道门术法：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "spells";
}

mapping *action = ({
([      "action": "$N掐动法诀，一道清心道术打出，$n心神一凛，攻势缓了下来",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "精神"
]),
([      "action": "$N施展五雷正法，晴空一声霹雳，雷光劈向$n",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "灼伤"
]),
([      "action": "$N身周紫气氤氲，紫气东来，护住周身",
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
    return notify_fail("「道门术法」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
