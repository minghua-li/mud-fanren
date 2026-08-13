// facility.c
// 门派设施命令 —— 查看/使用/升级/种植/收获/阅读/抄录/坊市购买/每日修行
// 依赖: adm/daemons/sect_facility_d.c (SECT_FACILITY_D)
// Created for ticket #60

#include <ansi.h>
#include <sect_facility.h>
#include <globals.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

// 主入口
int main(object me, string arg)
{
    string key;
    string cmd, subarg;
    mapping cfg;

    key = SECT_FACILITY_D->query_current_facility(me);

    if (!stringp(key))
    {
        // 不在设施房间内：仅允许 list / help
        if (stringp(arg) && sscanf(arg, "%s %s", cmd, subarg) == 2 && cmd == "list")
        {
            do_list(me);
            return 1;
        }
        if (arg == "list")
        {
            do_list(me);
            return 1;
        }
        if (arg == "help")
        {
            show_help(me);
            return 1;
        }
        write("你不在任何门派设施内。输入 facility list 查看本门设施，或进入设施房间后使用 facility。\n");
        return 1;
    }

    cfg = SECT_FACILITY_D->query_facility_config(key);

    if (!stringp(arg) || arg == "")
        return show_facility(me, key, cfg);

    if (sscanf(arg, "%s %s", cmd, subarg) != 2)
    {
        cmd = arg;
        subarg = "";
    }

    switch (cmd)
    {
    case "use":
        return do_use(me, key, cfg, subarg);
    case "upgrade":
        SECT_FACILITY_D->upgrade_facility(me, key);
        return 1;
    case "plant":
        if (subarg == "")
        {
            write("用法：facility plant <灵草|黄龙草|紫丹参>\n");
            return 1;
        }
        SECT_FACILITY_D->plant(me, key, subarg);
        return 1;
    case "harvest":
    {
        int idx;
        if (subarg == "")
        {
            write("用法：facility harvest <地块编号>\n");
            return 1;
        }
        idx = to_int(subarg) - 1;
        if (idx < 0) idx = 0;
        SECT_FACILITY_D->harvest(me, key, idx);
        return 1;
    }
    case "read":
        if (subarg == "")
        {
            write("用法：facility read <功法名>（藏经阁内）\n");
            return 1;
        }
        SECT_FACILITY_D->read_skill(me, key, subarg);
        return 1;
    case "copy":
        if (subarg == "")
        {
            write("用法：facility copy <功法名>（藏经阁内，耗贡献）\n");
            return 1;
        }
        SECT_FACILITY_D->transcribe_skill(me, key, subarg);
        return 1;
    case "buy":
        if (subarg == "")
        {
            write("用法：facility buy <灵草|黄龙草|紫丹参>（坊市内）\n");
            return 1;
        }
        SECT_FACILITY_D->market_buy(me, key, subarg, 1);
        return 1;
    case "practice":
        SECT_FACILITY_D->practice(me, key);
        return 1;
    case "list":
        do_list(me);
        return 1;
    case "help":
        show_help(me);
        return 1;
    default:
        write("未知子命令：" + cmd + "，输入 facility help 查看帮助。\n");
        return 1;
    }
}

// ======== 使用设施 ========

int do_use(object me, string key, mapping cfg, string arg)
{
    // 专项型设施：use 即展示对应面板
    if (cfg["type"] == SECT_FACILITY_PLANT)
    {
        write(HIC "灵田状态：\n" NOR + SECT_FACILITY_D->describe_plots(me, key));
        write("可种灵种：\n" + SECT_FACILITY_D->describe_seeds());
        write("指令：facility plant <灵种> / facility harvest <地块编号>\n");
        return 1;
    }
    if (cfg["type"] == SECT_FACILITY_LIBRARY)
    {
        show_library(me, key);
        return 1;
    }
    if (cfg["market"])
    {
        write(HIC "坊市货物：\n" NOR + SECT_FACILITY_D->describe_seeds());
        write("指令：facility buy <灵草|黄龙草|紫丹参>\n");
        return 1;
    }

    SECT_FACILITY_D->use_facility(me, key);
    return 1;
}

// ======== 设施面板 ========

int show_facility(object me, string key, mapping cfg)
{
    string output;
    int v;

    output = HIC "≡  ≡  ≡  ≡  ≡  ≡  【 门派设施 】 ≡  ≡  ≡  ≡  ≡  ≡\n" NOR;
    output += SECT_FACILITY_D->describe_facility(key);

    output += "\n你的设施效果：\n";
    if (cfg["type"] == SECT_FACILITY_PLANT)
    {
        output += SECT_FACILITY_D->describe_plots(me, key);
        output += "可种灵种：\n" + SECT_FACILITY_D->describe_seeds();
    }
    else if (cfg["type"] == SECT_FACILITY_LIBRARY)
    {
        output += "  本门功法阅读/抄录：facility read <功法> / facility copy <功法>\n";
    }
    else if (cfg["market"])
    {
        output += "  坊市货物：\n" + SECT_FACILITY_D->describe_seeds();
    }
    else
    {
        mapping eff = cfg["effect"];
        v = SECT_FACILITY_D->query_buff(me, key);
        if (v > 0)
            output += "  " + SECT_BUFF_NAME[eff["type"]] + "加成：+" + v + "%（生效中）\n";
        else
            output += "  " + SECT_BUFF_NAME[eff["type"]] + "加成：未激活（facility use 激活）\n";
    }

    if (mapp(cfg["daily_reward"]))
        output += "每日修行：" + cfg["daily_reward"]["verb"] + "（facility practice，每日 1 次）\n";

    output += "\n可用指令：facility use / facility upgrade / facility practice / facility list / facility help\n";
    me->start_more(output);
    return 1;
}

// ======== 藏经阁面板 ========

int show_library(object me, string key)
{
    string output;
    string sect_id = SECT_D->query_player_sect(me);

    if (!stringp(sect_id))
    {
        write("你尚未拜入门派。\n");
        return 1;
    }

    output = HIC "≡  ≡  ≡  【 藏经阁 · 本门功法 】 ≡  ≡  ≡\n\n" NOR;
    foreach (string skill_id, mapping skill in SECT_D->query_sect_config(sect_id)["skills"])
    {
        output += sprintf("  %-18s [%s] 贡献%d  %s\n",
                          skill["name"],
                          SECT_D->query_sect_ranks(sect_id)[skill["rank"]],
                          skill["cost"], skill["desc"]);
    }

    output += "\n指令：facility read <功法名>（免费阅读） / facility copy <功法名>（耗贡献抄录）\n";
    me->start_more(output);
    return 1;
}

// ======== 本门设施列表（任何位置） ========

void do_list(object me)
{
    string sect_id;
    string *keys;
    string output;

    sect_id = SECT_D->query_player_sect(me);
    if (!stringp(sect_id))
    {
        write("你尚未拜入任何门派，无法查看门派设施。\n");
        return;
    }

    output = HIC "≡  ≡  ≡  ≡  【 " + SECT_D->query_sect_name(sect_id) + " 设施 】 ≡  ≡  ≡  ≡\n\n" NOR;
    keys = SECT_FACILITY_D->query_facility_keys(sect_id);
    foreach (string k in keys)
        output += SECT_FACILITY_D->describe_facility(k) + "\n";

    output += "\n灵种（灵田可种、坊市可购）：\n" + SECT_FACILITY_D->describe_seeds();
    output += "\n进入设施房间后使用 facility 查看详情。\n";
    me->start_more(output);
}

// ======== 帮助 ========

int show_help(object me)
{
    write(@HELP
门派设施命令帮助（凡人修仙传九宗）

  facility                查看所在设施详情
  facility list           查看本门全部设施（任何位置可用）
  facility use            使用所在设施（丹房/护山大阵/演武场等）
  facility upgrade        捐献灵石+贡献，升级所在设施
  facility practice       每日修行（切磋/论道/驯兽等，每日 1 次）

灵田（百药园）：
  facility plant <灵种>   在空闲地块种植（灵草/黄龙草/紫丹参）
  facility harvest <编号> 收获成熟作物
藏经阁：
  facility read <功法>    免费阅读本门功法
  facility copy <功法>    耗贡献抄录功法入藏
坊市：
  facility buy <货物>     以灵石购买灵材

说明：
- 必须拜入本门、且身处设施房间内方可使用。
- 使用/升级消耗灵石与门派贡献；贡献经门派系统扣减（sect 查询）。
- 设施等级为全门共享，内门弟子以上可主持升级；等级越高效果越强。
HELP);
    return 1;
}
