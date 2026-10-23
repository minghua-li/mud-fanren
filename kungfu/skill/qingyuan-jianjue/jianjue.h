// jianjue.h
// 青元剑诀神通公共校验与结算辅助（#77）
//   设计依据：.knowledge/combat/1F-法术剑诀阵法战斗.md §2.2「青元剑诀设计」层数解锁表
//            §2.4 大庚剑阵、§2.5 剑影分光术/巨剑术
//   层数口径：残本九层（#62 交付的 kungfu/skill/qingyuan-jianjue.c 注释），每 10 级技能等级一层：
//            layer = (lv - 1) / 10 + 1，故 lv 1-10 = 层1、11-20 = 层2 … 21-30 = 层3（炼气段）、
//            31-40 = 层4 … 51-60 = 层6（筑基段）、61-70 = 层7 … 81-90 = 层9（结丹段），
//            与 valid_learn 的三段境界门槛（lv<=30 炼气 / lv<=60 筑基 / 其余结丹）严格对齐；
//            lv 91+ 即层 10+，供大庚剑阵等大神通
//   施展入口：perform sword.<action>（cmds/skill/perform.c 走
//            SKILL_D("qingyuan-jianjue")->perform_action_file(action)）

#ifndef __QINGYUAN_JIANJUE_H__
#define __QINGYUAN_JIANJUE_H__

#include <ansi.h>
#include <combat.h>
#include <sect.h>
#include <globals.h>

#define QINGYUAN_SKILL "qingyuan-jianjue"

// 当前剑诀层数（0 = 未修习）
int jianjue_layer(object me)
{
    int lv;

    if (!objectp(me)) return 0;
    lv = me->query_skill(QINGYUAN_SKILL, 1);
    if (lv < 1) return 0;
    return (lv - 1) / 10 + 1;
}

// 公共前置：持剑 + 激发青元剑诀为剑法
int jianjue_ready(object me)
{
    object weapon;

    if (!objectp(me)) return 0;
    if (!objectp(weapon = me->query_temp("weapon")) ||
        weapon->query("skill_type") != "sword")
        return notify_fail("你手中无剑，施展不出青元剑诀的神通。\n");
    if (me->query_skill_mapped("sword") != QINGYUAN_SKILL)
        return notify_fail("你须以青元剑诀御剑，方能施展此神通。\n");
    return 1;
}

// 战斗对手校验（与既有 perform 一致的文案口径）
int jianjue_target(object me, object target)
{
    if (!objectp(target) || !target->is_character())
        return notify_fail("这里没有你要攻击的对象。\n");
    if (!me->is_fighting(target))
        return notify_fail("此神通只能对战斗中的对手施展。\n");
    return 1;
}

// attack_val：让伤害结算按「剑法 + 青元剑诀」取值（damage.h 读 attack_val/skill/weapon）
mapping jianjue_attack_val()
{
    return ([
        "base"  : ([ "weapon" : "sword" ]),
        "skill" : ([ "weapon" : QINGYUAN_SKILL ]),
    ]);
}

// 一次剑气攻击（TYPE_SPECIAL，走 COMBAT_D->do_attack → #78 境界压制/灵根口径）
void jianjue_strike(object me, object target, object weapon)
{
    if (!objectp(me) || !objectp(target)) return;

    me->set_temp("attack_val", jianjue_attack_val());
    COMBAT_D->do_attack(me, target, weapon, TYPE_SPECIAL);
    me->delete_temp("attack_val");
}

#endif
