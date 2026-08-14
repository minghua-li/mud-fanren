// guidao-gongfa.c 鬼道功法
// 所属宗门：鬼灵门（九宗档案「鬼灵门」功法节）
// 特性：驱鬼役妖，操控鬼物
// 出处：九宗档案「鬼灵门」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 鬼道功法：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "spells";
}

mapping *action = ({
([      "action": "$N口中念念有词，召唤鬼物扑向$n，鬼气森森",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "精神"
]),
([      "action": "$N役使鬼气缠向$n，噬魂阴劲直侵心神",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "精神"
]),
([      "action": "$N周身鬼气大盛，鬼影幢幢，$n心神为之所夺",
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
    return notify_fail("「鬼道功法」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
