// spirit_refine.c
// 灵根洗练命令——通过消耗资源提升灵根强度和精纯度
// 基于《凡人修仙传》设定，对应设计文档：02-扩充内容/02-灵根养成与突破.md
//
// 用法：
//   refine linggen              — 常规洗练（消耗灵石×5000）
//   refine linggen deeper        — 深度洗练（消耗灵石×10000 + 门派贡献×2000）
//   refine linggen status        — 查看洗练状态和冷却时间

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
		write("你身无灵根，无法进行洗练。\n");
		return 1;
	}

	int method;
	string msg;

	if (!arg || arg == "")
	{
		// 默认显示洗练帮助
		write("你要如何洗练灵根？\n");
		write("  refine linggen          — 常规洗练（灵石×5000）\n");
		write("  refine linggen deeper   — 深度洗练（灵石×10000 + 贡献×2000）\n");
		write("  refine linggen status   — 查看洗练状态\n");
		return 1;
	}

	if (arg == "status")
	{
		// 显示洗练状态
		int remaining = ROOT_REFINE_D->calculate_refine_cooldown_remaining(me, REFINE_METHOD_NORMAL);
		int strength = ROOT_REFINE_D->query_spirit_root_strength(me);
		int purity = ROOT_REFINE_D->query_spirit_root_purity(me);
		string qname = ROOT_REFINE_D->query_spirit_root_quality_name(me);
		int max_str;
		switch (quality)
		{
		case SPIRIT_ROOT_PSEUDO:   max_str = 80;  break;
		case SPIRIT_ROOT_FAKE:     max_str = 85;  break;
		case SPIRIT_ROOT_TRUE:     max_str = 95;  break;
		case SPIRIT_ROOT_VARIANT:  max_str = 100; break;
		case SPIRIT_ROOT_HEAVENLY: max_str = 100; break;
		default: max_str = 0; break;
		}

		write(HIC "═══ 灵根洗练状态 ═══" NOR "\n");
		write(sprintf(" 品质：%s\n", qname));
		write(sprintf(" 灵根强度：%d/%d\n", strength, max_str));
		write(sprintf(" 灵根精纯度：%d%%\n", purity));
		if (remaining > 0)
			write(sprintf(HIR " 冷却中：还需等待约 %d 秒\n" NOR, remaining));
		else
			write(HIG " 可进行洗练！\n" NOR);
		return 1;
	}

	// 解析洗练方式
	if (arg == "linggen")
		method = REFINE_METHOD_NORMAL;
	else if (arg == "linggen deeper")
		method = REFINE_METHOD_DEEPER;
	else
	{
		write("不支持的洗练方式。输入 refine linggen 查看帮助。\n");
		return 1;
	}

	// 检查冷却时间
	int remaining = ROOT_REFINE_D->calculate_refine_cooldown_remaining(me, method);
	if (remaining > 0)
	{
		write(HIR "洗灵池尚在冷却中，还需等待 " + remaining + " 秒才能再次使用。\n" NOR);
		return 1;
	}

	// 检查消耗
	if (!ROOT_REFINE_D->refine_cost_check(me, method))
	{
		switch (method)
		{
		case REFINE_METHOD_NORMAL:
			write("你身上没有足够的灵石（需要" + REFINE_COST_BASE_NORMAL + "枚灵石）。\n");
			break;
		case REFINE_METHOD_DEEPER:
			write("你身上没有足够的灵石和门派贡献（需要" + REFINE_COST_BASE_DEEPER +
			      "枚灵石 + " + REFINE_CONTRIBUTION_DEEPER + "点贡献）。\n");
			break;
		}
		return 1;
	}

	// 检查灵根是否已达上限
	int strength = ROOT_REFINE_D->query_spirit_root_strength(me);
	int max_str;
	switch (quality)
	{
	case SPIRIT_ROOT_PSEUDO:   max_str = 80;  break;
	case SPIRIT_ROOT_FAKE:     max_str = 85;  break;
	case SPIRIT_ROOT_TRUE:     max_str = 95;  break;
	case SPIRIT_ROOT_VARIANT:  max_str = 100; break;
	case SPIRIT_ROOT_HEAVENLY: max_str = 100; break;
	default: max_str = 0; break;
	}
	if (strength >= max_str)
	{
		write("你的灵根已臻至当前品质的极致，无法再通过洗练提升。\n");
		return 1;
	}

	// 执行洗练
	if (!ROOT_REFINE_D->refine_cost_pay(me, method))
	{
		write("资源扣除失败，无法进行洗练。\n");
		return 1;
	}

	mapping result = ROOT_REFINE_D->refine_calculate_result(me, method);
	if (!mapp(result))
	{
		write("洗练失败，请稍后再试。\n");
		return 1;
	}

	ROOT_REFINE_D->refine_apply_result(me, result);

	int str_gain = result["strength_gain"];
	int pur_gain = result["purity_gain"];
	int res_type = result["result"];

	// 洗练过程描述
	message_vision(HIC "$N运转灵力，引导洗灵池中的灵气洗练自身灵根……\n" NOR, me);

	switch (res_type)
	{
	case REFINE_RESULT_MINOR:
		write(HIC "灵根微微颤动，有少量杂质被洗去。\n" NOR);
		break;
	case REFINE_RESULT_MODERATE:
		write(HIG "灵根在灵气浸润下渐渐变得清澈，效果不错！\n" NOR);
		break;
	case REFINE_RESULT_MAJOR:
		write(HIM "灵根迸发出耀眼光芒！大量杂质被排出体外，洗练效果极佳！\n" NOR);
		break;
	case REFINE_RESULT_BOOSTED:
		write(HIY "灵根如饥似渴地吸收灵气，洗练效果超乎预期！\n" NOR);
		break;
	default:
		write("灵根似乎没有太大变化……\n");
		break;
	}

	if (str_gain > 0)
		write(sprintf(HIG "灵根强度 +%d\n" NOR, str_gain));
	if (pur_gain > 0)
		write(sprintf(HIG "灵根精纯度 +%d%%\n" NOR, pur_gain));

	// 特殊事件
	if (result["event"])
	{
		mapping ev = result["event"];
		if (ev["msg"])
			write(ev["msg"]);
	}

	write(HIC "洗练完成！\n" NOR);

	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : refine linggen [方式]

灵根洗练命令。通过消耗资源提升灵根强度和精纯度。

参数：
  refine linggen           — 常规洗练，消耗灵石×5000
  refine linggen deeper    — 深度洗练，消耗灵石×10000 + 门派贡献×2000
  refine linggen status    — 查看当前灵根洗练状态

说明：
  洗练可提升灵根强度和精纯度，是灵根养成的重要途径。
  每次洗练后需要冷却（常规30天，深度15天），
  冷却期间无法再次洗练。

  灵根强度达到当前品质上限后，无法继续通过洗练提升，
  需要先提升灵根品质。

HELP
	);
	return 1;
}
