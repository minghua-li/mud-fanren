// dazuo.c
// 打坐修炼命令（修仙体系：时间/灵石换修为）
// 实现 #61 P5-1 修炼系统落地——修炼循环命令
// 设计依据：02-扩充内容/02-灵根养成与突破.md §4.1（修炼速度公式）
//           economy_bridge_d A1 循环（灵石灌注修炼）
//
// 用法：
//   dazuo            —— 开始打坐修炼（默认 20 心跳）
//   dazuo <心跳数>   —— 指定打坐心跳数（1~200）
//   dazuo lingshi <N>—— 灵石灌注修炼（一次性兑换 N 枚灵石为修为）
//   dazuo help       —— 帮助

#include <ansi.h>
#include <skill.h>
#include <command.h>
#include <globals.h>
#include <spirit_root.h>

inherit F_CLEAN_UP;

#define DAZUO_MAX_HEARTBEAT  200
#define DAZUO_DEFAULT_TICKS  20

int do_dazuo(object me);
int halt_dazuo(object me);
int do_stone_cultivation(object me, int amount);

int main(object me, string arg)
{
	int ticks, amount;

	seteuid(getuid());

	if (me->is_busy() || me->query_temp("pending/dazuo"))
		return notify_fail("你现在正忙着呢。\n");

	if (me->is_fighting())
		return notify_fail("战斗中不能打坐修炼，会走火入魔。\n");

	if (me->query("qi") < me->query("max_qi") / 5)
		return notify_fail("你现在的气太少了，无法静心打坐。\n");

	if (stringp(arg) && arg == "help")
		return help(me);

	// 灵石灌注修炼：dazuo lingshi <N>
	if (stringp(arg) && sscanf(arg, "lingshi %d", amount) == 1)
	{
		if (amount <= 0)
			return notify_fail("要灌注多少灵石？格式：dazuo lingshi <灵石数>\n");
		return do_stone_cultivation(me, amount);
	}

	if (stringp(arg) && arg != "" && sscanf(arg, "%d", ticks) != 1)
		return notify_fail("参数错误！格式：dazuo [心跳数] 或 dazuo lingshi <灵石数>。\n");

	if (!ticks || ticks <= 0)
		ticks = DAZUO_DEFAULT_TICKS;
	if (ticks > DAZUO_MAX_HEARTBEAT)
		ticks = DAZUO_MAX_HEARTBEAT;

	// 检查灵根：无灵根无法引气入体
	if (!mapp(me->query(SPIRIT_ROOT_DATA)))
		return notify_fail("你尚未检测灵根，无法引气入体。请先前往修仙门派的「测灵殿」检测灵根（root 查看）。\n");

	me->set_temp("pending/dazuo", 1);
	me->set_temp("dazuo_ticks", ticks);

	message_vision(HIW + "$N盘膝坐下，双目微闭，开始吐纳天地灵气……\n" + NOR, me);
	me->start_busy("do_dazuo", "halt_dazuo");
	return 1;
}

// 灵石灌注修炼（economy_bridge_d A1 循环）
int do_stone_cultivation(object me, int amount)
{
	int gain;

	if (!objectp(me)) return 0;

	gain = ECONOMY_BRIDGE_D->perform_spirit_stone_cultivation(me, amount);
	if (gain <= 0)
		return notify_fail("灵石不足或数量非法，无法灌注修炼。\n");

	ROOT_REFINE_D->add_xiuwei(me, gain);
	// 灵根经验同步成长
	ROOT_REFINE_D->gain_exp_from_cultivation(me, gain / 2);
	// 炼气期层数自动提升（只做升层检查，不重复加修为）
	ROOT_REFINE_D->check_qi_layer_up(me);

	write(sprintf(HIG "你以 %d 枚灵石引动灵气灌体，修为 +%d！\n" NOR, amount, gain));
	write(sprintf("当前修为：%d，境界：%s（输入 xiuwei 查看详情）\n",
	              ROOT_REFINE_D->query_xiuwei(me), ROOT_REFINE_D->query_player_realm(me)));
	return 1;
}

// 打坐心跳：每心跳获得修为 = 境界基准 × 灵根速度系数
int do_dazuo(object me)
{
	int gain, ticks;

	if (!objectp(me)) return 0;
	if (!living(me))
	{
		halt_dazuo(me);
		return 0;
	}

	if (me->is_fighting())
	{
		halt_dazuo(me);
		return 0;
	}

	ticks = me->query_temp("dazuo_ticks");
	if (ticks < 1)
	{
		me->delete_temp("pending/dazuo");
		return 0;
	}
	me->add_temp("dazuo_ticks", -1);

	// 修为增长（含炼气层数自动提升）
	gain = ROOT_REFINE_D->do_heartbeat_cultivation(me);
	if (gain <= 0)
	{
		halt_dazuo(me);
		return 0;
	}

	if (random(3) == 0)
		tell_object(me, sprintf("你缓缓吐纳，一缕灵气汇入丹田，修为 +%d。\n", gain));

	if (me->query_temp("dazuo_ticks") > 0)
		return 1;

	me->delete_temp("pending/dazuo");
	tell_object(me, HIG "你运功完毕，缓缓睁开双眼，只觉体内灵力又浑厚了几分。\n" NOR);
	return 0;
}

// 中断打坐
int halt_dazuo(object me)
{
	if (!objectp(me)) return 1;
	me->delete_temp("pending/dazuo");
	me->delete_temp("dazuo_ticks");
	tell_object(me, HIY "你强行收功，缓缓站起身来。\n" NOR);
	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : dazuo [心跳数] | dazuo lingshi <灵石数>

打坐修炼，引天地灵气入体化为修为。修为是境界突破的基础，
可通过打坐（时间）或灵石灌注（dazuo lingshi <数量>）两种途径获得。

dazuo           开始打坐（默认 20 心跳）
dazuo 100       打坐 100 心跳
dazuo lingshi 500  以 500 枚灵石灌注修炼

修为越高，突破境界（tupo）的成功把握越大。输入 xiuwei 查看当前修为与境界。
新玩家初始境界为炼气1层，修炼至炼气13层大圆满后即可尝试突破筑基。
HELP
	);
	return 1;
}
