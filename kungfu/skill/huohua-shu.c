// huohua-shu.c 火花术
// 五行法术 · 火 · 初级中阶
// 出处：1F §1.1「火：火弹术、火花术」（初级中阶功能型法术）
// 所属宗门传承：黄枫谷（sect_d.c sect_config「skills」节 + 传功长老 set_skill）
// 施法：shifa 火花术 [目标]（SPELL_D 结算法力消耗/五行克制/境界与灵根加成，见 #76）

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
        "id":       "huohua-shu",
        "name":     "火花术",
        "element":  "火",
        "level":    SPELL_MID,
        "type":     SPELL_TYPE_ATTACK,
        "cost":     28,
        "power":    95,
        "cooldown": 6,
        "fx":       SPELL_FX_BURN,
        "desc":     "弹指洒出漫天火星，触肤即燃，久而不熄。",
    ]);
}

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < spell_tier_need(SPELL_MID))
        return notify_fail("「火花术」需炼气中期修为方可修习。\n");
    return 1;
}

int valid_enable(string usage) { return usage == "spells"; }

// 以法术槽武功身份参与普通战斗时的招式描写（法术主要走 shifa 施法流程）
mapping *action = ({
([      "action": "$N弹指一挥，数十点火星溅射而出，落在$n身上嗤嗤作响",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "灼伤"
]),
([      "action": "$N袖袍一抖，一片火雨当空洒下，$n避无可避",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "灼伤"
]),
([      "action": "$N双手一分，火势大盛，无数火花绕着$n盘旋燃烧",
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
    return notify_fail("「火花术」只能向师长学(learn)或施法(shifa)中慢慢精进。\n");
}
