// root_test.c
// 灵根检测命令——查看自身灵根品质、属性、修炼速度等信息
// 基于《凡人修仙传》设定
// 

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
	
	write(me->query_spirit_root_display());
	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : root

灵根检测命令。显示你自身的灵根品质、五行属性、
修炼速度系数、灵根强度与精纯度等修仙资质信息。

注：灵根在创建角色时确定，决定你的修炼天赋和功法路线。

HELP
	);
	return 1;
}
