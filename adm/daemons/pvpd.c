// pvpd.c
// 凡人修仙传 MUD - PVP 守护进程
// 负责竞技场匹配、段位管理、积分计算、赛季周期、奖励发放

#include <ansi.h>
#include <pvp.h>
#include <localtime.h>

inherit F_DBASE;
inherit F_SAVE;

// ========== 数据结构 ==========

// 玩家段位数据: mapping id -> mapping
//   score:        int    当前积分
//   rank:         int    当前段位索引 (0-6)
//   wins:         int    总胜场
//   losses:       int    总负场
//   streak:       int    当前连胜/连败数 (正=连胜, 负=连败)
//   best_rank:    int    历史最高段位索引
//   season_wins:  int    本赛季胜场
//   total_fights: int    总战斗场次
//   last_fight:   int    最后战斗时间戳
nosave mapping player_data = ([]);

// 匹配队列: ({ player_object, ... })
nosave object *match_queue = ({});
// 队列加入时间: object -> time
nosave mapping queue_times = ([]);

// 赛季信息
nosave int current_season = 1;
nosave int season_start = 0;
nosave int season_end = 0;

// 防止重复处理
nosave mapping fighting_players = ([]);

// ========== 保存/恢复 ==========

string query_save_file()
{
    return "/data/pvpd";
}

void create()
{
    seteuid(getuid());
    set("channel_id", HIC "演武场" NOR);
    set("name", "PVP守护进程");

    restore();

    // 检查赛季状态
    check_season();

    // 启动匹配定时器（每5秒检查一次匹配队列）
    set heartbeat(1);
}

// 心跳 - 匹配队列处理
void heart_beat()
{
    if (sizeof(match_queue) >= 2)
        process_matchmaking();
}

// ========== 赛季管理 ==========

void check_season()
{
    int now = time();

    // 首次启动，初始化赛季
    if (!season_start)
    {
        current_season = 1;
        season_start = now;
        season_end = now + PVP_SEASON_DAYS * 86400;
        save();
        return;
    }

    // 检查是否需要结束赛季
    if (now >= season_end)
        end_season();
}

void end_season()
{
    string *ids;
    mapping data;
    int i, rank_idx, reward;
    object player;

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIY "【演武场】第%d赛季已结束，开始结算奖励...\n" NOR, current_season));

    // 计算所有在线玩家的赛季奖励
    ids = keys(player_data);
    for (i = 0; i < sizeof(ids); i++)
    {
        data = player_data[ids[i]];
        if (!mapp(data))
            continue;

        rank_idx = data[PVP_DATA_RANK];
        reward = PVP_RANK_REWARDS[rank_idx] +
                 data[PVP_DATA_SEASON_WINS] * 5;

        // 在线玩家直接发放
        player = find_player(ids[i]);
        if (objectp(player))
        {
            MONEY_D->pay_player(player, reward);
            tell_object(player, HIY "\n========= 赛季结算 =========\n" NOR);
            tell_object(player, sprintf("第%d赛季结束，你获得灵石 × %d。\n",
                current_season, reward));
            tell_object(player, sprintf("当前段位：%s（积分：%d）\n",
                PVP_RANK_NAMES[rank_idx], data[PVP_DATA_SCORE]));
            if (data[PVP_DATA_BEST_RANK] > rank_idx)
                tell_object(player, sprintf("历史最高：%s\n",
                    PVP_RANK_NAMES[data[PVP_DATA_BEST_RANK]]));
            tell_object(player, HIC "===========================\n\n" NOR);
        }
    }

    // 赛季重置：保留一半积分
    ids = keys(player_data);
    for (i = 0; i < sizeof(ids); i++)
    {
        data = player_data[ids[i]];
        if (!mapp(data))
            continue;

        data[PVP_DATA_SCORE] = data[PVP_DATA_SCORE] * PVP_SEASON_RESERVE_RATIO / 100;
        if (data[PVP_DATA_SCORE] < 0)
            data[PVP_DATA_SCORE] = 0;

        data[PVP_DATA_RANK] = calc_rank_by_score(data[PVP_DATA_SCORE]);
        data[PVP_DATA_SEASON_WINS] = 0;
        data[PVP_DATA_STREAK] = 0;
    }

    // 进入下一赛季
    current_season++;
    season_start = time();
    season_end = season_start + PVP_SEASON_DAYS * 86400;

    save();

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIY "【演武场】第%d赛季现已开始！祝各位道友仙运昌隆！\n" NOR,
            current_season));
}

int query_season() { return current_season; }
int query_season_end() { return season_end; }
int query_season_remaining()
{
    return (season_end - time()) / 86400;
}

// ========== 段位计算 ==========

int calc_rank_by_score(int score)
{
    int i;

    for (i = PVP_RANK_COUNT - 1; i >= 0; i--)
    {
        if (score >= PVP_RANK_THRESHOLDS[i])
            return i;
    }
    return 0;
}

string get_rank_name(int rank_idx)
{
    if (rank_idx < 0 || rank_idx >= PVP_RANK_COUNT)
        return "无名之辈";
    return PVP_RANK_NAMES[rank_idx];
}

string get_rank_color(int rank_idx)
{
    switch (rank_idx)
    {
    case 0: return CYN;     // 炼气
    case 1: return GRN;     // 筑基
    case 2: return HIC;     // 结丹
    case 3: return HIG;     // 元婴
    case 4: return HIY;     // 化神
    case 5: return HIR;     // 大乘
    case 6: return HIM;     // 渡劫
    default: return NOR;
    }
}

// ========== 玩家数据管理 ==========

mapping get_player_data(object player)
{
    string id;
    mapping data;

    if (!objectp(player))
        return 0;

    id = player->query("id");
    if (!stringp(id))
        return 0;

    if (!mapp(player_data[id]))
    {
        // 初始化新玩家数据
        player_data[id] = ([
            PVP_DATA_SCORE:         0,
            PVP_DATA_RANK:          0,
            PVP_DATA_WINS:          0,
            PVP_DATA_LOSSES:        0,
            PVP_DATA_STREAK:        0,
            PVP_DATA_BEST_RANK:     0,
            PVP_DATA_SEASON_WINS:   0,
            PVP_DATA_TOTAL_FIGHTS:  0,
            PVP_DATA_LAST_FIGHT:    0,
        ]);
    }

    return player_data[id];
}

void save_player_data(object player)
{
    save();
}

string query_arena_status(object player)
{
    mapping data;
    int rank_idx, score, streak;

    data = get_player_data(player);
    if (!mapp(data))
        return "暂无竞技场记录。\n";

    score = data[PVP_DATA_SCORE];
    rank_idx = data[PVP_DATA_RANK];
    streak = data[PVP_DATA_STREAK];

    return sprintf(
        HIC "≡" HIY "═══════════ 演武场信息 ═══════════" HIC "≡\n" NOR
        "  段位：%s%s" NOR "\n"
        "  积分：%d\n"
        "  总场次：%d  胜：%d  负：%d\n"
        "  连胜/连败：%s%d%s\n"
        "  历史最高：%s%s" NOR "\n"
        "  本赛季胜场：%d\n"
        "  赛季剩余：%d天\n"
        HIC "≡" HIY "══════════════════════════════════" HIC "≡\n" NOR,
        get_rank_color(rank_idx), get_rank_name(rank_idx),
        score,
        data[PVP_DATA_TOTAL_FIGHTS], data[PVP_DATA_WINS], data[PVP_DATA_LOSSES],
        (streak > 0 ? HIG : HIR), streak, NOR,
        get_rank_color(data[PVP_DATA_BEST_RANK]), get_rank_name(data[PVP_DATA_BEST_RANK]),
        data[PVP_DATA_SEASON_WINS],
        query_season_remaining()
    );
}

// ========== 匹配队列管理 ==========

int join_arena_queue(object player)
{
    if (!objectp(player))
        return 0;

    if (!userp(player))
        return 0;

    // 检查是否已在队列中
    if (member_array(player, match_queue) != -1)
    {
        tell_object(player, "你已经在匹配队列中了。\n");
        return 0;
    }

    // 检查是否已在战斗
    if (player->is_fighting())
    {
        tell_object(player, "你正在战斗中，无法加入匹配。\n");
        return 0;
    }

    if (player->query_temp(PVP_TEMP_IN_ARENA))
    {
        tell_object(player, "你正在演武场中。\n");
        return 0;
    }

    // 加入队列
    match_queue += ({ player });
    queue_times[player] = time();

    player->set_temp(PVP_TEMP_IN_QUEUE, 1);
    player->set_temp(PVP_TEMP_QUEUE_TIME, time());

    tell_object(player, HIC "你已加入演武场匹配队列，等待对手中...\n" NOR);

    // 尝试立即匹配
    if (sizeof(match_queue) >= 2)
        process_matchmaking();

    return 1;
}

int leave_arena_queue(object player)
{
    int idx;

    if (!objectp(player))
        return 0;

    idx = member_array(player, match_queue);
    if (idx == -1)
    {
        tell_object(player, "你不在匹配队列中。\n");
        return 0;
    }

    match_queue = match_queue[0..idx-1] + match_queue[idx+1..];
    map_delete(queue_times, player);

    player->delete_temp(PVP_TEMP_IN_QUEUE);
    player->delete_temp(PVP_TEMP_QUEUE_TIME);

    tell_object(player, "你已退出匹配队列。\n");
    return 1;
}

int query_in_queue(object player)
{
    return (member_array(player, match_queue) != -1);
}

// ========== 匹配处理 ==========

void process_matchmaking()
{
    int i, j, now;
    object a, b;
    int rank_a, rank_b, range, wait_a, wait_b;

    if (sizeof(match_queue) < 2)
        return;

    now = time();

    // 简单匹配：遍历队列找配对
    for (i = 0; i < sizeof(match_queue); i++)
    {
        a = match_queue[i];
        if (!objectp(a) || !living(a) || a->is_fighting())
        {
            // 清理无效对象
            match_queue = match_queue[0..i-1] + match_queue[i+1..];
            map_delete(queue_times, a);
            i--;
            continue;
        }

        rank_a = get_player_data(a)[PVP_DATA_RANK];
        wait_a = now - queue_times[a];

        // 根据等待时间确定搜索范围
        if (wait_a >= PVP_QUEUE_WIDE_TIMEOUT)
            range = PVP_MATCH_RANGE_ANY;
        else if (wait_a >= PVP_QUEUE_TIMEOUT)
            range = PVP_MATCH_RANGE_WIDE;
        else
            range = PVP_MATCH_RANGE_NARROW;

        // 找配对的对手
        for (j = i + 1; j < sizeof(match_queue); j++)
        {
            b = match_queue[j];
            if (!objectp(b) || !living(b) || b->is_fighting())
            {
                match_queue = match_queue[0..j-1] + match_queue[j+1..];
                map_delete(queue_times, b);
                j--;
                continue;
            }

            rank_b = get_player_data(b)[PVP_DATA_RANK];
            wait_b = now - queue_times[b];

            // 检查段位差是否在搜索范围内
            if (abs(rank_a - rank_b) <= range)
            {
                // 配对成功！
                match_queue = match_queue[0..i-1] +
                              match_queue[i+1..j-1] +
                              match_queue[j+1..];

                map_delete(queue_times, a);
                map_delete(queue_times, b);

                a->delete_temp(PVP_TEMP_IN_QUEUE);
                a->delete_temp(PVP_TEMP_QUEUE_TIME);
                b->delete_temp(PVP_TEMP_IN_QUEUE);
                b->delete_temp(PVP_TEMP_QUEUE_TIME);

                start_arena_fight(a, b);
                return;  // 一次只处理一对
            }
        }
    }
}

// ========== 战斗流程 ==========

void start_arena_fight(object a, object b)
{
    object room;
    object *inv;
    int i;

    if (!objectp(a) || !objectp(b))
    {
        // 如果有一方无效，通知另一方
        if (objectp(a))
            tell_object(a, "匹配失败，对手已离开。\n");
        if (objectp(b))
            tell_object(b, "匹配失败，对手已离开。\n");
        return;
    }

    // 获取或创建演武场房间
    room = find_object(PVP_ARENA_ROOM);
    if (!room)
        room = load_object(PVP_ARENA_ROOM);
    if (!room)
    {
        tell_object(a, "演武场暂时无法使用，请稍后再试。\n");
        tell_object(b, "演武场暂时无法使用，请稍后再试。\n");
        return;
    }

    // 清理房间里的其他人
    inv = all_inventory(room);
    for (i = 0; i < sizeof(inv); i++)
    {
        if (userp(inv[i]) && inv[i] != a && inv[i] != b)
        {
            tell_object(inv[i], "你被传送出演武场。\n");
            inv[i]->move(VOID_OB);
        }
    }

    // 记录玩家原位置
    a->set_temp(PVP_TEMP_ORIGIN_ROOM, base_name(environment(a)));
    b->set_temp(PVP_TEMP_ORIGIN_ROOM, base_name(environment(b)));

    // 标记竞技场状态
    a->set_temp(PVP_TEMP_IN_ARENA, 1);
    b->set_temp(PVP_TEMP_IN_ARENA, 1);
    fighting_players[a] = b;
    fighting_players[b] = a;

    // 传送玩家
    a->move(room);
    b->move(room);

    message_vision(HIY "\n※※※ 演武场对决 ※※※\n" NOR, a);
    message_vision(HIC "【" HIR + a->name(1) + HIC "】 VS 【" HIC + b->name(1) + HIC "】\n" NOR, a);
    message_vision(sprintf(HIW "准备时间 %d 秒...\n\n" NOR, PVP_ARENA_PREP_TIME), a);

    // 准备倒计时
    a->start_busy(PVP_ARENA_PREP_TIME);
    b->start_busy(PVP_ARENA_PREP_TIME);

    // 5秒后开始战斗
    call_out("begin_arena_fight", PVP_ARENA_PREP_TIME, a, b);
}

void begin_arena_fight(object a, object b)
{
    if (!objectp(a) || !objectp(b))
    {
        if (objectp(a)) return_player(a);
        if (objectp(b)) return_player(b);
        return;
    }

    if (!living(a) || !living(b))
    {
        if (living(a))
            PVP_D->end_arena_fight(a, b);
        else
            PVP_D->end_arena_fight(b, a);
        return;
    }

    message_vision(HIR "\n※ 战斗开始！※\n\n" NOR, a);

    a->stop_busy();
    b->stop_busy();

    // 让双方互相攻击
    a->kill_ob(b);
    b->kill_ob(a);
}

// ========== 战斗结算 ==========

void end_arena_fight(object winner, object loser)
{
    mapping win_data, lose_data;
    int score_change, winner_gain, loser_loss;
    int winner_rank, loser_rank, winner_streak;
    string winner_name, loser_name;

    // 处理断线/无效情况
    if (!objectp(winner) && !objectp(loser))
        return;

    // 如果只有获胜者（对手断线）
    if (!objectp(loser) && objectp(winner))
    {
        tell_object(winner, HIG "对手已离开，你获得胜利！\n" NOR);
        win_data = get_player_data(winner);
        if (mapp(win_data))
        {
            win_data[PVP_DATA_WINS]++;
            win_data[PVP_DATA_TOTAL_FIGHTS]++;
            win_data[PVP_DATA_SEASON_WINS]++;
            win_data[PVP_DATA_SCORE] +=
                PVP_SCORE_WIN_BASE / 2;  // 断线胜利减半
            if (win_data[PVP_DATA_SCORE] > PVP_SCORE_MAX)
                win_data[PVP_DATA_SCORE] = PVP_SCORE_MAX;
            win_data[PVP_DATA_RANK] = calc_rank_by_score(win_data[PVP_DATA_SCORE]);
            if (win_data[PVP_DATA_RANK] > win_data[PVP_DATA_BEST_RANK])
                win_data[PVP_DATA_BEST_RANK] = win_data[PVP_DATA_RANK];
            win_data[PVP_DATA_STREAK] =
                (win_data[PVP_DATA_STREAK] > 0 ? win_data[PVP_DATA_STREAK] + 1 : 1);
        }
        return_player(winner);
        map_delete(fighting_players, winner);
        save();
        return;
    }

    // 检查是否都是同一个房间的
    if (objectp(winner) && objectp(loser) &&
        environment(winner) != environment(loser))
    {
        // 不在同一个房间，可能是被拉走了
        return_player(winner);
        return_player(loser);
        map_delete(fighting_players, winner);
        map_delete(fighting_players, loser);
        return;
    }

    // 双方都在，正常计算
    if (!objectp(winner))
    {
        // 都死了或双输的情况（winner为0表示平局或异常）
        if (objectp(loser))
        {
            tell_object(loser, HIR "战斗异常结束。\n" NOR);
            return_player(loser);
        }
        map_delete(fighting_players, loser);
        return;
    }

    winner_name = winner->name(1);
    loser_name = loser->name(1);

    win_data = get_player_data(winner);
    lose_data = get_player_data(loser);

    if (!mapp(win_data) || !mapp(lose_data))
    {
        return_player(winner);
        return_player(loser);
        return;
    }

    winner_rank = win_data[PVP_DATA_RANK];
    loser_rank = lose_data[PVP_DATA_RANK];

    // 计算积分变化
    winner_gain = PVP_SCORE_WIN_BASE;
    loser_loss = PVP_SCORE_LOSE_BASE;

    // 对手段位更高，胜者额外加分
    if (loser_rank > winner_rank)
        winner_gain += (loser_rank - winner_rank) * PVP_SCORE_WIN_HIGHER;
    // 对手段位更低，败者少扣分
    if (winner_rank > loser_rank)
        loser_loss += (winner_rank - loser_rank) * PVP_SCORE_LOSE_LOWER;

    // 连胜奖励
    winner_streak = win_data[PVP_DATA_STREAK];
    if (winner_streak >= 10)
        winner_gain += PVP_STREAK_10_BONUS;
    else if (winner_streak >= 5)
        winner_gain += PVP_STREAK_5_BONUS;
    else if (winner_streak >= 3)
        winner_gain += PVP_STREAK_3_BONUS;

    // 应用积分变化
    win_data[PVP_DATA_SCORE] += winner_gain;
    if (win_data[PVP_DATA_SCORE] > PVP_SCORE_MAX)
        win_data[PVP_DATA_SCORE] = PVP_SCORE_MAX;

    lose_data[PVP_DATA_SCORE] -= loser_loss;
    if (lose_data[PVP_DATA_SCORE] < PVP_SCORE_MIN)
        lose_data[PVP_DATA_SCORE] = PVP_SCORE_MIN;

    // 更新段位
    win_data[PVP_DATA_RANK] = calc_rank_by_score(win_data[PVP_DATA_SCORE]);
    lose_data[PVP_DATA_RANK] = calc_rank_by_score(lose_data[PVP_DATA_SCORE]);

    // 第1阶保护（不掉阶）
    if (lose_data[PVP_DATA_RANK] < PVP_RANK_PROTECT)
        lose_data[PVP_DATA_RANK] = PVP_RANK_PROTECT;

    // 历史最高
    if (win_data[PVP_DATA_RANK] > win_data[PVP_DATA_BEST_RANK])
        win_data[PVP_DATA_BEST_RANK] = win_data[PVP_DATA_RANK];

    // 更新胜负场
    win_data[PVP_DATA_WINS]++;
    win_data[PVP_DATA_TOTAL_FIGHTS]++;
    win_data[PVP_DATA_SEASON_WINS]++;
    win_data[PVP_DATA_STREAK] =
        (win_data[PVP_DATA_STREAK] > 0 ? win_data[PVP_DATA_STREAK] + 1 : 1);

    lose_data[PVP_DATA_LOSSES]++;
    lose_data[PVP_DATA_TOTAL_FIGHTS]++;
    lose_data[PVP_DATA_STREAK] =
        (lose_data[PVP_DATA_STREAK] < 0 ? lose_data[PVP_DATA_STREAK] - 1 : -1);

    win_data[PVP_DATA_LAST_FIGHT] = time();
    lose_data[PVP_DATA_LAST_FIGHT] = time();

    // 广播战斗结果
    message_vision(HIR "\n========= 战斗结束 =========\n" NOR, winner);

    CHANNEL_D->do_channel(this_object(), "rw",
        sprintf(HIY "【演武场】%s" NOR " 击败了 " HIC "%s" NOR "！",
            winner_name, loser_name));

    // 显示结果详情
    tell_object(winner, sprintf(
        HIG "\n≡ 胜利 ≡\n" NOR
        "  对手：%s\n"
        "  积分：%s%d%s 点（当前：%d）\n"
        "  段位：%s%s" NOR "\n\n",
        loser_name,
        HIG, winner_gain, NOR, win_data[PVP_DATA_SCORE],
        get_rank_color(win_data[PVP_DATA_RANK]),
        get_rank_name(win_data[PVP_DATA_RANK])
    ));

    tell_object(loser, sprintf(
        HIR "\n≡ 败北 ≡\n" NOR
        "  对手：%s\n"
        "  积分：%s%d%s 点（当前：%d）\n"
        "  段位：%s%s" NOR "\n\n",
        winner_name,
        HIR, loser_loss, NOR, lose_data[PVP_DATA_SCORE],
        get_rank_color(lose_data[PVP_DATA_RANK]),
        get_rank_name(lose_data[PVP_DATA_RANK])
    ));

    // 检查段位晋升
    if (win_data[PVP_DATA_RANK] > winner_rank)
    {
        CHANNEL_D->do_channel(this_object(), "rw",
            sprintf(HIY "【演武场】恭喜 %s 晋升至 %s%s" NOR HIY "！" NOR,
                winner_name,
                get_rank_color(win_data[PVP_DATA_RANK]),
                get_rank_name(win_data[PVP_DATA_RANK])));
    }

    // 送玩家返回
    return_player(winner);
    return_player(loser);

    map_delete(fighting_players, winner);
    map_delete(fighting_players, loser);

    save();
}

// ========== 辅助函数 ==========

void return_player(object player)
{
    string origin;
    object room;

    if (!objectp(player))
        return;

    player->delete_temp(PVP_TEMP_IN_ARENA);
    player->remove_all_enemy();
    player->stop_busy();

    origin = player->query_temp(PVP_TEMP_ORIGIN_ROOM);
    if (stringp(origin))
    {
        room = find_object(origin);
        if (room)
        {
            player->move(room);
        }
        else
        {
            // 原房间已销毁，回重生点
            player->move("/d/city/kedian");
        }
        player->delete_temp(PVP_TEMP_ORIGIN_ROOM);
    }
}

void player_disconnect(object player)
{
    // 玩家断线处理
    if (!objectp(player))
        return;

    // 如果在队列中，移除
    if (player->query_temp(PVP_TEMP_IN_QUEUE))
        leave_arena_queue(player);

    // 如果在战斗中
    if (player->query_temp(PVP_TEMP_IN_ARENA))
    {
        object opponent = fighting_players[player];
        if (opponent)
        {
            // 对手获胜
            map_delete(fighting_players, opponent);
            end_arena_fight(opponent, 0);
        }
        return_player(player);
        map_delete(fighting_players, player);
    }
}

// ========== 挑战系统 ==========

int do_challenge(object challenger, object target)
{
    if (!objectp(challenger) || !objectp(target))
        return 0;

    if (challenger == target)
    {
        tell_object(challenger, "你不能挑战自己。\n");
        return 0;
    }

    if (!userp(target))
    {
        tell_object(challenger, "你不能挑战这个对象。\n");
        return 0;
    }

    if (target->is_fighting())
    {
        tell_object(challenger, "对方正在战斗中。\n");
        return 0;
    }

    if (target->query_temp(PVP_TEMP_IN_ARENA))
    {
        tell_object(challenger, "对方正在演武场中。\n");
        return 0;
    }

    // 设置挑战标记
    challenger->set_temp(PVP_TEMP_CHALLENGER, target);
    target->set_temp(PVP_TEMP_CHALLENGER, challenger);

    tell_object(challenger, sprintf("你向 %s 发起了演武挑战！\n", target->name(1)));
    tell_object(target, sprintf(HIR "\n%s 向你发起了演武挑战！\n" NOR
        HIY "输入 arena accept 接受挑战\n" NOR
        "或等待超时自动取消。\n\n", challenger->name(1)));

    // 30秒超时取消
    call_out("cancel_challenge", 30, challenger, target);

    return 1;
}

void cancel_challenge(object challenger, object target)
{
    if (!objectp(challenger) || !objectp(target))
        return;

    if (challenger->query_temp(PVP_TEMP_CHALLENGER) == target)
    {
        challenger->delete_temp(PVP_TEMP_CHALLENGER);
        tell_object(challenger, sprintf("你对 %s 的挑战已超时取消。\n", target->name(1)));
    }

    if (target->query_temp(PVP_TEMP_CHALLENGER) == challenger)
    {
        target->delete_temp(PVP_TEMP_CHALLENGER);
        tell_object(target, sprintf("%s 对你的挑战已超时取消。\n", challenger->name(1)));
    }
}

int accept_challenge(object player)
{
    object challenger;

    if (!objectp(player))
        return 0;

    challenger = player->query_temp(PVP_TEMP_CHALLENGER);
    if (!objectp(challenger))
    {
        tell_object(player, "没有人挑战你。\n");
        return 0;
    }

    if (challenger->query_temp(PVP_TEMP_CHALLENGER) != player)
    {
        tell_object(player, "挑战已取消。\n");
        player->delete_temp(PVP_TEMP_CHALLENGER);
        return 0;
    }

    // 双方确认，开始竞技
    challenger->delete_temp(PVP_TEMP_CHALLENGER);
    player->delete_temp(PVP_TEMP_CHALLENGER);

    start_arena_fight(challenger, player);
    return 1;
}

// ========== 排行查询 ==========

mapping *get_ranking_list()
{
    string *ids;
    mapping *result;
    mapping data;
    int i;

    ids = keys(player_data);
    result = ({});

    for (i = 0; i < sizeof(ids); i++)
    {
        data = player_data[ids[i]];
        if (!mapp(data))
            continue;
        result += ({ ([
            "id":    ids[i],
            "score": data[PVP_DATA_SCORE],
            "rank":  data[PVP_DATA_RANK],
            "wins":  data[PVP_DATA_WINS],
        ]) });
    }

    // 按积分排序
    result = sort_array(result, "sort_by_score", this_object());

    return result;
}

static int sort_by_score(mapping a, mapping b)
{
    if (a["score"] > b["score"]) return -1;
    if (a["score"] < b["score"]) return 1;

    if (a["wins"] > b["wins"]) return -1;
    if (a["wins"] < b["wins"]) return 1;

    return 0;
}

// 为外部工具提供接口
int query_player_rank(object player)
{
    mapping data = get_player_data(player);
    if (!mapp(data))
        return 0;
    return data[PVP_DATA_RANK];
}

int query_player_score(object player)
{
    mapping data = get_player_data(player);
    if (!mapp(data))
        return 0;
    return data[PVP_DATA_SCORE];
}
