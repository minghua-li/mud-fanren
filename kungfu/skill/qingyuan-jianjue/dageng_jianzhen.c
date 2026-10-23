// dageng_jianzhen.c 大庚剑阵（青元剑诀 第10层以上 · 结丹~元婴）
// 出处：1F §2.2「第10-13层 结丹~元婴：大庚剑阵、无形剑气」/ §2.4 大庚剑阵（终极群攻）
// 施展：perform sword.dageng_jianzhen <目标>（战斗中对手；剑阵同时绞杀同场敌人）
// 数值：内力 400，对场上至多 4 名敌人各出 1-2 道剑光（结丹期 2 道）
// 说明：§2.4 要求 36/72 口飞剑 + 庚精强化，飞剑数量与剑阵系统（#29）尚未接入，
//       当前以「青元剑诀第10层 + 结丹修为 + 内力」为门槛，后续法宝体系落地后补飞剑数校验

#include "jianjue.h"

inherit F_SSERVER;

int valid_target_arg(string target)
{
    if (target == "version") return 2;
    return target == "null" || target == "other";
}

int perform(object me, object target)
{
    int layer, i, j, cost, hits, cnt;
    object *enemies, weapon;

    if (!target) target = offensive_target(me);
    if (!jianjue_ready(me)) return 0;
    if (!jianjue_target(me, target)) return 0;

    layer = jianjue_layer(me);
    if (layer < 10)
        return notify_fail("大庚剑阵须青元剑诀第十层以上修为（当前第 " + layer + " 层）。\n");
    if (SECT_D->query_cultivation_tier(me) < SECT_TIER_JIE)
        return notify_fail("大庚剑阵须结丹期以上修为方可布下。\n");

    cost = 400;
    if (me->query("neili") < cost)
        return notify_fail("你内力不足，布不下大庚剑阵。\n");

    weapon = me->query_temp("weapon");
    hits = (SECT_D->query_cultivation_tier(me) >= SECT_TIER_JIE_LATE) ? 2 : 1;

    // 场上同室敌人，最多 4 名
    enemies = ({});
    {
        object *all;

        all = me->query_enemy();
        for (i = 0; i < sizeof(all); i++)
        {
            if (!objectp(all[i])) continue;
            if (environment(all[i]) != environment(me)) continue;
            if (!living(all[i])) continue;
            enemies += ({ all[i] });
            if (sizeof(enemies) >= 4) break;
        }
    }
    if (!sizeof(enemies)) enemies = ({ target });

    me->add("neili", -cost);
    me->start_busy(3);

    message_vision(HIC "\n$N剑气暴涨，数十道青色剑光呼啸而出，以自身为阵眼布成大庚剑阵！\n" NOR, me);

    cnt = 0;
    for (i = 0; i < sizeof(enemies); i++)
    {
        if (!objectp(enemies[i]) || enemies[i]->query("qi") < 0) continue;
        cnt++;
        message_vision(HIW "剑阵一转，数道剑光同时绞向$N。\n" NOR, enemies[i]);
        for (j = 0; j < hits; j++)
        {
            if (!objectp(enemies[i]) || enemies[i]->query("qi") < 0) break;
            jianjue_strike(me, enemies[i], weapon);
        }
    }

    me->delete_temp("attack_val");
    if (cnt > 1)
        tell_object(me, HIG "大庚剑阵绞杀 " + chinese_number(cnt) + " 名敌人。\n" NOR);
    return 1;
}
