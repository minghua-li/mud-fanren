// trade.c
// 交易系统命令 - 面对面交易
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第5.2章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if (!arg || arg == "" || arg == "help")
    {
        write("交易命令：\n");
        write("  trade <玩家ID>      - 发起交易请求\n");
        write("  trade accept <ID>   - 接受交易\n");
        write("  trade reject        - 拒绝交易\n");
        write("  trade log           - 查看交易记录\n");
        return 1;
    }

    string cmd, target;
    if (sscanf(arg, "%s %s", cmd, target) != 2)
        cmd = arg;

    if (cmd == "log")
        return cmd_log(me);
    else if (cmd == "accept")
        return cmd_accept(me, target);
    else if (cmd == "reject")
        return cmd_reject(me);
    else
        // 默认: 发起交易请求
        return init_trade(me, arg);

    return 1;
}

// 发起交易
int init_trade(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要和谁交易？\n");

    if (target == me->query("id"))
        return notify_fail("不能和自己交易。\n");

    object ob = find_player(target);
    if (!ob)
        return notify_fail("该玩家不在线。\n");

    // 检查是否已在交易中
    if (me->query_temp("pending_trade/to"))
        return notify_fail("你已经在交易中了。\n");

    return TRADE_D->send_trade_request(me, ob);
}

// 接受交易
int cmd_accept(object me, string target)
{
    if (!target || target == "")
        return notify_fail("参数错误。\n");

    object ob = find_player(target);
    if (!ob)
        return notify_fail("该玩家不在线。\n");

    // 验证交易请求
    if (me->query_temp("pending_trade/from") != target)
        return notify_fail("对方没有向你发起交易请求。\n");

    // 检查超时(60秒)
    int timestamp = me->query_temp("pending_trade/time");
    if (time() - timestamp > 60)
    {
        me->delete_temp("pending_trade");
        ob->delete_temp("pending_trade");
        return notify_fail("交易请求已过期。\n");
    }

    write(HIG "你接受了 " + ob->query("name") + " 的交易请求。\n" NOR);
    write("双方准备物品后使用 trade confirm 确认。\n");
    tell_object(ob, HIG + me->query("name") + " 接受了你的交易请求。\n" NOR);

    // 进入交易状态
    me->set_temp("trade/partner", target);
    ob->set_temp("trade/partner", me->query("id"));
    me->set_temp("trade/status", "preparing");
    ob->set_temp("trade/status", "preparing");

    return 1;
}

// 拒绝交易
int cmd_reject(object me)
{
    string from = me->query_temp("pending_trade/from");
    if (!from)
        return notify_fail("没有待处理的交易请求。\n");

    object ob = find_player(from);
    if (ob)
    {
        tell_object(ob, me->query("name") + " 拒绝了你的交易请求。\n");
        ob->delete_temp("pending_trade");
    }

    me->delete_temp("pending_trade");
    write("已拒绝交易。\n");
    return 1;
}

// 查看交易记录
int cmd_log(object me)
{
    write("交易记录查询功能暂未开放。\n");
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式: trade <子命令> [参数]

交易系统命令。
子命令:
  trade <玩家ID>       - 发起交易请求
  trade accept <ID>    - 接受交易
  trade reject         - 拒绝交易

HELP
    );
    return 1;
}
