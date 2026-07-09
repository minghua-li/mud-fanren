// reputation.c
// 声望查询命令 - 查看声望信息
// 设计文档: 02-扩充内容/02-声望与互动玩法.md

#include <ansi.h>
#include <reputation.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    if (!arg || arg == "" || arg == "list")
    {
        return show_all_reputations(me);
    }

    if (arg == "factions" || arg == "势力")
        return show_faction_list(me);

    if (arg == "races" || arg == "种族")
        return show_race_list(me);

    if (sscanf(arg, "faction %s", arg) || sscanf(arg, "势力 %s", arg))
        return show_faction_detail(me, arg);

    if (sscanf(arg, "race %s", arg) || sscanf(arg, "种族 %s", arg))
        return show_race_detail(me, arg);

    // 尝试直接搜索势力
    string *all = REPUTATION_D->get_all_factions();
    if (member_array(arg, all) != -1)
        return show_faction_detail(me, arg);

    string *races = REPUTATION_D->get_all_races();
    if (member_array(arg, races) != -1)
        return show_race_detail(me, arg);

    return notify_fail("未知的势力或种族。使用 reputation list 查看总览。\n");
}

// 显示所有声望总览
int show_all_reputations(object me)
{
    string output = HIG "╔══════════════ 声望总览 ══════════════╗\n" NOR;

    // 全局声望
    int right_rep = me->query(REP_PATH_GLOBAL + "/righteous");
    int evil_rep = me->query(REP_PATH_GLOBAL + "/evil");
    int right_level = REPUTATION_D->calculate_level(right_rep);
    int evil_level = REPUTATION_D->calculate_level(evil_rep);

    output += sprintf("  全局正道声望: %s %s (%d)\n",
                      REPUTATION_D->get_reputation_level_name(right_level),
                      right_rep >= 0 ? "+" + right_rep : "" + right_rep,
                      right_rep);
    output += sprintf("  全局魔道声望: %s %s (%d)\n",
                      REPUTATION_D->get_reputation_level_name(evil_level),
                      evil_rep >= 0 ? "+" + evil_rep : "" + evil_rep,
                      evil_rep);
    output += "\n";

    // 各势力声望
    output += HIY "─ 各势力声望 ─\n" NOR;
    string *factions = REPUTATION_D->get_all_factions();
    foreach (string f in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(f);
        int rep = me->query(REP_PATH_FACTION + "/" + f);
        int level = REPUTATION_D->calculate_level(rep);
        string lname = REPUTATION_D->get_reputation_level_name(level);

        output += sprintf("  %-12s: %s (%+d)\n",
                         info["name"], lname, rep);
    }

    output += "\n" HIY "─ 种族声望 ─\n" NOR;
    string *races = REPUTATION_D->get_all_races();
    foreach (string r in races)
    {
        mapping info = REPUTATION_D->get_race_info(r);
        int rep = me->query(REP_PATH_RACE + "/" + r);
        int level = REPUTATION_D->calculate_level(rep);
        string lname = REPUTATION_D->get_reputation_level_name(level);

        output += sprintf("  %-12s: %s (%+d)\n",
                         info["name"], lname, rep);
    }

    output += HIG "╚════════════════════════════════════════╝\n" NOR;

    me->start_more(output);
    return 1;
}

// 显示势力列表
int show_faction_list(object me)
{
    string output = HIY "====== 势力列表 ======\n" NOR;

    string *factions = REPUTATION_D->get_all_factions();
    foreach (string f in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(f);
        string type_name;

        switch (info["type"])
        {
        case FACTION_TYPE_RIGHTEOUS:   type_name = HIG "正道" NOR; break;
        case FACTION_TYPE_EVIL:        type_name = HIR "魔道" NOR; break;
        case FACTION_TYPE_NEUTRAL:     type_name = HIY "中立" NOR; break;
        case FACTION_TYPE_ORGANIZATION:type_name = HIC "组织" NOR; break;
        default: type_name = "未知";
        }

        output += sprintf("  %-16s %s - %s\n", info["name"], type_name, info["desc"]);
    }

    output += "\n使用 reputation faction <势力名> 查看详情。\n";
    me->start_more(output);
    return 1;
}

// 显示种族列表
int show_race_list(object me)
{
    string output = HIY "====== 灵界种族 ======\n" NOR;

    string *races = REPUTATION_D->get_all_races();
    foreach (string r in races)
    {
        mapping info = REPUTATION_D->get_race_info(r);
        int initial = REPUTATION_D->query_race_initial(r);
        string initial_name = REPUTATION_D->get_race_relation_name(initial);

        output += sprintf("  %-12s %s - %s\n",
                         info["name"],
                         initial_name == "友善" ? HIG "友善" NOR :
                         initial_name == "中立" ? HIY "中立" NOR :
                         initial_name == "不友好" ? HIR "不友好" NOR :
                         initial_name == "敌对" ? HIR "敌对" NOR : initial_name,
                         info["desc"]);
    }

    output += "\n使用 reputation race <种族名> 查看详情。\n";
    me->start_more(output);
    return 1;
}

// 显示单个势力详情
int show_faction_detail(object me, string faction_id)
{
    mapping info = REPUTATION_D->get_faction_info(faction_id);
    if (!info) return notify_fail("未知的势力。\n");

    int rep = me->query(REP_PATH_FACTION + "/" + faction_id);
    int level = REPUTATION_D->calculate_level(rep);
    string lname = REPUTATION_D->get_reputation_level_name(level);
    float discount = REPUTATION_D->query_discount(faction_id, me);

    string discount_str;
    if (discount < 0) discount_str = "无法交易";
    else if (discount >= 1.0) discount_str = sprintf("%d%%", to_int(discount * 100));
    else discount_str = sprintf("打%d折", to_int(discount * 10));

    string output = sprintf(
        "╔══════════ %s ══════════╗\n", info["name"]);
    output += sprintf("  %s\n", info["desc"]);
    output += sprintf("  声望: %s (%+d)\n", lname, rep);
    output += sprintf("  商店折扣: %s\n", discount_str);
    output += sprintf("  每日获取上限: %d\n", REPUTATION_D->query_daily_cap(me));
    output += "╚══════════════════════════════════════╝\n";

    // 显示可用交互
    mixed *actions = REPUTATION_D->get_available_actions(me, faction_id);
    if (sizeof(actions))
    {
        output += "当前可用交互: ";
        for (int i = 0; i < sizeof(actions); i++)
        {
            if (i > 0) output += ", ";
            output += REPUTATION_D->get_action_name(actions[i]);
        }
        output += "\n";
    }

    me->start_more(output);
    return 1;
}

// 显示单种族详情
int show_race_detail(object me, string race_id)
{
    mapping info = REPUTATION_D->get_race_info(race_id);
    if (!info) return notify_fail("未知的种族。\n");

    int rep = me->query(REP_PATH_RACE + "/" + race_id);
    int level = REPUTATION_D->calculate_level(rep);
    string lname = REPUTATION_D->get_reputation_level_name(level);
    int initial = REPUTATION_D->query_race_initial(race_id);

    string output = sprintf(
        "╔══════════ %s ══════════╗\n", info["name"]);
    output += sprintf("  %s\n", info["desc"]);
    output += sprintf("  初始关系: %s\n", REPUTATION_D->get_race_relation_name(initial));
    output += sprintf("  当前声望: %s (%+d)\n", lname, rep);
    output += "╚══════════════════════════════════════╝\n";

    me->start_more(output);
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式: reputation [子命令]

声望查询命令。
子命令:
  reputation                  - 查看所有声望
  reputation list             - 查看声望总览
  reputation factions         - 查看势力列表
  reputation races            - 查看种族列表
  reputation <势力/种族名>    - 查看详情

HELP
    );
    return 1;
}
