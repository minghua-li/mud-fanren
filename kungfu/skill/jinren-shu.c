// jinren-shu.c 金刃术
// 五行法术 · 金 · 初级下阶
// 出处：1F §1.1「金：金刃术」（初级法术，破甲/穿透）
// 所属宗门传承：化刀坞（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 金刃术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

#include <ansi.h>
#include <sect.h>
#include <globals.h>
#include <spell.h>

inherit SKILL;

string type() { return "martial"; }

// 法术元数据（单真源：SPELL_D 只读这里，不在 daemon 复制数值）
mapping query_spell_info()
{
    return ([
        "id":       "jinren-shu",
        "name":     "金刃术",
        "element":  "金",
        "level":    SPELL_LOW,
        "type":     SPELL_TYPE_ATTACK,
        "cost":     22,
        "power":    85,
        "cooldown": 5,
        "fx":       SPELL_FX_PIERCE,
        "desc":     "凝金气为薄刃，锋锐无匹，专破护体真气。",
    ]);
}

int valid_learn(object me)
{
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N并指成刃，一道金色薄光切向$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "割伤"
]),
([      "action": "$N真气化金，三道金刃错落斩落，锋锐难挡",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "割伤"
]),
([      "action": "$N金气缭绕指间，骤然暴射成一片刃网罩向$n",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "割伤"
]),
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「金刃术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
