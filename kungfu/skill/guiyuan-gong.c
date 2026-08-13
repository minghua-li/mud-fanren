// guiyuan-gong.c 归元功
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：防御功法，归元灵甲
// 出处：九宗档案「黄枫谷」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「归元功」需筑基期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N运转归元功，元气收敛归元，在体表凝成一层归元灵甲",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "内伤"
]),
([      "action": "$N气沉丹田，归元之力反震而出，$n攻势尽数被化解",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "内伤"
]),
([      "action": "$N周身灵甲闪耀，硬抗$n一击，稳如磐石",
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
    return notify_fail("「归元功」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
