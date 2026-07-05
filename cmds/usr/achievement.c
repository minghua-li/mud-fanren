// achievement.c
// 成就查看命令
// 玩家使用 achievement 查看成就进度

#include <ansi.h>
#include <achievement.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

int main(object me, string arg)
{
    string category, *ids, id;
    mapping ach_data;
    int i, page, total_pages;

    if (!arg || arg == "")
    {
        // 默认显示成就总览
        return show_overview(me);
    }

    if (sscanf(arg, "-c %s", category))
    {
        // 按分类查看
        return show_category(me, category);
    }

    if (sscanf(arg, "-d %s", id) || sscanf(arg, "-s %s", id))
    {
        // 查看成就详情
        return show_detail(me, id);
    }

    if (arg == "-n" || arg == "new")
    {
        // 显示新获得的成就
        return show_new(me);
    }

    if (arg == "-t" || arg == "tier")
    {
        // 显示段位信息
        return show_tier(me);
    }

    if (arg == "-a" || arg == "all")
    {
        // 显示全部成就列表
        return show_all(me);
    }

    if (arg == "-h" || arg == "help")
    {
        return show_help(me);
    }

    // 尝试作为分类名处理
    string *valid_cats = ({
        ACH_CAT_CULTIVATION, ACH_CAT_TASK, ACH_CAT_COMBAT,
        ACH_CAT_COLLECTION, ACH_CAT_EXPLORATION, ACH_CAT_SOCIAL,
        ACH_CAT_LIFE, ACH_CAT_HIDDEN
    });
    string *cn_cats = ({
        "修炼", "任务", "战斗",
        "收集", "探索", "社交",
        "生活", "隐藏"
    });

    for (i = 0; i < sizeof(cn_cats); i++)
    {
        if (arg == cn_cats[i])
            return show_category(me, valid_cats[i]);
    }

    return show_help(me);
}

// 显示成就总览
int show_overview(object me)
{
    string msg, *cats, *cn_cats;
    int total_unlocked, total_score, i, total_count;
    int tier;
    string tier_name;
    mapping all_ach;

    total_unlocked = ACHIEVEMENT_D->get_achievement_count(me);
    total_score = ACHIEVEMENT_D->get_achievement_score(me);
    total_count = sizeof(keys(ACHIEVEMENT_D->query_all_achievements()));
    tier = ACHIEVEMENT_D->query_tier(total_score);

    msg = HIC "\n╔══════════════════════════════════╗\n" NOR;
    msg += HIC "║           ★ 成就系统 ★             ║\n" NOR;
    msg += HIC "╠══════════════════════════════════╣\n" NOR;
    msg += sprintf("  已解锁: %d / %d  成就总分: %d\n", total_unlocked, total_count, total_score);

    if (tier > 0)
    {
        string *tier_names = ({ "", "成就新星", "成就达人", "成就宗师", "成就传说", "全境圆满" });
        msg += sprintf("  当前段位: %s\n", HIG + tier_names[tier] + NOR);
    }

    msg += HIC "╠══════════════════════════════════╣\n" NOR;
    msg += "  分类进度:\n";

    cats = ({ ACH_CAT_CULTIVATION, ACH_CAT_TASK, ACH_CAT_COMBAT,
              ACH_CAT_COLLECTION, ACH_CAT_EXPLORATION, ACH_CAT_SOCIAL,
              ACH_CAT_LIFE, ACH_CAT_HIDDEN });
    cn_cats = ({ ACH_CAT_CN_CULTIVATION, ACH_CAT_CN_TASK, ACH_CAT_CN_COMBAT,
                 ACH_CAT_CN_COLLECTION, ACH_CAT_CN_EXPLORATION, ACH_CAT_CN_SOCIAL,
                 ACH_CAT_CN_LIFE, ACH_CAT_CN_HIDDEN });

    for (i = 0; i < sizeof(cats); i++)
    {
        string *cat_ids = ACHIEVEMENT_D->query_achievements_by_category(cats[i]);
        int cat_total = sizeof(cat_ids);
        int cat_unlocked = 0;
        string *unlocked = ACHIEVEMENT_D->get_unlocked_achievements(me);
        int j;

        for (j = 0; j < sizeof(cat_ids); j++)
        {
            if (member_array(cat_ids[j], unlocked) != -1)
                cat_unlocked++;
        }

        msg += sprintf("  %-12s %s %s\n", cn_cats[i],
            color_progress(cat_unlocked, cat_total),
            sprintf("(%d/%d)", cat_unlocked, cat_total));
    }

    msg += HIC "╠══════════════════════════════════╣\n" NOR;
    msg += "  achievement 命令:\n";
    msg += "  achievement -c <分类> 查看分类详情\n";
    msg += "  achievement -d <ID>   查看成就详情\n";
    msg += "  achievement -t        查看段位奖励\n";
    msg += "  achievement -a        查看全部成就\n";
    msg += HIC "╚══════════════════════════════════╝\n" NOR;

    tell_object(me, msg);
    return 1;
}

// 显示分类详情
int show_category(object me, string category)
{
    string *ids, *unlocked;
    int i;
    string msg;

    ids = ACHIEVEMENT_D->query_achievements_by_category(category);
    if (!sizeof(ids))
    {
        tell_object(me, "没有这个分类。\n");
        return 1;
    }

    unlocked = ACHIEVEMENT_D->get_unlocked_achievements(me);

    msg = sprintf(HIC "\n╔══════════════════════════════════╗\n" NOR);
    msg += sprintf(HIC "║     %-20s        ║\n" NOR,
        ACHIEVEMENT_D->category_to_chinese(category));
    msg += HIC "╠══════════════════════════════════╣\n" NOR;

    for (i = 0; i < sizeof(ids); i++)
    {
        mapping ach = ACHIEVEMENT_D->query_achievement(ids[i]);
        int is_unlocked = (member_array(ids[i], unlocked) != -1);
        string status, progress;
        mapping reward;

        if (is_unlocked)
            status = HIG "✓" NOR;
        else
            status = HIW " " NOR;

        progress = ACHIEVEMENT_D->get_achievement_progress(me, ids[i]);

        msg += sprintf("  %s %-16s  %s\n",
            status, ach["name"],
            progress ? HIC + progress + NOR : "");
        msg += sprintf("     %s\n", ach["description"]);
    }

    msg += HIC "╚══════════════════════════════════╝\n" NOR;

    tell_object(me, msg);
    return 1;
}

// 显示成就详情
int show_detail(object me, string id)
{
    mapping ach = ACHIEVEMENT_D->query_achievement(id);
    int is_unlocked;
    string msg, progress;

    if (!ach)
    {
        tell_object(me, "没有这个成就。\n");
        return 1;
    }

    is_unlocked = ACHIEVEMENT_D->is_achievement_unlocked(me, id);

    msg = HIC "\n╔══════════════════════════════════╗\n" NOR;
    msg += sprintf(HIC "║      %-24s║\n" NOR, ach["name"]);
    msg += HIC "╠══════════════════════════════════╣\n" NOR;
    msg += sprintf("  分类: %s\n", ACHIEVEMENT_D->category_to_chinese(ach["category"]));
    msg += sprintf("  描述: %s\n", ach["description"]);
    msg += sprintf("  分值: %d\n", ach["score"]);
    msg += sprintf("  状态: %s\n", is_unlocked ? HIG "已解锁" NOR : HIW "未解锁" NOR);

    if (!is_unlocked)
    {
        progress = ACHIEVEMENT_D->get_achievement_progress(me, id);
        if (progress && progress != "")
            msg += sprintf("  进度: %s\n", progress);
    }

    if (ach["rewards"])
    {
        msg += "  奖励: ";
        mapping reward = ach["rewards"];
        if (reward["title"])
            msg += "称号 " + reward["title"] + " ";
        msg += "\n";
    }

    msg += HIC "╚══════════════════════════════════╝\n" NOR;

    tell_object(me, msg);
    return 1;
}

// 显示新增成就
int show_new(object me)
{
    if (me->query("achievement/new_notification"))
    {
        string *ids = ACHIEVEMENT_D->get_unlocked_achievements(me);
        string last_id;

        if (sizeof(ids) > 0)
            last_id = ids[sizeof(ids) - 1];
        ACHIEVEMENT_D->check_all(me);
        me->delete("achievement/new_notification");
        tell_object(me, "检查完成！\n");
        return 1;
    }
    tell_object(me, "没有新的成就。\n");
    return 1;
}

// 显示段位信息
int show_tier(object me)
{
    int score = ACHIEVEMENT_D->get_achievement_score(me);
    int tier = ACHIEVEMENT_D->query_tier(score);
    string msg;

    string *tier_names = ({ "", "成就新星", "成就达人", "成就宗师", "成就传说", "全境圆满" });
    string *tier_desc = ({
        "",
        HIG "修炼速度+2%" NOR,
        HIC "全属性+1%" NOR,
        HIY "全属性+2%" NOR,
        HIM "全属性+3%" NOR,
        HIR "全属性+5%\n  限量外观" NOR
    });
    int *tier_scores = ({ 0, ACH_TIER_1, ACH_TIER_2, ACH_TIER_3, ACH_TIER_4, ACH_TIER_5 });
    int i;

    msg = HIC "\n╔══════════════════════════════════╗\n" NOR;
    msg += HIC "║        ★ 成就段位 ★              ║\n" NOR;
    msg += HIC "╠══════════════════════════════════╣\n" NOR;
    msg += sprintf("  当前成就总分: %d\n", score);
    msg += sprintf("  当前段位: %s\n\n",
        tier > 0 ? HIG + tier_names[tier] + NOR : HIW "无段位" NOR);

    for (i = 1; i <= 5; i++)
    {
        string status = (tier >= i) ? HIG "✓" NOR : sprintf("%d分", tier_scores[i]);
        msg += sprintf("  %s %-12s %s\n",
            status, tier_names[i], tier_desc[i]);
    }

    msg += HIC "╚══════════════════════════════════╝\n" NOR;

    tell_object(me, msg);
    return 1;
}

// 显示全部成就
int show_all(object me)
{
    string *cats, *cn_cats;
    int i;
    string msg;

    cats = ({ ACH_CAT_CULTIVATION, ACH_CAT_TASK, ACH_CAT_COMBAT,
              ACH_CAT_COLLECTION, ACH_CAT_EXPLORATION, ACH_CAT_SOCIAL,
              ACH_CAT_LIFE, ACH_CAT_HIDDEN });
    cn_cats = ({ "修炼", "任务", "战斗", "收集", "探索", "社交", "生活", "隐藏" });

    msg = HIC "\n╔══════════════════════════════════╗\n" NOR;
    msg += HIC "║         ★ 全部成就 ★              ║\n" NOR;

    for (i = 0; i < sizeof(cats); i++)
    {
        string *ids = ACHIEVEMENT_D->query_achievements_by_category(cats[i]);
        string *unlocked = ACHIEVEMENT_D->get_unlocked_achievements(me);
        int j;

        msg += sprintf(HIC "╠════ %s ════╣\n" NOR, cn_cats[i]);

        for (j = 0; j < sizeof(ids); j++)
        {
            mapping ach = ACHIEVEMENT_D->query_achievement(ids[j]);
            int is_unlocked = (member_array(ids[j], unlocked) != -1);
            string status = is_unlocked ? HIG "✓" NOR : HIW " " NOR;
            msg += sprintf("  %s %s\n", status, ach["name"]);
        }
    }

    msg += HIC "╚══════════════════════════════════╝\n" NOR;

    tell_object(me, msg);
    return 1;
}

// 帮助
int show_help(object me)
{
    string msg;

    msg = HIC "\n╔══════════════════════════════════╗\n" NOR;
    msg += HIC "║      ★ 成就系统帮助 ★             ║\n" NOR;
    msg += HIC "╠══════════════════════════════════╣\n" NOR;
    msg += "  achievement             成就总览\n";
    msg += "  achievement <分类名>    查看分类\n";
    msg += "  achievement -c <分类>   查看分类\n";
    msg += "  achievement -d <ID>     查看详情\n";
    msg += "  achievement -t          段位信息\n";
    msg += "  achievement -a          全部成就\n";
    msg += "  achievement -n          检查新成就\n";
    msg += "  分类: 修炼 任务 战斗 收集 探索 社交 生活 隐藏\n";
    msg += HIC "╚══════════════════════════════════╝\n" NOR;

    tell_object(me, msg);
    return 1;
}

// 进度颜色
string color_progress(int current, int total)
{
    if (total == 0) return "[]";
    float ratio = to_float(current) / to_float(total);
    int filled = to_int(ratio * 20);
    string bar = "[";

    for (int i = 0; i < 20; i++)
    {
        if (i < filled)
            bar += HIG "■" NOR;
        else
            bar += HIW "□" NOR;
    }
    bar += "]";
    return bar;
}

int help(object me)
{
    show_help(me);
    return 1;
}
