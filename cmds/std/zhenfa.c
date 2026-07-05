// zhenfa.c
// 阵法查看命令 — 查看当前阵法状态和信息
// Created for #29 子任务：技能组合与阵法系统

#include <ansi.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    int ftype = me->query_formation_type();

    if ( !ftype || ftype == 0 )
    {
        write("你当前没有布置任何阵法。\n");
        write("使用 buzhen ? 查看可用阵法列表，buzhen <阵型名称> 布阵。\n");
        return 1;
    }

    mapping data = me->query_formation_data();
    if ( !mapp(data) )
    {
        write("阵法数据异常。\n");
        return 1;
    }

    write(HIC "╔══════════════════════════════════╗\n" NOR);
    write(HIC "║          「" + data["name"] + "」                   ║\n" NOR);
    write(HIC "╠══════════════════════════════════╣\n" NOR);
    write(sprintf("║ 类型    ： %-26s ║\n", data["type"]));
    write(sprintf("║ 状态    ： %-26s ║\n",
        me->is_formation_active() ? HIG "已激活" NOR : HIR "布阵中..." NOR));
    write(sprintf("║ 效果    ： %-26s ║\n", data["effect"]));
    write(sprintf("║ 消耗    ： %-26s ║\n",
        data["cost_per_tick"] > 0 ? sprintf("%d 灵石/回合", data["cost_per_tick"]) : "无消耗"));

    if ( !me->is_formation_active() )
    {
        int remain = me->query_formation_setup_ticks();
        write(sprintf("║ 还需    ： %-26s ║\n", sprintf("%d 回合完成布阵", remain)));
    }
    else
    {
        write(HIC "╠══════════════════════════════════╣\n" NOR);
        write(HIC "║          阵  法  已  激  活        ║\n" NOR);
    }

    // 阵眼信息
    string eye_id = me->query_formation_eye();
    if ( eye_id )
    {
        write(sprintf("║ 阵眼    ： %-26s ║\n", eye_id));
    }

    // 阵灵信息
    mapping spirit = me->query_formation_spirit_status();
    if ( mapp(spirit) && spirit["alive"] )
    {
        write(sprintf("║ 阵灵    ： %-26s ║\n", spirit["name"]));
        write(sprintf("║ 灵气血  ： %-26s ║\n",
            sprintf("%d/%d", spirit["hp"], spirit["max_hp"])));
        write(sprintf("║ 灵力    ： 攻击 %-3d 防御 %-3d 修为 %-3d ║\n",
            spirit["attack"], spirit["defense"], spirit["level"]));
    }

    // 成员信息
    object *members = me->query_formation_members();
    if ( arrayp(members) && sizeof(members) > 1 )
    {
        write(sprintf("║ 成员    ： %-26s ║\n", sprintf("%d 人", sizeof(members))));
        foreach ( object ob in members )
        {
            if ( objectp(ob) )
            {
                string pos_info = "";
                if ( ob->query_temp("formation/position") )
                    pos_info = "(" + ob->query_temp("formation/position") + ")";
                write(sprintf("║           %-24s ║\n",
                    ob->query("name") + "(" + ob->query("id") + ")" + pos_info));
            }
        }
    }

    write(HIC "╚══════════════════════════════════╝\n" NOR);

    return 1;
}

int help(object me)
{
    write(@HELP
指令格式：zhenfa

查看当前阵法的状态信息，包括阵法名称、类型、效果、
状态（激活/布阵中）、阵眼、以及成员信息。

HELP
    );
    return 1;
}
