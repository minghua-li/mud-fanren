// squad.c
// 修仙小队命令
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第5.3章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if (!arg || arg == "" || arg == "info")
    {
        string output = me->format_squad_info();
        me->start_more(output);
        return 1;
    }

    string cmd, target;
    if (sscanf(arg, "%s %s", cmd, target) != 2)
        cmd = arg;

    switch (cmd)
    {
    case "create":
        return cmd_create(me, target);
    case "invite":
        return cmd_invite(me, target);
    case "join":
        return cmd_join(me, target);
    case "leave":
        return cmd_leave(me);
    case "kick":
        return cmd_kick(me, target);
    case "disband":
        return cmd_disband(me);
    default:
        write("小队命令：\n");
        write("  squad               - 查看小队信息\n");
        write("  squad create <名称>  - 创建小队\n");
        write("  squad invite <ID>   - 邀请加入小队\n");
        write("  squad join <ID>     - 加入小队(被邀请后)\n");
        write("  squad leave         - 离开小队\n");
        write("  squad kick <ID>     - 踢出队员(队长)\n");
        write("  squad disband       - 解散小队(队长)\n");
        write("  squad info          - 查看小队信息\n");
        return 1;
    }

    return 1;
}

// 创建小队
int cmd_create(object me, string name)
{
    if (!name || name == "")
        return notify_fail("请指定小队名称。\n");

    if (me->has_squad())
        return notify_fail("你已经有小队了。\n");

    // 境界检查(简化)
    int exp = me->query("combat_exp");
    if (exp < 100000)
        return notify_fail("需要筑基期以上才能创建小队。\n");

    if (me->create_squad(name))
    {
        write(HIG "小队「" + name + "」创建成功！\n" NOR);
        CHANNEL_D->do_channel(me, "chat",
            sprintf(HIG "%s 创建了小队「%s」，诚邀各路道友加入！\n" NOR,
                    me->query("name"), name), -1);
        return 1;
    }

    return notify_fail("创建小队失败，请检查名称长度(不超过4个汉字)。\n");
}

// 邀请加入小队
int cmd_invite(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要邀请谁加入小队？\n");

    if (!me->has_squad())
        return notify_fail("你还没有小队。\n");

    if (!me->is_squad_leader())
        return notify_fail("只有队长可以邀请成员。\n");

    object ob = find_player(target);
    if (!ob)
        return notify_fail("找不到这个玩家。\n");

    if (ob->has_squad())
        return notify_fail("对方已有小队。\n");

    if (ob == me)
        return notify_fail("不能邀请自己。\n");

    // 检查亲密等级
    int intimate_level = me->query_intimate_level(target);
    if (intimate_level < SQUAD_MIN_INTIMATE)
        return notify_fail("你和对方的亲密等级不足(需要道友之交以上)。\n");

    me->set_temp("squad_invite/" + target, 1);
    tell_object(ob, HIG + me->query("name") + " 邀请你加入小队「" +
                me->query_squad_name() + "」。\n" NOR);
    tell_object(ob, "请使用 squad join " + me->query("id") + " 加入。\n");
    write("已向 " + ob->query("name") + " 发送小队邀请。\n");

    return 1;
}

// 加入小队
int cmd_join(object me, string target)
{
    if (!target || target == "")
        return notify_fail("你要加入谁的小队？\n");

    if (me->has_squad())
        return notify_fail("你已经有小队了。\n");

    // 检查是否有邀请
    object leader = find_player(target);
    if (!leader)
        return notify_fail("找不到该队长。\n");

    if (!leader->query_temp("squad_invite/" + me->query("id")))
        return notify_fail("对方没有邀请你加入小队。\n");

    int result = leader->join_squad(me);
    switch (result)
    {
    case 1:
        write(HIG "你加入了 " + leader->query("name") + " 的小队「" +
              leader->query_squad_name() + "」。\n" NOR);
        leader->delete_temp("squad_invite/" + me->query("id"));
        return 1;
    case -1:
        return notify_fail("亲密等级不足，无法加入该小队。\n");
    case -2:
        return notify_fail("小队人数已满。\n");
    case -3:
        return notify_fail("你已有小队。\n");
    default:
        return notify_fail("加入小队失败。\n");
    }
}

// 离开小队
int cmd_leave(object me)
{
    if (!me->has_squad())
        return notify_fail("你还没有小队。\n");

    me->leave_squad();
    write("你已离开小队。\n");
    return 1;
}

// 踢出成员
int cmd_kick(object me, string target)
{
    if (!target || target == "")
        return notify_fail("要踢出谁？\n");

    if (!me->has_squad())
        return notify_fail("你还没有小队。\n");

    if (!me->is_squad_leader())
        return notify_fail("只有队长可以踢人。\n");

    if (me->kick_member(target))
    {
        write("已将 " + target + " 踢出小队。\n");
        return 1;
    }

    return notify_fail("该成员不在你的小队中。\n");
}

// 解散小队
int cmd_disband(object me)
{
    if (!me->has_squad())
        return notify_fail("你还没有小队。\n");

    if (!me->is_squad_leader())
        return notify_fail("只有队长可以解散小队。\n");

    me->disband_squad();
    write("小队已解散。\n");
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式: squad <子命令> [参数]

修仙小队(固定队)管理命令。
子命令:
  squad                    - 查看小队信息
  squad create <名称>      - 创建小队(需筑基期)
  squad invite <玩家ID>    - 邀请加入小队(需道友之交以上)
  squad join <队长ID>      - 加入被邀请的小队
  squad leave              - 离开小队
  squad kick <玩家ID>      - 踢出队员(队长)
  squad disband            - 解散小队(队长)

HELP
    );
    return 1;
}
