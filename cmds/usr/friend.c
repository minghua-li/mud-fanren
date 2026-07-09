// friend.c
// 好友系统命令 - 好友管理、亲密值查看、互动
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第4章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if (!arg || arg == "" || arg == "list")
    {
        // 显示好友列表
        string output = me->format_friend_list();

        // 显示申请列表
        mapping requests = FRIEND_D->get_pending_requests(me->query("id"));
        if (mapp(requests) && sizeof(requests) > 0)
        {
            output += "\n" HIG "你有 " + sizeof(requests) + " 个好友申请：\n" NOR;
            foreach (string applicant, int time in requests)
            {
                object ob = find_player(applicant);
                string name = ob ? ob->query("name") : applicant;
                output += sprintf("  %s(%s)  - 输入 friend accept %s 接受\n", name, applicant, applicant);
            }
        }

        output += "\n好友上限：" + me->query_friend_max() + "\n";
        output += "使用格式：friend add <ID> / friend del <ID> / friend ranking\n";
        me->start_more(output);
        return 1;
    }

    string cmd, target;
    if (sscanf(arg, "%s %s", cmd, target) != 2)
        cmd = arg;

    switch (cmd)
    {
    case "add":
        return cmd_add(me, target);
    case "del":
    case "delete":
    case "remove":
        return cmd_remove(me, target);
    case "accept":
        return cmd_accept(me, target);
    case "reject":
        return cmd_reject(me, target);
    case "blacklist":
    case "block":
        return cmd_blacklist(me, target);
    case "unblock":
        return cmd_unblock(me, target);
    case "ranking":
    case "rank":
        return cmd_ranking(me);
    default:
        write("好友命令：\n");
        write("  friend                - 显示好友列表\n");
        write("  friend add <ID>       - 添加好友\n");
        write("  friend del <ID>       - 删除好友\n");
        write("  friend accept <ID>    - 接受好友申请\n");
        write("  friend reject <ID>    - 拒绝好友申请\n");
        write("  friend block <ID>     - 加入黑名单\n");
        write("  friend ranking        - 亲密好友排行\n");
        write("  friend list           - 显示好友列表\n");
        return 1;
    }

    return 1;
}

// 添加好友
int cmd_add(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要添加谁为好友？\n");

    if (target == me->query("id"))
        return notify_fail("不能添加自己为好友。\n");

    // 是否已是好友
    if (me->is_friend(target))
        return notify_fail("他已经是你的好友了。\n");

    // 检查好友上限
    if (me->query_friend_count() >= me->query_friend_max())
        return notify_fail("好友已达上限，请先删除一些好友。\n");

    // 查找目标玩家
    object ob = find_player(target);
    if (!ob || !ob->query("id"))
        return notify_fail("找不到这个玩家，可能不在线。\n");

    // 检查对方是否已将自己加入黑名单
    if (ob->is_blacklisted(me->query("id")))
        return notify_fail("对方已将你加入黑名单，无法添加好友。\n");

    // 发送好友申请
    FRIEND_D->send_request(me->query("id"), target);
    write("已向 " + ob->query("name") + "(" + target + ") 发送好友申请。\n");
    tell_object(ob, HIG + me->query("name") + "(" + me->query("id") + ") 请求添加你为好友。\n"
                "请使用 friend accept " + me->query("id") + " 接受，或 friend reject 拒绝。\n" NOR);
    return 1;
}

// 删除好友
int cmd_remove(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要删除哪个好友？\n");

    if (!me->is_friend(target))
        return notify_fail("他不是你的好友。\n");

    me->remove_friend(target);
    write("已将 " + target + " 从好友列表中删除。\n");

    // 通知对方
    object ob = find_player(target);
    if (ob)
        tell_object(ob, HIR + me->query("name") + " 已将你从好友列表中删除。\n" NOR);

    return 1;
}

// 接受好友申请
int cmd_accept(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要接受谁的好友申请？\n");

    // 检查好友上限
    if (me->query_friend_count() >= me->query_friend_max())
        return notify_fail("好友已达上限，无法接受新好友。\n");

    // 查找申请者
    object ob = find_player(target);
    if (!ob)
        return notify_fail("找不到该玩家。\n");

    // 添加双向好友
    if (!me->add_friend(target, ob->query("name")))
        return notify_fail("添加好友失败。\n");

    if (!ob->add_friend(me->query("id"), me->query("name")))
    {
        // 回滚
        me->remove_friend(target);
        return notify_fail("对方好友已达上限，无法添加。\n");
    }

    // 清除申请
    FRIEND_D->accept_request(target, me->query("id"));

    write("你已接受 " + ob->query("name") + " 的好友申请。\n");
    tell_object(ob, HIG + me->query("name") + " 接受了你的好友申请！\n" NOR);
    return 1;
}

// 拒绝好友申请
int cmd_reject(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要拒绝谁的好友申请？\n");

    FRIEND_D->reject_request(target, me->query("id"));
    write("已拒绝 " + target + " 的好友申请。\n");

    object ob = find_player(target);
    if (ob)
        tell_object(ob, me->query("name") + " 拒绝了你的好友申请。\n");

    return 1;
}

// 加入黑名单
int cmd_blacklist(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要屏蔽谁？\n");

    if (me->is_blacklisted(target))
        return notify_fail("他已在你的黑名单中。\n");

    me->add_blacklist(target);
    write("已将 " + target + " 加入黑名单。\n");
    return 1;
}

// 解除黑名单
int cmd_unblock(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要解除屏蔽谁？\n");

    if (!me->is_blacklisted(target))
        return notify_fail("他不在你的黑名单中。\n");

    me->remove_blacklist(target);
    write("已将 " + target + " 移出黑名单。\n");
    return 1;
}

// 好友排名
int cmd_ranking(object me)
{
    string output = FRIEND_D->format_intimate_ranking(me);
    me->start_more(output);
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式: friend <子命令> [参数]

好友系统管理命令。
子命令:
  friend                    - 显示好友列表
  friend add <玩家ID>       - 添加好友
  friend del <玩家ID>       - 删除好友
  friend accept <玩家ID>    - 接受好友申请
  friend reject <玩家ID>    - 拒绝好友申请
  friend block <玩家ID>     - 加入黑名单
  friend unblock <玩家ID>   - 解除黑名单
  friend ranking            - 查看亲密好友排行

HELP
    );
    return 1;
}
