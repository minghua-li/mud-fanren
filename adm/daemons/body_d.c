// adm/daemons/body_d.c
// 体修守护进程（BODY_D）—— 淬体 / 肉身搏击 / 体剑双修
// 设计文档：.knowledge/combat/COMBAT_SYSTEM.md §三 体修体系（四段路径）、
//          1F §4.7 技能树（体修功法）、1D 九宗档案「巨剑门」体剑双修
// 依赖：SECT_D->query_cultivation_tier（各体修功法的修习门槛在 skill 文件内把关）
// 数据：全部落在玩家 dbase 的 body_refine/*（见 include/body.h 注释），战斗热路径
//       （include/combat/damage.h、feature/attribute.c）直接读 dbase，不调 daemon
// Created for ticket #79

#include <ansi.h>
#include <body.h>
#include <globals.h>
#include <sect.h>

inherit F_DBASE;

// 阶段显示名（下标 = BODY_STAGE_*）
nosave string *body_stage_names = ({
    "未淬体", "基础炼体", "法体双修", "极限炼体", "毒体修炼",
});

// 阶段 → 淬体点数上限
int query_stage_cap(int stage)
{
    switch (stage)
    {
    case BODY_STAGE_BASIC:    return BODY_CAP_BASIC;
    case BODY_STAGE_DUAL:     return BODY_CAP_DUAL;
    case BODY_STAGE_ULTIMATE: return BODY_CAP_ULTIMATE;
    case BODY_STAGE_POISON:   return BODY_CAP_POISON;
    }
    return 0;
}

// 阶段名
string query_stage_name(int stage)
{
    if (stage < 0 || stage >= sizeof(body_stage_names)) return "未淬体";
    return body_stage_names[stage];
}

// 当前体修阶段（按四段功法掌握情况推导，见 COMBAT_SYSTEM §三 阶梯）
int query_stage(object me)
{
    int lv_basic, lv_dual, lv_ultimate, lv_poison;

    if (!objectp(me)) return BODY_STAGE_NONE;

    lv_basic = me->query_skill(BODY_SKILL_BASIC, 1);
    lv_dual = me->query_skill(BODY_SKILL_DUAL, 1);
    lv_ultimate = me->query_skill(BODY_SKILL_ULTIMATE, 1);
    lv_poison = me->query_skill(BODY_SKILL_POISON, 1);

    if (lv_basic < 1) return BODY_STAGE_NONE;

    // 逐段进阶：每段要求上一段功法达到 BODY_PREREQ_SKILL
    if (lv_dual >= 1 && lv_basic >= BODY_PREREQ_SKILL)
    {
        if (lv_ultimate >= 1 && lv_dual >= BODY_PREREQ_SKILL)
        {
            if (lv_poison >= 1 && lv_ultimate >= BODY_PREREQ_SKILL)
                return BODY_STAGE_POISON;
            return BODY_STAGE_ULTIMATE;
        }
        return BODY_STAGE_DUAL;
    }

    return BODY_STAGE_BASIC;
}

// 最高体修功法等级（决定单次淬体收益）
int query_max_body_skill(object me)
{
    int lv;

    if (!objectp(me)) return 0;
    lv = me->query_skill(BODY_SKILL_BASIC, 1);
    if (me->query_skill(BODY_SKILL_DUAL, 1) > lv) lv = me->query_skill(BODY_SKILL_DUAL, 1);
    if (me->query_skill(BODY_SKILL_ULTIMATE, 1) > lv) lv = me->query_skill(BODY_SKILL_ULTIMATE, 1);
    if (me->query_skill(BODY_SKILL_POISON, 1) > lv) lv = me->query_skill(BODY_SKILL_POISON, 1);
    return lv;
}

// 已积累淬体点数
int query_points(object me)
{
    int p;

    if (!objectp(me)) return 0;
    p = me->query("body_refine/points");
    if (!intp(p) || p < 0) return 0;
    return p;
}

// 重算派生值（点数 → max_qi/减伤/空手伤害），并夹在当前阶段上限内
void refresh(object me)
{
    int stage, cap, points;

    if (!objectp(me)) return;

    stage = query_stage(me);
    cap = query_stage_cap(stage);
    points = query_points(me);
    if (points > cap) points = cap;
    if (points < 0) points = 0;

    me->set("body_refine/points", points);
    me->set("body_refine/stage", stage);
    me->set("body_refine/qi", points * BODY_QI_PER_POINT);
    me->set("body_refine/defense", points / BODY_DEF_DIVISOR);
    me->set("body_refine/fist", points / BODY_FIST_DIVISOR);
}

// 体修面板数据（供 cuina 命令展示）
mapping query_body_info(object me)
{
    int stage, cap, points;

    if (!objectp(me)) return 0;

    stage = query_stage(me);
    cap = query_stage_cap(stage);
    points = query_points(me);

    return ([
        "stage"      : stage,
        "stage_name" : query_stage_name(stage),
        "points"     : points,
        "cap"        : cap,
        "qi"         : points * BODY_QI_PER_POINT,
        "defense"    : points / BODY_DEF_DIVISOR,
        "fist"       : points / BODY_FIST_DIVISOR,
        "sword"      : points / BODY_SWORD_DIVISOR,
        "max_skill"  : query_max_body_skill(me),
    ]);
}

// 淬体：消耗内力与精神，积累淬体点数（阶段上限内）
// 返回本次获得的点数（0 = 未成功，失败原因已 tell_object）
int train_body(object me)
{
    int stage, cap, points, gain, lv;

    if (!objectp(me)) return 0;

    stage = query_stage(me);
    if (stage < BODY_STAGE_BASIC)
    {
        tell_object(me, "你尚未修习任何炼体功法（可先学「象甲功」），无从淬体。\n");
        return 0;
    }

    if (me->is_busy() || me->is_fighting())
    {
        tell_object(me, "此刻心神不定，无法静心淬体。\n");
        return 0;
    }

    if (me->query("neili") < BODY_COST_NEILI)
    {
        tell_object(me, "你内力不足，淬体至少需 " + BODY_COST_NEILI + " 点内力。\n");
        return 0;
    }
    if (me->query("jing") < BODY_COST_JING)
    {
        tell_object(me, "你精神不济，淬体至少需 " + BODY_COST_JING + " 点精神。\n");
        return 0;
    }

    cap = query_stage_cap(stage);
    points = query_points(me);
    if (points >= cap)
    {
        tell_object(me, "你的肉身已淬炼至「" + query_stage_name(stage) +
                    "」的极限（" + cap + " 点），须修习更高阶炼体功法方能精进。\n");
        return 0;
    }

    lv = query_max_body_skill(me);
    gain = BODY_GAIN_BASE + lv / BODY_GAIN_PER_LEVEL;
    if (gain > BODY_GAIN_MAX) gain = BODY_GAIN_MAX;
    if (points + gain > cap) gain = cap - points;

    me->add("neili", -BODY_COST_NEILI);
    me->add("jing", -BODY_COST_JING);
    me->set("body_refine/points", points + gain);
    refresh(me);
    me->start_busy(BODY_TRAIN_BUSY);

    tell_object(me, HIG "你运转炼体功法，周身上下如受千锤百炼，肉身又坚实了一分。\n" NOR);
    return gain;
}
