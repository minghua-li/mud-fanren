// sect.c
// 门派命令 —— 九宗入宗/晋升/贡献/功法学习
// 依赖: adm/daemons/sect_d.c (SECT_D)

#include <ansi.h>
#include <sect.h>
#include <reputation.h>
#include <globals.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

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
    case "list":
        return list_sects(me);
    case "info":
        return show_sect_info(me, subarg);
    case "join":
        return do_join(me, subarg);
    case "leave":
        return do_leave(me);
    case "betray":
        return do_betray(me, subarg);
    case "promote":
        return do_promote(me);
    case "skills":
        return list_skills(me);
    case "learn":
        return do_learn(me, subarg);
    case "help":
        return show_help(me);
    default:
        write("未知子命令：" + cmd + "，输入 sect help 查看帮助。\n");
        return 1;
    }
}

// ======== 门派面板 ========

int show_panel(object me)
{
    string output;
    string sect_id = SECT_D->query_player_sect(me);

    output = HIC "≡  ≡  ≡  ≡  ≡  ≡  【 门派 】 ≡  ≡  ≡  ≡  ≡  ≡\n" NOR;

    if (!stringp(sect_id))
    {
        output += "你尚未拜入门派，是自由散修。\n\n";
        output += "可用命令：\n";
        output += "  " HIY "sect list" NOR "            查看九大宗门\n";
        output += "  " HIY "sect info <门派>" NOR "     查看宗门详情\n";
        output += "  " HIY "sect join <门派>" NOR "     拜入宗门（需炼气三层）\n\n";
        output += "越国七派：掩月宗、黄枫谷、灵兽山、清虚门、巨剑门（正道）；化刀坞、天阙堡（中立）\n";
        output += "天罗魔道：鬼灵门、御灵宗\n";
        me->start_more(output);
        return 1;
    }

    int rank = SECT_D->query_rank(me);
    int contrib = SECT_D->query_contribution(me);
    mixed *require = SECT_D->query_next_rank_require(me);
    string *learned = SECT_D->query_learned_skills(me);

    // 门派声望（REPUTATION_D 查询；c3 面板需展示声望信息）
    int sect_rep = REPUTATION_D->query_reputation_value(me, sect_id);
    string rep_name = REPUTATION_D->get_reputation_level_name(
                          REPUTATION_D->query_reputation_level(me, sect_id));

    output += sprintf("宗门：" HIY "%s" NOR "  阶位：" HIG "%s" NOR "\n",
                      SECT_D->query_sect_name(sect_id), SECT_D->query_rank_name(me));
    output += sprintf("修为：%s  门派贡献：" HIY "%d" NOR "\n",
                      SECT_D->query_realm_display(me), contrib);
    output += sprintf("门派声望：" HIY "%d" NOR "（%s）  已学功法：" HIY "%d" NOR " 门\n",
                      sect_rep, rep_name, sizeof(learned));

    if (arrayp(require))
        output += sprintf("下一阶晋升：需%s + 贡献%d\n",
                          SECT_D->tier_name(require[0]), require[1]);
    else
        output += "你已位极本门，无更高阶位。\n";

    output += "\n可用命令：sect skills（功法） / sect promote（晋升） / sect leave（退出）\n";
    me->start_more(output);
    return 1;
}

// ======== 宗门列表 ========

int list_sects(object me)
{
    string output;
    string *sects = SECT_D->query_sects();

    output = HIC "≡  ≡  ≡  ≡  【 九大宗门 】 ≡  ≡  ≡  ≡\n\n" NOR;
    output += HIC "◇ 越国七派（正道五派、中立两派）\n" NOR;

    foreach (string sid in sects)
    {
        mapping fi = REPUTATION_D->get_faction_info(sid);
        if (!mapp(fi)) continue;

        if (fi["type"] != FACTION_TYPE_EVIL)
            output += sprintf("  %-8s  %-14s %s\n", fi["name"], fi["type"] == FACTION_TYPE_RIGHTEOUS ? HIC "【正道】" NOR : HIY "【中立】" NOR, fi["desc"]);
    }

    output += "\n" HIC "◇ 天罗国魔道六宗\n" NOR;
    foreach (string sid in sects)
    {
        mapping fi = REPUTATION_D->get_faction_info(sid);
        if (mapp(fi) && fi["type"] == FACTION_TYPE_EVIL)
            output += sprintf("  %-8s  %-14s %s\n", fi["name"], HIR "【魔道】" NOR, fi["desc"]);
    }

    output += "\n输入 " HIY "sect info <门派>" NOR " 查看详情，"
              HIY "sect join <门派>" NOR " 拜入门派。\n";
    me->start_more(output);
    return 1;
}

// ======== 宗门详情 ========

int show_sect_info(object me, string arg)
{
    string output;
    string sect_id;
    mapping cfg;
    string *sects = SECT_D->query_sects();

    if (!stringp(arg) || arg == "")
    {
        write("用法：sect info <门派>，门派列表见 sect list。\n");
        return 1;
    }

    // 允许用中文名或 ID
    sect_id = 0;
    foreach (string sid in sects)
    {
        if (sid == arg || SECT_D->query_sect_name(sid) == arg)
        {
            sect_id = sid;
            break;
        }
    }
    if (!stringp(sect_id))
    {
        write("没有找到门派「" + arg + "」。\n");
        return 1;
    }

    cfg = SECT_D->query_sect_config(sect_id);
    mapping fi = REPUTATION_D->get_faction_info(sect_id);

    output = HIC "≡  ≡  ≡  【 " + SECT_D->query_sect_name(sect_id) + " 】 ≡  ≡  ≡\n\n" NOR;
    output += "定位：" + fi["desc"] + "\n";
    if (cfg["male_only"])
        output += HIR "收徒：仅收男弟子\n" NOR;

    output += "\n◇ 阶位体系（" HIY "sect promote" NOR " 晋升）\n";
    string *ranks = cfg["ranks"];
    mixed *promote = cfg["promote"];
    for (int i = 0; i < sizeof(ranks); i++)
    {
        if (i == 0)
            output += sprintf("  %s（入门，需炼气三层）\n", ranks[0]);
        else
            output += sprintf("  %s（需%s + 贡献%d）\n", ranks[i],
                              SECT_D->tier_name(promote[i - 1][0]), promote[i - 1][1]);
    }

    output += "\n◇ 本门功法（" HIY "sect learn <功法>" NOR " 学习，消耗贡献）\n";
    foreach (string skill_id, mapping skill in cfg["skills"])
    {
        string state = member_array(skill_id, SECT_D->query_learned_skills(me)) != -1 ? HIG "已学" NOR : "未学";
        output += sprintf("  %-18s [%s] %s 贡献%d  %s\n",
                          skill["name"], ranks[skill["rank"]], state, skill["cost"], skill["desc"]);
    }

    me->start_more(output);
    return 1;
}

// ======== 入宗 ========

int do_join(object me, string arg)
{
    string sect_id;
    string *sects = SECT_D->query_sects();

    if (!stringp(arg) || arg == "")
    {
        write("用法：sect join <门派>，门派列表见 sect list。\n");
        return 1;
    }

    sect_id = 0;
    foreach (string sid in sects)
    {
        if (sid == arg || SECT_D->query_sect_name(sid) == arg)
        {
            sect_id = sid;
            break;
        }
    }
    if (!stringp(sect_id))
    {
        write("没有找到门派「" + arg + "」。\n");
        return 1;
    }

    SECT_D->join_sect(me, sect_id);
    return 1;
}

// ======== 退出/叛门 ========

int do_leave(object me)
{
    string sect_id = SECT_D->query_player_sect(me);

    if (!stringp(sect_id))
    {
        write("你尚未拜入门派。\n");
        return 1;
    }

    SECT_D->leave_sect(me);
    return 1;
}

int do_betray(object me, string arg)
{
    string sect_id = SECT_D->query_player_sect(me);

    if (!stringp(sect_id))
    {
        write("你尚未拜入门派。\n");
        return 1;
    }

    if (arg != "confirm")
    {
        write(HIR "叛门后果：本门声望 -20000，被全派追杀，且此门永不再收。\n" NOR);
        write("确认请执行：" HIY "sect betray confirm" NOR "\n");
        return 1;
    }

    SECT_D->betray_sect(me);
    return 1;
}

// ======== 晋升 ========

int do_promote(object me)
{
    SECT_D->promote(me);
    return 1;
}

// ======== 功法 ========

int list_skills(object me)
{
    string sect_id = SECT_D->query_player_sect(me);
    string output;
    mapping cfg;
    int rank, contrib;

    if (!stringp(sect_id))
    {
        write("你尚未拜入门派。\n");
        return 1;
    }

    cfg = SECT_D->query_sect_config(sect_id);
    rank = SECT_D->query_rank(me);
    contrib = SECT_D->query_contribution(me);
    string *ranks = cfg["ranks"];

    output = HIC "≡  ≡  ≡  【 " + SECT_D->query_sect_name(sect_id) +
             " 功法 】 ≡  ≡  ≡\n\n" NOR;
    output += sprintf("当前阶位：%s  贡献：%d\n\n", ranks[rank], contrib);

    foreach (string skill_id, mapping skill in cfg["skills"])
    {
        string state;
        if (member_array(skill_id, SECT_D->query_learned_skills(me)) != -1)
            state = HIG "已学" NOR;
        else if (rank < skill["rank"])
            state = HIR "阶位不足" NOR;
        else if (contrib < skill["cost"])
            state = HIY "贡献不足" NOR;
        else
            state = "可学";

        output += sprintf("  %-18s [%s] %-8s %s\n", skill["name"], ranks[skill["rank"]], state, skill["desc"]);
    }

    output += "\n学习命令：" HIY "sect learn <功法名>" NOR "（消耗门派贡献）\n";
    me->start_more(output);
    return 1;
}

int do_learn(object me, string arg)
{
    string sect_id = SECT_D->query_player_sect(me);

    if (!stringp(sect_id))
    {
        write("你尚未拜入门派。\n");
        return 1;
    }

    if (!stringp(arg) || arg == "")
    {
        write("用法：sect learn <功法名>，功法列表见 sect skills。\n");
        return 1;
    }

    // 允许中文名或 ID
    string skill_id = 0;
    foreach (string sid in SECT_D->query_sect_skills(sect_id))
    {
        mapping skill = SECT_D->query_sect_skill_info(sect_id, sid);
        if (sid == arg || (mapp(skill) && skill["name"] == arg))
        {
            skill_id = sid;
            break;
        }
    }
    if (!stringp(skill_id))
    {
        write("本门无功法「" + arg + "」。\n");
        return 1;
    }

    SECT_D->learn_skill(me, skill_id);
    return 1;
}

// ======== 帮助 ========

int show_help(object me)
{
    write(@HELP
门派命令帮助（凡人修仙传九宗）

  sect                  查看门派面板
  sect list             查看九大宗门
  sect info <门派>      查看宗门详情（阶位体系/功法）
  sect join <门派>      拜入宗门（需炼气三层以上，无叛门记录）
  sect leave            退出门派（声望 -5000，贡献清空）
  sect betray confirm   叛门（声望 -20000，被全派追杀，永不再收）
  sect promote          晋升阶位（需满足修为+贡献门槛）
  sect skills           查看本门功法与学习状态
  sect learn <功法>     学习功法（消耗门派贡献）

说明：
- 改投他派须先 sect leave（声望 -5000）或 sect betray confirm（叛门）。
- 越国七派与魔道两宗敌对，投敌对阵营请用叛门。
- 巨剑门只收男弟子；黄枫谷内门需筑基；掩月宗最高为太上长老。
- 晋升门槛与功法树见各宗门档案（.knowledge/factions/sects/）。
HELP);
    return 1;
}
