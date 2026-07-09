// friend_d.c
// 好友系统守护进程 - 好友管理、亲密值、在线状态
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第4章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_DBASE;
inherit F_SAVE;

// 好友申请队列: ([ applicant_id: ([ target_id: time, ... ]), ... ])
nosave mapping pending_requests = ([]);

// 在线状态缓存: ([ player_id: status, ... ])
nosave mapping online_status = ([]);

void create()
{
    seteuid(getuid());
    restore();
    set("channel_id", HIG "好友系统" NOR);
}

string query_save_file()
{
    return "/data/friend_d";
}

// ======== 好友申请管理 ========

// 发送好友申请
int send_request(string applicant, string target)
{
    if (!pending_requests[applicant])
        pending_requests[applicant] = ([]);

    pending_requests[applicant][target] = time();
    return 1;
}

// 接受好友申请
int accept_request(string applicant, string target)
{
    if (!mapp(pending_requests[applicant]))
        return 0;
    if (undefinedp(pending_requests[applicant][target]))
        return 0;

    map_delete(pending_requests[applicant], target);
    if (sizeof(pending_requests[applicant]) == 0)
        map_delete(pending_requests, applicant);

    return 1;
}

// 拒绝好友申请
int reject_request(string applicant, string target)
{
    if (!mapp(pending_requests[applicant]))
        return 0;
    if (undefinedp(pending_requests[applicant][target]))
        return 0;

    map_delete(pending_requests[applicant], target);
    if (sizeof(pending_requests[applicant]) == 0)
        map_delete(pending_requests, applicant);

    return 1;
}

// 获取某玩家的好友申请列表
mapping get_pending_requests(string player_id)
{
    mapping result = ([]);

    foreach (string applicant, mapping targets in pending_requests)
    {
        if (!undefinedp(targets[player_id]))
            result[applicant] = targets[player_id];
    }

    return result;
}

// 清理过期申请(超过7天)
void clean_expired_requests()
{
    int now = time();
    int expire = 7 * 86400;

    foreach (string applicant, mapping targets in pending_requests)
    {
        foreach (string target, int timestamp in targets)
        {
            if (now - timestamp > expire)
                map_delete(targets, target);
        }
        if (sizeof(targets) == 0)
            map_delete(pending_requests, applicant);
    }
}

// ======== 在线状态管理 ========

// 更新在线状态
void update_status(string player_id, int status)
{
    if (status == FRIEND_STATUS_OFFLINE)
        map_delete(online_status, player_id);
    else
        online_status[player_id] = status;
}

// 查询在线状态
int query_status(string player_id)
{
    if (undefinedp(online_status[player_id]))
        return FRIEND_STATUS_OFFLINE;
    return online_status[player_id];
}

// ======== 互动玩法辅助 ========

// 组队任务亲密值计算
int calc_team_intimate(int party_size, string task_difficulty)
{
    int base = INTIMATE_TEAM_TASK;

    if (task_difficulty == "hard") base += 10;
    if (task_difficulty == "elite") base += 20;

    // 人数越多，亲密值略高(更多人配合)
    if (party_size >= 5) base += 10;
    else if (party_size >= 3) base += 5;

    return base + random(11);  // +0~10
}

// 礼物亲密值计算
int calc_gift_intimate(int item_value)
{
    // 根据物品价值计算亲密值
    if (item_value >= 100000) return 20;
    if (item_value >= 50000) return 15;
    if (item_value >= 10000) return 10;
    if (item_value >= 1000) return 5;
    return 1;
}

// 查询友好度排名
mixed *query_intimate_ranking(string player_id)
{
    object player = find_player(player_id);
    if (!player) return ({});

    string *friends = player->query_friends();
    if (!sizeof(friends)) return ({});

    // 构建排序数组
    mixed *ranking = ({});
    for (int i = 0; i < sizeof(friends); i++)
    {
        int val = player->query_intimate(friends[i]);
        ranking += ({ ({ friends[i], val }) });
    }

    // 按亲密值降序
    ranking = sort_array(ranking, (: $1[1] > $2[1] ? -1 : 1 :));

    return ranking;
}

// 格式化显示友好度排名
string format_intimate_ranking(object player)
{
    if (!player) return "";

    string *friends = player->query_friends();
    if (!sizeof(friends))
        return "你还没有任何好友。\n";

    mixed *ranking = query_intimate_ranking(player->query("id"));
    if (!sizeof(ranking))
        return "暂无亲密数据。\n";

    string output = "╔══════════ 亲密好友排行 ══════════╗\n";
    output += "  " + sprintf("%-4s %-12s %-10s %s", "排名", "好友ID", "亲密值", "等级") + "\n";
    output += "──────────────────────────────────────\n";

    int max_show = sizeof(ranking) > 10 ? 10 : sizeof(ranking);
    for (int i = 0; i < max_show; i++)
    {
        string fid = ranking[i][0];
        int val = ranking[i][1];
        string level_name = player->get_intimate_level_name(player->query_intimate_level(fid));

        string rank_str;
        if (i == 0) rank_str = HIR "★1" NOR;
        else if (i == 1) rank_str = HIY "★2" NOR;
        else if (i == 2) rank_str = HIC "★3" NOR;
        else rank_str = sprintf(" %d", i + 1);

        output += "  " + sprintf("%-4s %-12s %-8d %s", rank_str, fid, val, level_name) + "\n";
    }

    output += "╚══════════════════════════════════════╝\n";
    return output;
}
