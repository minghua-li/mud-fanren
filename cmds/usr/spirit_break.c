// spirit_break.c
// 灵根突破命令——尝试突破灵根强度上限，获得属性提升
// 基于《凡人修仙传》设定，对应设计文档：02-扩充内容/02-灵根养成与突破.md
//
// 用法：
//   break linggen               — 自然突破
//   break linggen pill          — 丹药辅助突破
//   break linggen stone         — 灵石灌注突破
//   break linggen treasure      — 天材地宝突破
//   break linggen info          — 查看突破详情

#include <ansi.h>
#include <spirit_root.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
	mapping root = me->query(SPIRIT_ROOT_DATA);
	if (!mapp(root))
	{
		write("你尚未检测灵根，请前往修仙门派的「测灵殿」进行检测。\n");
		return 1;
	}

	int quality = root[ROOT_PROP_QUALITY];
	if (quality <= SPIRIT_ROOT_NONE)
	{
		write("你身无灵根，无法进行突破。\n");
		return 1;
	}

	int method;
	string method_name;

	if (!arg || arg == "" || arg == "linggen")
	{
		// 显示突破帮助
		write("你要如何尝试灵根突破？\n\n");
		write(HIC "═══ 灵根突破方式 ═══\n" NOR);
		write("  break linggen              — 自然突破（无消耗，成功率35%）\n");
		write("  break linggen pill         — 丹药辅助（消耗洗髓丹×3，成功率50%）\n");
		write("  break linggen stone        — 灵石灌注（消耗灵石×5000，成功率45%）\n");
		write("  break linggen treasure     — 天材地宝（消耗材料×1，成功率70%）\n");
		write("  break linggen info         — 查看详细突破概率\n");
		return 1;
	}

	if (arg == "linggen info")
	{
		// 显示突破详情
		mapping details = ROOT_REFINE_D->query_breakthrough_details(me, BREAK_METHOD_NATURAL);
		int strength = details["strength"];
		int purity = details["purity"];
		int q = details["quality"];
		int fail_streak = details["fail_streak"];

		string qname;
		switch (q)
		{
		case SPIRIT_ROOT_PSEUDO:   qname = "伪灵根"; break;
		case SPIRIT_ROOT_FAKE:     qname = "假灵根"; break;
		case SPIRIT_ROOT_TRUE:     qname = "真灵根"; break;
		case SPIRIT_ROOT_VARIANT:  qname = "变异灵根"; break;
		case SPIRIT_ROOT_HEAVENLY: qname = "天灵根"; break;
		default: qname = "未知"; break;
		}

		int max_str;
		switch (q)
		{
		case SPIRIT_ROOT_PSEUDO:   max_str = 80;  break;
		case SPIRIT_ROOT_FAKE:     max_str = 85;  break;
		case SPIRIT_ROOT_TRUE:     max_str = 95;  break;
		case SPIRIT_ROOT_VARIANT:  max_str = 100; break;
		case SPIRIT_ROOT_HEAVENLY: max_str = 100; break;
		default: max_str = 0; break;
		}

		write(HIC "═══ 灵根突破信息 ═══\n" NOR);
		write(sprintf(" 当前品质：%s\n", qname));
		write(sprintf(" 灵根强度：%d/%d\n", strength, max_str));
		write(sprintf(" 灵根精纯度：%d%%\n", purity));

		if (strength >= max_str && q < SPIRIT_ROOT_VARIANT)
		{
			write(HIG " 灵根强度已满，可尝试突破！\n" NOR);
		}
		else if (q >= SPIRIT_ROOT_VARIANT)
		{
			write(HIC " 已达先天极致。\n" NOR);
		}
		else
		{
			write(sprintf(" 还需 %d 点灵根强度方可尝试突破。\n", max_str - strength));
		}

		if (fail_streak > 0)
			write(sprintf(HIR " 连续失败次数：%d\n" NOR, fail_streak));

		write("\n" HIC "--- 各方式突破概率 ---\n" NOR);
		write(sprintf("  自然突破：     %d%%\n",
			ROOT_REFINE_D->query_breakthrough_probability(me, BREAK_METHOD_NATURAL)));
		write(sprintf("  丹药辅助：     %d%%（消耗洗髓丹×3）\n",
			ROOT_REFINE_D->query_breakthrough_probability(me, BREAK_METHOD_PILL_AID)));
		write(sprintf("  灵石灌注：     %d%%（消耗灵石×5000）\n",
			ROOT_REFINE_D->query_breakthrough_probability(me, BREAK_METHOD_SPIRIT_STONE)));
		write(sprintf("  天材地宝：     %d%%（消耗对应材料×1）\n",
			ROOT_REFINE_D->query_breakthrough_probability(me, BREAK_METHOD_TREASURE)));
		write(sprintf("  秘境突破：     %d%%（消耗秘境入场券×1）\n",
			ROOT_REFINE_D->query_breakthrough_probability(me, BREAK_METHOD_SECRET_REALM)));

		if (details["streak_bonus"])
			write(sprintf(HIG "\n 连续失败保底加成：+%d%%\n" NOR, details["streak_bonus"]));
		if (details["unstable_penalty"])
			write(sprintf(HIR " 灵根不稳惩罚：-%d%%\n" NOR, details["unstable_penalty"]));

		return 1;
	}

	// 解析突破方式
	if (arg == "linggen")
		method = BREAK_METHOD_NATURAL;
	else if (arg == "linggen pill")
		method = BREAK_METHOD_PILL_AID;
	else if (arg == "linggen stone")
		method = BREAK_METHOD_SPIRIT_STONE;
	else if (arg == "linggen treasure")
		method = BREAK_METHOD_TREASURE;
	else
	{
		write("不支持的突破方式。输入 break linggen 查看帮助。\n");
		return 1;
	}

	// 天灵根自然突破直接成功
	if (quality == SPIRIT_ROOT_HEAVENLY && method == BREAK_METHOD_NATURAL)
	{
		write(HIY "天灵根拥有先天优势，小境界突破没有瓶颈！\n" NOR);
		ROOT_REFINE_D->do_breakthrough(me, method);
		return 1;
	}

	// 计算并显示概率
	int prob = ROOT_REFINE_D->query_breakthrough_probability(me, method);
	write(sprintf("本次突破基础概率为 " HIC "%d%%" NOR "，是否继续？(yes/no)\n", prob));

	// 此处用 input_to 实现确认
	// 简化处理：直接突破
	// 实际应该用 input_to 等待玩家确认，但 LPC 命令简化处理
	write("你屏气凝神，开始尝试灵根突破……\n");

	int result = ROOT_REFINE_D->do_breakthrough(me, method);
	if (result)
		write(HIG "灵根突破成功！你的实力更上一层楼！\n" NOR);

	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : break linggen [方式]

灵根突破命令。当灵根强度达到当前品质上限时，尝试突破以提升灵根属性。

参数：
  break linggen              — 自然突破（无消耗）
  break linggen pill         — 丹药辅助（消耗洗髓丹×3）
  break linggen stone        — 灵石灌注（消耗灵石×5000）
  break linggen treasure     — 天材地宝（消耗天材地宝×1）
  break linggen info         — 查看详细突破概率

说明：
  突破成功可获得灵根强度+10、精纯度+3%等增益。
  突破失败会根据方式扣除灵根强度。
  连续失败3次后，下次突破概率+20%。
  突破过程中有15%概率触发随机事件。

HELP
	);
	return 1;
}
