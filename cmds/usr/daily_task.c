// daily_task.c
// 日常任务玩家命令 — 查看/接取/提交/放弃
// Created for #36-A (#39) 日常任务系统

#include <ansi.h>
#include <daily_task.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

int main(object me, string arg)
{
        string cmd, subarg;

        if (!arg || arg == "")
        {
                return show_task_list(me);
        }

        if (sscanf(arg, "%s %s", cmd, subarg) != 2)
        {
                cmd = arg;
                subarg = "";
        }

        switch (cmd)
        {
        case "list":
                return show_task_list(me);

        case "accept":
        case "refresh":
                return refresh_tasks(me);

        case "submit":
        case "finish":
                if (subarg == "")
                        return notify_fail("用法：daily_task submit <任务编号>\n");
                return submit_task(me, atoi(subarg));

        case "abandon":
        case "giveup":
                if (subarg == "")
                        return notify_fail("用法：daily_task abandon <任务编号>\n");
                return abandon_task(me, atoi(subarg));

        case "streak":
                return show_streak(me);

        default:
                return notify_fail("未知指令。可用：list, accept, submit, abandon, streak\n");
        }
}

// ──────────────────────────────────────────────
// 显示任务列表
// ──────────────────────────────────────────────

int show_task_list(object me)
{
        mapping *tasks;
        int i, active_count, completed_count;
        string msg, quality_str, status_str, bar;

        // 获取或刷新任务（调用守护进程）
        tasks = DAILY_TASK_D->query_player_tasks(me);

        if (!tasks || sizeof(tasks) == 0)
        {
                // 没有任务，尝试刷新
                DAILY_TASK_D->refresh_player_tasks(me);
                tasks = DAILY_TASK_D->query_player_tasks(me);

                if (!tasks || sizeof(tasks) == 0)
                {
                        msg = HIC "你现在没有可用的日常任务。\n" NOR;
                        msg += "你的境界可能还没有开放日常任务。\n";
                        tell_object(me, msg);
                        return 1;
                }
        }

        // 统计
        active_count = 0;
        completed_count = 0;
        for (i = 0; i < sizeof(tasks); i++)
        {
                if (tasks[i]["status"] == TASK_STATUS_ACTIVE)
                        active_count++;
                else if (tasks[i]["status"] == TASK_STATUS_COMPLETED)
                        completed_count++;
        }

        msg = HIC "\n╔══════════════════════════════════════╗\n" NOR;
        msg += HIC "║           日 常 任 务               ║\n" NOR;
        msg += HIC "╚══════════════════════════════════════╝\n" NOR;
        msg += sprintf(
                HIC "今日任务：%d 个  |  已完成：%d 个\n" NOR,
                sizeof(tasks), completed_count);
        msg += sprintf(
                HIW "连击天数：%d 天  |  境界加成：+%d%%\n" NOR,
                DAILY_TASK_D->query_streak(me),
                DAILY_TASK_D->get_realm_bonus(
                        DAILY_TASK_D->estimate_realm_index(me),
                        REALM_QI_LOW));

        for (i = 0; i < sizeof(tasks); i++)
        {
                // 跳过已放弃和失败的任务
                if (tasks[i]["status"] == TASK_STATUS_ABANDONED ||
                    tasks[i]["status"] == TASK_STATUS_FAILED)
                        continue;

                // 品质颜色
                quality_str = DAILY_TASK_D->quality_name(tasks[i]["quality"]);

                // 状态标记
                if (tasks[i]["status"] == TASK_STATUS_COMPLETED)
                {
                        status_str = HIG "✔ 已完成" NOR;
                }
                else
                {
                        // 进度条
                        int cur, tgt;
                        cur = tasks[i]["progress"]["current"];
                        tgt = tasks[i]["progress"]["target"];
                        status_str = sprintf(HIW "%d/%d" NOR, cur, tgt);
                }

                // 任务类型名称
                string type_name;
                switch (tasks[i]["type"])
                {
                case TASK_KILL:     type_name = HIY "杀怪" NOR; break;
                case TASK_COLLECT:  type_name = HIG "采集" NOR; break;
                case TASK_VISIT:    type_name = HIC "拜访" NOR; break;
                case TASK_DELIVER:  type_name = HIM "送信" NOR; break;
                case TASK_ESCORT:   type_name = HIW "护送" NOR; break;
                case TASK_DONATE:   type_name = HIR "捐献" NOR; break;
                case TASK_PRACTICE: type_name = HIB "修炼" NOR; break;
                case TASK_DUNGEON:  type_name = HIM "副本" NOR; break;
                default:            type_name = "未知"; break;
                }

                // 进度条（仅活跃任务）
                if (tasks[i]["status"] == TASK_STATUS_ACTIVE)
                {
                        int cur, tgt, pct;
                        cur = tasks[i]["progress"]["current"];
                        tgt = tasks[i]["progress"]["target"];
                        if (tgt > 0)
                                pct = cur * 100 / tgt;
                        else
                                pct = 0;

                        bar = sprintf("│%s%s│",
                                repeat_string("■", pct / 5),
                                repeat_string("□", 20 - pct / 5));
                }
                else
                {
                        bar = "│" HIG "■■■■■■■■■■■■■■■■■■■■" NOR "│";
                }

                msg += sprintf(
                        "\n" HIW "  #%d  " NOR "%s %s\n"
                        "      类型：%s     品质：%s\n"
                        "      进度：%s\n"
                        "      " HIW "%s" NOR "\n",
                        i + 1,
                        tasks[i]["name"],
                        status_str,
                        type_name,
                        quality_str,
                        bar,
                        tasks[i]["desc"]);
        }

        msg += sprintf(
                "\n" HIC "指令：daily_task list/refresh/submit <编号>/abandon <编号>/streak\n" NOR);

        tell_object(me, msg);
        return 1;
}

// ──────────────────────────────────────────────
// 手动刷新（获取当天任务）
// ──────────────────────────────────────────────

int refresh_tasks(object me)
{
        int refreshed;

        refreshed = DAILY_TASK_D->refresh_player_tasks(me);
        if (refreshed)
        {
                tell_object(me, HIG "日常任务已刷新！使用 daily_task list 查看。\n" NOR);
        }
        else
        {
                tell_object(me, "你今天的日常任务已经准备好了，无需刷新。\n");
        }

        return show_task_list(me);
}

// ──────────────────────────────────────────────
// 提交任务
// ──────────────────────────────────────────────

int submit_task(object me, int index)
{
        int result;

        // 转换为 0-based
        index--;

        result = DAILY_TASK_D->submit_task(me, index);

        switch (result)
        {
        case 1:
                // 奖励已由守护进程发放并显示消息
                return 1;

        case -1:
                return notify_fail("不存在的任务编号。\n");

        case -2:
                return notify_fail("该任务尚未完成，无法提交。\n");

        case -3:
                return notify_fail("该任务奖励已领取。\n");

        default:
                return notify_fail("提交失败，请重试。\n");
        }
}

// ──────────────────────────────────────────────
// 放弃任务
// ──────────────────────────────────────────────

int abandon_task(object me, int index)
{
        int result;

        // 转换为 0-based
        index--;

        result = DAILY_TASK_D->abandon_task(me, index);

        switch (result)
        {
        case 1:
                // 守护进程已发送消息
                return 1;

        case -1:
                return notify_fail("不存在的任务编号，或该任务已不可放弃。\n");

        case -2:
                return notify_fail("你今天已经放弃了太多任务。\n");

        case -3:
                return notify_fail("放弃冷却中，请稍后再试。\n");

        default:
                return notify_fail("放弃失败，请重试。\n");
        }
}

// ──────────────────────────────────────────────
// 显示连击状态
// ──────────────────────────────────────────────

int show_streak(object me)
{
        int streak;
        string msg;

        streak = DAILY_TASK_D->query_streak(me);

        msg = HIC "\n╔══════════════════════════╗\n" NOR;
        msg += HIC "║      连 击 奖 励         ║\n" NOR;
        msg += HIC "╚══════════════════════════╝\n" NOR;

        msg += sprintf("  当前连击天数：\t" HIW "%d 天\n" NOR, streak);
        msg += sprintf("  连击加成：\t\t" HIG "+%d%%\n" NOR,
                DAILY_TASK_D->get_streak_bonus(streak));
        msg += sprintf("  下一档加成：\t\t" HIY "+%d%%\n" NOR,
                DAILY_TASK_D->get_streak_bonus(streak + 1));
        msg += sprintf("  加成上限：\t\t" HIR "+%d%%\n" NOR,
                MAX_STREAK_BONUS);

        msg += "\n" HIW "连续完成每日任务可获得额外奖励加成，中断后归零。\n" NOR;

        tell_object(me, msg);
        return 1;
}

// ──────────────────────────────────────────────
// 工具函数
// ──────────────────────────────────────────────

// LPC 中没有内置 repeat_string，自己实现
string repeat_string(string str, int count)
{
        string result;
        int i;

        result = "";
        for (i = 0; i < count; i++)
                result += str;
        return result;
}

int help(object me)
{
        write(@HELP
日常任务系统指令：

  daily_task              — 查看今日日常任务列表
  daily_task list         — 查看今日日常任务列表
  daily_task refresh      — 刷新/接取今日任务
  daily_task submit <N>   — 提交完成任务（编号从 1 开始）
  daily_task abandon <N>  — 放弃某个任务
  daily_task streak       — 查看连击奖励状态

说明：
  每天 0 点自动刷新，可获得最多 12 个日常任务。
  任务品质分为普通/优秀/稀有，稀有奖励更高。
  连续完成任务可获得连击加成，最高 +50%。
HELP
        );
        return 1;
}
