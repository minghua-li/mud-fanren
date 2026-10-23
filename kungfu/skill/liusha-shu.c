// liusha-shu.c 流沙术
// 五行法术 · 水 · 初级下阶
// 出处：1F §1.1「水：冰箭术、冰冻术、流沙术」（控制类基础法术）
// 所属宗门传承：掩月宗（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 流沙术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

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
        "id":       "liusha-shu",
        "name":     "流沙术",
        "element":  "水",
        "level":    SPELL_LOW,
        "type":     SPELL_TYPE_CONTROL,
        "cost":     18,
        "power":    45,
        "cooldown": 6,
        "fx":       SPELL_FX_SLOW,
        "desc":     "以水灵之力搅动地气，足下松软如陷流沙。",
    ]);
}

int valid_learn(object me)
{
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N拂袖引来一片沙尘，$n脚下地面忽地松软",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "$N以水灵搅动地气，流沙暗涌，缠住$n步伐",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "$N口中低诵，四野黄沙翻卷，将$n足胫没入沙中",
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
    return notify_fail("「流沙术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
