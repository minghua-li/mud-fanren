// chanrao-shu.c 缠绕术
// 五行法术 · 木 · 初级中阶
// 出处：1F §1.1「木：缠绕术、回春术」（初级中阶功能型法术）
// 所属宗门传承：黄枫谷（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 缠绕术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

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
        "id":       "chanrao-shu",
        "name":     "缠绕术",
        "element":  "木",
        "level":    SPELL_MID,
        "type":     SPELL_TYPE_CONTROL,
        "cost":     30,
        "power":    55,
        "cooldown": 8,
        "fx":       SPELL_FX_BIND,
        "desc":     "催生木灵之气，化为藤蔓缠住敌手脚。",
    ]);
}

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < spell_tier_need(SPELL_MID))
        return notify_fail("「缠绕术」需炼气中期修为方可修习。\n");
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N掐木灵诀，几缕藤蔓破土缠住$n双足",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N指尖青光一闪，藤蔓如蛇窜起，绕向$n四肢",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N催动木气，满场藤萝疯长，将$n层层困住",
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
    return notify_fail("「缠绕术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
