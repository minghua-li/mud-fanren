// huodan-shu.c 火弹术
// 五行法术 · 火 · 初级下阶
// 出处：1F §1.1「火：火弹术、火花术」/ §5.1 火弹术 LPC 范例
// 所属宗门传承：黄枫谷（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 火弹术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

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
        "id":       "huodan-shu",
        "name":     "火弹术",
        "element":  "火",
        "level":    SPELL_LOW,
        "type":     SPELL_TYPE_ATTACK,
        "cost":     20,
        "power":    80,
        "cooldown": 5,
        "fx":       SPELL_FX_BURN,
        "desc":     "凝火成弹，弹射而出，触物即燃。",
    ]);
}

int valid_learn(object me)
{
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N掐诀引火，掌心凝出一枚赤红火弹，向$n弹射而去",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "灼伤"
]),
([      "action": "$N口中念念有词，又是一枚火弹拖着火星，直取$n",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "灼伤"
]),
([      "action": "$N双手连弹，火弹迸散成流火，兜头盖脸罩向$n",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "灼伤"
]),
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「火弹术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
