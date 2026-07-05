// activity_d.c
// 活跃度与日常任务系统守护进程
//
// 功能：
//   1. 玩家活跃度计算与积累
//   2. 日活跃度/周活跃度阈值奖励
//   3. 每日/每周活跃度重置
//   4. 每日任务池刷新
//   5. 活跃度查询与奖励领取
//
// 数据存储：
//   玩家活跃度数据存储在玩家 dbase 的 "activity/" 路径下
//   随玩家 save/load 自动持久化

#include <ansi.h>
#include <activity.h>
#include <localtime.h>

inherit F_DBASE;

// 活跃度行为名称映射（用于显示）
nosave mapping act_type_names = ([
    ACT_TYPE_HERB       : "采集灵药",
    ACT_TYPE_HUNT       : "猎杀妖兽",
    ACT_TYPE_SECT_CHORE : "门派杂务",
    ACT_TYPE_ALCHEMY    : "炼制任务",
    ACT_TYPE_TRADE      : "坊市交易",
    ACT_TYPE_COURIER    : "送信任务",
    ACT_TYPE_MINE       : "采矿任务",
    ACT_TYPE_PATROL     : "巡山任务",
]);

// 活跃度行为基础分值映射
nosave mapping act_type_scores = ([
    ACT_TYPE_HERB       : ACT_SCORE_HERB,
    ACT_TYPE_HUNT       : ACT_SCORE_HUNT,
    ACT_TYPE_SECT_CHORE : ACT_SCORE_SECT_CHORE,
    ACT_TYPE_ALCHEMY    : ACT_SCORE_ALCHEMY,
    ACT_TYPE_TRADE      : ACT_SCORE_TRADE,
    ACT_TYPE_COURIER    : ACT_SCORE_COURIER,
    ACT_TYPE_MINE       : ACT_SCORE_MINE,
    ACT_TYPE_PATROL     : ACT_SCORE_PATROL,
]);

// 活跃度行为每日次数上限映射
nosave mapping act_type_limits = ([
    ACT_TYPE_HERB       : ACT_DAILY_LIMIT_HERB,
    ACT_TYPE_HUNT       : ACT_DAILY_LIMIT_HUNT,
    ACT_TYPE_SECT_CHORE : ACT_DAILY_LIMIT_SECT_CHORE,
    ACT_TYPE_ALCHEMY    : ACT_DAILY_LIMIT_ALCHEMY,
    ACT_TYPE_TRADE      : ACT_DAILY_LIMIT_TRADE,
    ACT_TYPE_COURIER    : ACT_DAILY_LIMIT_COURIER,
    ACT_TYPE_MINE       : ACT_DAILY_LIMIT_MINE,
    ACT_TYPE_PATROL     : ACT_DAILY_LIMIT_PATROL,
]);

// 日活跃度阈值奖励配置
nosave mapping daily_threshold_rewards = ([
    ACT_DAILY_THRESHOLD_1 : ([
        "name"  : "日活跃·初段",
        "exp"   : 100,
        "pot"   : 50,
        "coin"  : 200,
    ]),
    ACT_DAILY_THRESHOLD_2 : ([
        "name"  : "日活跃·中段",
        "exp"   : 300,
        "pot"   : 150,
        "coin"  : 500,
    ]),
    ACT_DAILY_THRESHOLD_3 : ([
        "name"  : "日活跃·高段",
        "exp"   : 600,
        "pot"   : 300,
        "coin"  : 1000,
    ]),
    ACT_DAILY_THRESHOLD_4 : ([
        "name"  : "日活跃·圆满",
        "exp"   : 1200,
        "pot"   : 600,
        "coin"  : 2000,
    ]),
]);

// 周活跃度阈值奖励配置
// 注意：物品奖励（item）的文件路径需确认对应物品存在，否则跳过
nosave mapping weekly_threshold_rewards = ([
    ACT_WEEKLY_THRESHOLD_1 : ([
        "name"   : "周活跃·初段",
        "exp"    : 2000,
        "pot"    : 1000,
        "coin"   : 5000,
    ]),
    ACT_WEEKLY_THRESHOLD_2 : ([
        "name"   : "周活跃·中段",
        "exp"    : 5000,
        "pot"    : 2500,
        "coin"   : 10000,
        "title"  : "勤勉修士",
    ]),
    ACT_WEEKLY_THRESHOLD_3 : ([
        "name"   : "周活跃·高段",
        "exp"    : 10000,
        "pot"    : 5000,
        "coin"   : 20000,
    ]),
    ACT_WEEKLY_THRESHOLD_4 : ([
        "name"   : "周活跃·圆满",
        "exp"    : 20000,
        "pot"    : 10000,
        "coin"   : 50000,
        "title"  : "天道酬勤",
    ]),
]);

// 每日任务池模板（每个玩家每天从此池中随机抽取）
nosave string *daily_task_pool = ({
    "采集灵药·炼气",     // 采集 N 株灵药
    "采集灵药·筑基",
    "采集灵药·结丹",
    "猎杀妖兽·炼气",     // 击杀 N 只妖兽
    "猎杀妖兽·筑基",
    "猎杀妖兽·结丹",
    "门派杂务·跑腿",     // 跑腿送物
    "门派杂务·巡逻",     // 门派区域巡逻
    "门派杂务·清洁",     // 清洁门派区域
    "炼制任务·丹药",     // 炼制指定丹药
    "炼制任务·法器",     // 炼制指定法器
    "坊市交易·采购",     // 在坊市采购物品
    "坊市交易·跑商",     // 跑商赚取灵石
    "送信任务·越国",     // 在越国区域送信
    "送信任务·乱星海",   // 在乱星海区域送信
    "采矿任务·矿脉",     // 在矿脉采集矿石
    "采矿任务·精矿",     // 采集高品质矿石
    "巡山任务·清剿",     // 巡查清剿沿路怪物
    "巡山任务·守护",     // 守护指定区域
});

// 各玩家每天随机抽取的任务数
nosave mapping realm_daily_task_count = ([
    "炼气期" : 5,
    "筑基期" : 6,
    "结丹期" : 7,
    "元婴期" : 8,
    "化神期" : 9,
    "炼虚期" : 10,
    "合体期" : 11,
    "大乘期" : 12,
]);

// 境界索引映射（用于奖励缩放）
nosave mapping realm_index_map = ([
    "凡人"   : 0,
    "炼气期" : 1,
    "筑基期" : 2,
    "结丹期" : 3,
    "元婴期" : 4,
    "化神期" : 5,
    "炼虚期" : 6,
    "合体期" : 7,
    "大乘期" : 8,
]);

// 境界基准值映射（用于奖励计算）
nosave mapping realm_base_reward = ([
    "炼气期" : 50,
    "筑基期" : 200,
    "结丹期" : 1000,
    "元婴期" : 5000,
    "化神期" : 20000,
    "炼虚期" : 80000,
    "合体期" : 300000,
    "大乘期" : 1000000,
]);

// 创建守护进程
void create()
{
    seteuid(getuid());
    set("channel_id", HIW "活跃度系统" NOR);
    CHANNEL_D->do_channel(this_object(), "sys", "活跃度系统已启动。");

    // 启动心跳，每 5 分钟检查一次日常/周常刷新
    set_heart_beat(300);
}

// 心跳函数：检查日常/周常刷新
void heart_beat()
{
    mixed *lt;
    int current_hour;

    lt = localtime(time());
    current_hour = lt[LT_HOUR];

    // 每日 0:00 刷新
    if (current_hour == 0 && lt[LT_MIN] < 5)
    {
        CHANNEL_D->do_channel(this_object(), "sys", "日常任务与活跃度已刷新。");
    }

    // 每周一 0:00 刷新周常
    if (lt[LT_WDAY] == 1 && current_hour == 0 && lt[LT_MIN] < 5)
    {
        CHANNEL_D->do_channel(this_object(), "sys", "周常任务已刷新。");
    }
}

// ---- 核心活跃度接口 ----

// 获取自纪元以来的天数（用于日重置判定）
int get_epoch_day()
{
    return time() / 86400;
}

// 检测是否需要日重置
int is_daily_reset(object player)
{
    int today;

    if (!objectp(player))
        return 0;

    today = get_epoch_day();
    return (player->query(ACT_DBASE_KEY "/daily/date") != today);
}

// 检测是否需要周重置
int is_weekly_reset(object player)
{
    int current_week;

    if (!objectp(player))
        return 0;

    current_week = get_epoch_day() / 7;
    return (player->query(ACT_DBASE_KEY "/weekly/week") != current_week);
}

// 执行玩家活跃度日重置
void daily_reset_player(object player)
{
    int today;

    if (!objectp(player))
        return;

    today = get_epoch_day();

    // 保存昨日活跃度到历史记录
    player->set(ACT_DBASE_KEY "/yesterday", player->query(ACT_DBASE_KEY "/daily/score"));
    // 重置日常数据
    player->delete(ACT_DBASE_KEY "/daily");
    player->set(ACT_DBASE_KEY "/daily/date", today);
    player->set(ACT_DBASE_KEY "/daily/score", 0);
    player->set(ACT_DBASE_KEY "/daily/claimed", ([]));
}

// 执行玩家活跃度周重置
void weekly_reset_player(object player)
{
    int current_week;

    if (!objectp(player))
        return;

    current_week = get_epoch_day() / 7;

    // 保存上周活跃度到历史记录
    player->set(ACT_DBASE_KEY "/last_week", player->query(ACT_DBASE_KEY "/weekly/score"));
    // 重置周常数据
    player->delete(ACT_DBASE_KEY "/weekly");
    player->set(ACT_DBASE_KEY "/weekly/week", current_week);
    player->set(ACT_DBASE_KEY "/weekly/score", 0);
    player->set(ACT_DBASE_KEY "/weekly/claimed", ([]));
}

// 确保玩家活跃度数据初始化
void ensure_init(object player)
{
    if (!objectp(player) || !userp(player))
        return;

    // 每日重置
    if (is_daily_reset(player))
        daily_reset_player(player);

    // 每周重置
    if (is_weekly_reset(player))
        weekly_reset_player(player);

    // 确保初始数据存在
    if (undefinedp(player->query(ACT_DBASE_KEY "/daily/score")))
        player->set(ACT_DBASE_KEY "/daily/score", 0);
    if (undefinedp(player->query(ACT_DBASE_KEY "/daily/claimed")))
        player->set(ACT_DBASE_KEY "/daily/claimed", ([]));
    if (undefinedp(player->query(ACT_DBASE_KEY "/weekly/score")))
        player->set(ACT_DBASE_KEY "/weekly/score", 0);
    if (undefinedp(player->query(ACT_DBASE_KEY "/weekly/claimed")))
        player->set(ACT_DBASE_KEY "/weekly/claimed", ([]));
    if (undefinedp(player->query(ACT_DBASE_KEY "/consecutive_days")))
        player->set(ACT_DBASE_KEY "/consecutive_days", 0);
    if (undefinedp(player->query(ACT_DBASE_KEY "/last_active_day")))
        player->set(ACT_DBASE_KEY "/last_active_day", 0);
}

// 获取玩家境界名称
string get_player_realm(object player)
{
    string realm;

    if (!objectp(player))
        return "凡人";

    // 优先使用 query_realm() 方法，退化到 query("realm")
    if (function_exists("query_realm", player))
        realm = player->query_realm();
    else
        realm = player->query("realm");

    if (!stringp(realm))
        return "凡人";

    return realm;
}

// 获取玩家当前境界索引
int get_realm_index(object player)
{
    string realm;
    int idx;

    if (!objectp(player))
        return 0;

    realm = get_player_realm(player);

    if (!undefinedp(realm_index_map[realm]))
        return realm_index_map[realm];

    return 0;
}

// 计算活跃度对应奖励的缩放系数
float calc_reward_scale(object player)
{
    int consecutive_days;
    float scale = 1.0;

    if (!objectp(player))
        return 1.0;

    consecutive_days = player->query(ACT_DBASE_KEY "/consecutive_days");

    // 连续完成活跃度奖励加成：每连续一天加成 5%，最高 50%
    if (consecutive_days > 0)
        scale += (consecutive_days * 0.05);

    if (scale > 1.5)
        scale = 1.5;

    return scale;
}

// 计算境界缩放系数
float calc_realm_scale(object player, int base_reward)
{
    float scale = 1.0;
    string realm;

    if (!objectp(player))
        return 1.0;

    realm = get_player_realm(player);
    if (stringp(realm) && !undefinedp(realm_base_reward[realm]))
    {
        scale = to_float(realm_base_reward[realm]) / 50.0; // 以炼气期为基准
        if (scale < 1.0) scale = 1.0;
    }

    return scale;
}

// 添加活跃度
// player: 玩家对象
// type: 活跃度行为类型（ACT_TYPE_* 常量）
// amount: 活跃度数值（0 则使用默认值）
// 返回值：实际添加的活跃度
varargs int add_activity(object player, string type, int amount)
{
    int today;
    string realm;
    int current_daily, max_daily, type_limit, current_type_count;
    int actual_amount;

    if (!objectp(player) || !userp(player))
        return 0;

    // 确保活跃度数据初始化
    ensure_init(player);
    today = get_epoch_day();

    // 如果未指定活跃度数值，使用默认值
    if (amount <= 0)
    {
        if (undefinedp(act_type_scores[type]))
            return 0;
        amount = act_type_scores[type];
    }

    // 检查每日上限
    current_daily = player->query(ACT_DBASE_KEY "/daily/score");
    max_daily = ACT_DAILY_MAX;
    if (current_daily >= max_daily)
    {
        tell_object(player, HIC "你今天的活跃度已达到上限，继续努力明天再来吧。\n" NOR);
        return 0;
    }

    // 检查该行为类型每日次数上限
    if (stringp(type) && !undefinedp(act_type_limits[type]))
    {
        type_limit = act_type_limits[type];
        current_type_count = player->query(ACT_DBASE_KEY "/daily/" + type + "_count");
        if (current_type_count >= type_limit)
        {
            tell_object(player, HIC "今天的" + act_type_names[type] + "已经做得够多了。\n" NOR);
            return 0;
        }
    }

    // 计算实际活跃度（不能超过每日上限）
    actual_amount = amount;
    if (current_daily + actual_amount > max_daily)
        actual_amount = max_daily - current_daily;

    if (actual_amount <= 0)
        return 0;

    // 更新活跃度
    player->add(ACT_DBASE_KEY "/daily/score", actual_amount);
    player->set(ACT_DBASE_KEY "/daily/date", today);

    // 更新该类型每日计数
    if (stringp(type) && type != "")
        player->add(ACT_DBASE_KEY "/daily/" + type + "_count", 1);

    // 更新周活跃度
    player->add(ACT_DBASE_KEY "/weekly/score", actual_amount);

    // 更新连续活跃天数
    check_consecutive_days(player);

    // 更新总活跃度
    player->add(ACT_DBASE_KEY "/total", actual_amount);

    // 提示玩家
    if (stringp(type) && !undefinedp(act_type_names[type]))
    {
        tell_object(player, HIW "完成「" + act_type_names[type] + "」，活跃度 +" + actual_amount + "。\n" NOR);
    }
    else
    {
        tell_object(player, HIW "活跃度 +" + actual_amount + "。\n" NOR);
    }

    // 检查是否可以领取新的活跃度奖励
    check_daily_reward_available(player);
    check_weekly_reward_available(player);

    return actual_amount;
}

// 检查并更新连续活跃天数
void check_consecutive_days(object player)
{
    int today, last_active;
    int consecutive;

    if (!objectp(player))
        return;

    today = get_epoch_day();
    last_active = player->query(ACT_DBASE_KEY "/last_active_day");
    consecutive = player->query(ACT_DBASE_KEY "/consecutive_days");

    if (last_active == 0)
    {
        // 首次活跃
        player->set(ACT_DBASE_KEY "/consecutive_days", 1);
    }
    else if (last_active == today)
    {
        // 同一天，不增加连续天数
        return;
    }
    else
    {
        // 检查是否是连续日期
        if (today - last_active == 1)
        {
            player->add(ACT_DBASE_KEY "/consecutive_days", 1);
        }
        else
        {
            // 中断，重新计数
            player->set(ACT_DBASE_KEY "/consecutive_days", 1);
        }
    }

    player->set(ACT_DBASE_KEY "/last_active_day", today);
}

// 检查可达的新奖励（用于提示）
void check_daily_reward_available(object player)
{
    int score;
    int *thresholds = ({ ACT_DAILY_THRESHOLD_1, ACT_DAILY_THRESHOLD_2, ACT_DAILY_THRESHOLD_3, ACT_DAILY_THRESHOLD_4 });
    mapping claimed;

    if (!objectp(player))
        return;

    score = player->query(ACT_DBASE_KEY "/daily/score");
    claimed = player->query(ACT_DBASE_KEY "/daily/claimed");
    if (!mapp(claimed))
        claimed = ([]);

    foreach (int th in thresholds)
    {
        if (score >= th && undefinedp(claimed[th]))
        {
            tell_object(player, HIY "你的活跃度已达到 " + th + "，可用 activity claim " + th + " 领取奖励。\n" NOR);
        }
    }
}

// 检查可达的周奖励
void check_weekly_reward_available(object player)
{
    int score;
    int *thresholds = ({ ACT_WEEKLY_THRESHOLD_1, ACT_WEEKLY_THRESHOLD_2, ACT_WEEKLY_THRESHOLD_3, ACT_WEEKLY_THRESHOLD_4 });
    mapping claimed;

    if (!objectp(player))
        return;

    score = player->query(ACT_DBASE_KEY "/weekly/score");
    claimed = player->query(ACT_DBASE_KEY "/weekly/claimed");
    if (!mapp(claimed))
        claimed = ([]);

    foreach (int th in thresholds)
    {
        if (score >= th && undefinedp(claimed[th]))
        {
            tell_object(player, HIY "你的周活跃度已达到 " + th + "，可用 activity claim_weekly " + th + " 领取周奖励。\n" NOR);
        }
    }
}

// ---- 奖励领取接口 ----

// 领取日活跃度阈值奖励
// 返回值：1=成功, 0=失败, -1=已领取, -2=活跃度不足
varargs int claim_daily_reward(object player, int threshold)
{
    mapping claimed, reward;
    float scale, realm_scale;

    if (!objectp(player) || !userp(player))
        return 0;

    ensure_init(player);

    // 检查是否已领取
    claimed = player->query(ACT_DBASE_KEY "/daily/claimed");
    if (!mapp(claimed))
        claimed = ([]);

    if (!undefinedp(claimed[threshold]))
    {
        tell_object(player, "该档活跃度奖励已领取过了。\n");
        return -1;
    }

    // 检查活跃度是否达标
    if (player->query(ACT_DBASE_KEY "/daily/score") < threshold)
    {
        tell_object(player, "你的活跃度尚未达到 " + threshold + "。\n");
        return -2;
    }

    // 检查阈值配置是否存在
    if (undefinedp(daily_threshold_rewards[threshold]))
    {
        tell_object(player, "无效的活跃度档位。\n");
        return 0;
    }

    reward = daily_threshold_rewards[threshold];
    scale = calc_reward_scale(player);
    realm_scale = calc_realm_scale(player, 1);

    // 发放奖励
    if (reward["exp"] > 0)
    {
        int exp_reward = to_int(reward["exp"] * scale * realm_scale);
        REWARD_D->add_exp(player, exp_reward);
    }

    if (reward["pot"] > 0)
    {
        int pot_reward = to_int(reward["pot"] * scale * realm_scale);
        REWARD_D->add_pot(player, pot_reward);
    }

    if (reward["coin"] > 0)
    {
        int coin_reward = to_int(reward["coin"] * scale);
        player->add("balance", coin_reward);
    }

    // 标记已领取
    claimed[threshold] = 1;
    player->set(ACT_DBASE_KEY "/daily/claimed", claimed);

    // 提示
    tell_object(player, HIW "领取「" + reward["name"] + "」奖励成功！\n" NOR);

    return 1;
}

// 领取周活跃度阈值奖励
// 返回值：1=成功, 0=失败, -1=已领取, -2=活跃度不足
varargs int claim_weekly_reward(object player, int threshold)
{
    mapping claimed, reward;
    float scale, realm_scale;

    if (!objectp(player) || !userp(player))
        return 0;

    ensure_init(player);

    // 检查是否已领取
    claimed = player->query(ACT_DBASE_KEY "/weekly/claimed");
    if (!mapp(claimed))
        claimed = ([]);

    if (!undefinedp(claimed[threshold]))
    {
        tell_object(player, "该档周活跃度奖励已领取过了。\n");
        return -1;
    }

    // 检查活跃度是否达标
    if (player->query(ACT_DBASE_KEY "/weekly/score") < threshold)
    {
        tell_object(player, "你的周活跃度尚未达到 " + threshold + "。\n");
        return -2;
    }

    // 检查阈值配置是否存在
    if (undefinedp(weekly_threshold_rewards[threshold]))
    {
        tell_object(player, "无效的活跃度档位。\n");
        return 0;
    }

    reward = weekly_threshold_rewards[threshold];
    scale = calc_reward_scale(player);
    realm_scale = calc_realm_scale(player, 1);

    // 发放经验奖励
    if (reward["exp"] > 0)
    {
        int exp_reward = to_int(reward["exp"] * scale * realm_scale);
        REWARD_D->add_exp(player, exp_reward);
    }

    // 发放潜能奖励
    if (reward["pot"] > 0)
    {
        int pot_reward = to_int(reward["pot"] * scale * realm_scale);
        REWARD_D->add_pot(player, pot_reward);
    }

    // 发放灵石奖励
    if (reward["coin"] > 0)
    {
        int coin_reward = to_int(reward["coin"] * scale);
        player->add("balance", coin_reward);
    }

    // 发放称号奖励
    if (stringp(reward["title"]))
    {
        string current_title = player->query("title");
        if (!stringp(current_title) || strsrch(current_title, reward["title"]) == -1)
        {
            if (stringp(current_title) && current_title != "")
                player->set("title", current_title + "·" + reward["title"]);
            else
                player->set("title", reward["title"]);

            tell_object(player, HIY "你获得了称号「" + reward["title"] + "」！\n" NOR);
        }
    }

    // 标记已领取
    claimed[threshold] = 1;
    player->set(ACT_DBASE_KEY "/weekly/claimed", claimed);

    // 提示
    tell_object(player, HIW "领取「" + reward["name"] + "」周活跃奖励成功！\n" NOR);

    return 1;
}

// ---- 查询接口 ----

// 获取玩家日活跃度
int query_daily_activity(object player)
{
    if (!objectp(player))
        return 0;

    ensure_init(player);
    return player->query(ACT_DBASE_KEY "/daily/score");
}

// 获取玩家周活跃度
int query_weekly_activity(object player)
{
    if (!objectp(player))
        return 0;

    ensure_init(player);
    return player->query(ACT_DBASE_KEY "/weekly/score");
}

// 获取玩家总活跃度
int query_total_activity(object player)
{
    if (!objectp(player))
        return 0;

    return player->query(ACT_DBASE_KEY "/total");
}

// 获取玩家连续活跃天数
int query_consecutive_days(object player)
{
    if (!objectp(player))
        return 0;

    return player->query(ACT_DBASE_KEY "/consecutive_days");
}

// 获取玩家完整活跃度信息
mapping query_activity_info(object player)
{
    mapping info;

    if (!objectp(player))
        return ([]);

    ensure_init(player);

    info = ([
        "daily_score"      : player->query(ACT_DBASE_KEY "/daily/score"),
        "daily_max"        : ACT_DAILY_MAX,
        "daily_claimed"    : player->query(ACT_DBASE_KEY "/daily/claimed"),
        "weekly_score"     : player->query(ACT_DBASE_KEY "/weekly/score"),
        "weekly_max"       : ACT_WEEKLY_MAX,
        "weekly_claimed"   : player->query(ACT_DBASE_KEY "/weekly/claimed"),
        "total"            : player->query(ACT_DBASE_KEY "/total"),
        "consecutive_days" : player->query(ACT_DBASE_KEY "/consecutive_days"),
        "realm"            : get_player_realm(player),
    ]);

    return info;
}

// ---- 每日任务池接口 ----

// 获取玩家每日任务池
// 返回：今日可做的日常任务列表
string *query_daily_task_pool(object player)
{
    string realm;
    int task_count;
    string *pool;

    if (!objectp(player))
        return ({});

    ensure_init(player);

    // 检查是否已生成当天的任务池
    pool = player->query(ACT_DBASE_KEY "/daily/task_pool");
    if (arrayp(pool) && sizeof(pool) > 0)
        return pool;

    // 新生成任务池
    return refresh_daily_task_pool(player);
}

// 刷新玩家每日任务池
string *refresh_daily_task_pool(object player)
{
    string realm;
    int task_count;
    string *pool, *available_pool;
    int pool_size, i, idx;

    if (!objectp(player))
        return ({});

    ensure_init(player);

    // 根据境界决定每日任务数量
    realm = get_player_realm(player);
    if (undefinedp(realm_daily_task_count[realm]))
        task_count = 5;  // 默认
    else
        task_count = realm_daily_task_count[realm];

    // 从任务池中随机抽取
    pool_size = sizeof(daily_task_pool);
    available_pool = daily_task_pool;
    pool = ({});

    // 随机抽取不重复的任务
    for (i = 0; i < task_count && i < pool_size; i++)
    {
        idx = random(sizeof(available_pool));
        pool += ({ available_pool[idx] });
        available_pool -= ({ available_pool[idx] });
    }

    // 保存到玩家数据
    player->set(ACT_DBASE_KEY "/daily/task_pool", pool);

    return pool;
}

// ---- 显示接口 ----

// 格式化显示活跃度信息
string format_activity_info(object player)
{
    mapping info;
    string output;
    string *daily_claimed, *weekly_claimed;

    if (!objectp(player))
        return "";

    info = query_activity_info(player);

    output = HIC "╔══════════════════════════════════╗\n" NOR;
    output += HIC "║       修 仙 活 跃 度 信 息        ║\n" NOR;
    output += HIC "╠══════════════════════════════════╣\n" NOR;

    // 境界
    output += sprintf(HIC "║  %-8s%-26s║\n" NOR,
                      "境界：", info["realm"]);

    // 日活跃度
    output += sprintf(HIC "║  %-8s%-26s║\n" NOR,
                      "日活跃：",
                      info["daily_score"] + " / " + info["daily_max"]);

    // 日活跃度进度条
    {
        int bar_len = 20;
        int filled = (info["daily_score"] * bar_len) / info["daily_max"];
        string bar = "";
        for (int i = 0; i < bar_len; i++)
        {
            if (i < filled)
                bar += HIY "█" NOR;
            else
                bar += HIK "░" NOR;
        }
        output += sprintf(HIC "║  %-8s%s║\n" NOR, "进度：", bar);
    }

    // 日奖励领取状态
    output += HIC "║  ── 日奖励 ──                     ║\n" NOR;
    {
        int *thresholds = ({ ACT_DAILY_THRESHOLD_1, ACT_DAILY_THRESHOLD_2, ACT_DAILY_THRESHOLD_3, ACT_DAILY_THRESHOLD_4 });
        foreach (int th in thresholds)
        {
            string status;
            if (info["daily_score"] >= th)
            {
                if (!undefinedp(info["daily_claimed"][th]))
                    status = HIG "已领取" NOR;
                else
                    status = HIY "可领取" NOR;
            }
            else
            {
                status = HIK "未达标" NOR;
            }
            output += sprintf(HIC "║  %-20s%s           ║\n" NOR,
                              "活跃度 " + th, status);
        }
    }

    // 周活跃度
    output += HIC "║  ─────────────────────────────       ║\n" NOR;
    output += sprintf(HIC "║  %-8s%-26s║\n" NOR,
                      "周活跃：",
                      info["weekly_score"] + " / " + info["weekly_max"]);

    // 周奖励领取状态
    output += HIC "║  ── 周奖励 ──                     ║\n" NOR;
    {
        int *thresholds = ({ ACT_WEEKLY_THRESHOLD_1, ACT_WEEKLY_THRESHOLD_2, ACT_WEEKLY_THRESHOLD_3, ACT_WEEKLY_THRESHOLD_4 });
        foreach (int th in thresholds)
        {
            string status;
            if (info["weekly_score"] >= th)
            {
                if (!undefinedp(info["weekly_claimed"][th]))
                    status = HIG "已领取" NOR;
                else
                    status = HIY "可领取" NOR;
            }
            else
            {
                status = HIK "未达标" NOR;
            }
            output += sprintf(HIC "║  %-20s%s           ║\n" NOR,
                              "周活跃 " + th, status);
        }
    }

    // 连续活跃和总活跃
    output += sprintf(HIC "║  %-8s%-26s║\n" NOR,
                      "连续：",
                      info["consecutive_days"] + " 天");
    output += sprintf(HIC "║  %-8s%-26s║\n" NOR,
                      "总活跃：",
                      info["total"]);
    output += HIC "╚══════════════════════════════════╝\n" NOR;

    return output;
}

// 显示每日任务列表
string format_daily_task_pool(object player)
{
    string *pool;
    string output;

    if (!objectp(player))
        return "";

    pool = query_daily_task_pool(player);

    output = HIC "╔══════════════════════════════════╗\n" NOR;
    output += HIC "║       今 日 可 做 任 务           ║\n" NOR;
    output += HIC "╠══════════════════════════════════╣\n" NOR;

    if (!arrayp(pool) || sizeof(pool) == 0)
    {
        output += HIC "║  %-30s║\n" NOR "今日暂无可用任务。";
    }
    else
    {
        for (int i = 0; i < sizeof(pool); i++)
        {
            output += sprintf(HIC "║  %d. %-28s║\n" NOR,
                              i + 1, pool[i]);
        }
    }

    output += HIC "╚══════════════════════════════════╝\n" NOR;

    return output;
}
