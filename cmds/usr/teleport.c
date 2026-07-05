// teleport.c
// 传送命令 —— 玩家传送阵交互入口
// Created for ticket #33
// 用法: teleport                   查看当前所在传送阵的可达目标
//       teleport list              同上
//       teleport <dest_id>         传送至指定目标
//       teleport info <dest_id>    查看目标详情

#include <ansi.h>
#include <teleport.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    string cmd, subj;
    mapping node, *dests;
    int i, ret;

    if (me->is_busy() || me->is_fighting())
        return notify_fail("你现在正忙着呢！\n");

    // 解析参数
    if (stringp(arg) && sscanf(arg, "%s %s", cmd, subj) == 2)
    {
        if (cmd == "info")
            return show_dest_info(me, subj);
        else if (cmd != "list")
            return do_teleport_to(me, arg);  // "teleport <dest_id>"
    }
    else if (stringp(arg) && arg != "list")
    {
        return do_teleport_to(me, arg);
    }

    // 默认 / list：列出可达目标
    node = TELEPORT_D->query_current_node(me);
    if (!mapp(node))
        return notify_fail("此处没有传送阵。\n");

    dests = TELEPORT_D->query_available_dests(me);
    if (!arrayp(dests) || sizeof(dests) == 0)
        return notify_fail("当前传送阵没有可用的传送目标。\n");

    write(HIC "╔══════════════════════════════════════════╗\n" NOR);
    write(HIC "║         " + node[TP_FIELD_NAME] + NOR HIC "           ║\n" NOR);
    write(HIC "╠══════════════════════════════════════════╣\n" NOR);
    write(sprintf("  " HIM "等级" NOR " : %s\n", TP_LEVEL_NAME[node[TP_FIELD_LEVEL]]));
    write(sprintf("  " HIM "群组" NOR " : %s\n", node[TP_FIELD_GROUP]));
    write(sprintf("  " HIM "说明" NOR " : %s\n", node[TP_FIELD_DESC]));
    write(HIC "╠══════════════════════════════════════════╣\n" NOR);
    write(HIC "║           可达传送目标                   ║\n" NOR);
    write(HIC "╠══════════════════════════════════════════╣\n" NOR);

    for (i = 0; i < sizeof(dests); i++)
    {
        string status_str, cost_str, cd_str;

        if (dests[i]["unlocked"])
            status_str = HIG "●已解锁" NOR;
        else
            status_str = HIR "●未解锁" NOR;

        cost_str = dests[i]["cost_str"];
        if (!stringp(cost_str))
            cost_str = "未知";

        cd_str = "";
        if (dests[i]["cooldown"] > 0)
            cd_str = HIM " [冷却:" + sprintf("%d秒", dests[i]["cooldown"]) + "]" NOR;

        write(sprintf("  " HIY "%-12s" NOR " %s %s %s\n",
            dests[i]["id"],
            status_str,
            cost_str,
            cd_str));
        write(sprintf("    %-20s [%s] %s\n",
            dests[i]["name"],
            TP_LEVEL_NAME[dests[i]["level"]],
            dests[i]["desc"]));
    }

    write(HIC "╚══════════════════════════════════════════╝\n" NOR);
    write("使用 " HIW "teleport <目标ID>" NOR " 进行传送。\n");

    return 1;
}

// 执行传送
int do_teleport_to(object me, string dest_id)
{
    int ret;

    if (!stringp(dest_id) || dest_id == "")
        return notify_fail("你要传送到哪里？\n");

    ret = TELEPORT_D->do_teleport(me, dest_id);

    switch (ret)
    {
    case 1:
        // 传送成功，消息已在 teleport_d 中输出
        return 1;
    case -1:
        return notify_fail(HIR "你的灵石不足以支付传送费用！\n" NOR);
    case -2:
        return notify_fail(HIY "你还未解锁目标传送阵！\n" NOR);
    case -3:
        return notify_fail(HIM "传送阵还在冷却中，请稍后再试！\n" NOR);
    case -4:
        return notify_fail(HIY "无法到达目标地点！\n" NOR);
    default:
        return notify_fail(HIR "传送失败！\n" NOR);
    }
}

// 查看目标详情
int show_dest_info(object me, string dest_id)
{
    mapping dest_node;
    int cost, remaining;

    if (!stringp(dest_id) || dest_id == "")
        return notify_fail("你要查看哪个目标的信息？\n");

    dest_node = TELEPORT_D->query_node(dest_id);
    if (!mapp(dest_node))
        return notify_fail("没有找到该传送目标。\n");

    write(HIC "╔══════════════════════════════════════════╗\n" NOR);
    write(sprintf(HIC "║       %-24s      ║\n" NOR, dest_node[TP_FIELD_NAME]));
    write(HIC "╠══════════════════════════════════════════╣\n" NOR);
    write(sprintf("  " HIM "ID" NOR "     : %s\n", dest_node[TP_FIELD_ID]));
    write(sprintf("  " HIM "等级" NOR "   : %s\n", TP_LEVEL_NAME[dest_node[TP_FIELD_LEVEL]]));
    write(sprintf("  " HIM "群组" NOR "   : %s\n", dest_node[TP_FIELD_GROUP]));
    write(sprintf("  " HIM "说明" NOR "   : %s\n", dest_node[TP_FIELD_DESC]));
    write(sprintf("  " HIM "房间" NOR "   : %s\n", dest_node[TP_FIELD_ROOM]));

    // 状态
    switch (dest_node[TP_FIELD_STATUS])
    {
    case TP_STATUS_ACTIVE:
        write(sprintf("  " HIM "状态" NOR "   : " HIG "●激活" NOR "\n"));
        break;
    case TP_STATUS_INACTIVE:
        write(sprintf("  " HIM "状态" NOR "   : " HIR "●关闭" NOR "\n"));
        break;
    case TP_STATUS_MAINT:
        write(sprintf("  " HIM "状态" NOR "   : " HIY "●维护" NOR "\n"));
        break;
    }

    write(HIC "╠══════════════════════════════════════════╣\n" NOR);
    write(HIC "║           解锁条件                       ║\n" NOR);
    write(HIC "╠══════════════════════════════════════════╣\n" NOR);

    write(sprintf("  " HIM "境界要求" NOR " : %s\n",
        dest_node[TP_FIELD_REALM_MIN] >= TP_REALM_MORTAL ?
        ({"凡人", "炼气期", "筑基期", "结丹期", "元婴期",
          "化神期", "炼虚期"})[dest_node[TP_FIELD_REALM_MIN]] : "无"));

    if (stringp(dest_node[TP_FIELD_UNLOCK_QUEST]))
        write(sprintf("  " HIM "任务要求" NOR " : %s\n", dest_node[TP_FIELD_UNLOCK_QUEST]));

    if (stringp(dest_node[TP_FIELD_UNLOCK_ITEM]))
        write(sprintf("  " HIM "物品要求" NOR " : %s\n", dest_node[TP_FIELD_UNLOCK_ITEM]));

    if (mapp(dest_node[TP_FIELD_UNLOCK_REPUT]))
    {
        string *factions = keys(dest_node[TP_FIELD_UNLOCK_REPUT]);
        int j;
        for (j = 0; j < sizeof(factions); j++)
        {
            write(sprintf("  " HIM "声望要求" NOR " : %s >= %d\n",
                factions[j], dest_node[TP_FIELD_UNLOCK_REPUT][factions[j]]));
        }
    }

    write(HIC "╚══════════════════════════════════════════╝\n" NOR);

    return 1;
}

int help(object me)
{
    write(@HELP
指令格式 :
  teleport                   查看当前传送阵的可达目标
  teleport list              同上
  teleport <目标ID>          传送至指定目标
  teleport info <目标ID>     查看目标详情

传送网络系统是连接各区域的核心基础设施。在不同区域间移动时，
可在有传送阵的地点使用此命令。

示例：
  teleport                   查看当前传送阵
  teleport tian_xing         传送至天星城
  teleport info kui_xing     查看魁星岛详情

参见：help map
HELP
    );
    return 1;
}
