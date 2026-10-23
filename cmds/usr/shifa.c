// shifa.c
// 五行法术施法命令（#76：1F §1.1 五行基础法术落地）
//
// 用法：
//   shifa                   —— 列出已学法术（品阶/属性/法力/冷却/效果/等级）
//   shifa <法术>            —— 对当前战斗对手施展
//   shifa <法术> <目标>     —— 对指定目标施展（可写 id 或中文名）
//   shifa help              —— 帮助
//
// 施法结算走 SPELL_D：法力消耗（neili）/冷却/境界门槛（#61）/五行相克（#28）/
// 境界压制与灵根加成（#78，同一套原语）/附加效果（灼烧·减速·束缚·护体·破甲）。
// 法术通过门派 learn（传功长老）或 sect learn 习得，施法成功积累熟练度。

#include <ansi.h>
#include <globals.h>
#include <spell.h>

inherit F_SSERVER;

int help(object me);

int main(object me, string arg)
{
    string id, target_arg;
    object target;
    mapping info;

    seteuid(getuid());

    if (stringp(arg) && (arg == "help" || arg == "?"))
        return help(me);

    if (!stringp(arg) || arg == "")
        return list_spells(me);

    // 解析法术名与可选目标
    target = 0;
    if (sscanf(arg, "%s %s", id, target_arg) == 2)
    {
        target = present(target_arg, environment(me));
        if (!objectp(target))
            return notify_fail("这里没有 " + target_arg + " 这个人。\n");
    }
    else
        id = arg;

    id = SPELL_D->resolve_spell_id(id);
    if (!stringp(id))
        return notify_fail("没有这种法术。输入 shifa 查看已学法术。\n");

    info = SPELL_D->query_spell_info(id);
    if (!mapp(info))
        return notify_fail("没有这种法术。输入 shifa 查看已学法术。\n");

    // 目标取舍：防御/辅助法术只能对自己
    if (info["type"] == SPELL_TYPE_DEFEND)
        target = me;
    else if (!objectp(target))
        target = offensive_target(me);

    if (info["type"] != SPELL_TYPE_DEFEND)
    {
        if (!objectp(target))
            return notify_fail("这里没有你要攻击的对象。\n");
        if (target == me)
            return notify_fail("你不能用法术攻击自己。\n");
        if (!target->is_character())
            return notify_fail("看清楚了，" + target->name() + "可不是个活物。\n");
        if (!living(target))
            return notify_fail(target->name() + "已经无法战斗了。\n");
        if (environment(target) != environment(me))
            return notify_fail("对方不在你身边。\n");
        if (environment(me)->query("no_fight"))
            return notify_fail("这里禁止战斗。\n");
        if (target->query("no_fight"))
            return notify_fail("对他施法没什么好处。\n");
        if (target->query("uni_target") && target->query("uni_target") != me)
            return notify_fail(target->name() + "不愿与你较量。\n");
    }

    return SPELL_D->cast_spell(me, id, target);
}

// 已学法术列表
int list_spells(object me)
{
    mapping *list;
    mapping info;
    string msg, level_name;
    int i, cd;

    list = SPELL_D->query_player_spells(me);
    if (!sizeof(list))
    {
        write("你尚未修习任何法术。可向本门传功长老学(learn)或输入 sect skills 查看本门法术。\n");
        return 1;
    }

    msg = HIC "═══ 已学法术 ═══\n" NOR;
    for (i = 0; i < sizeof(list); i++)
    {
        info = list[i];
        if (info["level"] == SPELL_HIGH) level_name = "初级上阶";
        else if (info["level"] == SPELL_MID) level_name = "初级中阶";
        else level_name = "初级下阶";

        cd = SPELL_D->query_cooldown(me, info["id"]);
        msg += sprintf("  %s%-4s%s %s%s%s %s  等级 %d  法力 %d  冷却 %d 秒%s\n",
                       HIG, info["name"], NOR,
                       query_element_color(spell_element_id(info["element"])),
                       info["element"], NOR,
                       level_name,
                       info["skill_level"],
                       SPELL_D->query_spell_cost(me, info),
                       info["cooldown"],
                       cd > 0 ? sprintf("（%d 秒后可再用）", cd) : "");
        msg += "      " + info["desc"] + "\n";
    }
    write(msg);
    write("用法：shifa <法术> [目标]\n");
    return 1;
}

int help(object me)
{
    write(@HELP
五行法术施法命令（shifa）

  shifa                    列出已学法术
  shifa <法术>             对当前战斗对手施展
  shifa <法术> <目标>      对指定目标施展

法术品阶与修为门槛（1F §1.1）：
  初级下阶（火弹术/冰箭术/流沙术/金刃术）      炼气期即可
  初级中阶（火花术/缠绕术/地刺术）              需炼气中期
  初级上阶（土墙术）                            需炼气后期

附加效果：灼烧（持续伤害）、减速（身法下降）、束缚（身法/招架下降 + 迟滞）、
          护体（自身减伤）、破甲（额外穿透）。

法术通过门派 learn（传功长老）或 sect learn 习得；施法命中可积累熟练度。
HELP
    );
    return 1;
}
