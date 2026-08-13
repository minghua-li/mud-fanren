// sectquest.c
// 宗门任务命令 —— 宗门任务链接取/查看/交任务 + 宗门事件触发
// 依赖: adm/daemons/sect_quest_d.c (SECT_QUEST_D) / quest_chain_d.c / sect_d.c
// 接 02-扩充内容/02-任务链与奖励曲线.md 与九宗档案「宗门事件与任务链」节

#include <ansi.h>
#include <quest_chain.h>
#include <sect_quest.h>
#include <sect.h>
#include <globals.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

int help(object me);

// 主入口
int main(object me, string arg)
{
    string cmd, subarg;

    if (!arg)
        return show_panel(me);

    if (sscanf(arg, "%s %s", cmd, subarg) != 2)
    {
        cmd = arg;
        subarg = "";
    }

    switch (cmd)
    {
    case "accept":
        return do_accept(me, subarg);
    case "info":
        return show_quest_info(me, subarg);
    case "progress":
        return show_progress(me, subarg);
    case "report":
        return do_report(me, subarg);
    case "event":
        return do_event(me, subarg);
    case "help":
        return show_help(me);
    default:
        write("未知子命令：" + cmd + "，输入 sectquest help 查看帮助。\n");
        return 1;
    }
}

// ======== 面板 ========

int show_panel(object me)
{
    string output;
    string sect_id = SECT_D->query_player_sect(me);
    mapping *quests;
    mapping *events;
    int i;

    output = HIC "≡  ≡  ≡  ≡  【 宗门任务与事件 】 ≡  ≡  ≡  ≡\n" NOR;

    if (!stringp(sect_id))
    {
        output += "你尚未拜入门派，无法领取宗门任务。\n";
        output += "先输入 " HIY "sect join <门派>" NOR " 拜入宗门。\n";
        me->start_more(output);
        return 1;
    }

    output += sprintf("宗门：" HIY "%s" NOR "\n", SECT_D->query_sect_name(sect_id));

    // 活跃度状态（连续活跃梯度）
    int streak = me->query(QUEST_CHAIN_DAILY_STREAK);
    output += sprintf("连续活跃：%s%d%s 天（任务奖励加成 "
                      HIW "%d%%" NOR "）\n",
                      HIG, streak ? streak : 0, NOR,
                      to_int((QUEST_CHAIN_D->calc_daily_bonus(streak) - 1.0) * 100));

    // 任务链列表
    quests = SECT_QUEST_D->query_sect_quests(me);
    output += "\n◇ 宗门任务链（" HIY "sectquest accept <id>" NOR " 接取）\n";
    for (i = 0; i < sizeof(quests); i++)
    {
        mapping t = quests[i];
        string state;

        switch (t["status"])
        {
        case QUEST_STATUS_ACTIVE:
            state = HIG "进行中" NOR;
            break;
        case QUEST_STATUS_AVAILABLE:
            state = HIW "可接取" NOR;
            break;
        case QUEST_STATUS_COMPLETED:
            state = HIC "已完成" NOR;
            break;
        default:
            state = HIW "未解锁" NOR;
            break;
        }

        output += sprintf("  %-16s [%s] %s\n", t["id"], state, t["name"]);
    }

    // 事件列表
    events = SECT_QUEST_D->query_sect_events(me);
    output += "\n◇ 宗门事件（" HIY "sectquest event <id>" NOR " 触发）\n";
    for (i = 0; i < sizeof(events); i++)
    {
        mapping ev = events[i];
        output += sprintf("  %-16s [%s] %s\n", ev["id"],
                          ev["triggered"] ? HIC "已参与" NOR : HIW "可触发" NOR,
                          ev["name"]);
    }

    output += "\n可用命令：sectquest accept/info/progress/report/event，详见 sectquest help\n";
    me->start_more(output);
    return 1;
}

// ======== 接取任务 ========

int do_accept(object me, string arg)
{
    if (!stringp(arg) || arg == "")
    {
        write("用法：sectquest accept <任务ID>，任务列表见 sectquest。\n");
        return 1;
    }
    SECT_QUEST_D->accept_quest(me, arg);
    return 1;
}

// ======== 任务详情 ========

int show_quest_info(object me, string arg)
{
    string output;
    mapping t;
    mapping *objectives;
    mapping rewards;
    int i;

    if (!stringp(arg) || arg == "")
    {
        write("用法：sectquest info <任务ID>。\n");
        return 1;
    }

    t = SECT_QUEST_D->query_quest(arg);
    if (!mapp(t))
    {
        write("没有找到宗门任务「" + arg + "」。\n");
        return 1;
    }

    output = HIC "≡  ≡  【 宗门任务：%s 】 ≡  ≡\n\n" NOR;
    output = sprintf(output, t["name"]);
    output += t["description"] + "\n\n";

    objectives = t["objectives"];
    output += "◇ 目标\n";
    for (i = 0; i < sizeof(objectives); i++)
    {
        mapping obj = objectives[i];
        string ot;

        switch (obj["type"])
        {
        case OBJ_REACH: ot = "到达"; break;
        case OBJ_TALK:  ot = "对话"; break;
        case OBJ_COLLECT: ot = "收集"; break;
        default:        ot = "完成"; break;
        }
        output += sprintf("  %s「%s」x%d\n", ot, obj["target"], obj["amount"]);
    }

    rewards = t["rewards"];
    output += "\n◇ 奖励\n";
    if (rewards["exp"])
        output += sprintf("  修为经验 %d\n", rewards["exp"]);
    if (rewards["coin"])
        output += sprintf("  灵石 %d\n", rewards["coin"]);
    if (rewards["contribution"])
        output += sprintf("  门派贡献 %d\n", rewards["contribution"]);
    if (arrayp(rewards["reputation"]))
    {
        for (i = 0; i < sizeof(rewards["reputation"]); i++)
            output += sprintf("  声望 %d\n", rewards["reputation"][i]["value"]);
    }
    if (arrayp(rewards["items"]))
    {
        for (i = 0; i < sizeof(rewards["items"]); i++)
            output += sprintf("  物品：%s\n", rewards["items"][i]);
    }
    if (arrayp(rewards["skills"]))
    {
        for (i = 0; i < sizeof(rewards["skills"]); i++)
        {
            string sid = rewards["skills"][i];
            mapping sinfo = SECT_D->query_sect_skill_info(t["sect"], sid);
            output += sprintf("  功法：%s\n", mapp(sinfo) ? sinfo["name"] : sid);
        }
    }

    me->start_more(output);
    return 1;
}

// ======== 任务进度 ========

int show_progress(object me, string arg)
{
    mapping active;
    mapping template;
    mapping objectives;
    mapping progress;
    int i;

    if (!stringp(arg) || arg == "")
    {
        write("用法：sectquest progress <任务ID>。\n");
        return 1;
    }

    active = QUEST_CHAIN_D->get_player_active_quest(me, arg);
    if (!mapp(active))
    {
        write("你当前没有进行中的任务「" + arg + "」\n");
        return 1;
    }

    template = SECT_QUEST_D->query_quest(arg);
    if (!mapp(template))
    {
        write("没有找到宗门任务「" + arg + "」。\n");
        return 1;
    }

    objectives = template["objectives"];
    progress = active["progress"];
    if (!mapp(progress)) progress = ([]);

    write(HIC "任务「" + template["name"] + "」进度：\n" NOR);
    for (i = 0; i < sizeof(objectives); i++)
    {
        mapping obj = objectives[i];
        string key = "obj_" + i;
        int cur = progress[key];
        if (!cur) cur = 0;
        write(sprintf("  %s  %d/%d\n", obj["target"], cur, obj["amount"]));
    }

    if (SECT_QUEST_D->quest_progress(me, arg))
        write(HIG "目标已全部达成，可回宗门驻地输入 sectquest report " + arg + " 交任务。\n" NOR);
    else
        write(HIW "继续推进目标吧（到达指定地点即视为达成）。\n" NOR);

    return 1;
}

// ======== 交任务 ========

int do_report(object me, string arg)
{
    if (!stringp(arg) || arg == "")
    {
        write("用法：sectquest report <任务ID>。\n");
        return 1;
    }
    SECT_QUEST_D->report_quest(me, arg);
    return 1;
}

// ======== 触发事件 ========

int do_event(object me, string arg)
{
    if (!stringp(arg) || arg == "")
    {
        write("用法：sectquest event <事件ID>，事件列表见 sectquest。\n");
        return 1;
    }
    SECT_QUEST_D->trigger_event(me, arg);
    return 1;
}

// ======== 帮助 ========

int show_help(object me)
{
    write(HIC "≡  ≡  【 宗门任务与事件 】 ≡  ≡\n" NOR);
    write("  sectquest               查看本宗任务链与事件\n");
    write("  sectquest accept <ID>   接取宗门任务（需在本宗驻地）\n");
    write("  sectquest info <ID>     查看任务详情与奖励\n");
    write("  sectquest progress <ID> 查看任务进度\n");
    write("  sectquest report <ID>   交任务并结算奖励（需在本宗驻地）\n");
    write("  sectquest event <ID>    触发宗门事件（境界/条件满足时）\n");
    write("\n连续完成宗门事务可累积活跃度，任务与事件奖励随连续活跃递增、断档回落。\n");
    return 1;
}

int help(object me)
{
    return show_help(me);
}
