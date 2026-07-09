// diplomacy_d.c
// 灵界种族外交事件守护进程
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第7章

#include <ansi.h>
#include <reputation.h>
#include <reputation_ext.h>

inherit F_DBASE;
inherit F_SAVE;

// 当前活跃的外交事件
nosave mapping active_events = ([]);
nosave int event_id_counter;

// 外交事件模板
nosave mapping event_templates = ([
    "border_conflict": ([
        "name": "边界冲突",
        "type": DIPLOMACY_BORDER,
        "desc": "角蚩族劫掠队入侵人族边境",
        "base_rep": DIPLOMACY_REP_BASE,
        "participants": ({ "jiaochi", "human" }),
        "related": ({ "demon", "wood" }),
        "frequency": DIPLOMACY_FREQ_BORDER
    ]),
    "race_dispute": ([
        "name": "种族纠纷",
        "type": DIPLOMACY_DISPUTE,
        "desc": "两族间因资源/领地发生矛盾",
        "base_rep": DIPLOMACY_REP_MINOR * 2,
        "participants": ({ "demon", "wood" }),
        "related": ({ "human", "jiaochi" }),
        "frequency": DIPLOMACY_FREQ_DISPUTE
    ]),
    "resource_discovery": ([
        "name": "资源发现",
        "type": DIPLOMACY_RESOURCE,
        "desc": "新矿脉/灵脉被发现，多族争夺",
        "base_rep": DIPLOMACY_REP_BASE,
        "participants": ({ "demon", "jiaochi", "wood" }),
        "related": ({ "human", "yun" }),
        "frequency": DIPLOMACY_FREQ_RESOURCE
    ]),
    "alliance_proposal": ([
        "name": "联盟提议",
        "type": DIPLOMACY_ALLIANCE,
        "desc": "某族提议建立军事/经济联盟",
        "base_rep": DIPLOMACY_REP_BASE * 2,
        "participants": ({ "demon", "human" }),
        "related": ({ "wood", "feiling" }),
        "frequency": DIPLOMACY_FREQ_ALLIANCE
    ]),
    "disaster_event": ([
        "name": "天灾事件",
        "type": DIPLOMACY_DISASTER,
        "desc": "某族领地遭遇天灾",
        "base_rep": DIPLOMACY_REP_BASE,
        "participants": ({ "demon", "wood" }),
        "related": ({ "yecha", "crystal" }),
        "frequency": DIPLOMACY_FREQ_DISASTER
    ]),
    "foreign_invasion": ([
        "name": "外族入侵",
        "type": DIPLOMACY_INVASION,
        "desc": "角蚩族大规模入侵",
        "base_rep": DIPLOMACY_REP_BASE * 3,
        "participants": ({ "jiaochi", "human", "demon" }),
        "related": ({ "wood", "sea_king", "feiling" }),
        "frequency": DIPLOMACY_FREQ_INVASION
    ]),
    "cross_race_auction": ([
        "name": "跨族拍卖会",
        "type": DIPLOMACY_AUCTION,
        "desc": "广源斋组织各族珍品拍卖",
        "base_rep": DIPLOMACY_REP_BASE,
        "participants": ({ "human", "demon", "wood", "yecha" }),
        "related": ({ "feiling", "crystal", "yun" }),
        "frequency": DIPLOMACY_FREQ_AUCTION
    ])
]);

void create()
{
    seteuid(getuid());
    restore();
    set("channel_id", HIG "外交系统" NOR);
    if (!event_id_counter) event_id_counter = 1;
}

string query_save_file()
{
    return "/data/diplomacy_d";
}

// ======== 外交事件生成 ========

// 生成随机外交事件
int generate_event()
{
    string *templates = keys(event_templates);
    string tmpl_key = templates[random(sizeof(templates))];
    mapping tmpl = event_templates[tmpl_key];

    int id = event_id_counter++;
    mapping participants = ([]);
    mapping related = ([]);

    // 初始化参与者声望变化
    foreach (string race in tmpl["participants"])
        participants[race] = ([ "base_change": 0, "importance": DIPLOMACY_IMPORT_CORE ]);

    foreach (string race in tmpl["related"])
        related[race] = ([ "base_change": 0, "importance": DIPLOMACY_IMPORT_RELATED ]);

    // 生成具体描述
    string desc = generate_event_desc(tmpl);

    active_events[id] = ([
        "id": id,
        "template": tmpl_key,
        "name": tmpl["name"],
        "type": tmpl["type"],
        "desc": desc,
        "base_rep": tmpl["base_rep"],
        "participants": participants,
        "related": related,
        "choices": generate_choices(tmpl),
        "status": "active",
        "expire_time": time() + 7 * 86400,  // 7天有效期
        "player_count": ({})  // 参与玩家
    ]);

    // 全服公告
    broadcast_event(id);

    save();
    return id;
}

// 生成事件描述
string generate_event_desc(mapping tmpl)
{
    string desc;

    switch (tmpl["type"])
    {
    case DIPLOMACY_BORDER:
        desc = sprintf("%s角蚩族劫掠队出现在%s边境，大肆掠夺资源，人族边境驻军请求支援。%s", HIR, NOR, NOR);
        break;
    case DIPLOMACY_DISPUTE:
        desc = sprintf("%s妖族与木族因灵脉归属发生争执，双方剑拔弩张，局势一触即发。%s", HIY, NOR);
        break;
    case DIPLOMACY_RESOURCE:
        desc = sprintf("%s在雷鸣大陆发现一座大型灵晶矿脉，多族闻风而动，争夺开采权。%s", HIG, NOR);
        break;
    case DIPLOMACY_ALLIANCE:
        desc = sprintf("%s妖族使者抵达天渊城，提议建立三族联合巡逻队，共同对抗魔界入侵。%s", HIB, NOR);
        break;
    case DIPLOMACY_DISASTER:
        desc = sprintf("%s木族领地遭遇灵潮爆发，大量灵植枯萎，木族向各族求援。%s", HIC, NOR);
        break;
    case DIPLOMACY_INVASION:
        desc = sprintf("%s角蚩族集结大军，大举入侵灵界边界！全灵界告急！%s", HIR, NOR);
        break;
    case DIPLOMACY_AUCTION:
        desc = sprintf("%s广源斋举办百年一度跨族拍卖会，各族珍品云集，各方势力齐聚一堂。%s", HIY, NOR);
        break;
    default:
        desc = "一起外交事件正在发生。";
    }

    return desc;
}

// 生成选项
mixed *generate_choices(mapping tmpl)
{
    mixed *choices = ({});

    switch (tmpl["type"])
    {
    case DIPLOMACY_BORDER:
        choices = ({
            ([
                "id": 1, "desc": "支援人族边境",
                "stance": DIPLOMACY_STANCE_HELP,
                "effects": ([ "human": 1000, "jiaochi": -1500 ]),
                "rep_total": DIPLOMACY_REP_BASE
            ]),
            ([
                "id": 2, "desc": "按兵不动，观望局势",
                "stance": DIPLOMACY_STANCE_NEUTRAL,
                "effects": ([ "human": -200 ]),
                "rep_total": 0
            ]),
            ([
                "id": 3, "desc": "暗中勾结角蚩族(风险)",
                "stance": DIPLOMACY_STANCE_ATTACK,
                "effects": ([ "jiaochi": 500, "human": -3000 ]),
                "rep_total": -1000
            ])
        });
        break;

    case DIPLOMACY_ALLIANCE:
        choices = ({
            ([
                "id": 1, "desc": "支持联盟提议，促成三族联合",
                "stance": DIPLOMACY_STANCE_HELP,
                "effects": ([ "demon": 2000, "human": 2000, "wood": 2000 ]),
                "rep_total": 2000
            ]),
            ([
                "id": 2, "desc": "表示中立，不参与联盟",
                "stance": DIPLOMACY_STANCE_NEUTRAL,
                "effects": ([ "demon": -500, "human": -500 ]),
                "rep_total": -200
            ]),
            ([
                "id": 3, "desc": "暗中破坏联盟(机密任务)",
                "stance": DIPLOMACY_STANCE_OPPOSE,
                "effects": ([ "jiaochi": 1000, "human": -1000 ]),
                "rep_total": -500
            ])
        });
        break;

    case DIPLOMACY_DISASTER:
        choices = ({
            ([
                "id": 1, "desc": "全力援助灾民",
                "stance": DIPLOMACY_STANCE_HELP,
                "effects": ([ "demon": 1500, "wood": 1500 ]),
                "rep_total": 1000
            ]),
            ([
                "id": 2, "desc": "提供有限援助",
                "stance": DIPLOMACY_STANCE_SUPPORT,
                "effects": ([ "demon": 500, "wood": 500 ]),
                "rep_total": 500
            ]),
            ([
                "id": 3, "desc": "趁火打劫，低价收购资源",
                "stance": DIPLOMACY_STANCE_ATTACK,
                "effects": ([ "wood": -2000, "demon": -1000 ]),
                "rep_total": -500
            ])
        });
        break;

    case DIPLOMACY_RESOURCE:
        choices = ({
            ([
                "id": 1, "desc": "加入人族勘探队",
                "stance": DIPLOMACY_STANCE_HELP,
                "effects": ([ "human": 1000, "jiaochi": -500 ]),
                "rep_total": 800
            ]),
            ([
                "id": 2, "desc": "独吞情报，自行开采",
                "stance": DIPLOMACY_STANCE_OPPOSE,
                "effects": ([ "human": -500, "demon": -500 ]),
                "rep_total": -300
            ]),
            ([
                "id": 3, "desc": "将情报卖给广源斋",
                "stance": DIPLOMACY_STANCE_NEUTRAL,
                "effects": ([ "guangyuan_pavilion": 800 ]),
                "rep_total": 500
            ])
        });
        break;

    default:
        choices = ({
            ([
                "id": 1, "desc": "积极参与",
                "stance": DIPLOMACY_STANCE_HELP,
                "effects": ([ "human": 500 ]),
                "rep_total": 500
            ]),
            ([
                "id": 2, "desc": "保持中立",
                "stance": DIPLOMACY_STANCE_NEUTRAL,
                "effects": ([]),
                "rep_total": 0
            ])
        });
    }

    return choices;
}

// ======== 玩家参与 ========

// 玩家做出选择
int player_choose(object player, int event_id, int choice_id)
{
    if (!active_events[event_id])
        return -1; // 事件不存在
    if (active_events[event_id]["status"] != "active")
        return -2; // 事件已结束

    string pid = player->query("id");
    string *players = active_events[event_id]["player_count"];

    // 防止重复选择
    if (member_array(pid, players) != -1)
        return -3; // 已参与

    mapping tmpl = event_templates[active_events[event_id]["template"]];
    mixed *choices = active_events[event_id]["choices"];
    mapping selected = 0;

    foreach (mapping c in choices)
    {
        if (c["id"] == choice_id)
        {
            selected = c;
            break;
        }
    }

    if (!selected) return -4; // 无效选项

    // 应用声望影响
    foreach (string race, int change in selected["effects"])
    {
        string rep_path;
        // 判断是种族还是势力
        if (member_array(race, REPUTATION_D->get_all_races()) != -1)
            rep_path = REP_PATH_RACE;
        else
            rep_path = REP_PATH_FACTION;

        player->add(rep_path + "/" + race, change);
    }

    // 全局正魔声望影响
    int rep_total = selected["rep_total"];
    if (rep_total > 0)
        player->add(REP_PATH_GLOBAL + "/righteous", rep_total);
    else if (rep_total < 0)
        player->add(REP_PATH_GLOBAL + "/evil", -rep_total);

    // 记录参与
    players += ({ pid });
    active_events[event_id]["player_count"] = players;

    save();

    tell_object(player, HIG "你做出了选择，声望已发生变化。\n" NOR);
    return 1;
}

// ======== 广播 ========

// 广播事件
void broadcast_event(int event_id)
{
    if (!active_events[event_id]) return;

    mapping ev = active_events[event_id];
    string msg = sprintf(
        "\n" HIR "╔══════════════ 外交事件 ══════════════╗\n" NOR
        HIR "  【%s】\n" NOR "%s\n"
        HIR "╚═══════════════════════════════════════╝\n" NOR,
        ev["name"], ev["desc"]);

    // 显示选项
    mixed *choices = ev["choices"];
    msg += "可能的行动:\n";
    foreach (mapping c in choices)
    {
        msg += sprintf("  " HIY "%d." NOR " %s\n", c["id"], c["desc"]);
    }
    msg += "使用指令: diplomacy <事件编号> <选项编号>\n";

    CHANNEL_D->do_channel(this_object(), "sys", msg, -1);
}

// ======== 查询 ========

// 获取当前活跃事件
mixed *get_active_events()
{
    mapping result = ([]);
    foreach (int id, mapping ev in active_events)
    {
        if (ev["status"] == "active" && ev["expire_time"] > time())
            result[id] = ev;
    }
    return values(result);
}

// 获取事件详情
mapping get_event_info(int event_id)
{
    if (!active_events[event_id]) return 0;
    return active_events[event_id];
}

// 格式化事件显示
string format_event(int event_id)
{
    if (!active_events[event_id]) return "事件不存在或已过期。\n";

    mapping ev = active_events[event_id];
    string output = sprintf(
        "╔══════════ %s ══════════╗\n", ev["name"]);
    output += ev["desc"] + "\n\n";
    output += HIY "可选行动:\n" NOR;

    mixed *choices = ev["choices"];
    foreach (mapping c in choices)
    {
        output += sprintf("  %d. %s\n", c["id"], c["desc"]);
    }

    output += sprintf("\n当前 %d 位玩家已参与。\n", sizeof(ev["player_count"]));
    output += "╚══════════════════════════════════════╝\n";

    return output;
}

// ======== 定时维护 ========

// 心跳(每小时检查)
void heartbeat()
{
    int now = time();

    // 清理过期事件
    foreach (int id, mapping ev in active_events)
    {
        if (ev["expire_time"] < now)
        {
            summarize_event(id);
            map_delete(active_events, id);
        }
    }

    // 随机生成新事件(10%概率每小时)
    if (random(100) < 10)
    {
        generate_event();
    }

    save();
}

// 事件总结
void summarize_event(int event_id)
{
    if (!active_events[event_id]) return;

    mapping ev = active_events[event_id];
    int player_count = sizeof(ev["player_count"]);

    if (player_count > 0)
    {
        string msg = sprintf(
            HIC "【外交事件】%s 已结束，共 %d 位玩家参与了这次事件。\n" NOR,
            ev["name"], player_count);
        CHANNEL_D->do_channel(this_object(), "sys", msg, -1);
    }
}

// 生成指定类型事件
int generate_specific_event(string template_key)
{
    if (!event_templates[template_key])
        return 0;

    return generate_event();
}
