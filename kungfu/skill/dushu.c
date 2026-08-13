// dushu.c 毒术
// 所属宗门：鬼灵门（九宗档案「鬼灵门」功法节）
// 特性：用毒之术
// 出处：九宗档案「鬼灵门」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "knowledge"; }

int valid_learn(object me)
{
    // 毒术：炼气期即可修习（九宗档案无更高境界限制）
    return 1;
}

int valid_enable(string usage)
{
    return 0;
}

int practice_skill(object me)
{
    return notify_fail("「毒术」为辅助类功法，需通过读书(study)或请教(learn)提升。\n");
}
