// tupo.c
// 境界突破命令（修仙体系：大境界突破，接 root_refine_d 概率/失败惩罚/任务链）
// 实现 #61 P5-1 修炼系统落地——突破玩家流程
// 设计依据：02-扩充内容/02-灵根养成与突破.md §5（突破事件玩法化）
//           .knowledge/cultivation/realm-breakthrough-failure-penalty.md（失败惩罚约定）
//
// 用法：
//   tupo            —— 自然突破（当前大境界圆满且修为足够时）
//   tupo lingshi    —— 灵石灌注辅助突破（消耗灵石，成功率提升）
//   tupo help       —— 帮助

#include <ansi.h>
#include <globals.h>
#include <spirit_root.h>

inherit F_CLEAN_UP;

#define TUPO_STONE_COST  5000    // 灵石灌注辅助突破消耗
#define TUPO_STONE_BONUS 10      // 灵石灌注成功率加成（%）

int main(object me, string arg)
{
	int idx, need, method, result;
	string chain;

	seteuid(getuid());

	if (me->is_busy())
		return notify_fail("你现在正忙着呢，无法静心突破。\n");

	if (me->is_fighting())
		return notify_fail("战斗中无法突破。\n");

	if (stringp(arg) && arg == "help")
		return help(me);

	idx = ROOT_REFINE_D->query_player_realm_index(me);
	if (idx <= 0)
		return notify_fail("你尚未踏入修仙之路，无法突破境界。\n");
	if (idx >= 8)
		return notify_fail("你已修炼至当前界面的最高境界，再无可突破之境。\n");

	// 大境界突破门槛检查
	need = ROOT_REFINE_D->query_major_break_need(me);
	if (need <= 0)
	{
		int next = ROOT_REFINE_D->query_next_layer_need(me);
		if (next > 0)
			return notify_fail(sprintf("你尚未修炼至炼气大圆满（还需修为 %d）。请继续打坐修炼（dazuo）。\n", next));
		return notify_fail("你尚未修炼至当前境界的圆满，无法尝试大境界突破。\n");
	}

	// 突破方式
	method = BREAK_METHOD_NATURAL;
	if (stringp(arg) && arg == "lingshi")
	{
		method = BREAK_METHOD_SPIRIT_STONE;
		if (!MONEY_D->player_pay(me, TUPO_STONE_COST))
			return notify_fail(sprintf("灵石不足：灵石灌注辅助突破需 %d 枚灵石。\n", TUPO_STONE_COST));
		me->set_temp("breakthrough/aux_bonus", TUPO_STONE_BONUS);
		write(sprintf(HIY "你耗费 %d 枚灵石布下聚灵阵，灵气浓度大增！\n" NOR, TUPO_STONE_COST));
	}

	// 展示突破任务链提示
	chain = ROOT_REFINE_D->query_current_break_task_chain(me);
	if (chain != "")
		write(chain);

	// 执行突破（1=成功 2=失败 0=条件不满足）
	result = ROOT_REFINE_D->do_major_breakthrough(me, method);
	me->delete_temp("breakthrough/aux_bonus");

	if (result == 1)
	{
		write("输入 xiuwei 查看新境界，root 查看灵根变化。\n");
		return 1;
	}
	if (result == 2)
	{
		write("突破失败，养精蓄锐后再试（dazuo / xiuwei 查看详情）。\n");
		return 1;
	}
	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : tupo | tupo lingshi

境界突破。需要当前境界修炼圆满（修为满足门槛）后方可尝试。
突破走概率判定：灵根品质、灵根强度、连续失败保底都会影响成功率。

tupo            自然突破（无额外消耗）
tupo lingshi    灵石灌注辅助（消耗 5000 灵石，成功率 +10%）

突破失败会有修为回退与冷却惩罚；天灵根者结丹及以下自动成功。
输入 xiuwei 查看突破所需与当前成功率。
HELP
	);
	return 1;
}
