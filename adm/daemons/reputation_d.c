// reputation_d.c
// 声望守护进程 — 管理六大势力声望体系：获取/等级判定/衰减/互斥/每日上限
// Created for 凡人修仙传MUD based on 02-扩充内容/02-声望与互动玩法.md
// 2011-11-15

inherit F_DBASE;
inherit F_SAVE;

#include <ansi.h>
#include <reputation.h>

// ===================== 势力定义 =====================

// 六大势力信息：名称、描述、阵营
nosave mapping faction_info = ([
    FACTION_RIGHTEOUS:      ([
        "name":        "正道联盟",
        "short":       "正道",
        "camp":        "righteous",
        "desc":        "以越国七派、正道盟为代表的人界正道势力",
    ]),
    FACTION_EVIL:           ([
        "name":        "魔道六宗",
        "short":       "魔道",
        "camp":        "evil",
        "desc":        "以魔道六宗为代表的人界魔道势力",
    ]),
    FACTION_STAR_PALACE:    ([
        "name":        "星宫",
        "short":       "星宫",
        "camp":        "neutral",
        "desc":        "乱星海最大势力，掌控天星城及传送阵",
    ]),
    FACTION_REBEL_ALLIANCE: ([
        "name":        "逆星盟",
        "short":       "逆星盟",
        "camp":        "neutral",
        "desc":        "乱星海中对抗星宫的联盟势力",
    ]),
    FACTION_DEMON_RACE:    ([
        "name":        "灵界妖族",
        "short":       "妖族",
        "camp":        "neutral",
        "desc":        "灵界各大妖族势力",
    ]),
    FACTION_JIAOCHI:       ([
        "name":        "角蚩族",
        "short":       "角蚩",
        "camp":        "neutral",
        "desc":        "灵界与人族敌对的强大种族",
    ]),
]);

// 声望等级名称与阈值
nosave mapping level_info = ([
    REP_LEVEL_ARCHENEMY:    ([ "name": "死敌",   "color": HIR,  "threshold": REP_THRESHOLD_ARCHENEMY ]),
    REP_LEVEL_HOSTILE:      ([ "name": "敌对",   "color": RED,  "threshold": REP_THRESHOLD_HOSTILE ]),
    REP_LEVEL_COLD:         ([ "name": "冷淡",   "color": HIC,  "threshold": REP_THRESHOLD_COLD ]),
    REP_LEVEL_NEUTRAL:      ([ "name": "中立",   "color": NOR,  "threshold": REP_THRESHOLD_NEUTRAL ]),
    REP_LEVEL_FRIENDLY:     ([ "name": "友善",   "color": GRN,  "threshold": REP_THRESHOLD_FRIENDLY ]),
    REP_LEVEL_TRUST:        ([ "name": "信任",   "color": BLU,  "threshold": REP_THRESHOLD_TRUST ]),
    REP_LEVEL_RESPECT:      ([ "name": "尊敬",   "color": MAG,  "threshold": REP_THRESHOLD_RESPECT ]),
    REP_LEVEL_ADORE:        ([ "name": "崇拜",   "color": HIY,  "threshold": REP_THRESHOLD_ADORE ]),
    REP_LEVEL_LEGEND:       ([ "name": "传说",   "color": HIM,  "threshold": REP_THRESHOLD_ADORE + 1 ]),  // 传说 > 80000
]);

// 互斥关系定义：[势力A, 势力B, 强度]
nosave mixed *mutex_relations = ({
    ({ FACTION_RIGHTEOUS,      FACTION_EVIL,            REP_MUTEX_STRONG }),
    ({ FACTION_STAR_PALACE,    FACTION_REBEL_ALLIANCE,  REP_MUTEX_STRONG }),
    ({ FACTION_DEMON_RACE,     FACTION_JIAOCHI,         REP_MUTEX_STRONG }),
    ({ FACTION_RIGHTEOUS,      FACTION_JIAOCHI,         REP_MUTEX_STRONG }),
    ({ FACTION_EVIL,           FACTION_DEMON_RACE,      REP_MUTEX_WEAK }),
    ({ FACTION_STAR_PALACE,    FACTION_EVIL,            REP_MUTEX_WEAK }),
    ({ FACTION_REBEL_ALLIANCE, FACTION_RIGHTEOUS,       REP_MUTEX_WEAK }),
});

// ===================== 初始化 =====================

string query_save_file()
{
    return "/data/reputationd";
}

void create()
{
    seteuid(getuid());
    restore();
}

// ===================== 势力信息查询 =====================

// 获取所有势力ID列表
string *query_all_factions()
{
    return keys(faction_info);
}

// 获取势力中文名称
string query_faction_name(string faction)
{
    if (!mapp(faction_info[faction]))
        return "未知势力";
    return faction_info[faction]["name"];
}

// 获取势力简称
string query_faction_short(string faction)
{
    if (!mapp(faction_info[faction]))
        return "未知";
    return faction_info[faction]["short"];
}

// 获取势力阵营
string query_faction_camp(string faction)
{
    if (!mapp(faction_info[faction]))
        return "neutral";
    return faction_info[faction]["camp"];
}

// ===================== 声望等级判定 =====================

// 根据声望值判定等级（返回 -3~5）
// 等级范围（设计文档 1.2 节）：
//   死敌 ≤ -10000 < 敌对 ≤ -2000 < 冷淡 ≤ -101 < 中立 ≤ 100 < 友善 ≤ 2000
//   < 信任 ≤ 8000 < 尊敬 ≤ 25000 < 崇拜 ≤ 80000 < 传说
int query_level(int value)
{
    if (value <= REP_THRESHOLD_ARCHENEMY) return REP_LEVEL_ARCHENEMY;
    if (value <= REP_THRESHOLD_HOSTILE)  return REP_LEVEL_HOSTILE;
    if (value <= REP_THRESHOLD_COLD)     return REP_LEVEL_COLD;
    if (value <= REP_THRESHOLD_NEUTRAL)  return REP_LEVEL_NEUTRAL;
    if (value <= REP_THRESHOLD_FRIENDLY) return REP_LEVEL_FRIENDLY;
    if (value <= REP_THRESHOLD_TRUST)    return REP_LEVEL_TRUST;
    if (value <= REP_THRESHOLD_RESPECT)  return REP_LEVEL_RESPECT;
    if (value <= REP_THRESHOLD_ADORE)    return REP_LEVEL_ADORE;
    return REP_LEVEL_LEGEND;
}

// 获取等级的中文名称
string query_level_name(int level)
{
    if (!mapp(level_info[level]))
        return "未知";
    return level_info[level]["name"];
}

// 获取等级的显示颜色
string query_level_color(int level)
{
    if (!mapp(level_info[level]))
        return NOR;
    return level_info[level]["color"];
}

// 获取等级的完整显示文本（带颜色）
string query_level_display(int level)
{
    string color = query_level_color(level);
    string name = query_level_name(level);
    return color + name + NOR;
}

// 获取等级的声望阈值下限
int query_level_threshold(int level)
{
    if (!mapp(level_info[level]))
        return 0;
    return level_info[level]["threshold"];
}

// 查询玩家在指定势力的声望等级
int query_player_level(object player, string faction)
{
    if (!player) return REP_LEVEL_NEUTRAL;
    return query_level(player->query("reputation/" + faction));
}

// 查询玩家在指定势力的声望值
int query_player_reputation(object player, string faction)
{
    if (!player) return 0;
    return player->query("reputation/" + faction);
}

// ===================== 声望增益 =====================

// 为玩家增加声望（主入口）
// 返回值：实际增加的声望值（扣除互斥和上限后）
varargs int add_reputation(object player, string faction, int amount, string source)
{
    int current, new_value;
    int actual_gain;
    string *factions;
    int daily_gain, daily_total, daily_cap, daily_total_cap;
    int i;
    
    if (!player || !amount || !faction_info[faction])
        return 0;
    
    // 检查每日上限
    daily_gain = player->query_temp("reputation_daily/" + faction);
    daily_total = player->query_temp("reputation_daily/total");
    daily_cap = query_daily_cap(player);
    daily_total_cap = query_daily_total_cap(player);
    
    if (amount > 0)
    {
        // 正声望：检查每日单势力上限
        if (daily_gain + amount > daily_cap)
            amount = daily_cap - daily_gain;
        if (amount <= 0) return 0;
        
        // 检查每日总上限
        if (daily_total + amount > daily_total_cap)
            amount = daily_total_cap - daily_total;
        if (amount <= 0) return 0;
    }
    
    // 应用等级效率修正（仅在正声望时）
    if (amount > 0)
    {
        float eff = query_level_efficiency(player, faction);
        amount = to_int(amount * eff);
        if (amount <= 0) amount = 1;  // 至少获得1点
    }
    
    // 获取当前声望值
    current = player->query("reputation/" + faction);
    
    // 计算新值，限制在范围内
    new_value = current + amount;
    if (new_value > REP_MAX_VALUE) new_value = REP_MAX_VALUE;
    if (new_value < REP_MIN_VALUE) new_value = REP_MIN_VALUE;
    actual_gain = new_value - current;
    
    // 更新声望
    player->set("reputation/" + faction, new_value);
    player->set("reputation/last_interact/" + faction, time());
    
    // 记录日志
    if (stringp(source) && source != "")
    {
        player->set("reputation/log/" + faction + "/" + time(), source);
    }
    
    // 计算每日增益（只累计正声望）
    if (actual_gain > 0)
    {
        player->add_temp("reputation_daily/" + faction, actual_gain);
        player->add_temp("reputation_daily/total", actual_gain);
    }
    
    // 处理互斥关系：提升这一方的同时降低对立方的声望
    if (actual_gain > 0)
    {
        handle_mutex_penalty(player, faction, actual_gain);
    }
    
    return actual_gain;
}

// 处理互斥声望扣减
void handle_mutex_penalty(object player, string faction, int gain)
{
    int i;
    string other_faction;
    float mutex_strength;
    int penalty;
    int current, new_value;
    
    for (i = 0; i < sizeof(mutex_relations); i++)
    {
        if (mutex_relations[i][0] == faction)
        {
            other_faction = mutex_relations[i][1];
            mutex_strength = mutex_relations[i][2];
        }
        else if (mutex_relations[i][1] == faction)
        {
            other_faction = mutex_relations[i][0];
            mutex_strength = mutex_relations[i][2];
        }
        else
            continue;
        
        // 计算扣减量
        penalty = -to_int(gain * mutex_strength);
        if (penalty >= 0) continue;
        
        current = player->query("reputation/" + other_faction);
        new_value = current + penalty;
        
        // 不低于死敌下限
        if (new_value < REP_THRESHOLD_ARCHENEMY)
            new_value = REP_THRESHOLD_ARCHENEMY;
        
        // 不把正声望扣成负的（互斥只影响负向区域）
        if (current > 0 && new_value < 0)
            new_value = 0;
        
        if (new_value != current)
        {
            player->set("reputation/" + other_faction, new_value);
        }
    }
}

// ===================== 效率与修正 =====================

// 计算等级效率修正
float query_level_efficiency(object player, string faction)
{
    int level = query_player_level(player, faction);
    
    switch (level)
    {
        case REP_LEVEL_ARCHENEMY:
        case REP_LEVEL_HOSTILE:
        case REP_LEVEL_COLD:
        case REP_LEVEL_NEUTRAL:
            return REP_EFF_NEWBIE;         // 从负声望或中立开始快速提升
        case REP_LEVEL_FRIENDLY:
            return REP_EFF_NORMAL;         // 友善阶段
        case REP_LEVEL_TRUST:
            return REP_EFF_SLOW;           // 信任阶段减速
        case REP_LEVEL_RESPECT:
            return REP_EFF_DEEP;           // 尊敬阶段更慢
        case REP_LEVEL_ADORE:
        case REP_LEVEL_LEGEND:
            return REP_EFF_BOTTLENECK;     // 崇拜以上瓶颈
        default:
            return 1.0;
    }
}

// 计算重复惩罚系数
float query_repeat_penalty(object player, string faction, string task_type)
{
    int count;
    
    count = player->query_temp("reputation_repeat/" + faction + "/" + task_type);
    
    if (count >= 5) return REP_REPEAT_PENALTY_5TH;
    if (count == 4) return REP_REPEAT_PENALTY_4TH;
    if (count == 3) return REP_REPEAT_PENALTY_3RD;
    if (count == 2) return REP_REPEAT_PENALTY_2ND;
    return REP_REPEAT_PENALTY_1ST;
}

// ===================== 声望折扣 =====================

// 计算玩家在指定势力的购买折扣（返回价格倍率）
// 1.0 = 原价, 0.6 = 6折, 2.0 = 2倍价
float query_discount(object player, string faction)
{
    int level = query_player_level(player, faction);
    float base;
    
    switch (level)
    {
        case REP_LEVEL_ARCHENEMY: return -1.0;    // 死敌→无法交易
        case REP_LEVEL_HOSTILE:   base = 3.0; break;  // 敌对→3倍价
        case REP_LEVEL_COLD:      base = 2.0; break;  // 冷淡→2倍价
        case REP_LEVEL_NEUTRAL:   base = 1.0; break;  // 中立→原价
        case REP_LEVEL_FRIENDLY:  base = 0.95; break; // 友善→9.5折
        case REP_LEVEL_TRUST:     base = 0.90; break; // 信任→9折
        case REP_LEVEL_RESPECT:   base = 0.80; break; // 尊敬→8折
        case REP_LEVEL_ADORE:     base = 0.60; break; // 崇拜→6折
        case REP_LEVEL_LEGEND:    base = 0.50; break; // 传说→5折
        default: base = 1.0;
    }
    
    return base;
}

// ===================== 每日上限 =====================

// 获取玩家单势力每日声望上限（根据境界）
int query_daily_cap(object player)
{
    if (!player) return REP_DAILY_CAP_QI_LIAN;
    
    string *levels = ({ "qi_lian", "zhu_ji", "jie_dan", "yuan_ying", "hua_shen", "lian_xu" });
    int *caps = ({ REP_DAILY_CAP_QI_LIAN, REP_DAILY_CAP_ZHU_JI, REP_DAILY_CAP_JIE_DAN,
                   REP_DAILY_CAP_YUAN_YING, REP_DAILY_CAP_HUA_SHEN, REP_DAILY_CAP_LIAN_XU });
    int i;
    string realm = player->query("realm");
    
    for (i = 0; i < sizeof(levels); i++)
    {
        if (realm == levels[i])
            return caps[i];
    }
    return REP_DAILY_CAP_QI_LIAN;
}

// 获取玩家全势力每日总声望上限
int query_daily_total_cap(object player)
{
    if (!player) return REP_DAILY_TOTAL_QI_LIAN;
    
    string *levels = ({ "qi_lian", "zhu_ji", "jie_dan", "yuan_ying", "hua_shen", "lian_xu" });
    int *caps = ({ REP_DAILY_TOTAL_QI_LIAN, REP_DAILY_TOTAL_ZHU_JI, REP_DAILY_TOTAL_JIE_DAN,
                   REP_DAILY_TOTAL_YUAN_YING, REP_DAILY_TOTAL_HUA_SHEN, REP_DAILY_TOTAL_LIAN_XU });
    int i;
    string realm = player->query("realm");
    
    for (i = 0; i < sizeof(levels); i++)
    {
        if (realm == levels[i])
            return caps[i];
    }
    return REP_DAILY_TOTAL_QI_LIAN;
}

// 获取玩家当前在某个势力的今日已获得声望
int query_daily_gained(object player, string faction)
{
    if (!player) return 0;
    return player->query_temp("reputation_daily/" + faction);
}

// 重置玩家每日声望计数器（每日0点由定时器调用）
void reset_daily_counter(object player)
{
    if (!player) return;
    player->delete_temp("reputation_daily");
    player->delete_temp("reputation_repeat");
}

// ===================== 任务声望计算 =====================

// 计算完成某个任务获得的声望
// task_type: courier(跑腿), collect(采集), hunt(猎杀), elite(精英), secret(秘密), epic(史诗)
varargs int calc_task_reputation(string faction, string task_type, object player, int repeat_count)
{
    int base;
    float mod = 1.0;
    
    // 任务基础声望
    switch (task_type)
    {
        case "courier":  base = 5 + random(6); break;    // 跑腿 5-10
        case "collect":  base = 10 + random(16); break;  // 采集 10-25
        case "hunt":     base = 15 + random(26); break;  // 猎杀 15-40
        case "elite":    base = 30 + random(51); break;  // 精英 30-80
        case "secret":   base = 80 + random(121); break; // 秘密 80-200
        case "epic":     base = 200 + random(301); break;// 史诗 200-500
        default: base = 10;
    }
    
    // 等级效率修正
    mod *= query_level_efficiency(player, faction);
    
    // 重复惩罚
    if (repeat_count > 0)
        player->set_temp("reputation_repeat/" + faction + "/" + task_type, repeat_count);
    mod *= query_repeat_penalty(player, faction, task_type);
    
    // 活动加成（由 faction_d 提供）
    // if (FACTION_D->is_festival(faction)) mod *= 2.0;
    // if (FACTION_D->is_wartime(faction)) mod *= 1.5;
    
    return to_int(base * mod);
}

// ===================== 声望衰减 =====================

// 计算玩家在指定势力的每日衰减值
int calc_daily_decay(object player, string faction)
{
    int rep, last_interact, days;
    int decay = 0;
    
    if (!player) return 0;
    
    rep = player->query("reputation/" + faction);
    last_interact = player->query("reputation/last_interact/" + faction);
    
    // 中立区不衰减
    if (rep >= REP_THRESHOLD_COLD && rep <= REP_THRESHOLD_NEUTRAL)
        return 0;
    
    // 检查是否有豁免条件
    days = (time() - last_interact) / 86400;
    
    if (days >= 7)
    {
        // 正声望衰减
        if (rep > REP_THRESHOLD_NEUTRAL)
        {
            if (rep >= REP_THRESHOLD_ADORE)
                decay = REP_DECAY_ADORE;     // 崇拜以上
            else if (rep >= REP_THRESHOLD_RESPECT)
                decay = REP_DECAY_RESPECT;   // 尊敬
            else if (rep >= REP_THRESHOLD_TRUST)
                decay = REP_DECAY_TRUST;     // 信任
            else if (rep > REP_THRESHOLD_NEUTRAL)
                decay = REP_DECAY_FRIENDLY;  // 友善
        }
        // 负声望回升（时间冲淡仇恨）
        else if (rep < REP_THRESHOLD_COLD)
        {
            decay = REP_DECAY_NEGATIVE;      // 回升（负值表示增加）
        }
    }
    
    return decay;
}

// 对玩家执行一次完整的声望衰减
int apply_decay(object player)
{
    string *factions;
    int i, rep, decay;
    int total_decay = 0;
    
    if (!player) return 0;
    
    factions = keys(query_all_factions());
    
    for (i = 0; i < sizeof(factions); i++)
    {
        decay = calc_daily_decay(player, factions[i]);
        if (decay != 0)
        {
            rep = player->query("reputation/" + factions[i]);
            
            // 确保衰减不降到阈值以下
            if (rep > REP_THRESHOLD_NEUTRAL)
            {
                int min_keep;
                if (rep >= REP_THRESHOLD_ADORE)
                    min_keep = REP_THRESHOLD_ADORE;
                else if (rep >= REP_THRESHOLD_RESPECT)
                    min_keep = REP_THRESHOLD_RESPECT;
                else if (rep >= REP_THRESHOLD_TRUST)
                    min_keep = REP_THRESHOLD_TRUST;
                else
                    min_keep = REP_THRESHOLD_NEUTRAL + 1;
                
                if (rep - decay < min_keep)
                    decay = rep - min_keep;
            }
            else if (rep < REP_THRESHOLD_COLD)
            {
                // 负声望回升不超过冷淡上限
                if (rep - decay > REP_THRESHOLD_COLD)
                    decay = rep - REP_THRESHOLD_COLD;
            }
            
            if (decay != 0)
            {
                player->add("reputation/" + factions[i], -decay);
                total_decay += decay;
            }
        }
    }
    
    return total_decay;
}

// ===================== 捐献声望计算 =====================

// 计算捐献物品获得的声望
// donate_type: ling_shi(灵石), cai_liao(材料), gong_fa(功法), fa_bao(法宝), dan_yao(丹药), qing_bao(情报)
varargs int calc_donate_reputation(string faction, string donate_type, int value, object player)
{
    int base;
    
    switch (donate_type)
    {
        case "ling_shi":  base = value / 100; break;                     // 灵石：+1/100灵石
        case "cai_liao":  base = value; break;                           // 材料：+5~+50/件
        case "gong_fa":   base = value; break;                           // 功法：+100~+500/本
        case "fa_bao":    base = value; break;                           // 法宝：+50~+300/件
        case "dan_yao":   base = value; break;                           // 丹药：+10~+100/枚
        case "qing_bao":  base = value; break;                           // 情报：+50~+200/条
        default: base = 10;
    }
    
    return base;
}

// ===================== 批量操作 =====================

// 获取玩家所有势力的声望摘要
// 返回值：mapping([ faction_id: ([ "name":..., "level":..., "value":..., "display":... ]) ])
mapping query_player_all_reputations(object player)
{
    mapping result = ([]);
    string *factions;
    int i, value, level;
    
    if (!player) return ([]);
    
    factions = query_all_factions();
    
    for (i = 0; i < sizeof(factions); i++)
    {
        value = player->query("reputation/" + factions[i]);
        level = query_level(value);
        
        result[factions[i]] = ([
            "name":    query_faction_name(factions[i]),
            "value":   value,
            "level":   level,
            "level_name": query_level_name(level),
            "display": query_faction_name(factions[i]) + "：" +
                       query_level_display(level) + "（" + value + "）",
        ]);
    }
    
    return result;
}

// ===================== 玩家初始化 =====================

// 初始化新玩家的声望数据（创建角色时调用）
void init_player_reputation(object player)
{
    string *factions;
    int i;
    
    if (!player) return;
    
    factions = query_all_factions();
    
    for (i = 0; i < sizeof(factions); i++)
    {
        // 所有势力初始声望为0（中立）
        if (undefinedp(player->query("reputation/" + factions[i])))
            player->set("reputation/" + factions[i], 0);
        
        // 初始化最后互动时间
        if (undefinedp(player->query("reputation/last_interact/" + factions[i])))
            player->set("reputation/last_interact/" + factions[i], time());
    }
}

// ===================== 等级解锁判定 =====================

// 判断玩家在某个势力是否已达到指定等级
int has_level(object player, string faction, int required_level)
{
    int player_level = query_player_level(player, faction);
    return (player_level >= required_level);
}

// 判断玩家声望是否满足指定值
int has_reputation(object player, string faction, int required_value)
{
    int rep = player->query("reputation/" + faction);
    return (rep >= required_value);
}

// ===================== 调试与信息 =====================

// 获取声望系统状态信息（用于wiz调试）
mapping query_system_info()
{
    return ([
        "factions":        keys(faction_info),
        "faction_count":   sizeof(faction_info),
        "mutex_relations": sizeof(mutex_relations),
    ]);
}
