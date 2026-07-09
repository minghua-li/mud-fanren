// friendship.c
// 好友系统特性 - 亲密等级、互动玩法
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第4章
// 混入到玩家对象，提供好友相关方法

#include <ansi.h>
#include <reputation.h>
#include <reputation_ext.h>

// ======== 好友基础操作 ========

// 获取好友列表
string *query_friends()
{
    mapping friends = query(FRIEND_PATH);
    if (!mapp(friends)) return ({});
    return keys(friends);
}

// 是否是好友
int is_friend(string id)
{
    mapping friends = query(FRIEND_PATH);
    if (!mapp(friends)) return 0;
    return !undefinedp(friends[id]);
}

// 添加好友
int add_friend(string id, string name)
{
    mapping friends = query(FRIEND_PATH);
    if (!mapp(friends)) friends = ([]);

    // 检查上限
    int max = FRIEND_MAX_DEFAULT;
    int exp = query("combat_exp");
    if (exp >= 50000000) max = FRIEND_MAX_YUANYING;
    else if (exp >= 10000000) max = FRIEND_MAX_JIEDAN;
    else if (exp >= 1000000) max = FRIEND_MAX_ZHUIJI;

    if (sizeof(friends) >= max) return 0;

    friends[id] = name;
    set(FRIEND_PATH, friends);
    return 1;
}

// 删除好友
int remove_friend(string id)
{
    mapping friends = query(FRIEND_PATH);
    if (!mapp(friends)) return 0;
    map_delete(friends, id);
    set(FRIEND_PATH, friends);

    // 同时清除亲密值
    mapping intimate = query(FRIEND_INTIMATE_PATH);
    if (mapp(intimate))
    {
        map_delete(intimate, id);
    }
    return 1;
}

// 添加黑名单
int add_blacklist(string id)
{
    mapping blacklist = query(FRIEND_BLACKLIST_PATH);
    if (!mapp(blacklist)) blacklist = ([]);
    blacklist[id] = 1;
    set(FRIEND_BLACKLIST_PATH, blacklist);

    // 如果是好友则自动删除
    if (is_friend(id))
        remove_friend(id);

    return 1;
}

// 是否在黑名单中
int is_blacklisted(string id)
{
    mapping blacklist = query(FRIEND_BLACKLIST_PATH);
    if (!mapp(blacklist)) return 0;
    return !undefinedp(blacklist[id]);
}

// 删除黑名单
int remove_blacklist(string id)
{
    mapping blacklist = query(FRIEND_BLACKLIST_PATH);
    if (!mapp(blacklist)) return 0;
    map_delete(blacklist, id);
    set(FRIEND_BLACKLIST_PATH, blacklist);
    return 1;
}

// ======== 亲密值系统 ========

// 查询与某好友的亲密值
int query_intimate(string id)
{
    mapping intimate = query(FRIEND_INTIMATE_PATH);
    if (!mapp(intimate)) return 0;
    return intimate[id];
}

// 增加亲密值
int add_intimate(string id, int amount)
{
    int current = query_intimate(id);
    mapping intimate = query(FRIEND_INTIMATE_PATH);
    if (!mapp(intimate)) intimate = ([]);

    // 每日上限检查
    string today_key = "friend/intimate_daily/" + id + "/" + (time() / 86400);
    int daily = query(today_key);
    if (daily + amount > INTIMATE_DAILY_CAP)
        amount = INTIMATE_DAILY_CAP - daily;
    if (amount <= 0) return 0;

    intimate[id] = current + amount;
    set(FRIEND_INTIMATE_PATH, intimate);
    set(today_key, daily + amount);

    return intimate[id];
}

// 查询亲密等级
int query_intimate_level(string id)
{
    int val = query_intimate(id);

    if (val >= FRIEND_INTIMATE_THRESHOLD_5) return FRIEND_INTIMATE_COUPLE;
    if (val >= FRIEND_INTIMATE_THRESHOLD_4) return FRIEND_INTIMATE_CLOSE;
    if (val >= FRIEND_INTIMATE_THRESHOLD_3) return FRIEND_INTIMATE_LIFEDEATH;
    if (val >= FRIEND_INTIMATE_THRESHOLD_2) return FRIEND_INTIMATE_DAOYOU;
    if (val >= FRIEND_INTIMATE_THRESHOLD_1) return FRIEND_INTIMATE_OFTEN;
    return FRIEND_INTIMATE_ACQUAINTANCE;
}

// 获取亲密等级名称
string get_intimate_level_name(int level)
{
    switch (level)
    {
    case FRIEND_INTIMATE_ACQUAINTANCE: return FRIEND_INTIMATE_NAME_0;
    case FRIEND_INTIMATE_OFTEN:        return HIG FRIEND_INTIMATE_NAME_1 NOR;
    case FRIEND_INTIMATE_DAOYOU:       return HIB FRIEND_INTIMATE_NAME_2 NOR;
    case FRIEND_INTIMATE_LIFEDEATH:    return HIM FRIEND_INTIMATE_NAME_3 NOR;
    case FRIEND_INTIMATE_CLOSE:        return HIY FRIEND_INTIMATE_NAME_4 NOR;
    case FRIEND_INTIMATE_COUPLE:       return HIR FRIEND_INTIMATE_NAME_5 NOR;
    default: return FRIEND_INTIMATE_NAME_0;
    }
}

// ======== 好友互动 ========

// 获取好友在线状态
int query_friend_status()
{
    // 被查询: 返回自己的在线状态
    if (query_temp("combat_exp") > 0 && query("qi") < query("max_qi") * 50/100)
        return FRIEND_STATUS_BUSY;
    return FRIEND_STATUS_ONLINE;
}

// 获取好友在线状态名
string get_friend_status_name(int status)
{
    switch (status)
    {
    case FRIEND_STATUS_OFFLINE: return FRIEND_STATUS_NAME_0;
    case FRIEND_STATUS_ONLINE:  return HIG FRIEND_STATUS_NAME_1 NOR;
    case FRIEND_STATUS_BUSY:    return HIR FRIEND_STATUS_NAME_2 NOR;
    case FRIEND_STATUS_CLOSED:  return HIW FRIEND_STATUS_NAME_3 NOR;
    case FRIEND_STATUS_IDLE:    return HIC FRIEND_STATUS_NAME_4 NOR;
    default: return FRIEND_STATUS_NAME_0;
    }
}

// 格式化好友列表
string format_friend_list()
{
    string *ids = query_friends();
    if (!sizeof(ids))
        return "你目前没有任何好友。\n";

    string output = "╔══════════════ 好友列表 ══════════════╗\n";
    output += "  " + sprintf("%-12s %-10s %-12s %s",
             "好友ID", "姓名", "亲密等级", "状态") + "\n";
    output += "──────────────────────────────────────────\n";

    for (int i = 0; i < sizeof(ids); i++)
    {
        mapping friends = query(FRIEND_PATH);
        string name = friends[ids[i]];
        string intimate_name = get_intimate_level_name(query_intimate_level(ids[i]));
        string status;

        // 查找玩家是否在线
        object ob = find_player(ids[i]);
        if (ob)
            status = ob->get_friend_status_name(ob->query_friend_status());
        else
            status = HIK "离线" NOR;

        output += "  " + sprintf("%-12s %-10s %-16s %s",
                 ids[i], name, intimate_name, status) + "\n";
    }

    output += "╚══════════════════════════════════════════╝\n";
    return output;
}

// 获取好友数量
int query_friend_count()
{
    return sizeof(query_friends());
}

// 获取好友上限
int query_friend_max()
{
    int exp = query("combat_exp");
    if (exp >= 50000000) return FRIEND_MAX_YUANYING;
    if (exp >= 10000000) return FRIEND_MAX_JIEDAN;
    if (exp >= 1000000) return FRIEND_MAX_ZHUIJI;
    return FRIEND_MAX_DEFAULT;
}
