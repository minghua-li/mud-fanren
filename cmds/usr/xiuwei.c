// xiuwei.c
// 修为/境界查询命令（修仙体系）
// 实现 #61 P5-1 修炼系统落地——修为可查询
//
// 用法：xiuwei

#include <ansi.h>
#include <globals.h>
#include <spirit_root.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
	int idx, layer, xiuwei, need, prob, cd;
	string realm, sub;

	seteuid(getuid());

	if (stringp(arg) && arg == "help")
		return help(me);

	idx = ROOT_REFINE_D->query_player_realm_index(me);
	realm = ROOT_REFINE_D->query_player_realm(me);
	sub = ROOT_REFINE_D->query_player_realm_sub(me);
	xiuwei = ROOT_REFINE_D->query_xiuwei(me);

	write(HIC "═══ 修为面板 ═══" NOR "\n");
	write(sprintf(" 境界：%s（%s）\n", realm, sub));
	write(sprintf(" 修为：%d\n", xiuwei));

	// 下一层/子境界需求
	if (idx == 1)
	{
		layer = ROOT_REFINE_D->query_player_realm_layer(me);
		if (layer >= 13)
		{
			write(HIG " 已至炼气13层大圆满，可尝试突破筑基（tupo）！\n" NOR);
		}
		else
		{
			need = ROOT_REFINE_D->query_next_layer_need(me);
			write(sprintf(" 下一层：炼气%d层，需修为 %d（还差 %d）\n",
			              layer + 1, need,
			              (need > xiuwei ? need - xiuwei : 0)));
		}
	}
	else if ((need = ROOT_REFINE_D->query_next_sub_need(me)) > 0)
	{
		write(sprintf(" 下一子境界：需修为 %d（还差 %d）\n", need,
		              (need > xiuwei ? need - xiuwei : 0)));
	}

	// 大境界突破需求与概率
	if ((need = ROOT_REFINE_D->query_major_break_need(me)) > 0)
	{
		prob = ROOT_REFINE_D->query_major_breakthrough_probability(me, idx + 1);
		write(sprintf(" 大境界突破：需修为 %d（当前%s），基础成功率约 %d%%\n",
		              need, (xiuwei >= need ? HIG "已满足" NOR : sprintf("还差 %d", need - xiuwei)), prob));
		cd = ROOT_REFINE_D->query_break_cooldown_remaining(me);
		if (cd > 0)
			write(sprintf(" 突破冷却中：还需 %d 秒\n", cd));
		write(" 输入 tupo 尝试突破。\n");
	}
	else
	{
		write(" 继续打坐修炼（dazuo）积累修为。\n");
	}

	write(sprintf(" 灵根：%s（root 查看详情）\n",
	              ROOT_REFINE_D->query_spirit_root_quality_name(me)));
	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : xiuwei

查看你当前的境界、修为值与突破需求。
境界通过打坐修炼（dazuo）提升，大境界突破使用 tupo 命令。
HELP
	);
	return 1;
}
