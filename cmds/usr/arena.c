// arena.c
// 演武场命令 - 玩家PVP交互入口
// 用法:
//   arena            - 查看自己的竞技场信息
//   arena queue      - 进入匹配队列
//   arena leave      - 离开匹配队列
//   arena challenge  <玩家> - 挑战指定玩家
//   arena accept     - 接受挑战
//   arena rank       - 查看段位排行
//   arena history    - 查看战绩简史

#include <ansi.h>
#include <pvp.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    string cmd, target_name;

    if (!arg || arg == "")
    {
        // 显示个人状态
        write(PVP_D->query_arena_status(me));
        return 1;
    }

    if (sscanf(arg, "%s %s", cmd, target_name) == 2)
    {
        // 带参数命令
        if (cmd == "challenge")
        {
            object target;

            target = find_player(target_name);
            if (!objectp(target))
            {
                write("找不到这个玩家。\n");
                return 1;
            }

            if (!PVP_D->do_challenge(me, target))
                return 1;

            return 1;
        }
    }

    // 单参数命令
    switch (arg)
    {
    case "queue":
        return cmd_queue(me);
    case "leave":
        return cmd_leave(me);
    case "accept":
        return cmd_accept(me);
    case "rank":
        return cmd_rank(me);
    case "history":
        return cmd_history(me);
    default:
        write("arena 命令用法：\n"
              "  arena              - 查看竞技场信息\n"
              "  arena queue        - 加入匹配队列\n"
              "  arena leave        - 离开匹配队列\n"
              "  arena challenge <id> - 挑战玩家\n"
              "  arena accept       - 接受挑战\n"
              "  arena rank         - 段位排行\n"
              "  arena history      - 战绩简史\n");
        return 1;
    }
}

// 加入匹配队列
int cmd_queue(object me)
{
    if (me->is_fighting())
    {
        write("你正在战斗中，无法加入匹配。\n");
        return 1;
    }

    if (me->query_temp(PVP_TEMP_IN_ARENA))
    {
        write("你正在演武场中，无法加入匹配。\n");
        return 1;
    }

    PVP_D->join_arena_queue(me);
    return 1;
}

// 离开匹配队列
int cmd_leave(object me)
{
    PVP_D->leave_arena_queue(me);
    return 1;
}

// 接受挑战
int cmd_accept(object me)
{
    PVP_D->accept_challenge(me);
    return 1;
}

// 查看段位排行
int cmd_rank(object me)
{
    mapping *list;
    int i, max_show;

    list = PVP_D->get_ranking_list();
    if (!arrayp(list) || sizeof(list) == 0)
    {
        write("暂无排行数据。\n");
        return 1;
    }

    max_show = (sizeof(list) > 20 ? 20 : sizeof(list));

    write(HIY "\n≡" HIC "═══════════ 演武场段位排行 ═══════════" HIY "≡\n" NOR);
    write(HIW "  排名  玩家ID                         段位              积分\n" NOR);
    write("  ─────────────────────────────────────────────\n");

    for (i = 0; i < max_show; i++)
    {
        string rank_color, rank_name, rank_display;
        int rank_idx;

        rank_idx = list[i]["rank"];

        if (rank_idx < 0 || rank_idx >= PVP_RANK_COUNT)
        {
            rank_color = NOR;
            rank_name = "未知";
        }
        else
        {
            rank_color = PVP_D->get_rank_color(rank_idx);
            rank_name = PVP_D->get_rank_name(rank_idx);
        }

        rank_display = rank_color + rank_name + NOR;

        write(sprintf("  %-4d  %-32s %s%s%s  %d\n",
            i + 1,
            list[i]["id"],
            rank_color, rank_name, NOR,
            list[i]["score"]));
    }

    write(HIY "≡" HIC "════════════════════════════════════════" HIY "≡\n\n" NOR);
    return 1;
}

// 查看战绩简史
int cmd_history(object me)
{
    mapping data;

    data = PVP_D->get_player_data(me);
    if (!mapp(data))
    {
        write("你还没有任何竞技场记录。\n");
        return 1;
    }

    write(HIC "\n≡ 个人战绩 ≡\n" NOR);
    write(sprintf("  总场次：%d\n", data[PVP_DATA_TOTAL_FIGHTS]));
    write(sprintf("  胜场：%d  负场：%d\n",
        data[PVP_DATA_WINS], data[PVP_DATA_LOSSES]));

    if (data[PVP_DATA_TOTAL_FIGHTS] > 0)
    {
        int win_rate = data[PVP_DATA_WINS] * 100 / data[PVP_DATA_TOTAL_FIGHTS];
        write(sprintf("  胜率：%d%%\n", win_rate));
    }

    write(sprintf("  当前连胜/连败：%d\n", data[PVP_DATA_STREAK]));
    write(sprintf("  最高段位：%s%s\n" NOR,
        PVP_D->get_rank_color(data[PVP_DATA_BEST_RANK]),
        PVP_D->get_rank_name(data[PVP_DATA_BEST_RANK])));
    write("\n");

    return 1;
}

int help(object me)
{
    write(@HELP
arena 命令 - 演武场竞技系统

用法:
  arena              - 查看自己的竞技场信息
  arena queue        - 加入匹配队列，自动寻找对手
  arena leave        - 离开匹配队列
  arena challenge <id> - 挑战指定玩家
  arena accept       - 接受他人的挑战
  arena rank         - 查看段位排行
  arena history      - 查看个人战绩简史

演武场采用段位匹配制，上限为第7阶「渡劫真仙」。
赛季周期30天，赛季结束根据段位发放灵石奖励。
HELP
    );
    return 1;
}
