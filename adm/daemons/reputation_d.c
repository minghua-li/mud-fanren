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
nosave mapping faction_info = ([
  "huangfeng_valley": ([
    "name": "黄枫谷", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，以符箓和阵法著称"
  ]),
  "yanyue_sect": ([
    "name": "掩月宗", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，以月华功法闻名"
  ]),
  "tianque_sect": ([
    "name": "天阙堡", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，擅防御和炼器"
  ]),
  "qianyuan_sect": ([
    "name": "千元派", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，主修符道和丹药"
  ]),
  "biling_sect": ([
    "name": "碧灵派", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，以灵植和药道著称"
  ]),
  "huayang_sect": ([
    "name": "化阳派", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，修火系功法"
  ]),
  "lingshou_mountain": ([
    "name": "灵兽山", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "越国七派之一，后叛投魔道，擅御兽之术"
  ]),
  "six_pulse_sword": ([
    "name": "六脉剑宗", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "正道剑修大宗，剑法天下无双"
  ]),
  "righteous_alliance": ([
    "name": "正道盟", "type": FACTION_TYPE_RIGHTEOUS,
    "desc": "天南正道联盟，统领正道各派"
  ]),
  "demon_six_sects": ([
    "name": "魔道六宗", "type": FACTION_TYPE_EVIL,
    "desc": "天南魔道核心势力"
  ]),
  "ghost_spirit_sect": ([
    "name": "鬼灵门", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，擅御鬼和幻术"
  ]),
  "blood_reincarnation": ([
    "name": "血影宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，以血煞功法著称"
  ]),
  "heavenly_corpse": ([
    "name": "天尸宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，炼尸控尸"
  ]),
  "yin_sect": ([
    "name": "阴煞宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，修阴寒功法"
  ]),
  "soul_refining": ([
    "name": "炼魂宗", "type": FACTION_TYPE_EVIL,
    "desc": "魔道六宗之一，炼魂夺舍"
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
  ({ "lingshou_mountain", "huangfeng_valley", MUTEX_STRONG }),
  ({ "lingshou_mountain", "yanyue_sect", MUTEX_STRONG }),
  ({ "lingshou_mountain", "tianque_sect", MUTEX_STRONG }),
  ({ "lingshou_mountain", "qianyuan_sect", MUTEX_STRONG }),
  ({ "lingshou_mountain", "biling_sect", MUTEX_STRONG }),
  ({ "lingshou_mountain", "huayang_sect", MUTEX_STRONG }),
  ({ "nine_nations_alliance", "mulan_legalists", MUTEX_STRONG }),
  ({ "demon", "jiaochi", MUTEX_WEAK }),
  ({ "demon", "demon_six_sects", MUTEX_STRONG }),
  ({ "righteous_alliance", "jiaochi", MUTEX_STRONG }),
});

// -------- 势力繁荣度（可运行时调整） --------
nosave mapping faction_prosperity = ([]);

// -------- 商店物品定义 --------
// 每项: item_id, name, type, file, price (灵石), rep_req (所需声望等级),
//        rep_cost (消耗声望), tier (商店层级), desc
nosave mixed *shop_items = ({
  // === 黄枫谷 ===
  // 基础商店 (中立)
  ({"hfg_blood_pill", "止血草", "pill", "/obj/remedy/blood_pill", 100, 0, 0, SHOP_TIER_BASIC, "huangfeng_valley", "基础疗伤丹药"}),
  ({"hfg_talisman", "低阶符箓", "item", "/obj/item/talisman_low", 200, 0, 0, SHOP_TIER_BASIC, "huangfeng_valley", "低阶攻击符箓"}),
  ({"hfg_qi_pill", "聚气丹", "pill", "/obj/remedy/qi_pill", 300, 0, 0, SHOP_TIER_BASIC, "huangfeng_valley", "恢复灵力的丹药"}),
  // 中级商店 (友善)
  ({"hfg_sword", "黄枫制式长剑", "weapon", "/obj/weapon/huangfeng_sword", 5000, REP_LEVEL_FRIENDLY, 100, SHOP_TIER_INTERMEDIATE, "huangfeng_valley", "黄枫谷弟子标配法器"}),
  ({"hfg_robe", "黄枫法袍", "armor", "/obj/armor/huangfeng_robe", 5000, REP_LEVEL_FRIENDLY, 100, SHOP_TIER_INTERMEDIATE, "huangfeng_valley", "黄枫谷制式法袍"}),
  ({"hfg_building_foundation", "筑基丹", "pill", "/obj/remedy/foundation_pill", 8000, REP_LEVEL_FRIENDLY, 200, SHOP_TIER_INTERMEDIATE, "huangfeng_valley", "筑基突破辅助丹药"}),
  // 高级商店 (信任)
  ({"hfg_spirit_sword", "中阶飞剑", "weapon", "/obj/weapon/spirit_sword", 30000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "huangfeng_valley", "中阶法器飞剑"}),
  ({"hfg_skill_book", "黄枫剑诀残篇", "skill", "/obj/skill/huangfeng_sword_skill", 50000, REP_LEVEL_TRUST, 800, SHOP_TIER_ADVANCED, "huangfeng_valley", "黄枫谷剑法残篇"}),
  ({"hfg_mana_crystal", "灵晶", "item", "/obj/item/mana_crystal", 20000, REP_LEVEL_TRUST, 300, SHOP_TIER_ADVANCED, "huangfeng_valley", "蕴含精纯灵力的水晶"}),
  // 核心宝库 (尊敬)
  ({"hfg_secret_manual", "大衍诀残篇", "skill", "/obj/skill/dayan_manual", 200000, REP_LEVEL_RESPECT, 5000, SHOP_TIER_CORE, "huangfeng_valley", "黄枫谷镇派功法残篇"}),
  ({"hfg_geng_essence", "庚精", "item", "/obj/item/geng_essence", 150000, REP_LEVEL_RESPECT, 3000, SHOP_TIER_CORE, "huangfeng_valley", "稀有炼器材料"}),
  ({"hfg_scroll", "秘境传送卷", "item", "/obj/item/secret_scroll", 100000, REP_LEVEL_RESPECT, 2000, SHOP_TIER_CORE, "huangfeng_valley", "通往黄枫谷秘境的卷轴"}),
  // 秘密仓库 (崇拜)
  ({"hfg_legacy_sword", "大衍神剑", "weapon", "/obj/weapon/dayan_sword", 500000, REP_LEVEL_ADORE, 20000, SHOP_TIER_SECRET, "huangfeng_valley", "大衍神君传承飞剑"}),
  ({"hfg_teleport_key", "古传送阵秘钥", "item", "/obj/item/teleport_key", 300000, REP_LEVEL_ADORE, 10000, SHOP_TIER_SECRET, "huangfeng_valley", "黄枫谷古传送阵通行密钥"}),

  // === 掩月宗 ===
  ({"yy_herb", "月华草", "pill", "/obj/remedy/moon_herb", 100, 0, 0, SHOP_TIER_BASIC, "yanyue_sect", "掩月宗特产灵草"}),
  ({"yy_moon_pill", "月华丹", "pill", "/obj/remedy/moon_pill", 5000, REP_LEVEL_FRIENDLY, 100, SHOP_TIER_INTERMEDIATE, "yanyue_sect", "掩月宗秘制丹药"}),
  ({"yy_moon_blade", "月华刃", "weapon", "/obj/weapon/moon_blade", 40000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "yanyue_sect", "蕴含月华之力的法器"}),
  ({"yy_moon_manual", "掩月心法", "skill", "/obj/skill/moon_manual", 250000, REP_LEVEL_RESPECT, 5000, SHOP_TIER_CORE, "yanyue_sect", "掩月宗核心功法"}),

  // === 灵兽山 ===
  ({"ls_beast_pill", "灵兽丸", "pill", "/obj/remedy/beast_pill", 150, 0, 0, SHOP_TIER_BASIC, "lingshou_mountain", "驯兽用灵丹"}),
  ({"ls_beast_ring", "御兽环", "item", "/obj/item/beast_ring", 6000, REP_LEVEL_FRIENDLY, 150, SHOP_TIER_INTERMEDIATE, "lingshou_mountain", "控制灵兽的法器"}),
  ({"ls_beast_armor", "灵兽甲", "armor", "/obj/armor/beast_armor", 35000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "lingshou_mountain", "以妖兽皮所制法袍"}),

  // === 星宫 ===
  ({"sp_star_pill", "星辰丹", "pill", "/obj/remedy/star_pill", 200, 0, 0, SHOP_TIER_BASIC, "star_palace", "引星辰之力炼制的丹药"}),
  ({"sp_nav_compass", "星罗盘", "item", "/obj/item/nav_compass", 8000, REP_LEVEL_FRIENDLY, 200, SHOP_TIER_INTERMEDIATE, "star_palace", "乱星海导航法器"}),
  ({"sp_star_sword", "星辰剑", "weapon", "/obj/weapon/star_sword", 50000, REP_LEVEL_TRUST, 800, SHOP_TIER_ADVANCED, "star_palace", "引星辰之力祭炼的飞剑"}),
  ({"sp_star_manual", "周天星斗诀", "skill", "/obj/skill/star_manual", 300000, REP_LEVEL_RESPECT, 6000, SHOP_TIER_CORE, "star_palace", "星宫镇派功法"}),

  // === 逆星盟 ===
  ({"ra_stealth_pill", "匿息丹", "pill", "/obj/remedy/stealth_pill", 250, 0, 0, SHOP_TIER_BASIC, "rebel_alliance", "隐藏气息的丹药"}),
  ({"ra_shadow_dagger", "暗影匕", "weapon", "/obj/weapon/shadow_dagger", 45000, REP_LEVEL_TRUST, 600, SHOP_TIER_ADVANCED, "rebel_alliance", "逆星盟刺客专用匕首"}),

  // === 广源斋 ===
  ({"gp_transfer_token", "传送符", "item", "/obj/item/transfer_token", 5000, 0, 0, SHOP_TIER_BASIC, "guangyuan_pavilion", "广源斋各分号免费传送凭证"}),
  ({"gp_spirit_stone", "灵石袋", "item", "/obj/item/spirit_bag", 10000, REP_LEVEL_FRIENDLY, 0, SHOP_TIER_INTERMEDIATE, "guangyuan_pavilion", "额外存储灵石的空间袋"}),
  ({"gp_auction_token", "拍卖令", "item", "/obj/item/auction_token", 50000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "guangyuan_pavilion", "广源斋高级拍卖会入场券"}),
  ({"gp_teleport_orb", "虚空珠", "item", "/obj/item/void_orb", 300000, REP_LEVEL_RESPECT, 5000, SHOP_TIER_CORE, "guangyuan_pavilion", "可跨界传送的稀世珍宝"}),
]);

// -------- 商店物品查找索引 --------
nosave mapping shop_index;

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

    // 构建商店索引
    rebuild_shop_index();
}

// 重建商店索引
void rebuild_shop_index()
{
    shop_index = ([]);
    for (int i = 0; i < sizeof(shop_items); i++)
    {
        mixed *item = shop_items[i];
        string faction = item[8]; // faction field
        if (!mapp(shop_index[faction]))
            shop_index[faction] = ([]);
        shop_index[faction][item[0]] = i; // item_id -> index
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

        // 不低于死敌下限，不把正声望扣成负的
        if (new_val < REP_VALUE_DEADLY)
            new_val = REP_VALUE_DEADLY;
        else if (current > 0 && new_val <= 0)
            new_val = 1; // 保护正声望不被直接扣到负

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
float query_discount(string faction)
{
    int level;

    if (!this_player()) return 1.0;
    level = query_reputation_level(this_player(), faction);

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

// ======== 商店系统 API ========

// 获取玩家在指定势力可用的商店物品列表
mixed *query_available_shop_items(object player, string faction)
{
    int level = query_reputation_level(player, faction);
    mixed *available = ({});
    mapping faction_items = shop_index[faction];

    if (!mapp(faction_items)) return ({});

    string *item_ids = keys(faction_items);
    for (int i = 0; i < sizeof(item_ids); i++)
    {
        int idx = faction_items[item_ids[i]];
        mixed *item = shop_items[idx];
        int req_level = item[5];  // rep_req

        if (level >= req_level)
            available += ({ item });
    }

    return available;
}

// 获取指定势力单个商店物品信息
mixed *get_shop_item(string faction, string item_id)
{
    if (!mapp(shop_index[faction])) return 0;
    int idx = shop_index[faction][item_id];
    if (idx < 0 || idx >= sizeof(shop_items)) return 0;
    return shop_items[idx];
}

// 计算购买物品实际价格
int calculate_price(mixed *item, object player)
{
    string faction = item[8];
    float discount = query_discount(faction);
    int base_price = item[4];  // price

    if (discount < 0) return -1; // 无法交易
    return to_int(base_price * discount);
}

// 购买物品
varargs int purchase_item(object player, string faction, string item_id, int quantity)
{
    mixed *item = get_shop_item(faction, item_id);
    if (!item) return 0;

    if (quantity < 1) quantity = 1;

    int level = query_reputation_level(player, faction);
    int req_level = item[5];  // rep_req

    if (level < req_level) return -1; // 声望不足

    // 检查死敌
    float discount = query_discount(faction);
    if (discount < 0) return -2; // 无法交易

    // 检查声望消耗
    int rep_cost = item[6] * quantity;  // rep_cost * quantity
    int current_rep = player->query(REP_PATH_FACTION + "/" + faction);

    if (current_rep < rep_cost) return -3; // 声望不足支付

    // 计算灵石价格
    int total_price = calculate_price(item, player) * quantity;
    int player_money = player->query("balance");

    if (player_money < total_price) return -4; // 灵石不足

    // 扣除声望和灵石
    player->add(REP_PATH_FACTION + "/" + faction, -rep_cost);
    player->add("balance", -total_price);
    player->set(REP_PATH_LAST_INTERACT + "/" + faction, time());

    // 生成物品（实际游戏中需调用对应克隆函数）
    // 这里返回成功状态，由调用方执行具体物品生成
    return 1;
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
