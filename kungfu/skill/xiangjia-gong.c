// xiangjia-gong.c 象甲功
// 体修·基础炼体（COMBAT_SYSTEM §三 体修体系「基础炼体：象甲功，力量/防御增强」）
// 所属宗门：巨剑门（体剑双修根基）
// 特性：打熬筋骨、皮糙肉厚；淬体（cuina）以此功为门槛，等级越高单次淬体收益越大

#include <ansi.h>
#include <sect.h>
#include <globals.h>
#include <body.h>

inherit SKILL;

string type() { return "martial"; }

// 炼体功法走 iron-cloth 槽（与既有护体类武学一致）
int valid_enable(string usage)
{
    return usage == "iron-cloth";
}

int valid_learn(object me)
{
    // 基础炼体：炼气期即可入手（BODY_STAGE_BASIC 门槛）
    return 1;
}

mapping *action = ({
([      "action": "$N运起象甲功，周身皮肉鼓胀如铁，一记横撞直取$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N双拳护胸，象甲之力灌注双臂，沉肩撞向$n胸口",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N一声低喝，筋骨噼啪作响，抬手一记重击砸向$n",
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

// 炼体可自行打熬筋骨（practice）：耗气血与内力
int practice_skill(object me)
{
    if ((int)me->query("qi") < 40)
        return notify_fail("你的气血不足，难以再打熬筋骨。\n");
    if ((int)me->query("neili") < 20)
        return notify_fail("你的内力不足，运不起象甲功。\n");

    me->receive_damage("qi", 30);
    me->add("neili", -20);
    return 1;
}
