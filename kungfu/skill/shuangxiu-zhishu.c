// shuangxiu-zhishu.c 双修之术
// 所属宗门：掩月宗（九宗档案「掩月宗」功法节）
// 特性：合修功法，阴阳相济，精进法力
// 出处：九宗档案「掩月宗」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit FORCE;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 双修之术：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N运起双修之术，阴阳二气相济相生，法力绵绵不绝地涌出",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "内伤"
]),
([      "action": "$N双掌一阴一阳交替拍出，阴阳合击，威力倍增",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "内伤"
]),
([      "action": "$N周身阴阳流转，气息圆融，$n的攻势如泥牛入海",
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
    return notify_fail("「双修之术」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}

string exert_function_file(string func)
{
    return __DIR__"shuangxiu-zhishu/" + func;
}
