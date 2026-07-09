// squad.c
// 修仙小队/固定队系统特性
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第5.3章

#include <ansi.h>
#include <reputation_ext.h>

// 小队数据存储在玩家身上
// squad: ([
//   "id": squad_id,       // 小队唯一ID
//   "name": squad_name,   // 小队名称
//   "leader": leader_id,  // 队长ID
//   "members": ({ ... }), // 成员ID列表
//   "created": timestamp,
//   "active_days": 0,     // 持续活跃天数
//   "realm_reached": 0    // 全员境界达标
// ])

// 获取小队信息
mapping query_squad()
{
    return query(SQUAD_PATH);
}

// 是否有小队
int has_squad()
{
    mapping sq = query(SQUAD_PATH);
    return mapp(sq);
}

// 获取小队ID
int query_squad_id()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return 0;
    return sq["id"];
}

// 获取小队名称
string query_squad_name()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return "";
    return sq["name"];
}

// 是否是队长
int is_squad_leader()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return 0;
    return sq["leader"] == query("id");
}

// 获取小队成员
string *query_squad_members()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return ({});
    return sq["members"];
}

// 在小队中
int in_squad()
{
    mapping sq = query(SQUAD_PATH);
    return mapp(sq);
}

// 创建小队
int create_squad(string name)
{
    if (has_squad()) return 0;
    if (strwidth(name) > SQUAD_NAME_MAX_LEN) return 0;

    // 检查境界要求
    // 实际应检查筑基期

    int squad_id = (time() % 1000000) + random(100000);
    set(SQUAD_PATH, ([
        "id": squad_id,
        "name": name,
        "leader": query("id"),
        "members": ({ query("id") }),
        "created": time(),
        "active_days": 0,
        "daily_activity": time() / 86400
    ]));

    return 1;
}

// 加入小队
int join_squad(object player)
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return 0;
    if (!is_squad_leader()) return 0;

    // 检查亲密等级
    if (player->query_intimate_level(query("id")) < SQUAD_MIN_INTIMATE)
        return -1;

    // 检查人数
    if (sizeof(sq["members"]) >= SQUAD_MAX_MEMBERS)
        return -2;

    // 检查是否已有小队
    if (player->has_squad())
        return -3;

    string *members = sq["members"];
    members += ({ player->query("id") });
    sq["members"] = members;

    // 同步更新对方
    player->set(SQUAD_PATH, ([
        "id": sq["id"],
        "name": sq["name"],
        "leader": sq["leader"],
        "members": members,
        "created": sq["created"],
        "active_days": sq["active_days"],
        "daily_activity": sq["daily_activity"]
    ]));

    set(SQUAD_PATH, sq);

    return 1;
}

// 离开小队
int leave_squad()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return 0;

    // 队长离开=解散
    if (is_squad_leader())
    {
        disband_squad();
        return 1;
    }

    string id = query("id");
    string *members = sq["members"];
    members -= ({ id });
    sq["members"] = members;

    set(SQUAD_PATH, sq);
    delete(SQUAD_PATH); // 自己删除小队数据

    // 通知队长
    object leader = find_player(sq["leader"]);
    if (leader)
        tell_object(leader, sprintf("%s(%s) 离开了小队。\n",
                    query("name"), id));

    return 1;
}

// 踢出成员
int kick_member(string member_id)
{
    if (!is_squad_leader()) return 0;

    mapping sq = query(SQUAD_PATH);
    string *members = sq["members"];

    if (member_array(member_id, members) == -1)
        return 0;

    members -= ({ member_id });
    sq["members"] = members;
    set(SQUAD_PATH, sq);

    // 通知被踢玩家
    object ob = find_player(member_id);
    if (ob)
    {
        ob->delete(SQUAD_PATH);
        tell_object(ob, HIR "你被队长移出了小队。\n" NOR);
    }

    return 1;
}

// 解散小队
int disband_squad()
{
    if (!is_squad_leader()) return 0;

    mapping sq = query(SQUAD_PATH);
    string *members = sq["members"];

    foreach (string mid in members)
    {
        if (mid == query("id")) continue;
        object ob = find_player(mid);
        if (ob)
        {
            ob->delete(SQUAD_PATH);
            tell_object(ob, HIR "你的小队已被队长解散。\n" NOR);
        }
    }

    delete(SQUAD_PATH);
    return 1;
}

// ======== 小队福利 ========

// 检查小队活跃度
void check_squad_activity()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return;

    int today = time() / 86400;
    int last_active = sq["daily_activity"];

    if (today != last_active)
    {
        sq["active_days"] += 1;
        sq["daily_activity"] = today;
        set(SQUAD_PATH, sq);

        // 同步更新所有成员
        foreach (string mid in sq["members"])
        {
            object ob = find_player(mid);
            if (ob && ob != this_object())
            {
                mapping other_sq = ob->query_squad();
                if (mapp(other_sq) && other_sq["id"] == sq["id"])
                {
                    other_sq["active_days"] = sq["active_days"];
                    other_sq["daily_activity"] = today;
                    ob->set(SQUAD_PATH, other_sq);
                }
            }
        }
    }
}

// 获取小队经验加成
int query_squad_exp_bonus(int party_size)
{
    if (party_size < 3) return 0;
    return SQUAD_BONUS_EXP;
}

// 获取小队声望加成
int query_squad_rep_bonus(int party_size)
{
    if (party_size < 3) return 0;
    return SQUAD_BONUS_REP;
}

// 获取小队掉落加成
int query_squad_drop_bonus(int party_size)
{
    if (party_size < 3) return 0;
    return SQUAD_BONUS_DROP;
}

// 是否有灵脉权限
int has_lingmai_access()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return 0;
    return sq["active_days"] >= SQUAD_DAYS_FOR_LINGMAI;
}

// 是否有洞府权限
int has_base_access()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return 0;
    return sq["active_days"] >= SQUAD_DAYS_FOR_BASE;
}

// ======== 格式化 ========

// 显示小队信息
string format_squad_info()
{
    mapping sq = query(SQUAD_PATH);
    if (!mapp(sq)) return "你还没有加入任何小队。\n";

    string output = sprintf(
        "╔══════════ 小队信息 ══════════╗\n");
    output += sprintf("  队名: " HIY "%s" NOR "\n", sq["name"]);
    output += sprintf("  队长: %s\n", sq["leader"]);

    string *members = sq["members"];
    output += sprintf("  成员(%d/%d):\n", sizeof(members), SQUAD_MAX_MEMBERS);
    for (int i = 0; i < sizeof(members); i++)
    {
        string flag = (members[i] == sq["leader"]) ? HIR "★" NOR : "  ";
        object ob = find_player(members[i]);
        string name = ob ? ob->query("name") : members[i];
        string online = ob ? HIG "在线" NOR : HIK "离线" NOR;
        output += sprintf("    %s %-12s %s\n", flag, name, online);
    }

    output += sprintf("\n  活跃天数: %d 天\n", sq["active_days"]);

    if (sq["active_days"] >= SQUAD_DAYS_FOR_LINGMAI)
        output += HIG "  ✓ 已解锁灵脉权限\n" NOR;
    else
        output += sprintf("  灵脉解锁还需 %d 天\n", SQUAD_DAYS_FOR_LINGMAI - sq["active_days"]);

    output += "╚═════════════════════════════════╝\n";

    return output;
}
