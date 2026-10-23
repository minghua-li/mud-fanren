// tufang-shu.c 土墙术
// 五行法术 · 土 · 初级上阶
// 出处：1F §1.1「土：土墙术」（初级上阶复合型法术，防御加成）
// 所属宗门传承：天阙堡（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 土墙术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

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
        "id":       "tufang-shu",
        "name":     "土墙术",
        "element":  "土",
        "level":    SPELL_HIGH,
        "type":     SPELL_TYPE_DEFEND,
        "cost":     40,
        "power":    0,
        "cooldown": 12,
        "fx":       SPELL_FX_GUARD,
        "desc":     "聚土成墙，厚土护身，可挡刀兵法术。",
    ]);
}

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < spell_tier_need(SPELL_HIGH))
        return notify_fail("「土墙术」需炼气后期修为方可修习。\n");
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N拍地立墙，一面厚土之壁自脚下升起，护住身前",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N双手虚抱，土石急速垒高成墙，将自己与$n隔开",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N以土灵结垒，四面土墙环身而立，坚若磐石",
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
    return notify_fail("「土墙术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
