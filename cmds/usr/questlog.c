// questlog.c
// 任务日志命令 — 查询所有类型任务的统一入口
//
// 用法：
//   questlog              - 查看所有活跃任务摘要
//   questlog main         - 查看主线任务进度
//   questlog side         - 查看支线任务
//   questlog daily        - 查看日常/周常任务
//   questlog encounter    - 查看奇遇/隐藏任务
//   questlog history      - 查看已完成任务历史
//   questlog abandon <id> - 放弃指定任务
//
// 关联系统：main_quest_d, side_quest_d, daily_task_d, encounter_d, quest_chain_d

#include <ansi.h>
#include <quest_chain.h>
#include <side_quest.h>
#include <encounter.h>
#include <main_quest.h>
#include <daily_task.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

int main(object me, string arg)
{
    if (!arg || arg == "")
    {
        return show_summary(me);
    }

    switch (arg)
    {
    case "main":
    case "主线":
        return show_main_quest(me);

    case "side":
    case "支线":
        return show_side_quests(me);

    case "daily":
    case "日常":
        return show_daily_tasks(me);

    case "weekly":
    case "周常":
        return show_weekly_tasks(me);

    case "encounter":
    case "奇遇":
        return show_encounters(me);

    case "history":
    case "历史":
        return show_history(me);

    default:
        if (sscanf(arg, "abandon %s", arg) || sscanf(arg, "放弃 %s", arg))
        {
            return abandon_quest(me, arg);
        }
        return notify_fail("未知指令。可用参数：main, side, daily, weekly, encounter, history, abandon\n");
    }
}

// ═══════════════════════════════════════════
//  任务总览
// ═══════════════════════════════════════════

int show_summary(object me)
{
    string msg;
    int main_active, side_count, daily_count, weekly_count, enc_count;
    int side_total, daily_total, weekly_total, enc_total;

    msg = HIC "\n╔══════════════════════════════╗\n" NOR;
    msg += HIC "║        修 行 录              ║\n" NOR;
    msg += HIC "╚══════════════════════════════╝\n\n" NOR;

    // 主线进度
    msg += HIW "【主线任务】\n" NOR;
    object mqd = find_object(MAIN_QUEST_D);
    if (mqd)
    {
        string node_id = mqd->query_current_node_id(me);
        if (stringp(node_id) && node_id != "")
        {
            mapping node = mqd->query_node_info(node_id);
            msg += sprintf("  当前：  " HIY "%s" NOR "\n", node["name"]);
            main_active = 1;
        }
        else
        {
            msg += "  当前无活跃主线任务。\n";
        }
        string *comp_nodes = me->query(MQ_KEY_COMP_NODES);
        if (!pointerp(comp_nodes)) comp_nodes = ({});
        msg += sprintf("  进度：  %d/50 节点完成\n", sizeof(comp_nodes));
    }

    // 支线
    msg += "\n" HIW "【支线任务】\n" NOR;
    mapping side_active_map = me->query(SIDE_QUEST_ACTIVE);
    mapping side_completed_map = me->query(SIDE_QUEST_COMPLETED);
    if (mapp(side_active_map)) side_count = sizeof(side_active_map);
    if (mapp(side_completed_map))
    {
        foreach (string k, int v in side_completed_map)
        {
            if (v > 0) side_total++;
        }
    }
    msg += sprintf("  进行中：%d 个    已完成：%d 个\n", side_count, side_total);

    // 日常
    msg += "\n" HIW "【日常任务】\n" NOR;
    mapping *daily_tasks = DAILY_TASK_D->query_player_tasks(me);
    if (arrayp(daily_tasks))
    {
        daily_count = sizeof(daily_tasks);
        foreach (mapping t in daily_tasks)
        {
            if (t["status"] == TASK_STATUS_COMPLETED)
                daily_total++;
        }
    }
    msg += sprintf("  今日可做：%d 个    已完成：%d 个\n", daily_count, daily_total);

    // 周常
    msg += "\n" HIW "【周常任务】\n" NOR;
    mapping *weekly_tasks = DAILY_TASK_D->query_weekly_player_tasks(me);
    if (arrayp(weekly_tasks))
    {
        weekly_count = sizeof(weekly_tasks);
        foreach (mapping t in weekly_tasks)
        {
            if (t["status"] == TASK_STATUS_COMPLETED)
                weekly_total++;
        }
    }
    msg += sprintf("  本周可做：%d 个    已完成：%d 个\n", weekly_count, weekly_total);

    // 奇遇
    msg += "\n" HIW "【奇遇/隐藏】\n" NOR;
    mapping enc_active = me->query(ENCOUNTER_ACTIVE);
    mapping enc_completed = me->query(ENCOUNTER_COMPLETED);
    if (mapp(enc_active)) enc_count = sizeof(enc_active);
    if (mapp(enc_completed)) enc_total = sizeof(enc_completed);
    msg += sprintf("  进行中：%d 个    已完成：%d 个\n", enc_count, enc_total);

    msg += "\n" HIC "╟──────────────────────────────╢\n" NOR;
    msg += "输入 " HIG "questlog <分类>" NOR " 查看详情。\n";
    msg += "分类：main, side, daily, weekly, encounter, history\n";

    tell_object(me, msg);
    return 1;
}

// ═══════════════════════════════════════════
//  主线进度
// ═══════════════════════════════════════════

int show_main_quest(object me)
{
    object mqd = find_object(MAIN_QUEST_D);
    if (!mqd)
        mqd = load_object(MAIN_QUEST_D);
    if (!mqd)
        return notify_fail("主线任务系统暂不可用。\n");

    string progress = mqd->query_progress(me);
    write(progress);
    return 1;
}

// ═══════════════════════════════════════════
//  支线任务
// ═══════════════════════════════════════════

int show_side_quests(object me)
{
    mapping active;
    mapping completed;
    string msg;
    int i;
    string *ids;
    string bar;

    msg = HIC "\n╔══════════════════════════════╗\n" NOR;
    msg += HIC "║        支 线 任 务           ║\n" NOR;
    msg += HIC "╚══════════════════════════════╝\n\n" NOR;

    // 活跃任务
    active = me->query(SIDE_QUEST_ACTIVE);
    if (mapp(active) && sizeof(active) > 0)
    {
        msg += HIW "进行中：\n" NOR;
        ids = keys(active);
        for (i = 0; i < sizeof(ids); i++)
        {
            mapping qs = active[ids[i]];
            mapping tmpl = SIDE_QUEST_D->query_side_quest(ids[i]);
            if (!mapp(tmpl)) continue;

            msg += sprintf("  " HIY "%d. %s" NOR "\n", i + 1, tmpl["name"]);
            msg += sprintf("     %s\n", tmpl["description"]);

            // 显示进度
            mapping progress = qs["progress"];
            mapping *objectives = tmpl["objectives"];
            if (arrayp(objectives))
            {
                int j;
                for (j = 0; j < sizeof(objectives); j++)
                {
                    string key = "obj_" + j;
                    int cur = progress[key];
                    int req = objectives[j]["amount"];
                    string target = objectives[j]["target"];

                    if (cur >= req)
                        msg += sprintf("     " HIG "✓ %s %d/%d" NOR "\n", target, cur, req);
                    else
                        msg += sprintf("     " HIY "○ %s %d/%d" NOR "\n", target, cur, req);
                }
            }
            msg += "\n";
        }
    }
    else
    {
        msg += "当前没有活跃的支线任务。\n";
        msg += "与各地NPC对话或探索新区域可能会触发支线。\n\n";
    }

    // 已完成
    completed = me->query(SIDE_QUEST_COMPLETED);
    if (mapp(completed) && sizeof(completed) > 0)
    {
        int total = 0;
        ids = keys(completed);
        foreach (string id in ids)
        {
            if (completed[id] > 0) total++;
        }
        msg += sprintf(HIW "已完成：%d 条支线" NOR "\n", total);
    }

    tell_object(me, msg);
    return 1;
}

// ═══════════════════════════════════════════
//  日常任务
// ═══════════════════════════════════════════

int show_daily_tasks(object me)
{
    mapping *tasks;
    string msg, quality_str, status_str, bar;
    int i;

    msg = HIC "\n╔══════════════════════════════╗\n" NOR;
    msg += HIC "║        日 常 任 务           ║\n" NOR;
    msg += HIC "╚══════════════════════════════╝\n\n" NOR;

    // 连击信息
    int streak = DAILY_TASK_D->query_streak(me);
    if (streak > 0)
        msg += sprintf("  连击天数：" HIY "%d" NOR " 天 (奖励+" HIG "%d%%" NOR ")\n\n",
                   streak, streak * STREAK_BONUS_PER_DAY);

    tasks = DAILY_TASK_D->query_player_tasks(me);

    if (!tasks || sizeof(tasks) == 0)
    {
        msg += "今日没有可接的日常任务。\n";
        msg += "输入 " HIG "daily_task accept" NOR " 刷新任务。\n";
    }
    else
    {
        msg += HIW "今日任务列表：\n" NOR;
        bar = HIC "──────────────────────────────\n" NOR;

        for (i = 0; i < sizeof(tasks); i++)
        {
            mapping t = tasks[i];

            // 品质颜色
            switch (t["quality"])
            {
            case QUALITY_RARE:
                quality_str = HIM "稀有" NOR;
                break;
            case QUALITY_GOOD:
                quality_str = HIB "优秀" NOR;
                break;
            default:
                quality_str = HIG "普通" NOR;
            }

            // 状态
            if (t["status"] == TASK_STATUS_COMPLETED)
                status_str = HIG "[已完成]" NOR;
            else
                status_str = HIY "[进行中]" NOR;

            msg += sprintf("  %d. %s %s %s\n",
                       i + 1, quality_str, t["name"], status_str);
            msg += sprintf("     %s\n", t["desc"]);
            msg += sprintf("     进度：" HIW "%d/%d" NOR "\n",
                       t["progress"]["current"],
                       t["progress"]["target"]);

            // 时间限制
            int remain = t["deadline"] - time();
            if (remain > 0 && remain < 7200)
                msg += sprintf("     剩余时间：" HIY "%d分钟" NOR "\n", remain / 60);
        }
    }

    msg += "\n" HIC "╟──────────────────────────────╢\n" NOR;
    msg += "用法：daily_task <submit|abandon> <编号>\n";

    tell_object(me, msg);
    return 1;
}

// ═══════════════════════════════════════════
//  周常任务
// ═══════════════════════════════════════════

int show_weekly_tasks(object me)
{
    mapping *tasks;
    string msg, quality_str, status_str;
    int i;

    msg = HIC "\n╔══════════════════════════════╗\n" NOR;
    msg += HIC "║        周 常 任 务           ║\n" NOR;
    msg += HIC "╚══════════════════════════════╝\n\n" NOR;

    tasks = DAILY_TASK_D->query_weekly_player_tasks(me);

    if (!tasks || sizeof(tasks) == 0)
    {
        msg += "本周没有可接的周常任务。\n";
    }
    else
    {
        msg += HIW "本周任务列表：\n" NOR;

        for (i = 0; i < sizeof(tasks); i++)
        {
            mapping t = tasks[i];

            switch (t["quality"])
            {
            case QUALITY_RARE:
                quality_str = HIM "稀有" NOR;
                break;
            case QUALITY_GOOD:
                quality_str = HIB "优秀" NOR;
                break;
            default:
                quality_str = HIG "普通" NOR;
            }

            if (t["status"] == TASK_STATUS_COMPLETED)
                status_str = HIG "[已完成]" NOR;
            else
                status_str = HIY "[进行中]" NOR;

            msg += sprintf("  %d. %s %s %s\n", i + 1, quality_str, t["name"], status_str);
            msg += sprintf("     %s\n", t["desc"]);
            msg += sprintf("     进度：" HIW "%d/%d" NOR "\n",
                       t["progress"]["current"],
                       t["progress"]["target"]);
        }
    }

    msg += "\n周常每周一 0:00 重置。\n";

    tell_object(me, msg);
    return 1;
}

// ═══════════════════════════════════════════
//  奇遇
// ═══════════════════════════════════════════

int show_encounters(object me)
{
    mapping active;
    mapping *history;
    string msg, rarity_str;
    int i;
    string *ids;

    msg = HIM "\n╔══════════════════════════════╗\n" NOR;
    msg += HIM "║        奇 遇 / 隐 藏         ║\n" NOR;
    msg += HIM "╚══════════════════════════════╝\n\n" NOR;

    // 活跃奇遇
    active = me->query(ENCOUNTER_ACTIVE);
    if (mapp(active) && sizeof(active) > 0)
    {
        msg += HIM "进行中：\n" NOR;
        ids = keys(active);
        for (i = 0; i < sizeof(ids); i++)
        {
            mapping es = active[ids[i]];
            mapping tmpl = ENCOUNTER_D->query_encounter(ids[i]);
            if (!mapp(tmpl)) continue;

            switch (tmpl["rarity"])
            {
            case ENC_RARE_LEGENDARY: rarity_str = HIR "【传说】" NOR; break;
            case ENC_RARE_RARE:      rarity_str = HIM "【稀有】" NOR; break;
            case ENC_RARE_UNCOMMON:  rarity_str = HIB "【优秀】" NOR; break;
            default:                 rarity_str = HIG "【普通】" NOR;
            }

            msg += sprintf("  %s %s\n", rarity_str, tmpl["name"]);
            msg += sprintf("     %s\n", tmpl["description"]);

            if (tmpl["time_limit"] > 0)
            {
                int remain = tmpl["time_limit"] - (time() - es["start_time"]);
                if (remain > 0)
                    msg += sprintf("     剩余时间：" HIY "%d分钟" NOR "\n", remain / 60);
                else
                    msg += HIR "     已超时！\n" NOR;
            }
        }
    }
    else
    {
        msg += "当前没有活跃的奇遇。\n";
        msg += "探索各处、参与战斗，机缘自会降临。\n\n";
    }

    // 历史记录
    history = me->query(ENCOUNTER_HISTORY);
    if (arrayp(history) && sizeof(history) > 0)
    {
        msg += "\n" HIM "奇遇记录（最近%d条）：\n" NOR;
        int start = sizeof(history) - 5;
        if (start < 0) start = 0;
        for (i = start; i < sizeof(history); i++)
        {
            mapping h = history[i];
            msg += sprintf("  · %s (%s)\n",
                       h["name"], ctime(h["time"]));
        }
    }

    tell_object(me, msg);
    return 1;
}

// ═══════════════════════════════════════════
//  历史记录
// ═══════════════════════════════════════════

int show_history(object me)
{
    string msg;

    msg = HIC "\n╔══════════════════════════════╗\n" NOR;
    msg += HIC "║      任 务 完 成 记 录       ║\n" NOR;
    msg += HIC "╚══════════════════════════════╝\n\n" NOR;

    // 主线
    string *comp_nodes = me->query(MQ_KEY_COMP_NODES);
    msg += sprintf(HIW "主线任务：%s" NOR "\n",
               pointerp(comp_nodes) ? sprintf("已完成 %d 个节点", sizeof(comp_nodes)) : "未开始");

    // 支线
    mapping side_comp = me->query(SIDE_QUEST_COMPLETED);
    int side_done = 0;
    if (mapp(side_comp))
    {
        foreach (string k, int v in side_comp)
        {
            if (v > 0) side_done++;
        }
    }
    msg += sprintf(HIW "支线任务：已完成 %d 条" NOR "\n", side_done);

    // 日常
    msg += HIW "日常任务：每日可完成，查看 questlog daily\n" NOR;

    // 周常
    msg += HIW "周常任务：每周可完成，查看 questlog weekly\n" NOR;

    // 奇遇
    mapping enc_comp = me->query(ENCOUNTER_COMPLETED);
    msg += sprintf(HIW "奇遇事件：已完成 %d 个" NOR "\n",
               mapp(enc_comp) ? sizeof(enc_comp) : 0);

    // 成就
    int ach_score = me->query("achievement/score");
    msg += sprintf(HIW "成就系统：%s" NOR "\n",
               ach_score ? sprintf("成就总分 %d 分", ach_score) : "未获得任何成就");

    msg += "\n" HIC "╟──────────────────────────────╢\n" NOR;
    msg += "修仙之路漫长，记录每一步的成长。\n";

    tell_object(me, msg);
    return 1;
}

// ═══════════════════════════════════════════
//  放弃任务
// ═══════════════════════════════════════════

int abandon_quest(object me, string quest_id)
{
    if (!stringp(quest_id) || quest_id == "")
        return notify_fail("用法：questlog abandon <任务ID>\n");

    // 尝试从活跃任务中查找
    mapping active = me->query(SIDE_QUEST_ACTIVE);
    if (mapp(active) && active[quest_id])
    {
        SIDE_QUEST_D->abandon_side_quest(me, quest_id);
        write("已放弃支线任务。\n");
        return 1;
    }

    // 检查日常
    // 日常放弃由 daily_task 命令处理
    write("无法放弃该任务，请使用对应任务的放弃指令。\n");
    return 1;
}
