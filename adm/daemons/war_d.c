// war_d.c
// 战争系统守护进程 - 门派战、正魔之战、乱星海争夺战、界面战争
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第6章

#include <ansi.h>
#include <reputation.h>
#include <reputation_ext.h>

inherit F_DBASE;
inherit F_SAVE;

// 当前战争列表
nosave mapping active_wars = ([]);
nosave int war_id_counter;

// 战区定义
nosave mapping war_zones = ([
    "jinyuan_plain": ([
        "name": "金鼓原", "type": WAR_TYPE_CLAN_BATTLE,
        "desc": "越国七派的主要战场"
    ]),
    "yue_state_border": ([
        "name": "越国边境", "type": WAR_TYPE_RIGHTEOUS_EVIL,
        "desc": "正魔之战的主要战场"
    ]),
    "tianxing_city": ([
        "name": "天星城", "type": WAR_TYPE_RANDOM_SEA,
        "desc": "乱星海争夺战的核心"
    ]),
    "tianyuan_fortress": ([
        "name": "天渊城", "type": WAR_TYPE_INTERFACE,
        "desc": "灵界对抗魔界的前线要塞"
    ]),
]);

void create()
{
    seteuid(getuid());
    restore();
    set("channel_id", HIG "战争系统" NOR);
    if (!war_id_counter) war_id_counter = 1;
}

string query_save_file()
{
    return "/data/war_d";
}

// ======== 战争管理 ========

// 创建战争
int create_war(string name, int war_type, string attacker, string defender,
               string zone_id, int duration)
{
    int id = war_id_counter++;
    int prep_end = time() + WAR_PREPARE_TIME;
    int battle_end = prep_end + (duration > 0 ? duration : WAR_BATTLE_TIME);

    active_wars[id] = ([
        "id": id,
        "name": name,
        "type": war_type,
        "attacker": attacker,
        "defender": defender,
        "zone": zone_id,
        "phase": WAR_PHASE_DECLARE,
        "prep_end": prep_end,
        "battle_end": battle_end,
        "attacker_score": 0,
        "defender_score": 0,
        "participants": ([
            "attacker": ({}),
            "defender": ({})
        ]),
        "merit": ([]),      // 个人战功: ([ player_id: merit ])
        "attackers_mvp": 0,  // 记录最高战功
        "defenders_mvp": 0,
        "announced": 0       // 是否已公告
    ]);

    save();

    // 全服公告
    string zone_name = war_zones[zone_id]["name"];
    string type_name = get_war_type_name(war_type);
    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIR "【战争】%s「%s」爆发！%s 对 %s 在 %s 发起%s！",
                type_name, name, attacker, defender, zone_name, type_name), -1);

    return id;
}

// 参战登记
int join_war(int war_id, object player, string side)
{
    if (!active_wars[war_id]) return 0;
    if (active_wars[war_id]["phase"] != WAR_PHASE_DECLARE)
        return 0; // 仅准备阶段可报名

    if (side != "attacker" && side != "defender") return 0;

    string pid = player->query("id");
    string *participants = active_wars[war_id]["participants"][side];

    if (member_array(pid, participants) != -1)
        return 2; // 已报名

    participants += ({ pid });
    active_wars[war_id]["participants"][side] = participants;
    active_wars[war_id]["merit"][pid] = 0;

    save();
    return 1;
}

// ======== 战斗阶段 ========

// 击杀记录(获取战功)
int record_kill(int war_id, object killer, object victim)
{
    if (!active_wars[war_id]) return 0;
    if (active_wars[war_id]["phase"] != WAR_PHASE_BATTLE)
        return 0;

    string killer_id = killer->query("id");
    string victim_id = victim->query("id");

    // 判断击杀者阵营
    string *attackers = active_wars[war_id]["participants"]["attacker"];
    string *defenders = active_wars[war_id]["participants"]["defender"];
    string killer_side;

    if (member_array(killer_id, attackers) != -1)
        killer_side = "attacker";
    else if (member_array(killer_id, defenders) != -1)
        killer_side = "defender";
    else
        return 0;

    // 验证受害者是敌对阵营
    if (killer_side == "attacker" && member_array(victim_id, defenders) == -1)
        return 0;
    if (killer_side == "defender" && member_array(victim_id, attackers) == -1)
        return 0;

    // 计算战功
    int merit_gain = WAR_REP_KILL + random(21); // 10~30
    int vic_exp = victim->query("combat_exp");
    int killer_exp = killer->query("combat_exp");

    // 击杀高阶对手获得更多战功
    if (vic_exp > killer_exp * 2) merit_gain = to_int(merit_gain * 1.5);
    if (vic_exp < killer_exp / 2) merit_gain = to_int(merit_gain * 0.5);

    // 记录战功
    active_wars[war_id]["merit"][killer_id] += merit_gain;
    if (killer_side == "attacker")
        active_wars[war_id]["attacker_score"] += 10;
    else
        active_wars[war_id]["defender_score"] += 10;

    // 声望影响
    string faction = get_war_faction(war_id, killer_side);
    if (faction)
        REPUTATION_D->add_reputation(killer, faction, merit_gain / 2,
                                     "war_kill:" + war_id);

    save();
    return merit_gain;
}

// 占领据点
int capture_point(int war_id, string side, int points)
{
    if (!active_wars[war_id]) return 0;

    if (side == "attacker")
        active_wars[war_id]["attacker_score"] += points;
    else if (side == "defender")
        active_wars[war_id]["defender_score"] += points;
    else
        return 0;

    save();
    return 1;
}

// ======== 阶段转换 ========

// 从宣战→战斗
void start_battle(int war_id)
{
    if (!active_wars[war_id]) return;
    if (active_wars[war_id]["phase"] != WAR_PHASE_DECLARE) return;

    active_wars[war_id]["phase"] = WAR_PHASE_BATTLE;
    active_wars[war_id]["battle_end"] = time() + WAR_BATTLE_TIME;

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIR "【战争】%s 战斗阶段开始！双方弟子可在战区厮杀！",
                active_wars[war_id]["name"]), -1);

    save();
}

// 战斗→结算
void start_settle(int war_id)
{
    if (!active_wars[war_id]) return;
    if (active_wars[war_id]["phase"] != WAR_PHASE_BATTLE) return;

    active_wars[war_id]["phase"] = WAR_PHASE_SETTLE;

    // 计算胜负
    int a_score = active_wars[war_id]["attacker_score"];
    int d_score = active_wars[war_id]["defender_score"];
    string winner, loser;

    if (a_score > d_score)
    {
        winner = "attacker";
        loser = "defender";
    }
    else if (d_score > a_score)
    {
        winner = "defender";
        loser = "attacker";
    }
    else
    {
        winner = "none";
        loser = "none";
    }

    // 发放奖励
    settle_rewards(war_id, winner, loser);

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIG "【战争】%s 结束！%s 获胜！",
                active_wars[war_id]["name"],
                winner != "none" ? (winner == "attacker" ?
                active_wars[war_id]["attacker"] : active_wars[war_id]["defender"]) : "平局"), -1);

    save();
}

// 结算奖励
void settle_rewards(int war_id, string winner, string loser)
{
    string *attackers = active_wars[war_id]["participants"]["attacker"];
    string *defenders = active_wars[war_id]["participants"]["defender"];
    string atk_name = active_wars[war_id]["attacker"];
    string def_name = active_wars[war_id]["defender"];

    // 胜方声望
    if (winner != "none")
    {
        string *winners = (winner == "attacker" ? attackers : defenders);
        string win_faction = get_war_faction(war_id, winner);

        foreach (string pid in winners)
        {
            object player = find_player(pid);
            if (!player) continue;

            // 全体声望
            if (win_faction)
                REPUTATION_D->add_reputation(player, win_faction, WAR_REP_WIN,
                                             "war_win:" + war_id);

            // 个人战功奖励
            int merit = active_wars[war_id]["merit"][pid];
            if (merit > 0)
            {
                int exp_reward = merit * 100;
                player->add("combat_exp", exp_reward);

                // 灵石奖励
                MONEY_D->player_receive(player, merit * 10);
            }
        }

        // 败方
        string *losers = (loser == "attacker" ? attackers : defenders);
        string lose_faction = get_war_faction(war_id, loser);

        foreach (string pid in losers)
        {
            object player = find_player(pid);
            if (!player) continue;

            if (lose_faction)
                REPUTATION_D->add_reputation(player, lose_faction, WAR_REP_LOSE,
                                             "war_lose:" + war_id);
        }
    }

    // MVP奖励
    string atk_mvp = get_mvp(attackers, war_id);
    string def_mvp = get_mvp(defenders, war_id);

    if (atk_mvp)
    {
        object player = find_player(atk_mvp);
        if (player)
        {
            REPUTATION_D->add_reputation(player, get_war_faction(war_id, "attacker"),
                                         WAR_REP_MVP, "war_mvp:" + war_id);
            tell_object(player, HIR "恭喜你在门派战中荣获 MVP！获得额外声望奖励！\n" NOR);
        }
    }

    if (def_mvp)
    {
        object player = find_player(def_mvp);
        if (player)
        {
            REPUTATION_D->add_reputation(player, get_war_faction(war_id, "defender"),
                                         WAR_REP_MVP, "war_mvp:" + war_id);
            tell_object(player, HIR "恭喜你在门派战中荣获 MVP！获得额外声望奖励！\n" NOR);
        }
    }
}

// 获取MVP
string get_mvp(string *participants, int war_id)
{
    if (!sizeof(participants)) return "";

    string mvp_id = participants[0];
    int max_merit = 0;

    foreach (string pid in participants)
    {
        int m = active_wars[war_id]["merit"][pid];
        if (m > max_merit)
        {
            max_merit = m;
            mvp_id = pid;
        }
    }

    return max_merit > 0 ? mvp_id : "";
}

// 获取战争对应势力
string get_war_faction(int war_id, string side)
{
    if (!active_wars[war_id]) return 0;

    int type = active_wars[war_id]["type"];
    string attacker = active_wars[war_id]["attacker"];
    string defender = active_wars[war_id]["defender"];

    // 按战争类型映射到 faction ID
    switch (type)
    {
    case WAR_TYPE_CLAN_BATTLE:
        // 门派战: attacker/defender 是门派ID
        return (side == "attacker" ? attacker : defender);

    case WAR_TYPE_RIGHTEOUS_EVIL:
        // 正魔之战
        if (side == "attacker")
            return "righteous_alliance";
        else
            return "demon_six_sects";

    case WAR_TYPE_RANDOM_SEA:
        if (side == "attacker")
            return "star_palace";
        else
            return "rebel_alliance";

    case WAR_TYPE_INTERFACE:
        return "tianyuan_city";
    }

    return 0;
}

// ======== 定时更新 ========

// 战争心跳(每分钟检查)
void war_heartbeat()
{
    int now = time();

    foreach (int id, mapping war in active_wars)
    {
        if (war["phase"] == WAR_PHASE_DECLARE)
        {
            if (now >= war["prep_end"])
            {
                // 准备结束，进入战斗
                start_battle(id);
            }
            else if (!war["announced"])
            {
                // 首次公告
                war["announced"] = 1;
                int remain = (war["prep_end"] - now) / 60;
                CHANNEL_D->do_channel(this_object(), "sys",
                    sprintf(HIY "【战争】%s 将于 %d 分钟后开始，请报名参战的弟子做好准备！",
                            war["name"], remain), -1);
            }
        }
        else if (war["phase"] == WAR_PHASE_BATTLE)
        {
            if (now >= war["battle_end"])
                start_settle(id);
        }
        else if (war["phase"] == WAR_PHASE_SETTLE)
        {
            // 结算后保留1小时，然后清理
            if (now > war["battle_end"] + 3600)
                map_delete(active_wars, id);
        }
    }
}

// ======== 查询接口 ========

// 获取战争类型名
string get_war_type_name(int type)
{
    switch (type)
    {
    case WAR_TYPE_CLAN_BATTLE:    return WAR_NAME_CLAN_BATTLE;
    case WAR_TYPE_RIGHTEOUS_EVIL: return WAR_NAME_RIGHTEOUS_EVIL;
    case WAR_TYPE_RANDOM_SEA:     return WAR_NAME_RANDOM_SEA;
    case WAR_TYPE_INTERFACE:      return WAR_NAME_INTERFACE;
    default: return "未知战争";
    }
}

// 获取进行中的战争
mixed *get_active_wars()
{
    mapping result = ([]);
    foreach (int id, mapping war in active_wars)
    {
        if (war["phase"] != WAR_PHASE_SETTLE)
            result[id] = war;
    }
    return values(result);
}

// 获取玩家战功
int query_player_merit(string player_id, int war_id)
{
    if (!active_wars[war_id]) return 0;
    return active_wars[war_id]["merit"][player_id];
}

// 获取玩家总战功
int query_total_merit(string player_id)
{
    int total = 0;
    foreach (int id, mapping war in active_wars)
    {
        total += war["merit"][player_id];
    }
    return total;
}

// 计算战功等级
int calc_merit_level(int total_merit)
{
    if (total_merit >= WAR_MERIT_THRESHOLD_7) return WAR_MERIT_MARSHAL;
    if (total_merit >= WAR_MERIT_THRESHOLD_6) return WAR_MERIT_GENERAL;
    if (total_merit >= WAR_MERIT_THRESHOLD_5) return WAR_MERIT_CAPTAIN;
    if (total_merit >= WAR_MERIT_THRESHOLD_4) return WAR_MERIT_VANGUARD;
    if (total_merit >= WAR_MERIT_THRESHOLD_3) return WAR_MERIT_SOLDIER;
    if (total_merit >= WAR_MERIT_THRESHOLD_2) return WAR_MERIT_SCOUT;
    return WAR_MERIT_RECRUIT;
}

// 获取战功等级名
string get_merit_level_name(int level)
{
    switch (level)
    {
    case WAR_MERIT_RECRUIT:   return "新兵";
    case WAR_MERIT_SCOUT:     return "哨兵";
    case WAR_MERIT_SOLDIER:   return "战士";
    case WAR_MERIT_VANGUARD:  return "先锋";
    case WAR_MERIT_CAPTAIN:   return "统领";
    case WAR_MERIT_GENERAL:   return "将军";
    case WAR_MERIT_MARSHAL:   return "元帅";
    default: return "新兵";
    }
}

// 格式化战争信息
string format_war_info(int war_id)
{
    if (!active_wars[war_id]) return "战争不存在。\n";

    mapping war = active_wars[war_id];
    string zone_name = war_zones[war["zone"]]["name"];
    string phase_name;
    string remain;

    switch (war["phase"])
    {
    case WAR_PHASE_DECLARE:
        phase_name = HIY "宣战准备期" NOR;
        remain = sprintf("%d分钟", (war["prep_end"] - time()) / 60);
        break;
    case WAR_PHASE_BATTLE:
        phase_name = HIR "战斗中" NOR;
        remain = sprintf("%d分钟", (war["battle_end"] - time()) / 60);
        break;
    case WAR_PHASE_SETTLE:
        phase_name = HIC "结算中" NOR;
        remain = "即将结束";
        break;
    default:
        phase_name = "未知";
        remain = "";
    }

    string output = sprintf(
        "╔══════════ %s ══════════╗\n",
        war["name"]);
    output += sprintf("  类型: %s\n", get_war_type_name(war["type"]));
    output += sprintf("  战区: %s\n", zone_name);
    output += sprintf("  阶段: %s (剩余%s)\n", phase_name, remain);
    output += sprintf("  进攻方: %s (积分: %d)\n",
                      war["attacker"], war["attacker_score"]);
    output += sprintf("  防守方: %s (积分: %d)\n",
                      war["defender"], war["defender_score"]);
    output += sprintf("  参战人数: %d vs %d\n",
                      sizeof(war["participants"]["attacker"]),
                      sizeof(war["participants"]["defender"]));
    output += "╚══════════════════════════════════════╝\n";

    return output;
}
