// PVP_D.c
// PVP 对战守护进程 - 竞技场匹配、段位系统、赛季管理
// 基于《凡人修仙传》设定，面向 LPC MUD 实现
// Created for #21 核心：战斗机制增强

#include <ansi.h>
#include <pvp.h>
#include <localtime.h>

inherit F_DBASE;

// ===== 匹配队列 =====
// 队列项: ({ player, queue_time, rank_range, mode })
nosave mixed *match_queue = ({});

// ===== 赛季数据 =====
nosave int season_start_time;

void create()
{
    seteuid(getuid());
    set("name", "PVP 竞技场");
    set("id", "pvp_d");

    // 初始化赛季
    season_start_time = time();
}

// ===== 段位查询 =====

// 获取玩家段位
int query_rank(object player)
{
    mapping data = player->query(PVP_DATA_KEY);
    if (!mapp(data))
        return 0;
    return data[PVP_DATA_RANK];
}

// 获取玩家积分
int query_score(object player)
{
    mapping data = player->query(PVP_DATA_KEY);
    if (!mapp(data))
        return 0;
    return data[PVP_DATA_SCORE];
}

// 根据积分计算对应段位
// 返回段位索引 (0 = 第1阶, 6 = 第7阶)
int calc_rank_by_score(int score)
{
    int *thresholds = PVP_RANK_THRESHOLDS;
    int i;

    for (i = PVP_RANK_COUNT - 1; i >= 0; i--)
    {
        if (score >= thresholds[i])
            return i;
    }
    return 0;
}

// 获取段位名称
string get_rank_name(int rank_index)
{
    string *names = PVP_RANK_NAMES;
    if (rank_index < 0) rank_index = 0;
    if (rank_index >= sizeof(names)) rank_index = sizeof(names) - 1;
    return names[rank_index];
}

// ===== 玩家数据初始化 =====

// 初始化或获取玩家的PVP数据
mapping ensure_pvp_data(object player)
{
    mapping data = player->query(PVP_DATA_KEY);
    if (!mapp(data))
    {
        data = ([
            PVP_DATA_SCORE       : 0,
            PVP_DATA_RANK        : 0,
            PVP_DATA_WINS        : 0,
            PVP_DATA_LOSSES      : 0,
            PVP_DATA_STREAK      : 0,
            PVP_DATA_BEST_RANK   : 0,
            PVP_DATA_SEASON_WINS : 0,
            PVP_DATA_TOTAL_FIGHTS: 0,
            PVP_DATA_LAST_FIGHT  : 0,
        ]);
        player->set(PVP_DATA_KEY, data);
    }
    return data;
}

// ===== 匹配系统 =====

// 玩家加入匹配队列
// 返回 1 表示成功加入
int join_queue(object player)
{
    if (!objectp(player) || !userp(player))
        return 0;

    // 检查是否已在队列中
    if (player->query_temp(PVP_TEMP_IN_QUEUE))
    {
        tell_object(player, "你已经在了匹配队列中。\n");
        return 0;
    }

    // 检查是否已在竞技场中
    if (player->query_temp(PVP_TEMP_IN_ARENA))
    {
        tell_object(player, "你已经在竞技场中了。\n");
        return 0;
    }

    // 初始化PVP数据
    ensure_pvp_data(player);

    // 加入队列
    match_queue += ({
        ({
            player,
            time(),
            0,          // 初始搜索范围标志
        })
    });

    player->set_temp(PVP_TEMP_IN_QUEUE, 1);
    player->set_temp(PVP_TEMP_QUEUE_TIME, time());

    int rank_idx = query_rank(player);
    string rank_name = get_rank_name(rank_idx);
    tell_object(player, HIY "你已加入竞技场匹配队列。当前段位: " + rank_name +
                " (积分: " + query_score(player) + ")\n" NOR);
    tell_object(player, "系统正在为你寻找合适的对手...\n");

    // 设置超时检测
    call_out("attempt_match", 5, player);

    return 1;
}

// 离开匹配队列
int leave_queue(object player)
{
    if (!objectp(player))
        return 0;

    if (!player->query_temp(PVP_TEMP_IN_QUEUE))
        return 1; // 不在队列中也算成功

    player->delete_temp(PVP_TEMP_IN_QUEUE);
    player->delete_temp(PVP_TEMP_QUEUE_TIME);

    // 从队列中移除
    for (int i = sizeof(match_queue) - 1; i >= 0; i--)
    {
        if (sizeof(match_queue[i]) >= 1 && match_queue[i][0] == player)
        {
            match_queue = match_queue[0..i-1] + match_queue[i+1..];
            break;
        }
    }

    tell_object(player, "你已离开匹配队列。\n");
    return 1;
}

// 尝试匹配
void attempt_match(object player)
{
    if (!objectp(player) || !player->query_temp(PVP_TEMP_IN_QUEUE))
        return;

    int queue_time = player->query_temp(PVP_TEMP_QUEUE_TIME);
    int elapsed = time() - queue_time;
    int my_rank = query_rank(player);
    int match_range;
    mixed *candidates = ({});

    // 根据等待时间确定搜索范围
    if (elapsed < PVP_QUEUE_TIMEOUT)
        match_range = PVP_MATCH_RANGE_NARROW;
    else if (elapsed < PVP_QUEUE_WIDE_TIMEOUT)
        match_range = PVP_MATCH_RANGE_WIDE;
    else
        match_range = PVP_MATCH_RANGE_ANY;

    // 在队列中查找匹配对手
    for (int i = 0; i < sizeof(match_queue); i++)
    {
        if (sizeof(match_queue[i]) < 1)
            continue;

        object candidate = match_queue[i][0];
        if (!objectp(candidate) || candidate == player)
            continue;
        if (!candidate->query_temp(PVP_TEMP_IN_QUEUE))
            continue;
        if (candidate->query_temp(PVP_TEMP_IN_ARENA))
            continue;

        int cand_rank = query_rank(candidate);
        int rank_diff = (my_rank > cand_rank) ? my_rank - cand_rank : cand_rank - my_rank;

        if (match_range >= PVP_MATCH_RANGE_ANY || rank_diff <= match_range)
        {
            candidates += ({ candidate });
        }
    }

    if (sizeof(candidates) > 0)
    {
        // 选最佳匹配（段位差最小）
        object best_match = candidates[0];
        int best_diff = abs(query_rank(player) - query_rank(best_match));

        for (int i = 1; i < sizeof(candidates); i++)
        {
            int diff = abs(query_rank(player) - query_rank(candidates[i]));
            if (diff < best_diff)
            {
                best_diff = diff;
                best_match = candidates[i];
            }
        }

        // 开始对战
        start_arena_fight(player, best_match);
        return;
    }

    // 没有找到匹配，继续等待
    int next_check = (match_range >= PVP_MATCH_RANGE_ANY) ? 10 : 5;
    call_out("attempt_match", next_check, player);

    // 提示等待信息
    if (elapsed % 15 == 0 && elapsed > 0)
    {
        tell_object(player, sprintf("等待匹配中... 已等待 %d 秒。\n", elapsed));
    }
}

// ===== 竞技场战斗 =====

// 开始竞技场战斗
void start_arena_fight(object a, object b)
{
    if (!objectp(a) || !objectp(b))
        return;

    // 将双方从匹配队列移除
    leave_queue(a);
    leave_queue(b);

    // 记录原始位置
    a->set_temp(PVP_TEMP_ORIGIN_ROOM, file_name(environment(a)));
    b->set_temp(PVP_TEMP_ORIGIN_ROOM, file_name(environment(b)));

    // 设置竞技场标记
    a->set_temp(PVP_TEMP_IN_ARENA, 1);
    b->set_temp(PVP_TEMP_IN_ARENA, 1);
    a->set_temp(PVP_TEMP_CHALLENGER, b->query("id"));
    b->set_temp(PVP_TEMP_CHALLENGER, a->query("id"));

    // 传送至竞技场
    object arena = find_object(PVP_ARENA_ROOM);
    if (!arena)
        arena = load_object(PVP_ARENA_ROOM);
    if (!arena)
    {
        // 回退处理
        a->delete_temp(PVP_TEMP_IN_ARENA);
        b->delete_temp(PVP_TEMP_IN_ARENA);
        tell_object(a, "竞技场房间不可用，匹配取消。\n");
        tell_object(b, "竞技场房间不可用，匹配取消。\n");
        return;
    }

    a->move(arena);
    b->move(arena);

    // 全状态恢复
    a->fullme();
    b->fullme();
    a->stop_busy();
    b->stop_busy();

    string rank_a = get_rank_name(query_rank(a));
    string rank_b = get_rank_name(query_rank(b));

    message_vision(HIY "\n☆☆☆ 竞技场对决开始 ☆☆☆\n\n" NOR, a, b);
    message_vision(sprintf(HIC "$N(%s)  VS  $N(%s)\n\n" NOR, rank_a, rank_b), a, b);
    message_vision(HIW "准备时间 %d 秒，之后战斗开始！\n" NOR, PVP_ARENA_PREP_TIME, a, b);

    // 设置准备时间
    a->start_busy(PVP_ARENA_PREP_TIME);
    b->start_busy(PVP_ARENA_PREP_TIME);

    call_out("start_arena_combat", PVP_ARENA_PREP_TIME, a, b);
}

// 开始实际战斗
void start_arena_combat(object a, object b)
{
    if (!objectp(a) || !objectp(b))
    {
        // 有人掉线了
        if (objectp(a))
        {
            a->delete_temp(PVP_TEMP_IN_ARENA);
            tell_object(a, "对手已消失，匹配取消。\n");
        }
        if (objectp(b))
        {
            b->delete_temp(PVP_TEMP_IN_ARENA);
            tell_object(b, "对手已消失，匹配取消。\n");
        }
        return;
    }

    message_vision(HIR "\n战斗开始！\n\n" NOR, a, b);
    a->start_busy(0);
    b->start_busy(0);

    // 互相攻击
    a->kill_ob(b);
    b->kill_ob(a);
}

// 结束竞技场战斗
void end_arena_fight(object winner, object loser)
{
    object a, b;
    int a_score, b_score, a_rank, b_rank;

    if (objectp(winner) && objectp(loser))
    {
        a = winner;
        b = loser;
    }
    else if (objectp(winner) && !objectp(loser))
    {
        // 对手没了，winner 获胜
        a = winner;
        b = 0;
    }
    else
    {
        return;
    }

    // 处理胜负
    if (objectp(a) && objectp(b))
    {
        // 双方都还在
        a->remove_enemy(b);
        b->remove_enemy(a);
        a->stop_busy();
        b->stop_busy();
        a->fullme();
        b->fullme();

        // 分数计算
        mapping data_a = ensure_pvp_data(a);
        mapping data_b = ensure_pvp_data(b);

        int score_a = data_a[PVP_DATA_SCORE];
        int score_b = data_b[PVP_DATA_SCORE];
        int rank_a = data_a[PVP_DATA_RANK];
        int rank_b = data_b[PVP_DATA_RANK];

        // 基础分
        int win_score = PVP_SCORE_WIN_BASE;
        int lose_score = PVP_SCORE_LOSE_BASE;

        // 段位差额外加减
        if (rank_b > rank_a)
            win_score += (rank_b - rank_a) * PVP_SCORE_WIN_HIGHER;
        if (rank_a > rank_b)
            lose_score += (rank_a - rank_b) * PVP_SCORE_LOSE_LOWER;

        // 连胜奖励
        data_a[PVP_DATA_STREAK] += 1;
        data_b[PVP_DATA_STREAK] = 0;
        int streak = data_a[PVP_DATA_STREAK];
        if (streak >= 3 && streak < 5)
            win_score += PVP_STREAK_3_BONUS;
        else if (streak >= 5 && streak < 10)
            win_score += PVP_STREAK_5_BONUS;
        else if (streak >= 10)
            win_score += PVP_STREAK_10_BONUS;

        // 更新分数
        data_a[PVP_DATA_SCORE] = score_a + win_score;
        data_b[PVP_DATA_SCORE] = (score_b - lose_score > PVP_SCORE_MIN) ? score_b - lose_score : PVP_SCORE_MIN;

        // 更新段位
        int new_rank_a = calc_rank_by_score(data_a[PVP_DATA_SCORE]);
        int new_rank_b = calc_rank_by_score(data_b[PVP_DATA_SCORE]);
        data_a[PVP_DATA_RANK] = new_rank_a;
        data_b[PVP_DATA_RANK] = new_rank_b;

        if (new_rank_a > data_a[PVP_DATA_BEST_RANK])
            data_a[PVP_DATA_BEST_RANK] = new_rank_a;

        // 统计
        data_a[PVP_DATA_WINS]++;
        data_b[PVP_DATA_LOSSES]++;
        data_a[PVP_DATA_TOTAL_FIGHTS]++;
        data_b[PVP_DATA_TOTAL_FIGHTS]++;
        data_a[PVP_DATA_SEASON_WINS]++;
        data_a[PVP_DATA_LAST_FIGHT] = time();
        data_b[PVP_DATA_LAST_FIGHT] = time();

        // 显示结果
        string rank_a_name = get_rank_name(new_rank_a);
        string rank_b_name = get_rank_name(new_rank_b);

        message_vision(HIY "\n※※※ 战斗结束 ※※※\n\n" NOR, a, b);
        message_vision(HIG "$N 获得了胜利！\n" NOR, a);
        message_vision(HIR "$N 被击败了。\n\n" NOR, b);

        tell_object(a, sprintf(HIW "胜利！+%d 积分 (当前: %d, 段位: %s)\n" NOR,
                    win_score, data_a[PVP_DATA_SCORE], rank_a_name));
        tell_object(b, sprintf(HIW "败北！-%d 积分 (当前: %d, 段位: %s)\n" NOR,
                    lose_score, data_b[PVP_DATA_SCORE], rank_b_name));

        if (new_rank_a > rank_a)
            tell_object(a, HIY "恭喜！段位提升至 " + rank_a_name + "！\n" NOR);
        if (new_rank_b < rank_b && rank_b > 0)
            tell_object(b, HIY "你的段位降至 " + rank_b_name + "。\n" NOR);

        if (streak >= 3)
            tell_object(a, HIY "你已经 " + streak + " 连胜了！\n" NOR);

        // 返回原位置
        return_from_arena(a);
        return_from_arena(b);
    }
    else if (objectp(a) && !objectp(b))
    {
        // 对手不见了，判定a获胜
        tell_object(a, HIY "你的对手已离开，本局判定你获胜。\n" NOR);
        mapping data = ensure_pvp_data(a);
        data[PVP_DATA_WINS]++;
        data[PVP_DATA_TOTAL_FIGHTS]++;
        data[PVP_DATA_SCORE] += PVP_SCORE_WIN_BASE / 2;
        data[PVP_DATA_LAST_FIGHT] = time();
        return_from_arena(a);
    }
}

// 从竞技场返回
void return_from_arena(object player)
{
    if (!objectp(player))
        return;

    string origin = player->query_temp(PVP_TEMP_ORIGIN_ROOM);
    player->delete_temp(PVP_TEMP_IN_ARENA);
    player->delete_temp(PVP_TEMP_CHALLENGER);

    if (stringp(origin) && file_size(origin + ".c") > 0)
    {
        player->move(origin);
    }
    else
    {
        // 回退到安全地点
        player->move("/d/city/kedian");
    }

    player->delete_temp(PVP_TEMP_ORIGIN_ROOM);
    tell_object(player, "你回到了原来的位置。\n");
}

// 玩家断线处理
void player_disconnect(object player)
{
    if (!objectp(player))
        return;

    // 如果是竞技场内断线，算负
    if (player->query_temp(PVP_TEMP_IN_ARENA))
    {
        object *inv;
        object opponent = 0;

        // 找对手
        inv = all_inventory(environment(player));
        foreach (object ob in inv)
        {
            if (ob != player && userp(ob) && ob->query_temp(PVP_TEMP_IN_ARENA))
            {
                opponent = ob;
                break;
            }
        }

        if (opponent)
            end_arena_fight(opponent, player);
        else
            return_from_arena(player);
    }

    // 如果在队列中，移除
    leave_queue(player);
}

// ===== 赛季系统 =====

// 检查赛季是否结束
void check_season()
{
    int elapsed = time() - season_start_time;
    if (elapsed >= PVP_SEASON_DAYS * 86400)
    {
        // 赛季结束，结算
        season_reward();
        // 开始新赛季
        season_start_time = time();
    }
}

// 赛季结算
void season_reward()
{
    // 遍历所有在线玩家，发放段位奖励
    // 注：实际应遍历所有玩家存档，这里简化处理在线玩家
    object *users = users();
    foreach (object ob in users())
    {
        if (!objectp(ob))
            continue;

        mapping data = ob->query(PVP_DATA_KEY);
        if (!mapp(data))
            continue;

        int rank_idx = data[PVP_DATA_RANK];
        int *rewards = PVP_RANK_REWARDS;
        int reward_lingshi = (rank_idx >= 0 && rank_idx < sizeof(rewards)) ? rewards[rank_idx] : 0;

        // 发放奖励（灵石）
        if (reward_lingshi > 0)
        {
            ob->add("balance", reward_lingshi);
            tell_object(ob, HIY "赛季结算！你获得了 " + reward_lingshi + " 灵石作为段位奖励。\n" NOR);
        }

        // 重置赛季数据（保留基础段位积分的一半）
        int carry_over = data[PVP_DATA_SCORE] * PVP_SEASON_RESERVE_RATIO / 100;
        data[PVP_DATA_SCORE] = carry_over;
        data[PVP_DATA_RANK] = calc_rank_by_score(carry_over);
        data[PVP_DATA_SEASON_WINS] = 0;
        data[PVP_DATA_STREAK] = 0;
    }
}

// ===== 查询命令 =====

// 查询自己的PVP状态
mapping query_pvp_status(object player)
{
    mapping data = ensure_pvp_data(player);
    if (!mapp(data))
        return 0;

    int rank_idx = data[PVP_DATA_RANK];
    string rank_name = get_rank_name(rank_idx);

    return ([
        "score"        : data[PVP_DATA_SCORE],
        "rank"         : rank_name,
        "rank_index"   : rank_idx,
        "wins"         : data[PVP_DATA_WINS],
        "losses"       : data[PVP_DATA_LOSSES],
        "streak"       : data[PVP_DATA_STREAK],
        "best_rank"    : get_rank_name(data[PVP_DATA_BEST_RANK]),
        "total_fights" : data[PVP_DATA_TOTAL_FIGHTS],
    ]);
}

// 排行榜（简化版：列出前10名在线玩家）
mixed *query_leaderboard()
{
    object *users = users();
    mixed *rankings = ({});

    foreach (object ob in users)
    {
        if (!objectp(ob))
            continue;
        mapping data = ob->query(PVP_DATA_KEY);
        if (!mapp(data))
            continue;

        rankings += ({
            ({
                ob->name(),
                ob->query("id"),
                data[PVP_DATA_SCORE],
                data[PVP_DATA_RANK],
                data[PVP_DATA_WINS],
            })
        });
    }

    // 按积分降序排序
    rankings = sort_array(rankings, (: $1[2] > $2[2] ? -1 : ($1[2] < $2[2] ? 1 : 0) :));

    // 取前10
    if (sizeof(rankings) > 10)
        rankings = rankings[0..9];

    return rankings;
}
