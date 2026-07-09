// sect_hq_d.c
// 宗门驻地系统守护进程 —— 驻地创建/升级/建筑管理/攻防战/收益结算
// 对应设计文档: 02-扩充内容/02-区域游戏玩法.md §4
// Created for ticket #22

#include <ansi.h>
#include <sect_hq.h>
#include <globals.h>

inherit F_DBASE;

// -------- 全局状态 -------------------------------------------------

// 所有宗门驻地数据
// sect_id -> ([
//     HQ_FIELD_ID       : sect_id,
//     HQ_FIELD_NAME     : "宗门名",
//     HQ_FIELD_TYPE     : HQ_TYPE_*,
//     HQ_FIELD_MASTER   : "player_id",
//     HQ_FIELD_MEMBERS  : ({ "id1", "id2", ... }),
//     HQ_FIELD_STATUS   : HQ_STATUS_*,
//     HQ_FIELD_CREATED  : time(),
//     HQ_FIELD_DURABILITY : int,
//     HQ_FIELD_BUILDINGS : ([ bldg_id -> ([ "built": time(), "level": 1, "status": 1 ]) ]),
//     HQ_FIELD_CONSTRUCTING : ([ bldg_id -> finish_time ]),
//     HQ_FIELD_FUNDS    : int,
//     HQ_FIELD_TAX_RATE : float,
//     HQ_FIELD_LOCATION : "room_path",
//     HQ_FIELD_SIEGE    : ([ siege data ]),
//     HQ_FIELD_LAST_SIEGE : time(),
//     HQ_FIELD_REPUTE   : int,
// ])
nosave mapping sect_hqs = ([]);

// 序列号
nosave int sect_serial = 0;

// -------- 前向声明 ------------------------------------------------
int calculate_daily_output(string sect_id);
string damage_level_name(int durability);
int calc_repair_cost(string sect_id);
int calc_repair_time(string sect_id);

// ======== 创建与心跳 ==============================================

void create()
{
    seteuid(getuid());
    set("channel_id", "宗门驻地精灵");
    set("name", "宗门驻地系统");

    CHANNEL_D->do_channel(this_object(), "sys", "宗门驻地系统已启动。");
    set_heart_beat(600);  // 每10分钟检查一次
}

void heart_beat()
{
    string *ids;
    int i;

    ids = keys(sect_hqs);

    for (i = 0; i < sizeof(ids); i++)
    {
        // 检查在建建筑
        check_construction(ids[i]);

        // 检查攻防战状态
        check_siege_status(ids[i]);

        // 检查是否需要每日结算（每10分钟触发一次）
        check_daily_output(ids[i]);
    }
}

// ======== 宗门驻地基础框架 ========================================

// 创建宗门驻地
// 返回生成的 sect_id，失败返回 0
string create_sect_hq(object me, string sect_name)
{
    string sect_id;

    if (!objectp(me) || !userp(me))
        return 0;

    sect_serial++;
    sect_id = sprintf("sect_%s_%d", me->query("id"), sect_serial);

    sect_hqs[sect_id] = ([
        HQ_FIELD_ID        : sect_id,
        HQ_FIELD_NAME      : sect_name,
        HQ_FIELD_TYPE      : HQ_TYPE_MOUNTAIN,
        HQ_FIELD_MASTER    : me->query("id"),
        HQ_FIELD_MEMBERS   : ({ me->query("id") }),
        HQ_FIELD_STATUS    : HQ_STATUS_NORMAL,
        HQ_FIELD_CREATED   : time(),
        HQ_FIELD_DURABILITY : HQ_DURABILITY_FULL,
        HQ_FIELD_BUILDINGS : ([]),
        HQ_FIELD_CONSTRUCTING : ([]),
        HQ_FIELD_FUNDS     : 0,
        HQ_FIELD_TAX_RATE  : 1.0,
        HQ_FIELD_LOCATION  : base_name(environment(me)),
        HQ_FIELD_SIEGE     : ([
            "phase"    : HQ_SIEGE_PHASE_NONE,
        ]),
        HQ_FIELD_LAST_SIEGE : 0,
        HQ_FIELD_REPUTE    : 0,
    ]);

    // 初始自带议事大殿（核心）
    sect_hqs[sect_id][HQ_FIELD_BUILDINGS][HQ_BLDG_CORE] = ([
        "built"  : time(),
        "level"  : 1,
        "status" : 1,
    ]);

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIC "【宗门公告】%s 在修仙界创立了「%s」山门！\n" NOR,
                me->name(), sect_name));

    return sect_id;
}

// 查询宗门驻地
mapping query_sect_hq(string sect_id)
{
    if (!stringp(sect_id) || undefinedp(sect_hqs[sect_id]))
        return ([]);

    return sect_hqs[sect_id];
}

// 删除宗门驻地（灭门/解散）
int destroy_sect_hq(string sect_id)
{
    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIR "【宗门公告】%s 山门已覆灭，消散于天地之间！\n" NOR,
                sect_hqs[sect_id][HQ_FIELD_NAME]));

    map_delete(sect_hqs, sect_id);
    return 1;
}

// 获取所有宗门ID
string *query_all_sects()
{
    return keys(sect_hqs);
}

// 获取玩家所属的宗门
string query_player_sect(string player_id)
{
    string *ids;
    int i;

    ids = keys(sect_hqs);
    for (i = 0; i < sizeof(ids); i++)
    {
        if (member_array(player_id, sect_hqs[ids[i]][HQ_FIELD_MEMBERS]) != -1)
            return ids[i];
    }

    return 0;
}

// 检查玩家是否为宗主
int is_sect_master(object me)
{
    string pid, sid;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    sid = query_player_sect(pid);

    if (!stringp(sid))
        return 0;

    return (sect_hqs[sid][HQ_FIELD_MASTER] == pid);
}

// ======== 成员管理 ================================================

// 加入宗门
int join_sect(string sect_id, object me)
{
    string pid;

    if (!objectp(me) || !userp(me))
        return 0;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    pid = me->query("id");

    // 已在宗门中
    if (stringp(query_player_sect(pid)))
        return 0;

    sect_hqs[sect_id][HQ_FIELD_MEMBERS] += ({ pid });

    tell_object(me, HIG "你加入了「" + sect_hqs[sect_id][HQ_FIELD_NAME] +
                "」宗门！\n" NOR);

    return 1;
}

// 离开宗门
int leave_sect(string sect_id, object me)
{
    string pid;
    string *members;
    int idx;

    if (!objectp(me) || !userp(me))
        return 0;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    pid = me->query("id");

    // 宗主不能离开
    if (sect_hqs[sect_id][HQ_FIELD_MASTER] == pid)
        return 0;

    members = sect_hqs[sect_id][HQ_FIELD_MEMBERS];
    idx = member_array(pid, members);
    if (idx == -1)
        return 0;

    members[idx..idx] = ({});
    sect_hqs[sect_id][HQ_FIELD_MEMBERS] = members;

    tell_object(me, HIY "你离开了「" + sect_hqs[sect_id][HQ_FIELD_NAME] +
                "」宗门。\n" NOR);

    return 1;
}

// 踢出成员
int kick_member(string sect_id, string target_pid)
{
    string *members;
    int idx;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    // 不能踢宗主
    if (sect_hqs[sect_id][HQ_FIELD_MASTER] == target_pid)
        return 0;

    members = sect_hqs[sect_id][HQ_FIELD_MEMBERS];
    idx = member_array(target_pid, members);
    if (idx == -1)
        return 0;

    members[idx..idx] = ({});
    sect_hqs[sect_id][HQ_FIELD_MEMBERS] = members;

    // 通知被踢玩家（如果在线）
    {
        object target;
        target = find_player(target_pid);
        if (objectp(target))
        {
            tell_object(target, HIR "你被「" + sect_hqs[sect_id][HQ_FIELD_NAME] +
                        "」踢出了宗门！\n" NOR);
        }
    }

    return 1;
}

// ======== 驻地升级 ================================================

// 升级驻地类型
// 返回 1=成功, 0=失败, 负值=错误码
int upgrade_hq_type(string sect_id)
{
    mapping hq;
    int cur_type, next_type, funds_needed, i;
    string *bldg_types;

    if (undefinedp(sect_hqs[sect_id]))
        return -10;

    hq = sect_hqs[sect_id];
    cur_type = hq[HQ_FIELD_TYPE];

    if (cur_type >= HQ_TYPE_SANCTUARY)
        return -20;  // 已达最高等级

    next_type = cur_type + 1;

    // 检查成员数量
    {
        int member_count = sizeof(hq[HQ_FIELD_MEMBERS]);
        int min_members;

        switch (next_type)
        {
        case HQ_TYPE_STANDARD:  min_members = 50;  break;
        case HQ_TYPE_CORE:      min_members = 200; break;
        case HQ_TYPE_SANCTUARY: min_members = 1000; break;
        default:                min_members = 0;
        }

        if (member_count < min_members)
            return -30;  // 成员不足
    }

    // 检查资金
    switch (next_type)
    {
    case HQ_TYPE_STANDARD:  funds_needed = 100000;  break;
    case HQ_TYPE_CORE:      funds_needed = 500000;  break;
    case HQ_TYPE_SANCTUARY: funds_needed = 2000000; break;
    default:                funds_needed = 0;
    }

    if (hq[HQ_FIELD_FUNDS] < funds_needed)
        return -40;  // 资金不足

    // 扣除资金
    hq[HQ_FIELD_FUNDS] -= funds_needed;

    // 升级
    hq[HQ_FIELD_TYPE] = next_type;

    // 升级时增加耐久上限（实际效果用比例体现）
    hq[HQ_FIELD_DURABILITY] = HQ_DURABILITY_FULL;

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIC "【宗门公告】%s 已升级为「%s」！\n" NOR,
                hq[HQ_FIELD_NAME], HQ_TYPE_NAME[next_type]));

    return 1;
}

// 升级失败文本
string upgrade_fail_reason(int code)
{
    switch (code)
    {
    case -10: return "宗门不存在。\n";
    case -20: return "已达最高驻地等级。\n";
    case -30: return "成员数量不足以升级驻地。\n";
    case -40: return "宗门资金不足以升级驻地。\n";
    default:  return "驻地升级失败。\n";
    }
}

// ======== 建筑系统 ================================================

// 建造建筑
// 返回 1=成功, 0=失败, 负值=错误码
int construct_building(string sect_id, int bldg_type)
{
    mapping hq, buildings, constructing;
    int cost, contrib, build_time, unlock_type;

    if (undefinedp(sect_hqs[sect_id]))
        return -10;

    hq = sect_hqs[sect_id];

    // 检查解锁条件
    unlock_type = HQ_BLDG_UNLOCK[bldg_type];
    if (hq[HQ_FIELD_TYPE] < unlock_type)
        return -20;  // 驻地等级不足

    // 检查是否已建造
    buildings = hq[HQ_FIELD_BUILDINGS];
    if (mapp(buildings[bldg_type]))
        return -30;  // 已建造

    // 检查是否正在建造
    constructing = hq[HQ_FIELD_CONSTRUCTING];
    if (mapp(constructing) && constructing[bldg_type] > 0)
        return -35;  // 已在建造中

    // 检查费用
    cost = HQ_BLDG_COST[bldg_type];
    if (hq[HQ_FIELD_FUNDS] < cost)
        return -40;  // 资金不足

    build_time = HQ_BLDG_BUILD_TIME[bldg_type];

    // 扣除资金
    hq[HQ_FIELD_FUNDS] -= cost;

    // 开始建造（记录完成时间）
    if (!mapp(constructing))
        hq[HQ_FIELD_CONSTRUCTING] = ([]);
    hq[HQ_FIELD_CONSTRUCTING][bldg_type] = time() + build_time;

    return 1;
}

// 检查在建建筑（由 heart_beat 调用）
void check_construction(string sect_id)
{
    mapping hq, constructing;
    string *bldg_ids;
    int i, bldg_type;

    if (undefinedp(sect_hqs[sect_id]))
        return;

    hq = sect_hqs[sect_id];
    constructing = hq[HQ_FIELD_CONSTRUCTING];

    if (!mapp(constructing))
        return;

    bldg_ids = keys(constructing);
    for (i = 0; i < sizeof(bldg_ids); i++)
    {
        bldg_type = to_int(bldg_ids[i]);

        if (constructing[bldg_type] <= time())
        {
            // 建造完成
            hq[HQ_FIELD_BUILDINGS][bldg_type] = ([
                "built"  : time(),
                "level"  : 1,
                "status" : 1,
            ]);
            map_delete(constructing, bldg_ids[i]);

            CHANNEL_D->do_channel(this_object(), "sys",
                sprintf(HIC "【宗门公告】%s 的「%s」建造完成！\n" NOR,
                        hq[HQ_FIELD_NAME], HQ_BLDG_NAME[bldg_type]));
        }
    }

    if (sizeof(keys(constructing)) == 0)
        hq[HQ_FIELD_CONSTRUCTING] = ([]);
}

// 升级建筑
// 返回 1=成功, 0=失败, 负值=错误码
int upgrade_building(string sect_id, int bldg_type)
{
    mapping hq, bldgs, bldg;
    int cur_level, new_level, cost;

    if (undefinedp(sect_hqs[sect_id]))
        return -10;

    hq = sect_hqs[sect_id];
    bldgs = hq[HQ_FIELD_BUILDINGS];

    if (!mapp(bldgs[bldg_type]))
        return -20;  // 建筑不存在

    bldg = bldgs[bldg_type];
    cur_level = bldg["level"];

    // 建筑等级上限 = 驻地类型 × 2
    if (cur_level >= hq[HQ_FIELD_TYPE] * 2)
        return -30;  // 已达上限

    new_level = cur_level + 1;

    // 升级费用 = 建设费用 × 新等级
    cost = HQ_BLDG_COST[bldg_type] * new_level;
    if (hq[HQ_FIELD_FUNDS] < cost)
        return -40;  // 资金不足

    hq[HQ_FIELD_FUNDS] -= cost;
    bldg["level"] = new_level;
    bldg["status"] = 1;

    return 1;
}

// 获取已建造的建筑列表
// 返回 ({ ({ bldg_type, level, status }), ... })
mixed *query_buildings(string sect_id)
{
    mapping hq, bldgs;
    string *types;
    mixed *result = ({});
    int i, type;

    if (undefinedp(sect_hqs[sect_id]))
        return ({});

    hq = sect_hqs[sect_id];
    bldgs = hq[HQ_FIELD_BUILDINGS];
    types = keys(bldgs);

    for (i = 0; i < sizeof(types); i++)
    {
        type = to_int(types[i]);
        result += ({ ({ type,
                        bldgs[types[i]]["level"],
                        bldgs[types[i]]["status"] }) });
    }

    return result;
}

// 计算宗门修炼加成（基于传功堂等级）
float query_training_bonus(string sect_id)
{
    mapping hq, bldgs, bldg;

    if (undefinedp(sect_hqs[sect_id]))
        return 1.0;

    hq = sect_hqs[sect_id];
    bldgs = hq[HQ_FIELD_BUILDINGS];

    if (mapp(bldgs[HQ_BLDG_TRAINING]))
    {
        bldg = bldgs[HQ_BLDG_TRAINING];
        return 1.0 + to_float(HQ_TRAINING_BONUS * bldg["level"]) / 100.0;
    }

    return 1.0;
}

// ======== 耐久度管理 ================================================

// 造成耐久度损失
int damage_sect_hq(string sect_id, int amount)
{
    mapping hq;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    hq[HQ_FIELD_DURABILITY] -= amount;

    if (hq[HQ_FIELD_DURABILITY] < 0)
        hq[HQ_FIELD_DURABILITY] = 0;

    // 根据损伤程度调整状态
    if (hq[HQ_FIELD_DURABILITY] < HQ_DAMAGE_CRITICAL)
        hq[HQ_FIELD_STATUS] = HQ_STATUS_DESTROYED;
    else if (hq[HQ_FIELD_DURABILITY] < HQ_DAMAGE_HEAVY)
        hq[HQ_FIELD_STATUS] = HQ_STATUS_DAMAGED;

    return 1;
}

// 修复驻地（消耗资金）
// 返回 1=成功, 0=失败
int repair_sect_hq(string sect_id)
{
    mapping hq;
    int cost, dur_ratio;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    dur_ratio = hq[HQ_FIELD_DURABILITY] * 100 / HQ_DURABILITY_FULL;

    // 按破坏程度计算修复费用
    cost = calc_repair_cost(sect_id);

    if (hq[HQ_FIELD_FUNDS] < cost)
        return 0;

    hq[HQ_FIELD_FUNDS] -= cost;
    hq[HQ_FIELD_DURABILITY] = HQ_DURABILITY_FULL;
    hq[HQ_FIELD_STATUS] = HQ_STATUS_NORMAL;

    return 1;
}

// 计算修复费用
int calc_repair_cost(string sect_id)
{
    mapping hq;
    int dur_ratio;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    dur_ratio = hq[HQ_FIELD_DURABILITY] * 100 / HQ_DURABILITY_FULL;

    if (dur_ratio >= HQ_DAMAGE_LIGHT)
        return HQ_REPAIR_COST_LIGHT;
    else if (dur_ratio >= HQ_DAMAGE_MODERATE)
        return HQ_REPAIR_COST_MODERATE;
    else if (dur_ratio >= HQ_DAMAGE_HEAVY)
        return HQ_REPAIR_COST_HEAVY;
    else
        return HQ_REPAIR_COST_CRITICAL;
}

// 计算修复时间
int calc_repair_time(string sect_id)
{
    mapping hq;
    int dur_ratio;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    dur_ratio = hq[HQ_FIELD_DURABILITY] * 100 / HQ_DURABILITY_FULL;

    if (dur_ratio >= HQ_DAMAGE_LIGHT)
        return HQ_REPAIR_TIME_LIGHT;
    else if (dur_ratio >= HQ_DAMAGE_MODERATE)
        return HQ_REPAIR_TIME_MODERATE;
    else if (dur_ratio >= HQ_DAMAGE_HEAVY)
        return HQ_REPAIR_TIME_HEAVY;
    else
        return HQ_REPAIR_TIME_CRITICAL;
}

// 耐久度等级描述
string damage_level_name(int durability)
{
    int ratio = durability * 100 / HQ_DURABILITY_FULL;

    if (ratio >= HQ_DAMAGE_LIGHT)
        return HIG "完好" NOR;
    else if (ratio >= HQ_DAMAGE_MODERATE)
        return HIY "轻微受损" NOR;
    else if (ratio >= HQ_DAMAGE_HEAVY)
        return HIR "严重受损" NOR;
    else
        return HIR "濒临毁灭" NOR;
}

// ======== 宗门资金管理 ============================================

// 存入资金
int deposit_funds(string sect_id, int amount)
{
    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    if (amount <= 0)
        return 0;

    sect_hqs[sect_id][HQ_FIELD_FUNDS] += amount;
    return 1;
}

// 支出资金
int withdraw_funds(string sect_id, int amount)
{
    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    if (amount <= 0)
        return 0;

    if (sect_hqs[sect_id][HQ_FIELD_FUNDS] < amount)
        return 0;

    sect_hqs[sect_id][HQ_FIELD_FUNDS] -= amount;
    return 1;
}

// 查询宗门资金
int query_funds(string sect_id)
{
    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    return sect_hqs[sect_id][HQ_FIELD_FUNDS];
}

// ======== 每日收益 ================================================

// 计算宗门每日收益
int calculate_daily_output(string sect_id)
{
    mapping hq, bldgs;
    string *bldg_ids;
    int base, total;
    int i, bldg_type, level;
    float efficiency;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    base = HQ_DAILY_OUTPUT[hq[HQ_FIELD_TYPE]];

    // 计算建筑效率加成
    total = base;
    bldgs = hq[HQ_FIELD_BUILDINGS];
    bldg_ids = keys(bldgs);

    for (i = 0; i < sizeof(bldg_ids); i++)
    {
        bldg_type = to_int(bldg_ids[i]);
        level = bldgs[bldg_ids[i]]["level"];
        efficiency = HQ_BLDG_EFFICIENCY[bldg_type];
        total += to_int(base * efficiency * to_float(level) * 0.1);
    }

    // 受损状态减产
    if (hq[HQ_FIELD_STATUS] == HQ_STATUS_DAMAGED)
        total = total * 7 / 10;
    else if (hq[HQ_FIELD_STATUS] == HQ_STATUS_DESTROYED)
        total = total * 3 / 10;

    return total;
}

// 检查并发放每日收益（由 heart_beat 调用）
void check_daily_output(string sect_id)
{
    mapping hq;
    int output;
    string *members;
    int i;

    if (undefinedp(sect_hqs[sect_id]))
        return;

    hq = sect_hqs[sect_id];

    // 简化处理：每10分钟增加收益（真实日收益的 1/144）
    output = calculate_daily_output(sect_id) / 144;
    if (output > 0)
    {
        hq[HQ_FIELD_FUNDS] += output;

        // 宗门声望自然增长
        hq[HQ_FIELD_REPUTE] += output / 100;
    }
}

// 设置税率
int set_tax_rate(string sect_id, float rate)
{
    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    if (rate < HQ_DAILY_TAX_RATE_MIN)
        rate = HQ_DAILY_TAX_RATE_MIN;

    if (rate > HQ_DAILY_TAX_RATE_MAX)
        rate = HQ_DAILY_TAX_RATE_MAX;

    sect_hqs[sect_id][HQ_FIELD_TAX_RATE] = rate;
    return 1;
}

// ======== 攻防战系统（对应设计文档 §4.3）===========================

// 宣战
// 返回 1=成功, 0=失败, 负值=错误码
int declare_siege(string attacker_id, string defender_id)
{
    mapping attacker, defender, siege;
    int attacker_lv, defender_lv, last_siege;

    if (undefinedp(sect_hqs[attacker_id]) || undefinedp(sect_hqs[defender_id]))
        return -10;

    attacker = sect_hqs[attacker_id];
    defender = sect_hqs[defender_id];

    // 检查攻方等级条件
    attacker_lv = attacker[HQ_FIELD_TYPE];
    defender_lv = defender[HQ_FIELD_TYPE];
    if (attacker_lv < defender_lv + HQ_SIEGE_MIN_LEVEL_DIFF)
        return -20;  // 攻方等级不足

    // 检查宣战冷却（每周最多1次）
    last_siege = attacker[HQ_FIELD_LAST_SIEGE];
    if (last_siege > 0 && time() - last_siege < 7 * 86400)
        return -30;  // 冷却中

    // 检查是否已在攻防战中
    siege = attacker[HQ_FIELD_SIEGE];
    if (mapp(siege) && siege["phase"] != HQ_SIEGE_PHASE_NONE)
        return -40;

    siege = defender[HQ_FIELD_SIEGE];
    if (mapp(siege) && siege["phase"] != HQ_SIEGE_PHASE_NONE)
        return -50;  // 防守方已在战斗中

    // 扣除宣战消耗
    {
        int declare_cost;
        declare_cost = 50000 * defender_lv;
        if (attacker[HQ_FIELD_FUNDS] < declare_cost)
            return -60;  // 资金不足
        attacker[HQ_FIELD_FUNDS] -= declare_cost;
    }

    // 设置攻防战状态
    attacker[HQ_FIELD_SIEGE] = ([
        "phase"    : HQ_SIEGE_PHASE_DECLARE,
        "target"   : defender_id,
        "declared" : time(),
    ]);

    defender[HQ_FIELD_SIEGE] = ([
        "phase"    : HQ_SIEGE_PHASE_DECLARE,
        "attacker" : attacker_id,
        "declared" : time(),
    ]);

    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIR "【宗门战报】%s 向 %s 正式宣战！24小时后攻防战开启！\n" NOR,
                attacker[HQ_FIELD_NAME], defender[HQ_FIELD_NAME]));

    return 1;
}

// 检查攻防战状态（由 heart_beat 调用）
void check_siege_status(string sect_id)
{
    mapping hq, siege;
    int elapsed;

    if (undefinedp(sect_hqs[sect_id]))
        return;

    hq = sect_hqs[sect_id];
    siege = hq[HQ_FIELD_SIEGE];

    if (!mapp(siege))
        return;

    switch (siege["phase"])
    {
    case HQ_SIEGE_PHASE_DECLARE:
        // 宣战后经过备战时间，进入战斗
        elapsed = time() - siege["declared"];
        if (elapsed >= HQ_SIEGE_PREP_TIME)
        {
            siege["phase"] = HQ_SIEGE_PHASE_ACTIVE;
            siege["started"] = time();
            siege["progress"] = ({});
            siege["node_hp"] = ([ 1 : 5000, 2 : 5000, 3 : 5000 ]);

            CHANNEL_D->do_channel(this_object(), "sys",
                sprintf(HIR "【宗门战报】%s 攻防战正式开始！双方弟子速回山门！\n" NOR,
                        hq[HQ_FIELD_NAME]));
        }
        break;

    case HQ_SIEGE_PHASE_ACTIVE:
        // 检查是否超时
        elapsed = time() - siege["started"];
        if (elapsed >= HQ_SIEGE_DURATION)
        {
            // 攻防战超时，防守方胜利
            settle_siege(sect_id, 0);
        }
        break;
    }
}

// 攻防战推进（突破目标）
// target 为突破目标编号（HQ_SIEGE_TARGET_*）
// 返回 1=成功, 0=失败
int advance_siege(string sect_id, int target)
{
    mapping hq, siege;
    int *progress;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    siege = hq[HQ_FIELD_SIEGE];

    if (!mapp(siege) || siege["phase"] != HQ_SIEGE_PHASE_ACTIVE)
        return 0;

    progress = siege["progress"];
    if (member_array(target, progress) != -1)
        return 0;  // 已突破

    // 检查突破顺序
    if (target == HQ_SIEGE_TARGET_TOWER &&
        member_array(HQ_SIEGE_TARGET_GATE, progress) == -1)
        return 0;  // 必须先突破山门

    if (target == HQ_SIEGE_TARGET_NODE &&
        member_array(HQ_SIEGE_TARGET_TOWER, progress) == -1)
        return 0;  // 必须先突破哨塔

    if (target == HQ_SIEGE_TARGET_HALL &&
        member_array(HQ_SIEGE_TARGET_NODE, progress) == -1)
        return 0;  // 必须先突破阵法节点

    progress += ({ target });
    siege["progress"] = progress;

    // 如果攻入核心大殿，攻方胜利
    if (target == HQ_SIEGE_TARGET_HALL)
    {
        settle_siege(sect_id, 1);
    }

    return 1;
}

// 对阵法节点造成伤害（防守方调用）
int damage_node(string sect_id, int node_id, int damage)
{
    mapping hq, siege;
    mapping node_hp;

    if (undefinedp(sect_hqs[sect_id]))
        return 0;

    hq = sect_hqs[sect_id];
    siege = hq[HQ_FIELD_SIEGE];

    if (!mapp(siege) || siege["phase"] != HQ_SIEGE_PHASE_ACTIVE)
        return 0;

    node_hp = siege["node_hp"];
    if (!mapp(node_hp) || undefinedp(node_hp[node_id]))
        return 0;

    node_hp[node_id] -= damage;
    if (node_hp[node_id] <= 0)
    {
        node_hp[node_id] = 0;
        // 节点被摧毁视为突破一个阵法节点
        advance_siege(sect_id, HQ_SIEGE_TARGET_NODE);
    }

    return 1;
}

// 结算攻防战
// winner: 1=进攻方胜, 0=防守方胜
void settle_siege(string sect_id, int winner)
{
    mapping hq, siege, target_hq;
    string attacker_id, defender_id;

    if (undefinedp(sect_hqs[sect_id]))
        return;

    hq = sect_hqs[sect_id];
    siege = hq[HQ_FIELD_SIEGE];

    if (!mapp(siege))
        return;

    // 判断攻守身份
    if (mapp(siege["target"]))
    {
        // 进攻方视角
        attacker_id = sect_id;
        defender_id = siege["target"];
    }
    else if (stringp(siege["attacker"]))
    {
        // 防守方视角
        attacker_id = siege["attacker"];
        defender_id = sect_id;
    }
    else
    {
        return;
    }

    if (undefinedp(sect_hqs[attacker_id]) || undefinedp(sect_hqs[defender_id]))
        return;

    target_hq = sect_hqs[defender_id];

    if (winner)
    {
        // 进攻方胜利
        int loot = target_hq[HQ_FIELD_FUNDS] * 3 / 10;
        sect_hqs[attacker_id][HQ_FIELD_FUNDS] += loot;
        sect_hqs[attacker_id][HQ_FIELD_REPUTE] += 1000;
        target_hq[HQ_FIELD_FUNDS] -= loot;

        // 防守方耐久度下降
        damage_sect_hq(defender_id, HQ_DURABILITY_FULL * 3 / 10);

        CHANNEL_D->do_channel(this_object(), "sys",
            sprintf(HIR "【宗门战报】%s 攻破 %s 山门，掠走大量资源！\n" NOR,
                    sect_hqs[attacker_id][HQ_FIELD_NAME],
                    sect_hqs[defender_id][HQ_FIELD_NAME]));
    }
    else
    {
        // 防守方胜利
        int penalty = sect_hqs[attacker_id][HQ_FIELD_FUNDS] * 1 / 10;
        if (penalty > 100000) penalty = 100000;
        sect_hqs[attacker_id][HQ_FIELD_FUNDS] -= penalty;
        sect_hqs[defender_id][HQ_FIELD_REPUTE] += 500;

        CHANNEL_D->do_channel(this_object(), "sys",
            sprintf(HIY "【宗门战报】%s 成功防守 %s 的进攻！\n" NOR,
                    sect_hqs[defender_id][HQ_FIELD_NAME],
                    sect_hqs[attacker_id][HQ_FIELD_NAME]));
    }

    // 清理状态
    sect_hqs[attacker_id][HQ_FIELD_SIEGE] = ([ "phase" : HQ_SIEGE_PHASE_NONE ]);
    sect_hqs[attacker_id][HQ_FIELD_LAST_SIEGE] = time();

    sect_hqs[defender_id][HQ_FIELD_SIEGE] = ([ "phase" : HQ_SIEGE_PHASE_NONE ]);
    if (target_hq[HQ_FIELD_STATUS] == HQ_STATUS_NORMAL)
        target_hq[HQ_FIELD_STATUS] = HQ_STATUS_NORMAL;
}

// ======== 信息查询 ================================================

// 生成驻地状态描述
string describe_sect_hq(string sect_id)
{
    mapping hq, constructing, bldgs;
    string result;
    string *con_keys;
    int i;
    mixed *buildings;

    if (undefinedp(sect_hqs[sect_id]))
        return "该宗门不存在。\n";

    hq = sect_hqs[sect_id];

    result = sprintf(
        HIW "╔══════════════════════════════════════╗\n" NOR
        HIW "║  " HIC "【宗门驻地】%s" HIW "              ║\n" NOR
        HIW "╚══════════════════════════════════════╝\n" NOR
        "\n"
        "驻地类型 ：%s（Lv.%d）\n"
        "宗主     ：%s\n"
        "成员数量 ：%d\n"
        "宗门资金 ：%d 灵石\n"
        "宗门声望 ：%d\n"
        "耐久度   ：%d/%d（%s）\n"
        "驻地状态 ：%s\n"
        "\n",
        hq[HQ_FIELD_NAME],
        HQ_TYPE_NAME[hq[HQ_FIELD_TYPE]], hq[HQ_FIELD_TYPE],
        hq[HQ_FIELD_MASTER],
        sizeof(hq[HQ_FIELD_MEMBERS]),
        hq[HQ_FIELD_FUNDS],
        hq[HQ_FIELD_REPUTE],
        hq[HQ_FIELD_DURABILITY], HQ_DURABILITY_FULL,
        damage_level_name(hq[HQ_FIELD_DURABILITY]),
        (hq[HQ_FIELD_STATUS] == HQ_STATUS_NORMAL) ? HIG "正常" NOR :
        (hq[HQ_FIELD_STATUS] == HQ_STATUS_SIEGE) ? HIR "攻防战中" NOR :
        (hq[HQ_FIELD_STATUS] == HQ_STATUS_DAMAGED) ? HIY "受损" NOR :
        HIR "濒毁" NOR
    );

    // 建筑列表
    result += "\n" HIY "【建筑列表】\n" NOR;
    buildings = query_buildings(sect_id);
    if (sizeof(buildings) == 0)
    {
        result += "  （暂无建筑）\n";
    }
    else
    {
        for (i = 0; i < sizeof(buildings); i++)
        {
            int btype, blevel, bstatus;
            btype = buildings[i][0];
            blevel = buildings[i][1];
            bstatus = buildings[i][2];

            result += sprintf("  %-12s Lv.%d %s\n",
                              HQ_BLDG_NAME[btype],
                              blevel,
                              bstatus ? HIG "●" NOR : HIR "●受损" NOR);
        }
    }

    // 在建建筑
    constructing = hq[HQ_FIELD_CONSTRUCTING];
    if (mapp(constructing))
    {
        con_keys = keys(constructing);
        if (sizeof(con_keys) > 0)
        {
            result += "\n" HIC "【在建建筑】\n" NOR;
            for (i = 0; i < sizeof(con_keys); i++)
            {
                int btype = to_int(con_keys[i]);
                int remain = constructing[btype] - time();
                if (remain < 0) remain = 0;
                result += sprintf("  %-12s 剩余 %d 秒\n",
                                  HQ_BLDG_NAME[btype], remain);
            }
        }
    }

    // 攻防战状态
    if (mapp(hq[HQ_FIELD_SIEGE]) && hq[HQ_FIELD_SIEGE]["phase"] != HQ_SIEGE_PHASE_NONE)
    {
        mapping siege = hq[HQ_FIELD_SIEGE];
        result += "\n" HIR "【攻防战状态】\n" NOR;

        switch (siege["phase"])
        {
        case HQ_SIEGE_PHASE_DECLARE:
            result += sprintf("  状态：宣战期（备战剩余 %d 秒）\n",
                              HQ_SIEGE_PREP_TIME - (time() - siege["declared"]));
            break;
        case HQ_SIEGE_PHASE_ACTIVE:
            result += sprintf("  状态：激战中（剩余 %d 秒）\n",
                              HQ_SIEGE_DURATION - (time() - siege["started"]));
            break;
        default:
            result += "  状态：结算中\n";
        }
    }

    return result;
}

// ======== 调试与维护 ==============================================

int clean_up()
{
    return 1;
}

// Wiz 指令：查看所有宗门驻地状态
string debug_status()
{
    string *ids;
    string result;
    int i;

    ids = keys(sect_hqs);
    result = sprintf("宗门驻地系统状态\n");
    result += sprintf("═══════════════════════════════\n");
    result += sprintf("已注册宗门：%d 个\n\n", sizeof(ids));

    for (i = 0; i < sizeof(ids); i++)
    {
        result += sprintf("  [%s] %s（类型:%d, 成员:%d, 资金:%d, 耐久:%d/%d）\n",
                          ids[i],
                          sect_hqs[ids[i]][HQ_FIELD_NAME],
                          sect_hqs[ids[i]][HQ_FIELD_TYPE],
                          sizeof(sect_hqs[ids[i]][HQ_FIELD_MEMBERS]),
                          sect_hqs[ids[i]][HQ_FIELD_FUNDS],
                          sect_hqs[ids[i]][HQ_FIELD_DURABILITY],
                          HQ_DURABILITY_FULL);
    }

    return result;
}
