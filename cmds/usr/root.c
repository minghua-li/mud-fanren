// root.c
// 灵根检测命令——查看自身灵根品质、属性、修炼速度等信息
// 基于《凡人修仙传》设定
// #61 P5-1 转正：接入新手流程——新玩家创建时（human.c）已生成灵根，
// 本命令为灵根检测正式入口；无灵根时给出检测引导。
// 注：基线文件为 root_test.c（命令动词=文件名），本轮更名 root.c 使命令动词
// 与文案/设计一致（全库无 root_test 命令引用，更名无破坏）。
//

#include <ansi.h>
#include <spirit_root.h>
#include <globals.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
	mapping root = me->query(SPIRIT_ROOT_DATA);

	if (!mapp(root))
	{
		write(HIY "你尚未检测灵根。\n" NOR);
		write("灵根在角色创建时生成，若缺失请前往修仙门派的「测灵殿」进行检测，或联系管理员补测。\n");
		return 1;
	}

	write(me->query_spirit_root_display());

	// 附：境界与修炼引导（新玩家首次查看时给出）
	write(sprintf(HIC " 境界：%s\n" NOR,
	              ROOT_REFINE_D->query_player_realm(me)));
	write(sprintf(" 修为：%d\n", ROOT_REFINE_D->query_xiuwei(me)));
	if (me->query("combat_exp") < 2000)
	{
		write(HIG "\n你初入仙途，可输入 dazuo 打坐修炼积累修为，xiuwei 查看修为，tupo 尝试突破境界。\n" NOR);
	}
	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : root

灵根检测命令。显示你自身的灵根品质、五行属性、
修炼速度系数、灵根强度与精纯度等修仙资质信息。

注：灵根在创建角色时确定，决定你的修炼天赋和功法路线。
相关命令：xiuwei 查看修为与境界，dazuo 打坐修炼，tupo 境界突破。
HELP
	);
	return 1;
}
