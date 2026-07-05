// activity.c
// 活跃度系统玩家命令
// usage:
//   activity              - 查看当前活跃度信息
//   activity claim <num>  - 领取日活跃度奖励（如: activity claim 20）
//   activity claim_weekly <num> - 领取周活跃度奖励
//   activity tasks        - 查看今日可做任务

#include <ansi.h>
#include <activity.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    string cmd, param;

    if (!userp(me))
        return 0;

    // 确保活跃度守护进程已加载
    if (!objectp(ACTIVITY_D))
    {
        tell_object(me, "活跃度系统暂不可用，请联系管理员。\n");
        return 1;
    }

    if (!arg || arg == "")
    {
        // 无参数：显示活跃度信息
        tell_object(me, ACTIVITY_D->format_activity_info(me));
        return 1;
    }

    // 解析参数
    if (sscanf(arg, "%s %s", cmd, param) != 2)
    {
        cmd = arg;
        param = "";
    }

    switch (cmd)
    {
    case "claim":
    case "daily":
    case "receive":
    {
        int threshold;

        if (param == "")
        {
            // 显示可领取列表
            mapping info = ACTIVITY_D->query_activity_info(me);
            tell_object(me, HIY "可领取的日活跃度奖励：\n" NOR);

            int *thresholds = ({
                ACT_DAILY_THRESHOLD_1,
                ACT_DAILY_THRESHOLD_2,
                ACT_DAILY_THRESHOLD_3,
                ACT_DAILY_THRESHOLD_4,
            });

            foreach (int th in thresholds)
            {
                string status;
                if (undefinedp(info["daily_claimed"][th]) && info["daily_score"] >= th)
                {
                    status = HIY "可领取" NOR;
                    tell_object(me, sprintf("  activity claim %-4d  %s\n", th, status));
                }
                else if (!undefinedp(info["daily_claimed"][th]))
                {
                    status = HIG "已领取" NOR;
                    tell_object(me, sprintf("  activity claim %-4d  %s\n", th, status));
                }
            }
            return 1;
        }

        if (!sscanf(param, "%d", threshold) || threshold <= 0)
        {
            tell_object(me, "用法：activity claim <活跃度阈值>\n");
            return 1;
        }

        ACTIVITY_D->claim_daily_reward(me, threshold);
        return 1;
    }

    case "claim_weekly":
    case "weekly":
    {
        int threshold;

        if (param == "")
        {
            // 显示可领取列表
            mapping info = ACTIVITY_D->query_activity_info(me);
            tell_object(me, HIY "可领取的周活跃度奖励：\n" NOR);

            int *thresholds = ({
                ACT_WEEKLY_THRESHOLD_1,
                ACT_WEEKLY_THRESHOLD_2,
                ACT_WEEKLY_THRESHOLD_3,
                ACT_WEEKLY_THRESHOLD_4,
            });

            foreach (int th in thresholds)
            {
                string status;
                if (undefinedp(info["weekly_claimed"][th]) && info["weekly_score"] >= th)
                {
                    status = HIY "可领取" NOR;
                    tell_object(me, sprintf("  activity claim_weekly %-4d  %s\n", th, status));
                }
                else if (!undefinedp(info["weekly_claimed"][th]))
                {
                    status = HIG "已领取" NOR;
                    tell_object(me, sprintf("  activity claim_weekly %-4d  %s\n", th, status));
                }
            }
            return 1;
        }

        if (!sscanf(param, "%d", threshold) || threshold <= 0)
        {
            tell_object(me, "用法：activity claim_weekly <活跃度阈值>\n");
            return 1;
        }

        ACTIVITY_D->claim_weekly_reward(me, threshold);
        return 1;
    }

    case "task":
    case "tasks":
    {
        tell_object(me, ACTIVITY_D->format_daily_task_pool(me));
        return 1;
    }

    default:
        tell_object(me, "未知的子命令。可用命令：\n");
        tell_object(me, "  activity             查看活跃度信息\n");
        tell_object(me, "  activity claim      查看/领取日奖励\n");
        tell_object(me, "  activity claim_weekly 查看/领取周奖励\n");
        tell_object(me, "  activity tasks      查看今日可做任务\n");
        return 1;
    }

    return 1;
}

int help(object me)
{
    write(@HELP
活跃度系统命令

活动度是记录玩家每日在修仙世界活跃程度的指标。
通过完成日常行为（采集、猎杀、门派任务等）积累活跃度，
达到特定阈值可领取奖励。

用法：
  activity              - 查看当前活跃度信息
  activity claim        - 查看可领取的日奖励
  activity claim <阈值> - 领取指定档位的日奖励
  activity weekly       - 查看可领取的周奖励
  activity claim_weekly <阈值> - 领取指定档位的周奖励
  activity tasks        - 查看今日可做任务

日活跃度档位：20 / 50 / 80 / 100
周活跃度档位：100 / 200 / 350 / 500

连续活跃天数越多，奖励加成越高（最多+50%）。
HELP);

    return 1;
}
