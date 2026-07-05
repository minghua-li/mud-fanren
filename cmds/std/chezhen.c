// chezhen.c
// 撤阵命令 — 解散当前阵法
// Created for #29 子任务：技能组合与阵法系统

#include <ansi.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if ( !me->query_formation_type() || me->query_formation_type() == 0 )
        return notify_fail("你现在没有任何阵法。\n");

    string formation_name = "未知阵法";
    mapping data = me->query_formation_data();
    if ( mapp(data) )
        formation_name = data["name"];

    write( HIY "你决定撤去「" + formation_name + "」。\n" NOR );
    message_vision( HIY "$N" HIY "一挥袖，收起了" + (me->is_formation_active() ? "已激活的" : "正在布置的") +
        "「" + formation_name + "」。\n" NOR, me);

    me->dismiss_formation();
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式：chezhen

解散当前正在布置或已激活的阵法。
所有阵法的效果将立即消失。

HELP
    );
    return 1;
}
