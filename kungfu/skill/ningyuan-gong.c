// ningyuan-gong.c 凝元功
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：辅助功法，聚灵术
// 出处：九宗档案「黄枫谷」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_JIE)
        return notify_fail("「凝元功」需结丹期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N运转凝元功，聚灵术施展开来，天地灵气疯狂涌入$P体内",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "内伤"
]),
([      "action": "$N元气凝练，化作一道灵光击向$n，虽不凌厉却绵绵不绝",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "内伤"
]),
([      "action": "$N凝元护体，灵力引动，将$n的攻势生生架住",
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
    return notify_fail("「凝元功」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
