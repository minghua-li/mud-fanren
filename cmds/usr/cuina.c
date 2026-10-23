// cuina.c
// 淬体命令（#79：体修路径 基础炼体→法体双修→极限炼体→毒体修炼）
//
// 用法：
//   cuina            查看体修面板（阶段/淬体点数/加成/上限）
//   cuina lian       运功淬体（消耗内力与精神，积累淬体点数）
//   cuina help       帮助
//
// 淬体点数换取：永久气血上限（feature/attribute.c query_max_qi）、
// 固定减伤（include/combat/damage.h defatk）、空手与重剑体修伤害（damage.h）。
// 阶段由体修功法掌握情况推导（BODY_D::query_stage），越高阶功法可淬炼的点数上限越高。

#include <ansi.h>
#include <globals.h>
#include <body.h>

inherit F_CLEAN_UP;

int help(object me);
int show_panel(object me);

int main(object me, string arg)
{
    int gain;

    seteuid(getuid());

    if (stringp(arg) && (arg == "help" || arg == "?"))
        return help(me);

    // 无参 / 显式查询
    if (!stringp(arg) || arg == "" || arg == "cha" || arg == "查看")
        return show_panel(me);

    if (arg != "lian" && arg != "train" && arg != "练" && arg != "淬")
        return help(me);

    gain = BODY_D->train_body(me);
    if (gain <= 0)
        return 1;

    tell_object(me, HIG "淬体有成：本次积累 " + gain + " 点（面板如下）。\n" NOR);
    show_panel(me);
    return 1;
}

int show_panel(object me)
{
    mapping info;
    string msg;

    info = BODY_D->query_body_info(me);
    if (!mapp(info)) return 1;

    msg = HIC "═══ 体修面板 ═══\n" NOR;
    msg += sprintf("  当前阶段：%s%s%s\n", HIY, info["stage_name"], NOR);
    msg += sprintf("  淬体点数：%d / %d%s\n", info["points"], info["cap"],
                   info["cap"] > 0 && info["points"] >= info["cap"] ? HIR "（已达当前阶段极限）" NOR : "");
    msg += sprintf("  肉身加成：气血上限 +%d，固定减伤 +%d，空手伤害 +%d，重剑伤害 +%d\n",
                   info["qi"], info["defense"], info["fist"], info["sword"]);
    msg += sprintf("  体修功法：最高 %d 级（%s）\n", info["max_skill"],
                   info["max_skill"] > 0 ? "单次淬体收益随等级提升" : "尚未修习炼体功法");
    msg += "  四段路径：基础炼体（象甲功）→ 法体双修（梵圣真魔功）→ 极限炼体（涅槃圣体）→ 毒体修炼（万毒混元身）\n";
    msg += "  每段须上一段功法 " + BODY_PREREQ_SKILL + " 级方可修习。\n";
    write(msg);
    if (info["stage"] < BODY_STAGE_BASIC)
        write("你尚未修习炼体功法，可向巨剑门传功长老学(learn)「象甲功」。\n");
    else
        write("输入 cuina lian 运功淬体（消耗内力 " + BODY_COST_NEILI +
              "、精神 " + BODY_COST_JING + "）。\n");
    return 1;
}

int help(object me)
{
    write(@HELP
淬体命令（cuina）

  cuina            查看体修面板（阶段/点数/加成）
  cuina lian       运功淬体，积累淬体点数

体修四段（COMBAT_SYSTEM §三）：
  基础炼体   象甲功        炼气期即可
  法体双修   梵圣真魔功    需筑基期 + 象甲功 50 级（exert santou_liubi 现三头六臂法相）
  极限炼体   涅槃圣体      需元婴期 + 梵圣真魔功 50 级
  毒体修炼   万毒混元身    需结丹期 + 涅槃圣体 50 级（空手自带五毒，抗毒更强）

淬体点数收益：气血上限、固定减伤、空手伤害、重剑伤害（体剑双修）。
阶段越高，可积累的点数上限越高（200/450/800/1200）。
HELP
    );
    return 1;
}
