// war.c
// 战争系统命令 - 查看战争信息、参战登记
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第6章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if (!arg || arg == "" || arg == "info" || arg == "list")
    {
        // 显示当前战争列表
        mixed *wars = WAR_D->get_active_wars();
        if (!sizeof(wars))
        {
            write("目前没有进行中的战争。\n");
            return 1;
        }

        string output = HIR "╔════════ 当前战争 ════════╗\n" NOR;
        for (int i = 0; i < sizeof(wars); i++)
        {
            output += sprintf("  %d. %s (%s)\n", i + 1,
                              wars[i]["name"],
                              WAR_D->get_war_type_name(wars[i]["type"]));
        }
        output += HIR "╚══════════════════════════════╝\n" NOR;
        output += "使用 war <编号> 查看详情。\n";
        output += "使用 war join <编号> <阵营> 参战(attacker/defender)。\n";

        me->start_more(output);
        return 1;
    }

    string cmd, target;
    if (sscanf(arg, "%s %s", cmd, target) != 2)
    {
        // 可能是数字编号
        int id = atoi(arg);
        if (id > 0)
        {
            mixed *wars = WAR_D->get_active_wars();
            if (id > sizeof(wars)) return notify_fail("无效的战争编号。\n");
            int war_id = wars[id - 1]["id"];
            string output = WAR_D->format_war_info(war_id);
            me->start_more(output);
            return 1;
        }
        cmd = arg;
    }

    if (cmd == "join")
    {
        string war_idx, side;
        if (sscanf(arg, "join %s %s", war_idx, side) != 2)
            return notify_fail("格式: war join <编号> <attacker/defender>\n");

        int idx = atoi(war_idx);
        mixed *wars = WAR_D->get_active_wars();
        if (idx < 1 || idx > sizeof(wars))
            return notify_fail("无效的战争编号。\n");

        int war_id = wars[idx - 1]["id"];
        int result = WAR_D->join_war(war_id, me, side);

        if (result == 1)
            write(HIG "你已成功报名参战！请等待战争开始。\n" NOR);
        else if (result == 2)
            write("你已经报名了。\n");
        else
            write("报名失败，战争可能已进入战斗阶段。\n");

        return 1;
    }

    if (cmd == "merit")
    {
        int total = WAR_D->query_total_merit(me->query("id"));
        int level = WAR_D->calc_merit_level(total);
        string level_name = WAR_D->get_merit_level_name(level);

        write(sprintf("你的战争功勋：%d\n军衔：%s\n", total, level_name));
        return 1;
    }

    return notify_fail("未知子命令，使用 war 查看帮助。\n");
}

int help(object me)
{
    write(@HELP
指令格式: war <子命令> [参数]

战争系统命令。
子命令:
  war                   - 查看当前战争列表
  war <编号>            - 查看战争详情
  war join <编号> <阵营> - 报名参战(阵营: attacker/defender)
  war merit             - 查看个人战功

HELP
    );
    return 1;
}
