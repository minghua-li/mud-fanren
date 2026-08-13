// reputation_d.c
// 声望与阵营交互系统 - 核心守护进程
// 设计文档: 02-扩充内容/02-声望与互动玩法.md
// 提供: 声望管理、阵营交互、种族外交、折扣计算、商店API

#include <ansi.h>
#include <reputation.h>
#include <globals.h>

inherit F_DBASE;
inherit F_SAVE;

// -------- 势力定义 --------
// 门派 ID 以 .knowledge/factions/sects/ 档案与 1D 文档为准
nosave mapping faction_info = ([
  // 越国七派
  "yanyue_sect": ([
    "name": "掩月宗", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之首，提倡双修之术，标志性法器天月神舟"
  ]),
  "huangfeng_valley": ([
    "name": "黄枫谷", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，山门在建州太岳山脉，以丹药符箓起家，兼修剑道"
  ]),
  "lingshou_mountain": ([
    "name": "灵兽山", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，擅长御兽役虫，实为御灵宗安插的暗桩"
  ]),
  "qingxu_sect": ([
    "name": "清虚门", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，道门传承，清心寡欲，飞行法器雪虹绫"
  ]),
  "huadao_dock": ([
    "name": "化刀坞", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，刀修汇聚，炼器工艺突出"
  ]),
  "tianque_fort": ([
    "name": "天阙堡", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，筑堡建州，擅防御与阵法，镇派法宝为黄色大印"
  ]),
  "jujian_gate": ([
    "name": "巨剑门", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，全男弟子黑衣背负无鞘巨剑，体剑双修"
  ]),
  // 魔道六宗（天罗国）
  "hehuan_sect": ([
    "name": "合欢宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，双修魅惑，弟子多俊男美女，擅长阴阳采补"
  ]),
  "tiansha_sect": ([
    "name": "天煞宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，杀戮功法，以杀证道，煞气越重功法越强"
  ]),
  "guiling_sect": ([
    "name": "鬼灵门", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，驱鬼役妖，毒术暗术，镇派功法血灵大法"
  ]),
  "yuling_sect": ([
    "name": "御灵宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，万灵归宗，虫兽双修，与越国灵兽山同源"
  ]),
  "tianmo_sect": ([
    "name": "天魔宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，天魔功法，召唤天魔附体，短时暴增战力"
  ]),
  "yinluo_sect": ([
    "name": "阴罗宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，毒功暗杀，兼掌情报"
  ]),
  // 天南超级势力
  "righteous_alliance": ([
    "name": "正道盟", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "天南正道联盟，统领正道各派，与魔道六宗对峙"
  ]),
  "demon_six_sects": ([
    "name": "魔道六宗", "type": FACTION_TYPE_EVIL,
    "desc": "天罗国魔道六宗联盟，实力远超越国七派"
  ]),
  "star_palace": ([
    "name": "星宫", "type": FACTION_TYPE_NEUTRAL,
    "desc": "乱星海最大势力，统治天星城"
  ]),
  "rebel_alliance": ([
    "name": "逆星盟", "type": FACTION_TYPE_NEUTRAL,
    "desc": "乱星海反星宫联盟"
  ]),
  "guangyuan_pavilion": ([
    "name": "广源斋", "type": FACTION_TYPE_ORGANIZATION,
    "desc": "横跨人界灵界的商盟，中立交易所"
  ]),
  "tianyuan_city": ([
    "name": "天渊城", "type": FACTION_TYPE_ORGANIZATION,
    "desc": "灵界人族前线要塞，对抗魔界"
  ]),
  "nine_nations_alliance": ([
    "name": "九国盟", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "天南九国正道联盟，对抗慕兰法士"
  ]),
  "mulan_legalists": ([
    "name": "慕兰法士", "type": FACTION_TYPE_NEUTRAL,
    "desc": "慕兰草原的法师势力"
  ]),
]);

// -------- 灵界种族定义 --------
nosave mapping race_info = ([
  "human":        (["name": "人族", "initial": RACE_RELATION_ALLY,    "desc": "玩家所属种族"]),
  "demon":        (["name": "妖族", "initial": RACE_RELATION_NEUTRAL, "desc": "灵界大族，与人族有盟约"]),
  "wood":         (["name": "木族", "initial": RACE_RELATION_NEUTRAL, "desc": "灵界古老种族，掌控森林"]),
  "jiaochi":      (["name": "角蚩族", "initial": RACE_RELATION_DEADLY, "desc": "灵界最具侵略性的种族"]),
  "sea_king":     (["name": "海王族", "initial": RACE_RELATION_HOSTILE,"desc": "海域霸主，与人族不友好"]),
  "yecha":        (["name": "夜叉族", "initial": RACE_RELATION_NEUTRAL, "desc": "血天大陆种族，擅长锻造"]),
  "feiling":      (["name": "飞灵族", "initial": RACE_RELATION_NEUTRAL, "desc": "风属性种族，天空行者"]),
  "kongyu":       (["name": "空鱼族", "initial": RACE_RELATION_NEUTRAL, "desc": "精通空间秘术的神秘种族"]),
  "crystal":      (["name": "晶族", "initial": RACE_RELATION_NEUTRAL, "desc": "以晶体生命形态存在的种族"]),
  "yun":          (["name": "云族", "initial": RACE_RELATION_NEUTRAL, "desc": "居住在高空的炼器种族"]),
  "blood_race":   (["name": "血族", "initial": RACE_RELATION_HOSTILE, "desc": "血天大陆的嗜血种族"]),
  "thunder":      (["name": "雷族", "initial": RACE_RELATION_NEUTRAL, "desc": "掌控雷电之力的种族"]),
  "rock_giant":   (["name": "石巨人", "initial": RACE_RELATION_NEUTRAL, "desc": "力大无穷的岩石生命"]),
  "phantom":      (["name": "幻族", "initial": RACE_RELATION_NEUTRAL, "desc": "精通幻术的神秘种族"]),
  "frost":        (["name": "冰族", "initial": RACE_RELATION_NEUTRAL, "desc": "极寒之地的冰霜种族"]),
  "shadow":       (["name": "影族", "initial": RACE_RELATION_HOSTILE, "desc": "暗影中的刺客种族"]),
  "ancient_spirit":(["name": "古灵族", "initial": RACE_RELATION_NEUTRAL, "desc": "灵界最古老的种族之一"]),
  "flame":        (["name": "炎族", "initial": RACE_RELATION_NEUTRAL, "desc": "生活在火山地带的火焰种族"]),
  "light":        (["name": "光族", "initial": RACE_RELATION_NEUTRAL, "desc": "光明属性的稀有种族"]),
  "darkness":     (["name": "暗族", "initial": RACE_RELATION_HOSTILE, "desc": "黑暗属性的神秘种族"]),
  "spirit_beast":(["name": "灵兽族", "initial": RACE_RELATION_NEUTRAL, "desc": "妖兽进化而来的智慧种族"]),
  "tree_elf":     (["name": "木精灵", "initial": RACE_RELATION_NEUTRAL, "desc": "木族的分支，亲近自然"]),
  "deep_sea":     (["name": "深海族", "initial": RACE_RELATION_NEUTRAL, "desc": "海王族的分支，深海居民"]),
  "bone":         (["name": "骨族", "initial": RACE_RELATION_HOSTILE, "desc": "亡灵属性的不死种族"]),
  "poison":       (["name": "毒族", "initial": RACE_RELATION_HOSTILE, "desc": "用毒出名的危险种族"]),
  "metal":        (["name": "金族", "initial": RACE_RELATION_NEUTRAL, "desc": "金属生命形态的种族"]),
  "silk":         (["name": "丝族", "initial": RACE_RELATION_NEUTRAL, "desc": "以编织灵丝为生的种族"]),
  "scale":        (["name": "鳞族", "initial": RACE_RELATION_NEUTRAL, "desc": "半人半妖的鳞甲种族"]),
  "wing":         (["name": "翼族", "initial": RACE_RELATION_NEUTRAL, "desc": "长有翅膀的天空种族"]),
  "dream":        (["name": "梦族", "initial": RACE_RELATION_NEUTRAL, "desc": "存在于梦境维度的种族"]),
]);

// -------- 互斥关系 --------
// 每项: ({ faction_a, faction_b, strength })
nosave mixed *mutex_relations = ({
  ({ "righteous_alliance", "demon_six_sects", MUTEX_STRONG }),
  ({ "star_palace", "rebel_alliance", MUTEX_STRONG }),
  // 灵兽山叛出后与其余越国六派敌对
  ({ "lingshou_mountain", "yanyue_sect", MUTEX_STRONG }),
  ({ "lingshou_mountain", "huangfeng_valley", MUTEX_STRONG }),
  ({ "lingshou_mountain", "qingxu_sect", MUTEX_STRONG }),
  ({ "lingshou_mountain", "huadao_dock", MUTEX_STRONG }),
  ({ "lingshou_mountain", "tianque_fort", MUTEX_STRONG }),
  ({ "lingshou_mountain", "jujian_gate", MUTEX_STRONG }),
  ({ "nine_nations_alliance", "mulan_legalists", MUTEX_STRONG }),
  ({ "demon", "jiaochi", MUTEX_WEAK }),
  ({ "demon", "demon_six_sects", MUTEX_STRONG }),
  ({ "righteous_alliance", "jiaochi", MUTEX_STRONG }),
});

// -------- 势力繁荣度（可运行时调整） --------
nosave mapping faction_prosperity = ([]);

void create()
{
    seteuid(getuid());
    restore();

    // 初始化繁荣度
    if (!mapp(faction_prosperity) || sizeof(faction_prosperity) == 0)
    {
        string *factions = keys(faction_info);
        foreach (string f in factions)
            faction_prosperity[f] = PROSPERITY_PEAK;
    }
}

// ======== 声望查询接口 ========

// 查询玩家在指定势力的声望值
int query_reputation_value(object player, string faction)
{
    return player->query(REP_PATH_FACTION + "/" + faction);
}

// 查询声望等级 (-3 ~ 5)
int query_reputation_level(object player, string faction)
{
    int rep;

    if (faction == "righteous")
        rep = player->query(REP_PATH_GLOBAL + "/righteous");
    else if (faction == "evil")
        rep = player->query(REP_PATH_GLOBAL + "/evil");
    else
        rep = player->query(REP_PATH_FACTION + "/" + faction);

    return calculate_level(rep);
}

// 计算声望等级
int calculate_level(int rep)
{
    if (rep <= REP_VALUE_DEADLY)     return REP_LEVEL_DEADLY;
    if (rep <= REP_VALUE_HOSTILE)    return REP_LEVEL_HOSTILE;
    if (rep <= REP_VALUE_COLD)       return REP_LEVEL_COLD;
    if (rep < REP_VALUE_FRIENDLY)    return REP_LEVEL_NEUTRAL;
    if (rep < REP_VALUE_TRUST)       return REP_LEVEL_FRIENDLY;
    if (rep < REP_VALUE_RESPECT)     return REP_LEVEL_TRUST;
    if (rep < REP_VALUE_ADORE)       return REP_LEVEL_RESPECT;
    if (rep < REP_VALUE_LEGENDARY)   return REP_LEVEL_ADORE;
    return REP_LEVEL_LEGENDARY;
}

// 获取声望等级名称（带颜色）
string get_reputation_level_name(int level)
{
    switch (level)
    {
    case REP_LEVEL_DEADLY:   return REP_COLOR_DEADLY + REP_NAME_DEADLY + NOR;
    case REP_LEVEL_HOSTILE:  return REP_COLOR_HOSTILE + REP_NAME_HOSTILE + NOR;
    case REP_LEVEL_COLD:     return REP_COLOR_COLD + REP_NAME_COLD + NOR;
    case REP_LEVEL_NEUTRAL:  return REP_NAME_NEUTRAL;
    case REP_LEVEL_FRIENDLY: return REP_COLOR_FRIENDLY + REP_NAME_FRIENDLY + NOR;
    case REP_LEVEL_TRUST:    return REP_COLOR_TRUST + REP_NAME_TRUST + NOR;
    case REP_LEVEL_RESPECT:  return REP_COLOR_RESPECT + REP_NAME_RESPECT + NOR;
    case REP_LEVEL_ADORE:    return REP_COLOR_ADORE + REP_NAME_ADORE + NOR;
    case REP_LEVEL_LEGENDARY:return REP_COLOR_LEGENDARY + REP_NAME_LEGENDARY + NOR;
    default:                  return REP_NAME_NEUTRAL;
    }
}

// ======== 声望修改接口 ========

// 增加声望（含互斥处理）
varargs int add_reputation(object player, string faction, int amount, string reason)
{
    int new_val;
    string *mutex_affected;

    // 检查每日上限
    int cap = query_daily_cap(player);
    int today = player->query("reputation/daily/" + faction + "/date");
    int daily_used = player->query("reputation/daily/" + faction + "/amount");
    int now = time();
    int today_key = now / 86400;

    if (today != today_key)
    {
        daily_used = 0;
        player->set("reputation/daily/" + faction + "/date", today_key);
        player->set("reputation/daily/" + faction + "/amount", 0);
    }

    if (amount > 0 && daily_used + amount > cap)
        amount = cap - daily_used;

    if (amount <= 0) return 0;

    // 更新声望
    new_val = player->add(REP_PATH_FACTION + "/" + faction, amount);
    player->set(REP_PATH_LAST_INTERACT + "/" + faction, now);
    player->add("reputation/daily/" + faction + "/amount", amount);

    // 互斥处理
    mutex_affected = apply_mutex(player, faction, amount);

    // 记录日志
    if (reason)
        log_file("reputation", sprintf("%s %s %s %s %+d (mutex: %s)\n",
                  ctime(now), player->query("id"), faction, reason, amount,
                  sizeof(mutex_affected) > 0 ? implode(mutex_affected, ",") : "none"));

    return new_val;
}

// 减少声望（消耗）
varargs int deduct_reputation(object player, string faction, int amount)
{
    int current = player->query(REP_PATH_FACTION + "/" + faction);

    if (current < amount) return 0;

    player->add(REP_PATH_FACTION + "/" + faction, -amount);
    player->set(REP_PATH_LAST_INTERACT + "/" + faction, time());
    return 1;
}

// 应用互斥关系
string *apply_mutex(object player, string faction, int amount)
{
    string *affected = ({});
    int mutex_amount;

    for (int i = 0; i < sizeof(mutex_relations); i++)
    {
        mixed *rel = mutex_relations[i];
        string a = rel[0], b = rel[1];
        int strength = rel[2];
        string target;

        if (a == faction) target = b;
        else if (b == faction) target = a;
        else continue;

        // 确定互斥扣除量
        if (strength == MUTEX_STRONG)
            mutex_amount = to_int(amount * 0.3);
        else
            mutex_amount = to_int(amount * 0.1);

        if (mutex_amount <= 0) continue;

        int current = player->query(REP_PATH_FACTION + "/" + target);
        int new_val = current - mutex_amount;

        // 不低于死敌下限
        if (new_val < REP_VALUE_DEADLY)
            new_val = REP_VALUE_DEADLY;

        if (new_val != current)
        {
            player->set(REP_PATH_FACTION + "/" + target, new_val);
            affected += ({ target });
        }
    }
    return affected;
}

// ======== 折扣计算 ========

// 查询该势力当前折扣率
float query_discount(string faction, object player)
{
    int level;

    if (!player) return 1.0;
    level = query_reputation_level(player, faction);

    float base;
    switch (level)
    {
    case REP_LEVEL_DEADLY:   return -1.0;   // 无法交易
    case REP_LEVEL_HOSTILE:  base = REP_DISCOUNT_HOSTILE; break;
    case REP_LEVEL_COLD:     base = REP_DISCOUNT_COLD; break;
    case REP_LEVEL_NEUTRAL:  base = REP_DISCOUNT_NEUTRAL; break;
    case REP_LEVEL_FRIENDLY: base = REP_DISCOUNT_FRIENDLY; break;
    case REP_LEVEL_TRUST:    base = REP_DISCOUNT_TRUST; break;
    case REP_LEVEL_RESPECT:  base = REP_DISCOUNT_RESPECT; break;
    case REP_LEVEL_ADORE:    base = REP_DISCOUNT_ADORE; break;
    case REP_LEVEL_LEGENDARY:base = REP_DISCOUNT_LEGENDARY; break;
    default:                 base = 1.0;
    }

    // 繁荣度修正
    float mod = 1.0;
    int prosperity = faction_prosperity[faction];
    switch (prosperity)
    {
    case PROSPERITY_DECLINE: mod = PROSPERITY_MOD_DECLINE; break;
    case PROSPERITY_HARD:    mod = PROSPERITY_MOD_HARD; break;
    case PROSPERITY_PEAK:    mod = PROSPERITY_MOD_PEAK; break;
    case PROSPERITY_EXPAND:  mod = PROSPERITY_MOD_EXPAND; break;
    }

    return base * mod;
}

// ======== 每日声望上限 ========

int query_daily_cap(object player)
{
    int exp = player->query("combat_exp");

    // 按经验估算境界
    if (exp < 100000)      return REP_DAILY_CAP_QIYIN;
    if (exp < 1000000)     return REP_DAILY_CAP_ZHUIJI;
    if (exp < 10000000)    return REP_DAILY_CAP_JIEDAN;
    if (exp < 50000000)    return REP_DAILY_CAP_YUANYING;
    if (exp < 200000000)   return REP_DAILY_CAP_HUASHEN;
    return REP_DAILY_CAP_LIANXU;
}

// ======== 阵营交互 ========

// 获取玩家与某势力的交互选项
mixed *get_available_actions(object player, string faction)
{
    int level = query_reputation_level(player, faction);
    mixed *actions = ({});

    if (level >= REP_LEVEL_NEUTRAL)
        actions += ({ FACTION_ACTION_TRADE });

    if (level >= REP_LEVEL_FRIENDLY)
        actions += ({ FACTION_ACTION_QUEST, FACTION_ACTION_TRAIN });

    if (level >= REP_LEVEL_TRUST)
        actions += ({ FACTION_ACTION_ALLY });

    return actions;
}

// 获取交互选项名称
string get_action_name(int action)
{
    switch (action)
    {
    case FACTION_ACTION_ALLY:    return "结盟";
    case FACTION_ACTION_HOSTILE: return "敌对";
    case FACTION_ACTION_NEUTRAL: return "中立";
    case FACTION_ACTION_TRADE:   return "交易";
    case FACTION_ACTION_QUEST:   return "任务";
    case FACTION_ACTION_TRAIN:   return "修炼";
    default: return "未知";
    }
}

// 设置阵营关系
int set_faction_relation(object player, string faction, string relation)
{
    if (member_array(relation, ({ "ally", "hostile", "neutral" })) == -1)
        return 0;

    player->set(REP_PATH_FACTION_REL + "/" + faction, relation);
    return 1;
}

// 查询阵营关系
string query_faction_relation(object player, string faction)
{
    string rel = player->query(REP_PATH_FACTION_REL + "/" + faction);
    if (!rel) return "neutral";
    return rel;
}

// ======== 种族外交 ========

// 获取种族基础信息
mapping get_race_info(string race_id)
{
    return race_info[race_id];
}

// 获取玩家与某种族的关系等级
int query_race_relation_level(object player, string race_id)
{
    int rep = player->query(REP_PATH_RACE + "/" + race_id);
    return calculate_level(rep);
}

// 获取种族初始关系
int query_race_initial(string race_id)
{
    if (!race_info[race_id]) return RACE_RELATION_NEUTRAL;
    return race_info[race_id]["initial"];
}

// 种族初始关系名称
string get_race_relation_name(int relation)
{
    switch (relation)
    {
    case RACE_RELATION_ALLY:  return RACE_REL_NAME_ALLY;
    case RACE_RELATION_NEUTRAL: return RACE_REL_NAME_NEUTRAL;
    case RACE_RELATION_HOSTILE: return RACE_REL_NAME_HOSTILE;
    case RACE_RELATION_DEADLY:  return RACE_REL_NAME_DEADLY;
    default: return RACE_REL_NAME_NEUTRAL;
    }
}

// 获取所有种族ID
string *get_all_races()
{
    return keys(race_info);
}

// 获取所有势力ID
string *get_all_factions()
{
    return keys(faction_info);
}

// 获取势力信息
mapping get_faction_info(string faction_id)
{
    return faction_info[faction_id];
}

// ======== 声望衰减 ========

void decay_reputation(object player)
{
    int now = time();
    mapping faction_reps = player->query(REP_PATH_FACTION);

    if (!mapp(faction_reps)) return;

    string *factions = keys(faction_reps);
    for (int i = 0; i < sizeof(factions); i++)
    {
        string f = factions[i];
        int rep = faction_reps[f];

        if (rep <= REP_VALUE_NEUTRAL_HIGH && rep >= REP_VALUE_NEUTRAL_LOW)
            continue; // 中立稳定区

        int last = player->query(REP_PATH_LAST_INTERACT + "/" + f);
        if (!last) continue;

        int days = (now - last) / 86400;
        if (days < 7) continue; // 7天内互动过

        int decay;
        if (rep > REP_VALUE_ADORE)            decay = 10;
        else if (rep > REP_VALUE_RESPECT)     decay = 5;
        else if (rep > REP_VALUE_TRUST)       decay = 3;
        else if (rep > REP_VALUE_FRIENDLY)    decay = 1;
        else if (rep < REP_VALUE_COLD)        decay = -2; // 负声望回升
        else continue;

        player->add(REP_PATH_FACTION + "/" + f, -decay);
    }
}

// ======== 辅助函数 ========

// 格式化显示声望条
string format_reputation_bar(int current, int next_threshold, int width)
{
    if (width < 1) width = 20;

    int filled;
    int range = next_threshold - (REP_VALUE_NEUTRAL_HIGH + 1);

    // 超过阈值则显示满
    if (current >= next_threshold)
        filled = width;
    else if (current <= REP_VALUE_NEUTRAL_HIGH)
        filled = 0;
    else
        filled = (current - (REP_VALUE_NEUTRAL_HIGH + 1)) * width / range;

    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    string bar = "";
    for (int i = 0; i < width; i++)
    {
        if (i < filled)
            bar += HIG "■" NOR;
        else
            bar += CYN "□" NOR;
    }
    return bar;
}

// 获取势力繁荣度
int query_prosperity(string faction)
{
    if (!faction_prosperity[faction])
        return PROSPERITY_PEAK;
    return faction_prosperity[faction];
}

// 设置势力繁荣度（管理员）
void set_prosperity(string faction, int value)
{
    faction_prosperity[faction] = value;
    save();
}

// 保存数据
string query_save_file()
{
    return "/data/reputation_d";
}

// 显示势力外交关系总览（调试）
string dump_faction_info()
{
    string output = "====== 势力信息 ======\n";
    string *factions = keys(faction_info);
    foreach (string f in factions)
    {
        mapping info = faction_info[f];
        string type_name;
        switch (info["type"])
        {
        case FACTION_TYPE_RIGHTEOUS: type_name = "正道"; break;
        case FACTION_TYPE_EVIL:      type_name = "魔道"; break;
        case FACTION_TYPE_NEUTRAL:   type_name = "中立"; break;
        case FACTION_TYPE_ORGANIZATION: type_name = "组织"; break;
        default: type_name = "未知";
        }
        output += sprintf("%-20s | %-10s | %s\n", info["name"], type_name, info["desc"]);
    }

    output += "\n====== 种族信息 ======\n";
    string *races = keys(race_info);
    foreach (string r in races)
    {
        mapping info = race_info[r];
        output += sprintf("%-12s | %-10s | %s\n", info["name"],
                          get_race_relation_name(info["initial"]), info["desc"]);
    }

    return output;
}
