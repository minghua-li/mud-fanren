// buzhen.c
// 布阵命令 — 在战斗中布置阵法
// Created for #29 子任务：技能组合与阵法系统

#include <ansi.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    object *team;
    int formation_type;

    if ( me->is_busy() )
        return notify_fail("你正在忙，无法布阵！\n");

    if ( me->query_temp("formation/type") )
        return notify_fail("你已经在一个阵法中了！\n");

    if ( !arg || arg == "" )
        return notify_fail("指令格式：buzhen <阵型名称> 或 buzhen ? 查看可用的阵型列表\n");

    // 帮助
    if ( arg == "?" || arg == "list" )
    {
        write("可用的阵法列表：\n");
        write("  buzhen juling       — 聚灵阵（辅助，每回合恢复法力）\n");
        write("  buzhen diandao      — 颠倒五行阵（防御+幻境，五行伤害减免）\n");
        write("  buzhen tiangang     — 天罡北斗阵（防御，全员防御提升）\n");
        write("  buzhen dageng       — 大庚剑阵（攻击，持续剑气伤害）\n");
        write("  buzhen sancai       — 三才阵（组队3人，前排防御后排输出）\n");
        write("  buzhen sixiang      — 四象阵（组队4人，属性加成）\n");
        write("  buzhen wuxing       — 五行阵（组队5人，五行相生连锁）\n");
        write("\n");
        write("单人阵法需要较高修为和阵法熟练度。\n");
        write("组队阵法需要队伍成员达到对应人数。\n");
        return 1;
    }

    // 解析阵型参数
    switch ( arg )
    {
    case "juling":
        formation_type = 1;   // FORMATION_JULING
        break;
    case "diandao":
        formation_type = 2;   // FORMATION_DIANDAO
        break;
    case "tiangang":
        formation_type = 3;   // FORMATION_TIANGANG
        break;
    case "dageng":
        formation_type = 4;   // FORMATION_DAGENG
        break;
    case "sancai":
        formation_type = 5;   // FORMATION_SANCAI
        break;
    case "sixiang":
        formation_type = 6;   // FORMATION_SIXIANG
        break;
    case "wuxing":
        formation_type = 7;   // FORMATION_WUXING
        break;
    default:
        return notify_fail("未知的阵法。用 buzhen ? 查看可用阵法。\n");
    }

    // 检查是否在战斗中（单人阵可在战斗外布置）
    if ( formation_type <= 4 && !me->is_fighting() )
    {
        // 预设阵（非战斗时也可布置）
        team = ({ me });
    }
    else if ( formation_type >= 5 )
    {
        // 组队阵型：必须和队友一起
        team = me->query_team();
        if ( !arrayp(team) || sizeof(team) < formation_type - 2 )
        {
            // 三才=3人, 四象=4人, 五行=5人
            int req = (formation_type == 5) ? 3 : (formation_type == 6 ? 4 : 5);
            return notify_fail(sprintf("「%s」需要 %d 名队员才能布阵！\n",
                ({"三才阵", "四象阵", "五行阵"})[formation_type - 5], req));
        }

        // 阵眼必须是队长
        if ( team[0] != me )
            return notify_fail("只有队长才能布置阵法！\n");

        // 通知所有队员同意
        write("你开始召集队友布阵...（当前仅支持队长直接布阵）\n");
    }
    else
    {
        // 单人阵：需要正在战斗中才能临场布置
        if ( !me->is_fighting() )
            return notify_fail("临场阵法只能在战斗中布置！\n");

        team = ({ me });
    }

    // 检查境界要求（根据不同阵型有不同要求）
    int realm = me->query("realm_level");
    if ( !realm ) realm = 1; // 默认炼气初期

    if ( formation_type == 4 && realm < 4 )   // FORMATION_DAGENG
        return notify_fail("大庚剑阵需要筑基期以上修为才能布置！\n");

    if ( formation_type == 2 && realm < 3 )   // FORMATION_DIANDAO
        return notify_fail("颠倒五行阵需要炼气后期以上修为！\n");

    // 执行布阵
    if ( me->start_formation(formation_type, team) )
    {
        me->start_busy(1);
        message_vision(HIY "$N" HIY "开始布置阵法！\n" NOR, me);
        return 1;
    }

    return notify_fail("布阵失败！\n");
}

int help(object me)
{
    write(@HELP
指令格式：buzhen <阵型名称>

在战斗中布置阵法。阵法分为单人阵和组队阵两种。

单人阵（战斗中布置）：
  buzhen juling     聚灵阵         — 每回合恢复法力
  buzhen diandao    颠倒五行阵     — 五行伤害减免
  buzhen tiangang   天罡北斗阵     — 全员防御提升，分摊伤害
  buzhen dageng     大庚剑阵       — 持续剑气伤害

组队阵（需队长布置，队员自动加入）：
  buzhen sancai     三才阵         — 3人，前排防御后排输出
  buzhen sixiang    四象阵         — 4人，属性加成
  buzhen wuxing     五行阵         — 5人，五行相生连锁

使用 chezhen 命令撤阵。
HELP
    );
    return 1;
}
