// diplomacy.c
// 灵界种族外交命令 - 查看外交事件、做出选择
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第7章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if (!arg || arg == "" || arg == "list")
    {
        // 显示当前外交事件列表
        mixed *events = DIPLOMACY_D->get_active_events();
        if (!sizeof(events))
        {
            write("目前没有活跃的外交事件。\n");
            return 1;
        }

        string output = HIC "╔════════ 外交事件 ════════╗\n" NOR;
        for (int i = 0; i < sizeof(events); i++)
        {
            output += sprintf("  %d. %s\n", i + 1, events[i]["name"]);
        }
        output += HIC "╚══════════════════════════════╝\n" NOR;
        output += "使用 diplomacy <编号> 查看详情。\n";
        output += "使用 diplomacy <编号> <选项> 做出选择。\n";

        me->start_more(output);
        return 1;
    }

    int event_idx, choice;
    if (sscanf(arg, "%d %d", event_idx, choice) == 2)
    {
        mixed *events = DIPLOMACY_D->get_active_events();
        if (event_idx < 1 || event_idx > sizeof(events))
            return notify_fail("无效的事件编号。\n");

        mapping ev = events[event_idx - 1];
        int ev_id = ev["id"];

        // 检查选项是否有效
        mixed *choices = ev["choices"];
        int valid = 0;
        foreach (mapping c in choices)
        {
            if (c["id"] == choice)
            {
                valid = 1;
                break;
            }
        }

        if (!valid)
            return notify_fail("无效的选项编号。\n");

        int result = DIPLOMACY_D->player_choose(me, ev_id, choice);
        switch (result)
        {
        case 1:
            write(HIG "你做出了选择，声望已更新。\n" NOR);
            return 1;
        case -1:
            return notify_fail("事件不存在。\n");
        case -2:
            return notify_fail("事件已结束。\n");
        case -3:
            return notify_fail("你已经参与过这个事件了。\n");
        default:
            return notify_fail("操作失败。\n");
        }
    }

    // 查看单个事件
    int id = atoi(arg);
    if (id > 0)
    {
        mixed *events = DIPLOMACY_D->get_active_events();
        if (id < 1 || id > sizeof(events))
            return notify_fail("无效的事件编号。\n");

        mapping ev = events[id - 1];
        string output = DIPLOMACY_D->format_event(ev["id"]);
        me->start_more(output);
        return 1;
    }

    return notify_fail("格式: diplomacy <编号> [选项]\n");
}

int help(object me)
{
    write(@HELP
指令格式: diplomacy <子命令> [参数]

灵界种族外交系统命令。
子命令:
  diplomacy              - 查看当前外交事件
  diplomacy <编号>       - 查看事件详情
  diplomacy <编号> <选项> - 做出外交选择

HELP
    );
    return 1;
}
