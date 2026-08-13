// dan.c
// 凡人修仙体系丹药基类（#73：#64 子单，丹药实体接入物品经济）
//
// 继承 ITEM（F_DBASE/F_MOVE/F_NAME），丹药作为普通物品获得/持有/交易/消耗。
// 通用服用逻辑按 pill_type 分发（修为丹/突破丹/疗伤丹），品质系数作用于效果。
//
// 服用入口：eat <id>（丹药基类自带 do_eat，玩家可用 eat 服用）
//
// 子类需设置（1E §4.1）：
//   pill_type / stage / effect / quality / side_effect / refine_level
// 见 include/pill.h 类型与品级常量。

#include <ansi.h>
#include <globals.h>
#include <spirit_root.h>
#include <pill.h>

inherit ITEM;

void init()
{
        add_action("do_eat", "eat");
}

// 品质效果系数（1E §2.3 / 02 §1.2：凡品×1.0 良品×1.5 上品×2.0）
int quality_factor()
{
        int q = query("quality");
        if (q == PILL_QUALITY_LIANG) return 150;
        if (q == PILL_QUALITY_SHANG) return 200;
        return 100;
}

// 服用丹药
int do_eat(string arg)
{
        object me;
        string ptype;

        if (!id(arg))
                return 0;

        me = this_player();
        if (me->is_busy() || me->is_fighting())
                return notify_fail("你正忙着呢，无法服用丹药。\n");

        ptype = query("pill_type");
        if (ptype == PILL_TYPE_BREAKTHROUGH)
                return do_eat_breakthrough(me);
        if (ptype == PILL_TYPE_XIUWEI)
                return do_eat_xiuwei(me);
        if (ptype == PILL_TYPE_HEAL)
                return do_eat_heal(me);

        return notify_fail("这颗丹药似乎没有明确的功效。\n");
}

// 突破丹：服用后写 breakthrough/pill_bonus（叠加，上限 PILL_BREAK_MAX_STACK 颗），
// 实际影响 ROOT_REFINE_D->query_major_breakthrough_probability（#61 突破概率）。
// 境界校验：筑基丹(stage=2)须炼气期服用、结金丹(stage=3)须筑基期服用……
int do_eat_breakthrough(object me)
{
        int target, cur, bonus, max_bonus, cur_bonus;

        target = query("stage");
        cur = ROOT_REFINE_D->query_player_realm_index(me);
        if (target > 1 && cur != target - 1)
                return notify_fail(sprintf("此丹药用于冲击%s瓶颈，你当前境界不适用。\n",
                        ROOT_REFINE_D->realm_name(target, 0)));
        if (cur <= 0)
                return notify_fail("你尚未踏入修仙之路，服用此丹药毫无效果。\n");

        bonus = query("effect");
        max_bonus = bonus * PILL_BREAK_MAX_STACK;
        cur_bonus = me->query_temp("breakthrough/pill_bonus");
        if (cur_bonus + bonus > max_bonus)
                return notify_fail(sprintf("同类突破丹药已达服用上限（最多 %d 颗，加成 %d%%）。\n",
                        PILL_BREAK_MAX_STACK, max_bonus));

        me->add_temp("breakthrough/pill_bonus", bonus);
        message_vision(HIG "$N服下一颗" + name() + HIG "，只觉一股精纯药力沉淀丹田，等待突破之时助其一臂之力。\n" NOR, me);
        write(sprintf("当前突破丹药加成：+%d%%（服用 tupo 突破时生效）。\n",
                me->query_temp("breakthrough/pill_bonus")));

        destruct(this_object());
        return 1;
}

// 修为丹：增加修为 + 累积丹毒（副作用轻量版，不做惩罚层——见 KB）
int do_eat_xiuwei(object me)
{
        int amount, toxin;

        amount = to_int(query("effect") * quality_factor() / 100);
        ROOT_REFINE_D->add_xiuwei(me, amount);

        // 副作用：丹毒累积（02 §5 简化——只累积与提示，不施加修炼惩罚）
        toxin = me->query(PILL_TOXIN);
        toxin += 5;
        me->set(PILL_TOXIN, toxin);
        if (toxin > PILL_TOXIN_WARN)
                tell_object(me, HIY "你感到丹毒在体内渐渐淤积，长期大量服药恐有隐患。\n" NOR);

        message_vision("$N服下一颗" + name() + "，修为精进。\n", me);
        write(sprintf("修为 +%d（当前 %d）。\n", amount, ROOT_REFINE_D->query_xiuwei(me)));

        // 修为增加后可能触发炼气层数自动提升
        if (ROOT_REFINE_D->query_player_realm_index(me) == REALM_QI_REFINERY)
                ROOT_REFINE_D->check_qi_layer_up(me);

        destruct(this_object());
        return 1;
}

// 疗伤丹：回复气血
int do_eat_heal(object me)
{
        int amount;

        amount = to_int(query("effect") * quality_factor() / 100);
        me->receive_curing("qi", amount);
        me->receive_curing("jing", amount / 2);
        message_vision("$N服下一颗" + name() + "，气色好了许多。\n", me);
        write(sprintf("气血回复 %d 点。\n", amount));

        destruct(this_object());
        return 1;
}
