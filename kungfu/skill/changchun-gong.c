// changchun-gong.c 长春功
// 所属宗门：黄枫谷（九宗档案「黄枫谷」功法节）
// 特性：木系基础功法，洗髓开智，不宜实战，1-13 层
// 出处：九宗档案「黄枫谷」功法节 / 1C §3.3

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "martial"; }

int valid_learn(object me)
{
    // 长春功：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return usage == "force";
}

mapping *action = ({
([      "action": "$N运起长春功，一股温润的木系灵气自丹田涌出，缓缓流转周身经脉，涤荡身心",
        "dodge": -10,
        "parry": -5,
        "force": 90,
        "damage": 25,
        "damage_type": "瘀伤"
]),
([      "action": "你双手结印，木气缠绕而出，$n但觉四肢被无形藤蔓缚住，行动迟滞",
        "dodge": -5,
        "parry": 2,
        "force": 120,
        "damage": 40,
        "damage_type": "瘀伤"
]),
([      "action": "你周身泛起淡青色木灵光晕，长春导引，生生不息，护住周身要害",
        "dodge": 0,
        "parry": 9,
        "force": 150,
        "damage": 55,
        "damage_type": "瘀伤"
])
});

mapping query_action(object me, object weapon)
{
    return action[random(sizeof(action))];
}

int practice_skill(object me)
{
    return notify_fail("「长春功」只能用学(learn)或修炼(xiulian)的来增加熟练度。\n");
}
