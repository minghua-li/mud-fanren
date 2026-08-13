// yichong-shu.c 役虫术
// 所属宗门：灵兽山（九宗档案「灵兽山」功法节）
// 特性：操控灵虫作战
// 出处：九宗档案「灵兽山」功法节

#include <ansi.h>
#include <sect.h>
#include <globals.h>

inherit SKILL;

string type() { return "knowledge"; }

int valid_learn(object me)
{
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_ZHU)
        return notify_fail("「役虫术」需筑基期以上修为方可修习。\n");
    return 1;
}

int valid_enable(string usage)
{
    return 0;
}

int practice_skill(object me)
{
    return notify_fail("「役虫术」为辅助类功法，需通过读书(study)或请教(learn)提升。\n");
}
