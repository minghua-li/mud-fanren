// mansion_d.c
// 洞府经营系统守护进程 —— 洞府创建/升级/建筑树/药圃种植/修炼加成/维护管理
// Created for ticket #32

#include <ansi.h>
#include <mansion.h>

inherit F_DBASE;

// -------- 全局状态 -------------------------------------------------

// 所有玩家的洞府数据
// player_id -> ([
//     "id"              : player_id,
//     "level"           : MANSION_LV*,
//     "aura"            : AURA_*,
//     "created"         : time(),
//     "buildings"       : ([ bldg_id -> ([ "level": N, "built": time() ]) ]),
//     "garden"          : ([
//         "land"        : LAND_*,
//         "plots"       : ([ plot_idx -> ([
//             "status"  : PLOT_*,
//             "seed"    : seed_path / "",
//             "planted" : time(),
//             "mature"  : time(),
//             "yield"   : 收获次数,
//         ]) ]),
//     ]),
//     "last_maint"      : time(),
//     "last_barrier_charge" : time(),
// ])
nosave mapping mansions = ([]);

// 洞府入口注册表 player_id -> entry_room_path
nosave mapping mansion_entries = ([]);

// 建筑升级成本基数（灵石，后续乘以等级）
#define BLDG_BUILD_COST_BASE  2000

// 序列号
nosave int mansion_serial = 0;

// -------- 前向声明 ------------------------------------------------
// (none needed - all internal calls use defined-before-use order)

// ======== 创建与心跳 ==============================================

void create()
{
    seteuid(getuid());
    set("channel_id", "洞府精灵");
    set("name", "洞府系统");

    CHANNEL_D->do_channel(this_object(), "sys", "洞府经营系统已启动。");
    set_heart_beat(300);  // 每5分钟检查一次
}

void heart_beat()
{
    string *pids;
    int i, overdue;

    pids = keys(mansions);
    for (i = 0; i < sizeof(pids); i++)
    {
        // 检查维护逾期
        overdue = check_mansion_overdue(pids[i]);
        if (overdue > 0)
        {
            // 维护逾期超过 7 天，洞府降级
            if (overdue > 7)
            {
                degrade_mansion(pids[i]);
            }
        }

        // 检查药圃生长状态
        check_garden_growth(pids[i]);
    }
}

// ======== 洞府基础框架 ============================================

// 创建玩家洞府
// 返回 1=成功, 0=失败（已有洞府）
int create_mansion(object me)
{
    string pid;
    mapping mansion;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (!stringp(pid))
        return 0;

    // 已有洞府
    if (mapp(mansions[pid]))
        return 0;

    // 初始为草庐（Lv.1），无灵脉
    mansion = ([
        MNS_FIELD_ID      : pid,
        MNS_FIELD_LEVEL   : MANSION_LV1,
        MNS_FIELD_AURA    : AURA_NONE,
        MNS_FIELD_CREATED : time(),
        MNS_FIELD_BLDGS   : ([]),
        MNS_FIELD_GARDEN  : ([
            "land"        : LAND_NORMAL,
            "plots"       : ([]),
        ]),
        MNS_FIELD_MAINT   : time(),
        MNS_FIELD_BARRIER_CHARGE : time(),
    ]);

    // 初始自带打坐室 Lv.1 和储物室 Lv.1
    mansion[MNS_FIELD_BLDGS][MANSION_BLDG_CULTIVATE] = ([
        "level" : 1,
        "built" : time(),
    ]);
    mansion[MNS_FIELD_BLDGS][MANSION_BLDG_STORAGE] = ([
        "level" : 1,
        "built" : time(),
    ]);
    mansion[MNS_FIELD_BLDGS][MANSION_BLDG_REST] = ([
        "level" : 1,
        "built" : time(),
    ]);

    mansions[pid] = mansion;

    tell_object(me, HIG "你在灵气汇聚之处开辟了一处洞府，命名为草庐。\n" NOR);
    tell_object(me, "可用 " HIY "mansion_info" NOR " 查看洞府状态。\n");

    return 1;
}

// 查询玩家洞府数据
mapping query_mansion(string pid)
{
    if (!stringp(pid) || undefinedp(mansions[pid]))
        return ([]);

    return mansions[pid];
}

// 玩家是否有洞府
int has_mansion(object me)
{
    string pid;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (!stringp(pid))
        return 0;

    return mapp(mansions[pid]);
}

// 获取洞府入口房间
string query_mansion_entry(string pid)
{
    if (!stringp(pid))
        return 0;

    return mansion_entries[pid];
}

// 注册洞府入口房间
int set_mansion_entry(string pid, string entry_path)
{
    if (!stringp(pid) || !stringp(entry_path))
        return 0;

    mansion_entries[pid] = entry_path;
    return 1;
}

// 删除洞府（极低概率事件，如玩家删号）
int destroy_mansion(string pid)
{
    if (undefinedp(mansions[pid]))
        return 0;

    map_delete(mansions, pid);
    map_delete(mansion_entries, pid);
    return 1;
}

// ======== 洞府升级 ================================================

// 检查洞府升级条件
// 返回 0=可升级, 正数=错误码
int check_upgrade_condition(object me, int target_level)
{
    mapping mansion;
    string pid;
    int cur_level, cost, contribution;

    if (!objectp(me) || !userp(me))
        return 1;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return 10;  // 没有洞府

    mansion = mansions[pid];
    cur_level = mansion[MNS_FIELD_LEVEL];

    if (target_level <= cur_level)
        return 20;  // 等级未提高

    if (target_level > MANSION_LV5)
        return 21;  // 已达最高等级

    // 必须逐级升级
    if (target_level > cur_level + 1)
        return 22;  // 不能跳级

    // 检查灵石
    cost = MANSION_UPGRADE_COST[target_level];
    if (cost > 0 && me->query("balance") < cost)
        return 30;  // 灵石不足

    // 检查门派贡献（Lv.3+ 需要贡献）
    if (target_level >= MANSION_LV3)
    {
        contribution = me->query("family/contribution");
        if (contribution < 5000 && target_level >= MANSION_LV3)
            return 40;  // 贡献不足
    }

    // Lv.4+ 需要声望
    if (target_level >= MANSION_LV4)
    {
        if (me->query("reputation") < 10000)
            return 41;  // 声望不足
    }

    return 0;  // 可升级
}

// 获取升级失败文本
string upgrade_fail_reason(int errcode)
{
    switch (errcode)
    {
    case 1:  return "只有玩家才能升级洞府。\n";
    case 10: return "你还没有洞府。\n";
    case 20: return "目标等级不高于当前等级。\n";
    case 21: return "已达洞府最高等级。\n";
    case 22: return "洞府必须逐级升级，不能跳级。\n";
    case 30: return "你的灵石不足以升级洞府。\n";
    case 40: return "你的门派贡献不足（需要 5,000）。\n";
    case 41: return "你的声望不足（需要 10,000）。\n";
    default: return "无法升级洞府。\n";
    }
}

// 执行洞府升级
// 返回 1=成功, 0=失败
int upgrade_mansion(object me)
{
    mapping mansion;
    string pid;
    int cur_level, target_level, cost, errcode;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return 0;

    mansion = mansions[pid];
    cur_level = mansion[MNS_FIELD_LEVEL];
    target_level = cur_level + 1;

    errcode = check_upgrade_condition(me, target_level);
    if (errcode != 0)
        return 0;

    // 扣除灵石
    cost = MANSION_UPGRADE_COST[target_level];
    if (cost > 0)
        me->add("balance", -cost);

    // 扣除贡献
    if (target_level >= MANSION_LV3)
        me->add("family/contribution", -5000);

    // 升级
    mansion[MNS_FIELD_LEVEL] = target_level;

    // 升级奖励：灵脉品质提升
    if (target_level == MANSION_LV2 && mansion[MNS_FIELD_AURA] == AURA_NONE)
        mansion[MNS_FIELD_AURA] = AURA_LOW;
    else if (target_level == MANSION_LV3 && mansion[MNS_FIELD_AURA] == AURA_LOW)
        mansion[MNS_FIELD_AURA] = AURA_MIDDLE;
    else if (target_level == MANSION_LV4 && mansion[MNS_FIELD_AURA] == AURA_MIDDLE)
        mansion[MNS_FIELD_AURA] = AURA_HIGH;
    else if (target_level == MANSION_LV5 && mansion[MNS_FIELD_AURA] == AURA_HIGH)
        mansion[MNS_FIELD_AURA] = AURA_SUPREME;

    // 升级后新增药圃地块
    if (target_level >= MANSION_LV2 && !mapp(mansion[MNS_FIELD_GARDEN]["plots"]))
        mansion[MNS_FIELD_GARDEN]["plots"] = ([ 0 : ([ "status": PLOT_EMPTY ]) ]);

    tell_object(me, HIM "═══════════════════════════════════════\n" NOR);
    tell_object(me, sprintf(HIM "  洞府升级成功！当前等级：%s\n" NOR,
                            MANSION_LEVEL_NAME[target_level]));
    tell_object(me, sprintf("  灵石消耗：%d\n", cost));
    tell_object(me, sprintf("  灵脉品质：%s\n", AURA_NAME[mansion[MNS_FIELD_AURA]]));
    tell_object(me, sprintf("  修炼加成：×%.1f\n",
                            MANSION_CULTIVATE_SPEED[target_level] *
                            AURA_FACTOR[mansion[MNS_FIELD_AURA]]));
    tell_object(me, HIM "═══════════════════════════════════════\n" NOR);

    return 1;
}

// ======== 洞府降级（维护逾期） ====================================

// 检查洞府维护逾期天数
int check_mansion_overdue(string pid)
{
    mapping mansion;
    int last, elapsed, cycle;

    if (undefinedp(mansions[pid]))
        return 0;

    mansion = mansions[pid];
    last = mansion[MNS_FIELD_MAINT];
    cycle = 7 * 86400;  // 7 天

    elapsed = time() - last;
    if (elapsed <= cycle)
        return 0;

    return (elapsed - cycle) / 86400;  // 逾期天数
}

// 执行洞府降级
int degrade_mansion(string pid)
{
    mapping mansion;
    object player;
    int cur_lv;

    if (undefinedp(mansions[pid]))
        return 0;

    mansion = mansions[pid];
    cur_lv = mansion[MNS_FIELD_LEVEL];

    if (cur_lv <= MANSION_LV1)
        return 0;  // 最低等级，不降级

    mansion[MNS_FIELD_LEVEL] = cur_lv - 1;
    mansion[MNS_FIELD_MAINT] = time();  // 重置计时

    // 通知玩家（如果在线）
    player = find_player(pid);
    if (objectp(player))
    {
        tell_object(player, HIR "你的洞府因长期未缴纳维护费，降级为" +
                    MANSION_LEVEL_NAME[cur_lv - 1] + "！\n" NOR);
    }

    return 1;
}

// ======== 建筑树系统 ==============================================

// 建造建筑
// 返回 1=成功, 0=失败, 负值=错误码
int build_building(object me, int bldg_type)
{
    mapping mansion, bldgs;
    string pid;
    int mansion_lv, unlock_lv, cost, errcode;

    if (!objectp(me) || !userp(me))
        return -1;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return -10;  // 无洞府

    mansion = mansions[pid];
    mansion_lv = mansion[MNS_FIELD_LEVEL];

    // 检查解锁等级
    unlock_lv = MANSION_BLDG_UNLOCK[bldg_type];
    if (mansion_lv < unlock_lv)
        return -20;  // 洞府等级不足

    // 检查是否已建造
    bldgs = mansion[MNS_FIELD_BLDGS];
    if (mapp(bldgs[bldg_type]))
        return -30;  // 已建造

    // 计算建造费用
    cost = BLDG_BUILD_COST_BASE * mansion_lv;
    if (me->query("balance") < cost)
        return -40;  // 灵石不足

    // 建造
    me->add("balance", -cost);
    bldgs[bldg_type] = ([
        "level" : 1,
        "built" : time(),
    ]);

    tell_object(me, HIC "你在洞府中建造了" + MANSION_BLDG_NAME[bldg_type] +
                "（Lv.1），消耗灵石 " + cost + "。\n" NOR);

    return 1;
}

// 升级建筑
// 返回 1=成功, 0=失败, 负值=错误码
int upgrade_building(object me, int bldg_type)
{
    mapping mansion, bldgs, bldg;
    string pid;
    int mansion_lv, cur_level, new_level, max_level, cost;

    if (!objectp(me) || !userp(me))
        return -1;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return -10;

    mansion = mansions[pid];
    mansion_lv = mansion[MNS_FIELD_LEVEL];
    bldgs = mansion[MNS_FIELD_BLDGS];

    if (!mapp(bldgs[bldg_type]))
        return -20;  // 建筑不存在

    bldg = bldgs[bldg_type];
    cur_level = bldg["level"];
    max_level = MAX_BLDG_LEVEL(mansion_lv);

    if (cur_level >= max_level)
        return -30;  // 已达当前洞府等级的上限

    new_level = cur_level + 1;

    // 升级费用 = 建造基数 * 新等级
    cost = BLDG_BUILD_COST_BASE * new_level;
    if (me->query("balance") < cost)
        return -40;  // 灵石不足

    me->add("balance", -cost);
    bldg["level"] = new_level;

    tell_object(me, HIC "你已将" + MANSION_BLDG_NAME[bldg_type] +
                "升级到 Lv." + new_level + "，消耗灵石 " + cost + "。\n" NOR);

    return 1;
}

// 查询已建造的建筑列表
// 返回 ({ ({ bldg_type, level }), ... })
mixed *query_buildings(string pid)
{
    mapping mansion, bldgs;
    string *types;
    mixed *result = ({});
    int i, type;

    if (undefinedp(mansions[pid]))
        return ({});

    mansion = mansions[pid];
    bldgs = mansion[MNS_FIELD_BLDGS];
    types = keys(bldgs);

    for (i = 0; i < sizeof(types); i++)
    {
        type = to_int(types[i]);
        result += ({ ({ type, bldgs[types[i]]["level"] }) });
    }

    return result;
}

// ======== 修炼加成计算 ============================================

// 计算玩家在洞府修炼时的速度加成系数
// 返回 float 系数（如 1.0 = 无加成）
float query_cultivate_bonus(object me)
{
    string pid;
    mapping mansion, bldgs, bldg;
    float bonus, aura_factor, cultivate_bonus;
    int lv, cultivate_level;

    if (!objectp(me) || !userp(me))
        return 1.0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return 1.0;

    mansion = mansions[pid];
    lv = mansion[MNS_FIELD_LEVEL];

    // 洞府等级加成（基础）
    bonus = MANSION_CULTIVATE_SPEED[lv];

    // 灵脉品质加成
    aura_factor = AURA_FACTOR[mansion[MNS_FIELD_AURA]];
    bonus = bonus * aura_factor;

    // 打坐室额外加成（每级 +10%）
    bldgs = mansion[MNS_FIELD_BLDGS];
    if (mapp(bldgs[MANSION_BLDG_CULTIVATE]))
    {
        bldg = bldgs[MANSION_BLDG_CULTIVATE];
        cultivate_level = bldg["level"];
        cultivate_bonus = 1.0 + to_float(cultivate_level) * 0.1;
        bonus = bonus * cultivate_bonus;
    }

    return bonus;
}

// ======== 药圃种植系统 ============================================

// 升级药圃土地品质
// 返回 1=成功, 0=失败
int upgrade_garden_land(object me)
{
    mapping mansion, garden;
    string pid;
    int cur_land, next_land, cost;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return 0;

    mansion = mansions[pid];
    garden = mansion[MNS_FIELD_GARDEN];
    cur_land = garden["land"];

    if (cur_land >= LAND_ANCIENT)
    {
        tell_object(me, "你的药圃土地已达最高品质。\n");
        return 0;
    }

    next_land = cur_land + 1;

    // 检查灵石
    cost = LAND_UPGRADE_COST[cur_land];
    if (cost > 0 && me->query("balance") < cost)
    {
        tell_object(me, sprintf("你的灵石不足。需要 %d 灵石。\n", cost));
        return 0;
    }

    if (cost > 0)
        me->add("balance", -cost);

    garden["land"] = next_land;

    // 扩展地块（新品质解锁更多地块）
    {
        mapping plots;
        int max_plots, current_count, j;

        if (mapp(garden["plots"]))
            plots = garden["plots"];
        else
            plots = ([]);

        max_plots = LAND_MAX_PLOTS[next_land];
        current_count = sizeof(plots);

        for (j = current_count; j < max_plots; j++)
        {
            if (undefinedp(plots[j]))
                plots[j] = ([ "status": PLOT_EMPTY ]);
        }

        garden["plots"] = plots;
    }

    tell_object(me, HIG "你的药圃土地已升级为" + LAND_NAME[next_land] +
                "！土地速度系数 ×" + sprintf("%.1f", LAND_SPEED[next_land]) +
                "，最大地块 " + LAND_MAX_PLOTS[next_land] + " 块。\n" NOR);

    return 1;
}

// 播种
// 返回 1=成功, 0=失败
int plant_seed(object me, int plot_idx, string seed_name, int growth_time)
{
    mapping mansion, garden, plots, plot;
    string pid;
    int land_type, max_plots, actual_growth, maint_cost;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
    {
        tell_object(me, "你还没有洞府。\n");
        return 0;
    }

    mansion = mansions[pid];
    garden = mansion[MNS_FIELD_GARDEN];
    land_type = garden["land"];
    max_plots = LAND_MAX_PLOTS[land_type];

    if (plot_idx < 0 || plot_idx >= max_plots)
    {
        tell_object(me, "地块编号无效。\n");
        return 0;
    }

    plots = garden["plots"];
    if (!mapp(plots))
        plots = ([]);

    if (undefinedp(plots[plot_idx]))
        plots[plot_idx] = ([ "status": PLOT_EMPTY ]);

    plot = plots[plot_idx];

    if (plot["status"] != PLOT_EMPTY && plot["status"] != PLOT_FALLOW)
    {
        tell_object(me, "该地块当前不可播种。\n");
        return 0;
    }

    if (plot["status"] == PLOT_FALLOW && plot["yield"] >= 3)
    {
        tell_object(me, "该地块已收获 3 次，需要休整或使用肥料恢复肥力。\n");
        return 0;
    }

    // 计算实际生长时间（受土地品质影响）
    actual_growth = to_int(to_float(growth_time) / LAND_SPEED[land_type]);

    // 扣除维护费
    maint_cost = GARDEN_MAINTENANCE[land_type];
    if (me->query("balance") < maint_cost)
    {
        tell_object(me, sprintf("灵石不足，药圃维护需要 %d 灵石。\n", maint_cost));
        return 0;
    }
    me->add("balance", -maint_cost);

    // 播种
    plots[plot_idx] = ([
        "status"  : PLOT_GROWING,
        "seed"    : seed_name,
        "planted" : time(),
        "mature"  : time() + actual_growth,
        "yield"   : plot["yield"] + 1,
    ]);

    tell_object(me, HIG "你在第 " + (plot_idx + 1) + " 号地块种下了 " +
                seed_name + "，预计 " +
                format_time_remaining(actual_growth) + " 后成熟。\n" NOR);

    return 1;
}

// 收获
// 返回 收获物名称，失败返回 0
string harvest_plot(object me, int plot_idx)
{
    mapping mansion, garden, plots, plot;
    string pid, seed_name;
    float land_speed_factor;
    int base_yield;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return 0;

    mansion = mansions[pid];
    garden = mansion[MNS_FIELD_GARDEN];
    plots = garden["plots"];

    if (!mapp(plots) || undefinedp(plots[plot_idx]))
    {
        tell_object(me, "该地块没有种植作物。\n");
        return 0;
    }

    plot = plots[plot_idx];

    if (plot["status"] != PLOT_GROWING && plot["status"] != PLOT_MATURE)
    {
        tell_object(me, "该地块当前没有可收获的作物。\n");
        return 0;
    }

    // 未成熟
    if (plot["status"] == PLOT_GROWING && time() < plot["mature"])
    {
        tell_object(me, "作物尚未成熟。\n");
        return 0;
    }

    seed_name = plot["seed"];

    // 收获产出 = 基础产量 × 土地品质系数 × 随机波动
    land_speed_factor = LAND_SPEED[garden["land"]];
    base_yield = 1 + random(to_int(land_speed_factor) + 1);
    if (base_yield < 1) base_yield = 1;

    // 累计种植次数，每 3 次需休整
    {
        int total_yield = plot["yield"];
        total_yield++;

        if (total_yield >= 3)
        {
            // 3 次收获后进入休整
            plots[plot_idx] = ([
                "status"  : PLOT_FALLOW,
                "seed"    : "",
                "yield"   : total_yield,
            ]);
            tell_object(me, "该地块已收获 3 次，进入休整期。\n");
        }
        else
        {
            plots[plot_idx] = ([
                "status"  : PLOT_EMPTY,
                "seed"    : "",
                "yield"   : total_yield,
            ]);
        }
    }

    tell_object(me, HIG "你收获了 " + seed_name + " × " + base_yield + "！\n" NOR);

    // 给予产出物
    // 产出的具体物品交给调用处的命令脚本处理
    return seed_name;
}

// 检查药圃生长状态（由 heart_beat 调用）
void check_garden_growth(string pid)
{
    mapping mansion, garden, plots;
    string *idxs;
    object player;
    int i, idx;

    if (undefinedp(mansions[pid]))
        return;

    mansion = mansions[pid];
    garden = mansion[MNS_FIELD_GARDEN];
    plots = garden["plots"];

    if (!mapp(plots))
        return;

    idxs = keys(plots);
    for (i = 0; i < sizeof(idxs); i++)
    {
        idx = to_int(idxs[i]);
        if (plots[idxs[i]]["status"] == PLOT_GROWING)
        {
            if (time() >= plots[idxs[i]]["mature"])
            {
                plots[idxs[i]]["status"] = PLOT_MATURE;

                // 如果玩家在线，通知
                player = find_player(pid);
                if (objectp(player))
                {
                    tell_object(player, HIG "你的洞府药圃中，第 " +
                                (idx + 1) + " 号地块的 " +
                                plots[idxs[i]]["seed"] + " 已成熟，尽快收获！\n" NOR);
                }
            }
        }
    }
}

// ======== 维护管理 ================================================

// 缴纳洞府维护费
// 返回 1=成功, 0=失败
int pay_maintenance(object me)
{
    mapping mansion;
    string pid;
    int cost, lv;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
    {
        tell_object(me, "你还没有洞府。\n");
        return 0;
    }

    mansion = mansions[pid];
    lv = mansion[MNS_FIELD_LEVEL];
    cost = MANSION_MAINTENANCE[lv];

    if (me->query("balance") < cost)
    {
        tell_object(me, sprintf("灵石不足，维护需要 %d 灵石。\n", cost));
        return 0;
    }

    me->add("balance", -cost);
    mansion[MNS_FIELD_MAINT] = time();

    tell_object(me, sprintf("你缴纳了 %d 灵石的洞府维护费（%s等级）。\n", cost, MANSION_LEVEL_NAME[lv]));
    return 1;
}

// 禁制充能
// 返回 1=成功, 0=失败
int charge_barrier(object me)
{
    mapping mansion, bldgs;
    string pid;
    int cost, lv;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
    {
        tell_object(me, "你还没有洞府。\n");
        return 0;
    }

    mansion = mansions[pid];
    bldgs = mansion[MNS_FIELD_BLDGS];

    if (!mapp(bldgs[MANSION_BLDG_BARRIER]))
    {
        tell_object(me, "你的洞府还没有建造护府禁制。\n");
        return 0;
    }

    lv = mansion[MNS_FIELD_LEVEL];
    cost = BARRIER_CHARGE[lv];

    if (me->query("balance") < cost)
    {
        tell_object(me, sprintf("灵石不足，禁制充能需要 %d 灵石。\n", cost));
        return 0;
    }

    me->add("balance", -cost);
    mansion[MNS_FIELD_BARRIER_CHARGE] = time();

    tell_object(me, sprintf("你消耗了 %d 灵石为护府禁制充能，效果持续 30 天。\n", cost));
    return 1;
}

// ======== 显示与查询 ==============================================

// 生成洞府状态描述文本
string describe_mansion(object me)
{
    string pid;
    mapping mansion, garden, bldgs, bldg;
    string result;
    int i, lv, overdue_days, plot_count, ready_count;
    mixed *buildings;

    if (!objectp(me) || !userp(me))
        return "无效的查询。\n";

    pid = me->query("id");
    if (undefinedp(mansions[pid]))
        return "你还没有洞府。使用 " HIY "create_mansion" NOR " 创建洞府。\n";

    mansion = mansions[pid];
    lv = mansion[MNS_FIELD_LEVEL];

    result = sprintf(
        HIW "╔══════════════════════════════════════╗\n" NOR
        HIW "║  " HIC "【洞府信息】" HIW "                         ║\n" NOR
        HIW "╚══════════════════════════════════════╝\n" NOR
        "\n"
        "洞府等级 ：%s（Lv.%d）\n"
        "灵脉品质 ：%s\n"
        "修炼加成 ：×%.1f（基础洞府加成 ×%.1f × 灵脉 ×%.1f）\n"
        "创建时间 ：%s\n"
        "\n",
        MANSION_LEVEL_NAME[lv], lv,
        AURA_NAME[mansion[MNS_FIELD_AURA]],
        query_cultivate_bonus(me),
        to_float(MANSION_CULTIVATE_SPEED[lv]),
        to_float(AURA_FACTOR[mansion[MNS_FIELD_AURA]]),
        ctime(mansion[MNS_FIELD_CREATED])
    );

    // 维护状态
    overdue_days = check_mansion_overdue(pid);
    if (overdue_days > 0)
        result += sprintf(HIR "⚠ 维护已逾期 %d 天！请尽快缴纳维护费，否则洞府会降级。\n" NOR, overdue_days);
    else
    {
        int remaining;
        remaining = 7 - (time() - mansion[MNS_FIELD_MAINT]) / 86400;
        if (remaining < 0) remaining = 0;
        result += sprintf("维护状态 ：良好（下次维护剩余 %d 天）\n", remaining);
    }

    // 建筑列表
    result += "\n" HIY "【建筑列表】\n" NOR;
    buildings = query_buildings(pid);
    if (sizeof(buildings) == 0)
    {
        result += "  （暂无建筑，使用 " HIY "build_building" NOR " 建造）\n";
    }
    else
    {
        for (i = 0; i < sizeof(buildings); i++)
        {
            int btype, blevel;
            btype = buildings[i][0];
            blevel = buildings[i][1];
            result += sprintf("  %-12s  Lv.%d  %s\n",
                              MANSION_BLDG_NAME[btype],
                              blevel,
                              (btype == MANSION_BLDG_FORGE) ?
                                  sprintf("炼器成功率 +%d%%", FORGE_BONUS_BASE + blevel * FORGE_BONUS_PER_LEVEL) :
                              (btype == MANSION_BLDG_ALCHEMY) ?
                                  sprintf("炼丹成功率 +%d%%", ALCHEMY_BONUS_BASE + blevel * ALCHEMY_BONUS_PER_LEVEL) :
                              (btype == MANSION_BLDG_CULTIVATE) ?
                                  sprintf("每级修炼 +10%%") :
                              "");
        }
    }

    // 药圃信息
    result += "\n" HIG "【药圃信息】\n" NOR;
    garden = mansion[MNS_FIELD_GARDEN];
    result += sprintf("  土地品质 ：%s（速度 ×%.1f，最多 %d 块）\n",
                      LAND_NAME[garden["land"]],
                      LAND_SPEED[garden["land"]],
                      LAND_MAX_PLOTS[garden["land"]]);

    if (mapp(garden["plots"]))
    {
        plot_count = sizeof(garden["plots"]);
        ready_count = 0;
        for (i = 0; i < plot_count; i++)
        {
            if (mapp(garden["plots"][i]) && garden["plots"][i]["status"] == PLOT_MATURE)
                ready_count++;
        }
        result += sprintf("  地块数量 ：%d 块（%d 块可收获）\n", plot_count, ready_count);

        // 列出各地块状态
        for (i = 0; i < plot_count; i++)
        {
            string status_str;
            if (!mapp(garden["plots"][i]))
            {
                status_str = "空闲";
            }
            else
            {
                switch (garden["plots"][i]["status"])
                {
                case PLOT_EMPTY:  status_str = "空闲"; break;
                case PLOT_SEEDED: status_str = "已播种"; break;
                case PLOT_GROWING:
                {
                    if (time() >= garden["plots"][i]["mature"])
                        status_str = HIG "已成熟！" NOR;
                    else
                    {
                        int remain = garden["plots"][i]["mature"] - time();
                        status_str = "生长中(" + format_time_remaining(remain) + ")";
                    }
                    break;
                }
                case PLOT_MATURE: status_str = HIG "可收获！" NOR; break;
                case PLOT_FALLOW: status_str = "休整中"; break;
                default:          status_str = "未知"; break;
                }
            }
            result += sprintf("  地块 %d：%s\n", i + 1, status_str);
        }
    }
    else
    {
        result += "  （尚无地块，升级洞府至 Lv.2 解锁药圃）\n";
    }

    return result;
}

// ======== 工具函数 ================================================

// 格式化剩余时间
string format_time_remaining(int seconds)
{
    if (seconds <= 0)
        return "已到期";

    if (seconds < 60)
        return sprintf("%d 秒", seconds);
    else if (seconds < 3600)
        return sprintf("%d 分钟", seconds / 60);
    else if (seconds < 86400)
        return sprintf("%d 小时 %d 分钟", seconds / 3600, (seconds % 3600) / 60);
    else
        return sprintf("%d 天 %d 小时", seconds / 86400, (seconds % 86400) / 3600);
}

// 玩家是否在自家洞府中
int is_in_own_mansion(object me)
{
    string pid, entry;

    if (!objectp(me) || !userp(me))
        return 0;

    pid = me->query("id");
    entry = mansion_entries[pid];

    if (!stringp(entry))
        return 0;

    if (base_name(environment(me)) == entry)
        return 1;

    return 0;
}

// ======== 调试与维护 ==============================================

int clean_up()
{
    return 1;
}

// wiz 指令：查看洞府系统状态
string debug_status()
{
    string *pids;
    string result;
    int i, total_players, total_online;
    int lv1, lv2, lv3, lv4, lv5;

    pids = keys(mansions);
    total_players = sizeof(pids);

    total_online = 0;
    for (i = 0; i < total_players; i++)
    {
        if (objectp(find_player(pids[i])))
            total_online++;
    }

    result = sprintf(
        "洞府系统状态\n"
        "═══════════════════════════════\n"
        "总洞府数量：%d\n"
        "当前在线洞府主：%d\n"
        "\n"
        "洞府等级分布：\n", total_players, total_online);

    lv1 = lv2 = lv3 = lv4 = lv5 = 0;
    for (i = 0; i < total_players; i++)
    {
        switch (mansions[pids[i]][MNS_FIELD_LEVEL])
        {
        case MANSION_LV1: lv1++; break;
        case MANSION_LV2: lv2++; break;
        case MANSION_LV3: lv3++; break;
        case MANSION_LV4: lv4++; break;
        case MANSION_LV5: lv5++; break;
        }
    }
    result += sprintf(
        "  Lv.1 草庐      ：%d\n"
        "  Lv.2 普通洞府   ：%d\n"
        "  Lv.3 灵脉洞府   ：%d\n"
        "  Lv.4 顶级洞府   ：%d\n"
        "  Lv.5 圣地洞府   ：%d\n",
        lv1, lv2, lv3, lv4, lv5);

    return result;
}
