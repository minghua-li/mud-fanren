// liandan.c
// 炼丹命令（#73：#64 子单 丹药炼制链路）
//
// 用法：
//   liandan            —— 列出当前可炼制的丹方（含材料/成功率/等级门槛）
//   liandan <丹方名/id> —— 炼制指定丹药
//   liandan help       —— 帮助
//
// 炼丹不受地点限制；身处丹房并激活丹房设施 buff 时，炼丹成功率获得
// 丹房加成（#60 SECT_FACILITY_D->query_danfang_bonus，本命令消费该钩子）。
// 药材从身上扣除（坊市 buy / 灵田种植获得，material_id 匹配）。

#include <ansi.h>
#include <globals.h>
#include <pill.h>
#include <spirit_root.h>

inherit F_CLEAN_UP;

int help(object me);

int main(object me, string arg)
{
    string id;

    seteuid(getuid());

    if (stringp(arg) && arg == "help")
        return help(me);

    if (me->is_busy() || me->is_fighting())
        return notify_fail("你正忙着呢，无法炼丹。\n");

    // 无参：列出丹方
    if (!stringp(arg) || arg == "")
        return list_danfang(me);

    // 解析丹方（支持中文名）
    id = arg;
    if (!PILL_D->query_danfang(id))
    {
        id = PILL_D->query_danfang_id(arg);
        if (!id)
            return notify_fail("没有这种丹方。输入 liandan 查看可炼制的丹方。\n");
    }

    // 炼丹术等级门槛
    mapping df = PILL_D->query_danfang(id);
    if (PILL_D->query_refine_level(me) < df["refine_level"])
        return notify_fail(sprintf("你炼丹术等级不足：炼制%s需炼丹术 %d 级（当前 %d 级）。\n",
                    df["name"], df["refine_level"],
                    PILL_D->query_refine_level(me)));

    if (PILL_D->refine_pill(me, id))
        write("炼丹成功！输入 inventory 查看收获。\n");
    else
        write("本次炼制未能成丹，材料已消耗。\n");
    return 1;
}

// 列出丹方
int list_danfang(object me)
{
    string *ids;
    string msg, id, *mats;
    mapping df;
    int i, j, need;

    ids = PILL_D->query_danfang_ids();
    msg = HIC "═══ 丹方一览（炼丹术 " + PILL_D->query_refine_level(me) + " 级）═══\n" NOR;

    for (i = 0; i < sizeof(ids); i++)
    {
        id = ids[i];
        df = PILL_D->query_danfang(id);
        msg += sprintf("  %-10s %s　所需炼丹术 %d 级　成功率约 %d%%\n",
                id, df["name"], df["refine_level"], PILL_D->query_success_rate(me, id));
        mats = keys(df["ingredients"]);
        msg += "        药材：";
        for (j = 0; j < sizeof(mats); j++)
            msg += sprintf("%s×%d%s", mats[j], df["ingredients"][mats[j]],
                    (j < sizeof(mats) - 1 ? "、" : ""));
        msg += "\n";
    }
    msg += "输入 liandan <丹方名> 炼制。药材可从坊市购买（太南谷坊市 buy）。\n";
    write(msg);
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式 : liandan [丹方名]

炼丹之术，以灵草为引，炉火为媒。收集药材、知晓丹方后，即可尝试炼制。

liandan            —— 列出当前可炼制的丹方
liandan 筑基丹     —— 炼制筑基丹（丹方名或拼音 id 均可）

说明：
- 炼丹需消耗对应药材（身上扣除），失败则材料作废
- 炼丹术等级随成功炼制次数提升，等级越高成功率越高
- 身处门派丹房并激活丹房设施 buff 时，成功率额外加成
- 药材可在太南谷坊市购买（buy lingcao / buy huanglongcao）
HELP
    );
    return 1;
}
