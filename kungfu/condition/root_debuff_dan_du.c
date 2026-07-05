// root_debuff_dan_du.c
// 丹毒积淤条件——灵根丹药副作用的 debuff 效果
// 联动：root_refine_d.c 处理 debuff 逻辑
//
// 丹毒机制（依据设计文档 02-灵根养成与突破.md 六.1）：
//   丹毒值 = ∑(单颗丹药丹毒累积) - ∑(自然衰减)
//   自然衰减：每天 -5%（游戏时间）
//   丹毒 ≥ 50% → 灵根精纯度 -10%
//   丹毒 ≥ 70% → 灵根强度 -20，突破概率 -15%
//   丹毒 ≥ 90% → 所有灵根相关效果减半
//   丹毒 = 100% → 触发「丹毒反噬」：灵根暂时封闭

#include <ansi.h>
#include <condition.h>

inherit F_CLEAN_UP;

// 丹毒触发阈值
#define DAN_DU_THRESHOLD_WARN      50   // 警告线
#define DAN_DU_THRESHOLD_SEVERE    70   // 严重线
#define DAN_DU_THRESHOLD_CRITICAL  90   // 危险线
#define DAN_DU_THRESHOLD_SEAL      100  // 封闭线

// 丹毒每日自然衰减量（游戏时间）
#define DAN_DU_DECAY_PER_DAY       5

int update_condition(object me, int duration)
{
    if (!me) return 0;

    int dan_du = duration;

    // 自然衰减（每心跳检查一次，衰减按比例降低）
    // 实际衰减由 root_refine_d 的心跳处理，这里只做效果应用
    if (dan_du <= 0)
    {
        // 丹毒已清零
        return 0;
    }

    // 应用丹毒效果
    if (dan_du >= DAN_DU_THRESHOLD_SEAL)
    {
        // 灵根封闭 - 所有效果归零
        ROOT_REFINE_D->apply_root_debuff(me, 4, 604800);  // ROOT_DEBUFF_SEAL
        tell_object(me, HIR "丹毒反噬！你的灵根被毒素彻底封闭，所有灵力无法运转！\n" NOR);
    }
    else if (dan_du >= DAN_DU_THRESHOLD_CRITICAL)
    {
        // 所有灵根相关效果减半
        tell_object(me, HIM "丹毒已深入骨髓，你的灵根效能大幅下降！\n" NOR);
    }
    else if (dan_du >= DAN_DU_THRESHOLD_SEVERE)
    {
        // 灵根强度 -20，突破概率 -15%
        tell_object(me, HIY "丹毒积累严重，灵根已受损伤，突破变得异常困难。\n" NOR);
    }
    else if (dan_du >= DAN_DU_THRESHOLD_WARN)
    {
        // 灵根精纯度 -10%
        tell_object(me, HIW "丹毒已经开始影响你的灵根精纯度。\n" NOR);
    }

    // 条件持续直到丹毒清零
    me->apply_condition("root_debuff_dan_du", duration);
    return CND_CONTINUE;
}

int query_irregular_times()
{
    // 每心跳检查一次（不规则间隔）
    return 1;
}
