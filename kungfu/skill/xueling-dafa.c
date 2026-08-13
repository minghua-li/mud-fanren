// xueling-dafa.c 血灵大法
// 所属宗门：鬼灵门（九宗档案「鬼灵门」功法节）
// 特性：《万灵真经》第一魔功，需天灵根+暗灵根双修
// 出处：九宗档案「鬼灵门」功法节 / 1C §3.3

#include <ansi.h>
#include <sect.h>
#include <globals.h>
#include <spirit_root.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    mapping root = me->query(SPIRIT_ROOT_DATA);

    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「血灵大法」需筑基期以上修为方可修习。\n");
    if (!mapp(root))
        return notify_fail("你尚未检测灵根，无法修习血灵大法。\n");
    // 需天灵根或暗灵根（原著：燕如嫣天灵根 + 王蝉暗灵根双修）
    if (root[SR_QUALITY_IDX] != ROOT_QUALITY_T0 &&
        root[SR_VARIANT] != ROOT_VAR_DARK)
        return notify_fail("血灵大法需天灵根或暗灵根方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N运转血灵大法，周身血光大盛，血灵之气噬向$n",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "内伤"
]),
([      "action": "$N血光护身，$n的攻势触到血光便被侵蚀消融",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "内伤"
]),
([      "action": "$N引动万灵血引，$n只觉气血翻涌，真元不受控制地外泄",
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
    return notify_fail("「血灵大法」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
