// main_quest.c
// 主线任务玩家命令：查看/接取/提交
// 用法：main_quest           - 查看主线进度
//       main_quest accept    - 接取下一个主线任务
//       main_quest submit    - 提交当前主线任务
//
// 注意：此命令与已有 quest.c（师门任务）功能分离，
//       专用于主线任务框架。

#include <ansi.h>
#include <main_quest.h>

int main(object me, string arg)
{
    object mqd;
    string node_id, msg;
    int result;

    // 确保守护进程已加载
    mqd = find_object(MAIN_QUEST_D);
    if (!mqd)
    {
        mqd = load_object(MAIN_QUEST_D);
        if (!mqd)
            return notify_fail("主线任务系统暂不可用，请稍后再试。\n");
    }

    if (!arg || arg == "")
    {
        // 无参数：显示进度
        msg = mqd->query_progress(me);
        write(msg);
        return 1;
    }

    // ── accept：接取任务 ──
    if (arg == "accept")
    {
        // 先尝试 start_quest（自动推进到下一个节点）
        result = mqd->start_quest(me);
        if (result == 1)
        {
            node_id = mqd->query_current_node_id(me);
            write(HIG "你开始了一段新的主线任务。\n" NOR);
            write(mqd->query_progress(me));
            return 1;
        }

        // 若有节点但未激活，尝试接受
        node_id = mqd->query_current_node_id(me);
        if (stringp(node_id) && node_id != "")
        {
            // 已有活跃节点
            write("你已经有一个活跃的主线任务在进行中。\n");
            write(mqd->query_progress(me));
            return 1;
        }

        write("当前无可接取的主线任务。请完成前置任务或提升境界后重试。\n");
        return 1;
    }

    // ── submit：提交当前任务 ──
    if (arg == "submit")
    {
        node_id = mqd->query_current_node_id(me);
        if (!stringp(node_id) || node_id == "")
        {
            write("你没有活跃的主线任务需要提交。\n");
            return 1;
        }

        result = mqd->complete_node(me, node_id);
        switch (result)
        {
        case 1:
            write(HIG "主线任务完成！已自动推进到下一节点。\n" NOR);
            write(mqd->query_progress(me));
            break;

        case 2:
            write(HIC "\n★ 恭喜！当前章节已全部完成！★\n" NOR);
            write(mqd->query_progress(me));
            break;

        case 3:
            write(HIY "\n☆★☆ 恭喜你完成了全部主线任务！飞升上界！☆★☆\n" NOR);
            break;

        case 0:
            write("无法完成该任务。请确认已将任务所需条件准备妥当。\n");
            break;

        default:
            write("系统错误，请联系管理员。\n");
            break;
        }

        return 1;
    }

    // 未知参数
    write("用法：main_quest           - 查看主线进度\n");
    write("      main_quest accept    - 接取下一个主线任务\n");
    write("      main_quest submit    - 提交当前主线任务\n");
    return 1;
}

int help(object me)
{
    write(@HELP
指令：main_quest

用法：
  main_quest            - 查看主线任务进度
  main_quest accept     - 接取下一个可接取的主线任务
  main_quest submit     - 提交当前进行中的主线任务

主线任务按章节推进，共 5 章：
  凡人篇 → 越国篇 → 乱星海篇 → 灵界篇 → 飞升篇

每章由若干串行任务节点构成，完成前置节点后自动解锁后续节点。
章节完成后获得里程碑奖励（经验、灵石、称号、特殊物品）。

注意：
- 主线任务不可重复完成
- 未达到对应境界无法接取对应章节任务
- 不可跳过前置节点直接完成后置节点
HELP
    );
    return 1;
}
