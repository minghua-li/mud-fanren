// dici-shu.c 地刺术
// 五行法术 · 土 · 初级中阶
// 出处：1F §1.1「土：土墙术、地刺术」（初级中阶功能型法术）
// 所属宗门传承：天阙堡（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 地刺术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

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
        "id":       "dici-shu",
        "name":     "地刺术",
        "element":  "土",
        "level":    SPELL_MID,
        "type":     SPELL_TYPE_ATTACK,
        "cost":     32,
        "power":    120,
        "cooldown": 7,
        "fx":       SPELL_FX_NONE,
        "desc":     "引地脉之力，石刺破土，自下而上穿敌。",
    ]);
}

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < spell_tier_need(SPELL_MID))
        return notify_fail("「地刺术」需炼气中期修为方可修习。\n");
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N足下一顿，一道石刺轰然破土，直挑$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N以掌按地，地脉微鸣，数道石刺次第穿刺而出",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N借土气结阵，方圆数丈石刺林立，尽数刺向$n",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "瘀伤"
]),
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「地刺术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
