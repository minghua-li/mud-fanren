// sect_facility_d.c
// 门派设施守护进程 —— 通用设施框架（灵田/丹房/藏经阁/护山大阵/演武场）+ 九宗特色设施
// 设计文档: .knowledge/factions/sects/ 九宗档案「设施」节
//           02-扩充内容/02-区域游戏玩法.md §4 宗门驻地玩法
// 依赖: SECT_D（门派身份/贡献 add_contribution）、MONEY_D（灵石结算 player_pay）
// Created for ticket #60

#include <ansi.h>
#include <sect_facility.h>
#include <sect.h>
#include <globals.h>
#include <mansion.h>

inherit F_DBASE;

// -------- 设施配置 --------
// facility_key -> ([
//   "sect"      : 所属门派 ID（对齐 sect_d.c 九宗 ID）,
//   "type"      : SECT_FACILITY_*,
//   "name"      : 设施名,
//   "desc"      : 描述,
//   "room"      : 设施房间路径（不含 .c，对齐 base_name）,
//   "level"     : 当前等级（门派共享，随弟子捐献升级）,
//   "max_level" : 等级上限,
//   "effect"    : ([ "type": buff 类型/效果标识, "base": 基础值, "per_level": 每级递增 ]),
//   "duration"  : buff 时长（秒，非 buff 类可省）,
//   "use_stone" / "use_contrib" : 每次使用消耗,
//   "upgrade"   : ([ 目标等级: ({ 灵石, 贡献 }) ]),
//   "daily_limit": 每日使用上限（0=不限）,
//   "daily_reward": ([ "exp": 每日修行经验, "contrib": 每日贡献, "verb": 动作描述 ]),
//   "market"    : 1 表示坊市（可买灵材）,
// ])
nosave mapping facility_config = ([
  // ================= 掩月宗 =================
  "yanyue_yanwu": ([
    "sect": "yanyue_sect", "type": SECT_FACILITY_TRAINING,
    "name": "演武场",
    "desc": "掩月宗演武场底蕴为越国七派之最，法修双修弟子皆在此切磋精进。",
    "room": "/d/yueguo/yanyue/fac/yanwu",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_TRAIN, "base": 10, "per_level": 5 ]),
    "duration": 3600, "use_stone": 5, "use_contrib": 50,
    "upgrade": SECT_UPGRADE_COMMON, "daily_limit": 0,
    "daily_reward": ([ "exp": SECT_SPAR_EXP_BASE, "contrib": SECT_SPAR_CONTRIB, "verb": "切磋" ]),
  ]),
  "yanyue_tianyuezhou": ([
    "sect": "yanyue_sect", "type": SECT_FACILITY_SPECIAL,
    "name": "天月神舟船坞",
    "desc": "掩月宗巨型飞行法器天月神舟之船坞，全宗集体出行的中枢。",
    "room": "/d/yueguo/yanyue/fac/tianyuezhou",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_SPIRIT, "base": 5, "per_level": 2 ]),
    "duration": 7200, "use_stone": 50, "use_contrib": 500,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 1,
  ]),

  // ================= 黄枫谷 =================
  "huangfeng_baiyaoyuan": ([
    "sect": "huangfeng_valley", "type": SECT_FACILITY_PLANT,
    "name": "百药园",
    "desc": "黄枫谷太岳药园，谷内灵田成片，炼丹原料基地，韩立当年曾在此劳作。",
    "room": "/d/yueguo/huangfeng/fac/baiyaoyuan",
    "level": 1, "max_level": 3,
    "effect": ([ "type": "herb_efficiency", "base": 50, "per_level": 10 ]),
    "use_stone": SECT_USE_STONE_DEFAULT, "use_contrib": 0,
    "upgrade": SECT_UPGRADE_PLANT, "daily_limit": 0,
  ]),
  "huangfeng_danfang": ([
    "sect": "huangfeng_valley", "type": SECT_FACILITY_ALCHEMY,
    "name": "岳麓殿丹房",
    "desc": "黄枫谷炼丹核心，全谷丹方配方藏于岳麓殿藏室，丹炉青烟常年不绝。",
    "room": "/d/yueguo/huangfeng/fac/danfang",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_ALCHEMY, "base": 10, "per_level": 2 ]),
    "duration": 7200, "use_stone": SECT_USE_STONE_DEFAULT, "use_contrib": SECT_USE_CONTRIB_DEFAULT,
    "upgrade": SECT_UPGRADE_COMMON, "daily_limit": 0,
  ]),
  "huangfeng_cangjing": ([
    "sect": "huangfeng_valley", "type": SECT_FACILITY_LIBRARY,
    "name": "岳麓殿藏经阁",
    "desc": "传功阁与岳麓殿藏室，法器丹药配方书籍密术收藏之地，谷内大部分丹方已失传。",
    "room": "/d/yueguo/huangfeng/yuexudian",
    "level": 1, "max_level": 3,
    "effect": ([ "type": "copy_reduce", "base": 0, "per_level": 10 ]),
    "use_stone": 0, "use_contrib": 0,
    "upgrade": SECT_UPGRADE_COMMON, "daily_limit": 0,
  ]),
  "huangfeng_fangshi": ([
    "sect": "huangfeng_valley", "type": SECT_FACILITY_SPECIAL,
    "name": "坊市",
    "desc": "黄枫谷坊市位于太岳山脉东北边缘，与元武国交界，修士交易之所，灵材云集。",
    "room": "/d/yueguo/huangfeng/fac/fangshi",
    "level": 1, "max_level": 3,
    "effect": ([ "type": "market_discount", "base": 0, "per_level": 5 ]),
    "use_stone": 0, "use_contrib": 0,
    "upgrade": SECT_UPGRADE_MARKET, "daily_limit": 0,
    "market": 1,
  ]),
  "huangfeng_hushan": ([
    "sect": "huangfeng_valley", "type": SECT_FACILITY_DEFENSE,
    "name": "护山大阵",
    "desc": "黄枫谷谷外防护大阵，灵光泛泛，有结丹期师叔祖坐镇主持。",
    "room": "/d/yueguo/huangfeng/fac/hushan",
    "level": 1, "max_level": 4,
    "effect": ([ "type": SECT_BUFF_DEFENSE, "base": 20, "per_level": 10 ]),
    "duration": 7200, "use_stone": 20, "use_contrib": 200,
    "upgrade": SECT_UPGRADE_DEFENSE, "daily_limit": 0,
  ]),
  "huangfeng_qingjianchang": ([
    "sect": "huangfeng_valley", "type": SECT_FACILITY_TRAINING,
    "name": "青元剑场",
    "desc": "黄枫谷演武场，剑修比试之所，青元剑诀弟子常于此较技。",
    "room": "/d/yueguo/huangfeng/fac/qingjianchang",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_TRAIN, "base": 10, "per_level": 5 ]),
    "duration": 3600, "use_stone": 5, "use_contrib": 50,
    "upgrade": SECT_UPGRADE_COMMON, "daily_limit": 0,
    "daily_reward": ([ "exp": SECT_SPAR_EXP_BASE, "contrib": SECT_SPAR_CONTRIB, "verb": "切磋" ]),
  ]),

  // ================= 灵兽山 =================
  "lingshou_shoulan": ([
    "sect": "lingshou_mountain", "type": SECT_FACILITY_SPECIAL,
    "name": "兽栏",
    "desc": "灵兽山御兽驯养之所，灵兽栖息训练场。",
    "room": "/d/yueguo/lingshou/fac/shoulan",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_BEAST, "base": 10, "per_level": 5 ]),
    "duration": 7200, "use_stone": 30, "use_contrib": 300,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
    "daily_reward": ([ "exp": 600, "contrib": 30, "verb": "驯兽" ]),
  ]),
  "lingshou_chongfang": ([
    "sect": "lingshou_mountain", "type": SECT_FACILITY_SPECIAL,
    "name": "虫房",
    "desc": "灵兽山灵虫培育之所，灵虫孵化进阶之地。",
    "room": "/d/yueguo/lingshou/fac/chongfang",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_BEAST, "base": 5, "per_level": 3 ]),
    "duration": 7200, "use_stone": 20, "use_contrib": 200,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
  ]),

  // ================= 清虚门 =================
  "qingxu_daoguan": ([
    "sect": "qingxu_sect", "type": SECT_FACILITY_SPECIAL,
    "name": "道观",
    "desc": "清虚门道观，道门经卷收藏之藏经阁，兼作修士论道之所，清净无为。",
    "room": "/d/yueguo/qingxu/fac/daoguan",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_DAO, "base": 10, "per_level": 5 ]),
    "duration": 7200, "use_stone": SECT_USE_STONE_DEFAULT, "use_contrib": SECT_USE_CONTRIB_DEFAULT,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
    "daily_reward": ([ "exp": 500, "contrib": 20, "verb": "论道" ]),
  ]),
  "qingxu_yanwu": ([
    "sect": "qingxu_sect", "type": SECT_FACILITY_TRAINING,
    "name": "演武场",
    "desc": "清虚门演武场，道剑符双修比试之所。",
    "room": "/d/yueguo/qingxu/fac/yanwu",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_TRAIN, "base": 10, "per_level": 5 ]),
    "duration": 3600, "use_stone": 5, "use_contrib": 50,
    "upgrade": SECT_UPGRADE_COMMON, "daily_limit": 0,
    "daily_reward": ([ "exp": 500, "contrib": 20, "verb": "切磋" ]),
  ]),

  // ================= 化刀坞 =================
  "huadao_lianqi": ([
    "sect": "huadao_dock", "type": SECT_FACILITY_SPECIAL,
    "name": "炼器工坊",
    "desc": "化刀坞炼器坊，炼器工艺为越国七派之最，刀器炼制皆出此处。",
    "room": "/d/yueguo/huadao/fac/lianqi",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_FORGE, "base": 10, "per_level": 2 ]),
    "duration": 7200, "use_stone": SECT_USE_STONE_DEFAULT, "use_contrib": SECT_USE_CONTRIB_DEFAULT,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
  ]),

  // ================= 天阙堡 =================
  "tianque_chengbang": ([
    "sect": "tianque_fort", "type": SECT_FACILITY_DEFENSE,
    "name": "城堡工事",
    "desc": "天阙堡城防体系，城墙箭楼与堡外护山大阵互为犄角，筑堡建州之术。",
    "room": "/d/yueguo/tianque/fac/chengbang",
    "level": 1, "max_level": 4,
    "effect": ([ "type": SECT_BUFF_DEFENSE, "base": 15, "per_level": 10 ]),
    "duration": 7200, "use_stone": 20, "use_contrib": 200,
    "upgrade": SECT_UPGRADE_DEFENSE, "daily_limit": 0,
  ]),

  // ================= 巨剑门 =================
  "jujian_jianzhong": ([
    "sect": "jujian_gate", "type": SECT_FACILITY_SPECIAL,
    "name": "剑冢",
    "desc": "巨剑门历代巨剑收藏之剑冢，剑修参悟剑意之所。",
    "room": "/d/yueguo/jujian/fac/jianzhong",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_SWORD, "base": 10, "per_level": 5 ]),
    "duration": 7200, "use_stone": 20, "use_contrib": 200,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
    "daily_reward": ([ "exp": 600, "contrib": 30, "verb": "观摩剑意" ]),
  ]),
  "jujian_yanwu": ([
    "sect": "jujian_gate", "type": SECT_FACILITY_TRAINING,
    "name": "演武场",
    "desc": "巨剑门演武场，门内比剑试剑之所，银色巨剑横扫。",
    "room": "/d/yueguo/jujian/fac/yanwu",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_TRAIN, "base": 10, "per_level": 5 ]),
    "duration": 3600, "use_stone": 5, "use_contrib": 50,
    "upgrade": SECT_UPGRADE_COMMON, "daily_limit": 0,
    "daily_reward": ([ "exp": 500, "contrib": 20, "verb": "切磋" ]),
  ]),

  // ================= 鬼灵门 =================
  "guiling_lianshifang": ([
    "sect": "guiling_sect", "type": SECT_FACILITY_SPECIAL,
    "name": "炼尸房",
    "desc": "鬼灵门炼尸地，尸傀炼制场，鬼道核心，阴气森森。",
    "room": "/d/tianluo/guiling/fac/lianshifang",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_GHOST, "base": 10, "per_level": 5 ]),
    "duration": 7200, "use_stone": 20, "use_contrib": 200,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
    "daily_reward": ([ "exp": 600, "contrib": 30, "verb": "驭鬼练习" ]),
  ]),

  // ================= 御灵宗 =================
  "yuling_wanshouyuan": ([
    "sect": "yuling_sect", "type": SECT_FACILITY_SPECIAL,
    "name": "万兽园",
    "desc": "御灵宗兽苑，灵兽栖息驯养之园，虫兽双修，万兽归宗。",
    "room": "/d/tianluo/yuling/fac/wanshouyuan",
    "level": 1, "max_level": 3,
    "effect": ([ "type": SECT_BUFF_BEAST, "base": 10, "per_level": 5 ]),
    "duration": 7200, "use_stone": 20, "use_contrib": 200,
    "upgrade": SECT_UPGRADE_SPECIAL, "daily_limit": 0,
    "daily_reward": ([ "exp": 600, "contrib": 30, "verb": "驯兽" ]),
  ]),
]);

// -------- 灵田种子配置 --------
// seed_id -> ([ name/id/unit/growth/yield/cost/value/desc ])
// growth 对齐 mansion.h GROWTH_BASE_*；cost 为种子灵石价（另加种植维护费）
nosave mapping seed_config = ([
  "lingcao": ([
    "name": "灵草", "id": "lingcao", "unit": "株",
    "growth": GROWTH_BASE_SHORT, "yield": 2, "cost": 10, "value": 200,
    "desc": "一株灵气氤氲的灵草，炼丹炼药的基础材料。",
  ]),
  "huanglongcao": ([
    "name": "黄龙草", "id": "huanglongcao", "unit": "株",
    "growth": GROWTH_BASE_MEDIUM, "yield": 3, "cost": 30, "value": 600,
    "desc": "黄枫谷特产灵草，叶如黄龙鳞甲，药性温和，可炼灵药。",
  ]),
  "zidanshen": ([
    "name": "紫丹参", "id": "zidanshen", "unit": "株",
    "growth": GROWTH_BASE_LONG, "yield": 4, "cost": 100, "value": 2000,
    "desc": "紫叶丹心之参，数百年药龄，炼制筑基丹的主药。",
  ]),
]);

void create()
{
    seteuid(getuid());
    set("name", "门派设施系统");
    set("id", "sect_facility_d");
}

// ======== 查询接口 ========

string *query_facility_keys(string sect_id)
{
    string *result = ({});
    string key;

    foreach (key in keys(facility_config))
        if (facility_config[key]["sect"] == sect_id)
            result += ({ key });
    return result;
}

mapping query_facility_config(string key)
{
    if (!mapp(facility_config[key])) return 0;
    return facility_config[key];
}

mapping query_seed_config()
{
    return seed_config;
}

string *query_seed_ids()
{
    return keys(seed_config);
}

int query_facility_level(string key)
{
    mapping cfg = facility_config[key];

    if (!mapp(cfg)) return 0;
    return cfg["level"];
}

// 效果值 = base + (level-1) * per_level
int eff_value(mapping cfg)
{
    mapping eff = cfg["effect"];

    if (!mapp(eff)) return 0;
    return eff["base"] + (cfg["level"] - 1) * eff["per_level"];
}

// 玩家当前所在设施 key（不在设施房间返回 0）
string query_current_facility(object player)
{
    object env;
    string key;

    if (!objectp(player)) return 0;
    env = environment(player);
    if (!objectp(env)) return 0;

    foreach (key in keys(facility_config))
        if (base_name(env) == facility_config[key]["room"])
            return key;
    return 0;
}

// 设施是否属于玩家所在门派
int is_sect_facility(object player, string key)
{
    mapping cfg = facility_config[key];
    string sect_id;

    if (!mapp(cfg)) return 0;
    sect_id = SECT_D->query_player_sect(player);
    if (!stringp(sect_id) || sect_id != cfg["sect"]) return 0;
    return 1;
}

// 可访问检查：是本派弟子、且人在设施房间内；返回 0=可访问，字符串=拒绝原因
string check_access(object player, string key)
{
    mapping cfg;

    if (!objectp(player)) return "无效的玩家";
    cfg = facility_config[key];
    if (!mapp(cfg)) return "不存在的设施";

    if (!stringp(SECT_D->query_player_sect(player)))
        return "你尚未拜入任何门派，无法使用门派设施。";

    if (!is_sect_facility(player, key))
        return "此设施不属于你所在的门派。";

    if (query_current_facility(player) != key)
        return "请先进入「" + cfg["name"] + "」再使用。";

    return 0;
}

// ======== 效果 buff ========

void grant_buff(object player, string key, int value, int duration)
{
    mapping buffs;

    buffs = player->query(SECT_FACILITY_PATH_BUFFS);
    if (!mapp(buffs)) buffs = ([]);
    buffs[key] = ([ "expire": time() + duration, "value": value ]);
    player->set(SECT_FACILITY_PATH_BUFFS, buffs);
}

// 查询某设施 buff 效果值（过期返回 0）
int query_buff(object player, string key)
{
    mapping buffs, b;

    if (!objectp(player)) return 0;
    buffs = player->query(SECT_FACILITY_PATH_BUFFS);
    if (!mapp(buffs)) return 0;
    b = buffs[key];
    if (!mapp(b)) return 0;
    if (b["expire"] <= time())
    {
        buffs[key] = 0;
        player->set(SECT_FACILITY_PATH_BUFFS, buffs);
        return 0;
    }
    return b["value"];
}

// 按效果类型查询（供炼丹/炼器等系统接入；同一类型多设施取最大值）
int query_effect_value(object player, string buff_type)
{
    string *klist = keys(facility_config);
    int value, max;
    string k;

    max = 0;
    foreach (k in klist)
    {
        mapping cfg = facility_config[k];
        mapping eff = cfg["effect"];
        if (eff["type"] != buff_type) continue;
        value = query_buff(player, k);
        if (value > max) max = value;
    }
    return max;
}

// 炼丹成功率加成（对外接入钩子，供炼丹系统查询）
int query_danfang_bonus(object player)
{
    return query_effect_value(player, SECT_BUFF_ALCHEMY);
}

// 炼器成功率加成（对外接入钩子，供炼器系统查询）
int query_forge_bonus(object player)
{
    return query_effect_value(player, SECT_BUFF_FORGE);
}

// 修行效率加成（演武场，供打坐/修炼系统查询）
int query_training_bonus(object player)
{
    return query_effect_value(player, SECT_BUFF_TRAIN);
}

// 防御加成（护山大阵/城堡工事，供战斗减伤系统查询）
int query_defense_bonus(object player)
{
    return query_effect_value(player, SECT_BUFF_DEFENSE);
}

// ======== 每日使用计数 ========

int query_daily_count(object player, string key)
{
    mapping daily, d;
    int today;

    daily = player->query(SECT_FACILITY_PATH_DAILY);
    if (!mapp(daily)) return 0;
    d = daily[key];
    if (!mapp(d)) return 0;
    today = to_int(time() / 86400);
    if (d["day"] != today) return 0;
    return d["count"];
}

void add_daily_count(object player, string key)
{
    mapping daily, d;
    int today;

    daily = player->query(SECT_FACILITY_PATH_DAILY);
    if (!mapp(daily)) daily = ([]);
    d = daily[key];
    if (!mapp(d)) d = ([ "day": 0, "count": 0 ]);
    today = to_int(time() / 86400);
    if (d["day"] != today)
        d = ([ "day": today, "count": 1 ]);
    else
        d["count"] += 1;
    daily[key] = d;
    player->set(SECT_FACILITY_PATH_DAILY, daily);
}

// ======== 消耗结算（灵石 + 贡献，贡献走 SECT_D->add_contribution） ========
// 返回 0=成功，字符串=失败原因（先查贡献后扣灵石，避免部分扣费）
string pay_cost(object player, string key, int stone, int contrib)
{
    mapping cfg = facility_config[key];
    string reason;

    if (contrib > 0 && SECT_D->query_contribution(player) < contrib)
        return "门派贡献不足，需要 " + contrib + " 点贡献。";

    if (stone > 0 && !MONEY_D->player_pay(player, stone * 100))
        return "灵石不足，需要 " + stone + " 灵石。";

    if (contrib > 0)
    {
        reason = "使用设施：" + cfg["name"];
        SECT_D->add_contribution(player, -contrib, reason);
    }
    return 0;
}

// ======== 使用设施（buff 类） ========
// 返回 1=成功 0=失败
int use_facility(object player, string key)
{
    mapping cfg, eff;
    string err, buff_name;
    int value, dur, stone, contrib, daily, count;

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    cfg = facility_config[key];
    eff = cfg["effect"];
    if (cfg["type"] == SECT_FACILITY_PLANT || cfg["type"] == SECT_FACILITY_LIBRARY || cfg["market"])
    {
        tell_object(player, "此处设施请使用专项指令：facility plant/harvest 或 facility read/copy 或 facility buy。\n");
        return 0;
    }

    dur = cfg["duration"];
    stone = cfg["use_stone"];
    contrib = cfg["use_contrib"];
    daily = cfg["daily_limit"];

    if (daily > 0)
    {
        count = query_daily_count(player, key);
        if (count >= daily)
        {
            tell_object(player, "今日已使用过「" + cfg["name"] + "」，明日再来。\n");
            return 0;
        }
    }

    err = pay_cost(player, key, stone, contrib);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    if (daily > 0)
        add_daily_count(player, key);

    value = eff_value(cfg);

    // 天月神舟：巡游机缘 + 送达越国传送中枢（集体出行中枢）
    if (eff["type"] == SECT_BUFF_SPIRIT)
    {
        grant_buff(player, key, value, dur);
        tell_object(player, HIG "你登上天月神舟，随舟巡游越国诸峰，机缘 +" + value + "%（" +
                    (dur / 60) + " 分钟）。\n" NOR);
        tell_object(player, "神舟将你送至越国传送中枢。\n");
        player->move("/d/yueguo/transmit");
        return 1;
    }

    grant_buff(player, key, value, dur);
    buff_name = SECT_BUFF_NAME[eff["type"]];
    if (!stringp(buff_name)) buff_name = "效果";
    tell_object(player, HIG "你在「" + cfg["name"] + "」中修行一番，获得" + buff_name +
                " +" + value + "% 加成（持续 " + (dur / 60) + " 分钟）。\n" NOR);
    return 1;
}

// ======== 灵田种植/收获 ========

// 当前灵田地块数（随等级：Lv1=2, Lv2=5, Lv3=8，对齐 LAND_MAX_PLOTS）
int query_plot_count(string key)
{
    mapping cfg = facility_config[key];

    if (!mapp(cfg)) return 0;
    return SECT_PLANT_PLOTS_BASE + (cfg["level"] - 1) * SECT_PLANT_PLOTS_PER_LEVEL;
}

mapping query_player_plots(object player, string key)
{
    mapping all_plots, plots;

    all_plots = player->query(SECT_FACILITY_PATH_PLOTS);
    if (!mapp(all_plots)) return ([]);
    plots = all_plots[key];
    if (!mapp(plots)) return ([]);
    return plots;
}

void save_plots(object player, string key, mapping plots)
{
    mapping all_plots;

    all_plots = player->query(SECT_FACILITY_PATH_PLOTS);
    if (!mapp(all_plots)) all_plots = ([]);
    all_plots[key] = plots;
    player->set(SECT_FACILITY_PATH_PLOTS, all_plots);
}

string format_remaining(int sec)
{
    int h, m;

    if (sec < 0) sec = 0;
    h = sec / 3600;
    m = (sec % 3600) / 60;
    if (h > 0) return sprintf("%d小时%d分", h, m);
    if (m > 0) return sprintf("%d分", m);
    return sprintf("%d秒", sec);
}

string query_plot_state(object player, string key, int idx)
{
    mapping plots, plot;
    int remain, total;

    plots = query_player_plots(player, key);
    plot = plots[idx];
    if (!mapp(plot)) return "空闲";
    if (plot["status"] == PLOT_MATURE)
        return "可收获";
    if (plot["status"] == PLOT_FALLOW)
        return "休整中";
    remain = plot["mature"] - time();
    if (remain < 0) remain = 0;
    total = plot["mature"] - plot["planted"];
    if (total < 1) total = 1;
    return "生长中（" + plot["seed"] + "，还需 " + format_remaining(remain) + "）";
}

string describe_plots(object player, string key)
{
    mapping plots;
    string output = "";
    int i, total;

    plots = query_player_plots(player, key);
    total = query_plot_count(key);
    for (i = 0; i < total; i++)
        output += sprintf("  第 %d 号地块：%s\n", i + 1, query_plot_state(player, key, i));
    return output;
}

// 播种；返回 1=成功 0=失败
int plant(object player, string key, string seed_id)
{
    mapping cfg, plots, plot, sc;
    string err;
    int i, total, found, cost, speed_factor, growth;

    cfg = facility_config[key];
    if (!mapp(cfg) || cfg["type"] != SECT_FACILITY_PLANT)
    {
        tell_object(player, "此处不是灵田，无法种植。\n");
        return 0;
    }

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    sc = seed_config[seed_id];
    if (!mapp(sc))
    {
        tell_object(player, "没有这种灵种。输入 facility list 查看可种灵种。\n");
        return 0;
    }

    total = query_plot_count(key);
    plots = query_player_plots(player, key);
    found = -1;
    for (i = 0; i < total; i++)
    {
        plot = plots[i];
        if (!mapp(plot) || plot["status"] == PLOT_EMPTY || plot["status"] == PLOT_FALLOW)
        {
            found = i;
            break;
        }
    }
    if (found == -1)
    {
        tell_object(player, "灵田已满，没有空闲地块。\n");
        return 0;
    }

    // 灵石消耗 = 种子价 + 种植维护费（对齐 GARDEN_MAINTENANCE）
    cost = sc["cost"] + SECT_PLANT_MAINTENANCE;
    if (!MONEY_D->player_pay(player, cost * 100))
    {
        tell_object(player, "灵石不足，种植需要 " + cost + " 灵石。\n");
        return 0;
    }

    // 生长时间随灵田等级缩短（Lv1=1.0, Lv2=1.25, Lv3=1.5，对齐 LAND_SPEED）
    speed_factor = 100 + (cfg["level"] - 1) * 25;
    growth = to_int(to_float(sc["growth"]) * 100.0 / to_float(speed_factor));

    if (!mapp(plots)) plots = ([]);
    plots[found] = ([
        "status" : PLOT_GROWING,
        "seed"   : sc["name"],
        "seed_id": seed_id,
        "planted": time(),
        "mature" : time() + growth,
        "yield"  : 1,
    ]);
    save_plots(player, key, plots);

    tell_object(player, HIG "你在第 " + (found + 1) + " 号地块种下「" + sc["name"] +
                "」，消耗 " + cost + " 灵石，预计 " + format_remaining(growth) + " 后成熟。\n" NOR);
    return 1;
}

// 收获；返回收获数量，失败返回 -1
int harvest(object player, string key, int idx)
{
    mapping cfg, plots, plot, sc;
    string err;
    int bonus, base_yield, total, count, i;

    cfg = facility_config[key];
    if (!mapp(cfg) || cfg["type"] != SECT_FACILITY_PLANT)
    {
        tell_object(player, "此处不是灵田。\n");
        return -1;
    }

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return -1;
    }

    plots = query_player_plots(player, key);
    plot = plots[idx];
    if (!mapp(plot))
    {
        tell_object(player, "该地块没有种植作物。\n");
        return -1;
    }

    if (plot["status"] != PLOT_GROWING && plot["status"] != PLOT_MATURE)
    {
        tell_object(player, "该地块当前没有可收获的作物。\n");
        return -1;
    }

    if (plot["status"] == PLOT_GROWING && time() < plot["mature"])
    {
        tell_object(player, "作物尚未成熟。\n");
        return -1;
    }

    sc = seed_config[plot["seed_id"]];
    if (!mapp(sc))
    {
        tell_object(player, "未知作物。\n");
        return -1;
    }

    // 产量 = 基础 × (1 + 药园效率加成/100) + 随机波动
    bonus = eff_value(cfg);
    base_yield = sc["yield"];
    total = base_yield + to_int(to_float(base_yield) * to_float(bonus) / 100.0);
    count = total > 0 ? total : 1;
    if (random(100) < 30) count += 1;

    for (i = 0; i < count; i++)
    {
        object herb = new("/d/yueguo/obj/spirit_herb");
        if (!objectp(herb)) continue;
        herb->setup_herb(sc["name"], sc["id"], sc["unit"], sc["value"], sc["desc"]);
        if (!herb->move(player))
            herb->move(environment(player));
    }

    plots[idx] = 0;
    save_plots(player, key, plots);

    tell_object(player, HIG "你收获了「" + sc["name"] + "」× " + count + "！\n" NOR);
    return count;
}

// ======== 藏经阁：阅读/抄录 ========

// 阅读本门功法（免费）
int read_skill(object player, string key, string skill_id)
{
    mapping cfg, info;
    string err, sect_id, rank_name;
    string *ranks;
    int r;

    cfg = facility_config[key];
    if (!mapp(cfg) || cfg["type"] != SECT_FACILITY_LIBRARY)
    {
        tell_object(player, "此处不是藏经阁。\n");
        return 0;
    }

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    sect_id = SECT_D->query_player_sect(player);
    info = SECT_D->query_sect_skill_info(sect_id, skill_id);
    if (!mapp(info))
    {
        tell_object(player, "本门无此功法。输入 facility list 查看本门功法。\n");
        return 0;
    }

    ranks = SECT_D->query_sect_ranks(sect_id);
    r = info["rank"];
    rank_name = (r >= 0 && r < sizeof(ranks)) ? ranks[r] : "未知";

    tell_object(player, sprintf(HIW "「%s」\n" NOR +
                "阶位要求：%s\n" +
                "研习消耗：门派贡献 %d\n" +
                "%s\n",
                info["name"], rank_name, info["cost"], info["desc"]));
    return 1;
}

// 抄录功法（扣除贡献写入门派已学，复用 sect/learned 结构，藏经阁等级可降耗）
int transcribe_skill(object player, string key, string skill_id)
{
    mapping cfg, info, learned;
    string err, sect_id;
    int cost, reduce;

    cfg = facility_config[key];
    if (!mapp(cfg) || cfg["type"] != SECT_FACILITY_LIBRARY)
    {
        tell_object(player, "此处不是藏经阁。\n");
        return 0;
    }

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    sect_id = SECT_D->query_player_sect(player);
    info = SECT_D->query_sect_skill_info(sect_id, skill_id);
    if (!mapp(info))
    {
        tell_object(player, "本门无此功法。\n");
        return 0;
    }

    learned = player->query(SECT_PATH_LEARNED);
    if (mapp(learned) && learned[skill_id])
    {
        tell_object(player, "你已学过这门功法。\n");
        return 0;
    }

    // 藏经阁等级降低抄录消耗（每级 -10%）
    reduce = eff_value(cfg);
    cost = info["cost"];
    if (reduce > 0)
        cost = to_int(to_float(cost) * (100.0 - to_float(reduce)) / 100.0);

    if (SECT_D->query_contribution(player) < cost)
    {
        tell_object(player, sprintf("贡献不足：抄录「%s」需门派贡献 %d（当前 %d）。\n",
                    info["name"], cost, SECT_D->query_contribution(player)));
        return 0;
    }

    SECT_D->add_contribution(player, -cost, "藏经阁抄录：" + info["name"]);
    if (!mapp(learned)) learned = ([]);
    learned[skill_id] = time();
    player->set(SECT_PATH_LEARNED, learned);

    tell_object(player, HIG "你耗门派贡献 " + cost + "，抄录「" + info["name"] + "」入藏。\n" NOR);
    return 1;
}

// ======== 坊市：购买灵材 ========

int market_buy(object player, string key, string good_id, int amount)
{
    mapping cfg, sc;
    string err;
    int i, unit_price, discount, price;

    cfg = facility_config[key];
    if (!mapp(cfg) || !cfg["market"])
    {
        tell_object(player, "此处不是坊市。\n");
        return 0;
    }

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    sc = seed_config[good_id];
    if (!mapp(sc))
    {
        tell_object(player, "坊市没有这种货物。输入 facility list 查看货物。\n");
        return 0;
    }

    if (amount < 1) amount = 1;
    if (amount > 10) amount = 10;

    // 坊市升级折扣（每级 -5%）
    discount = eff_value(cfg);
    unit_price = sc["cost"];
    if (discount > 0)
        unit_price = to_int(to_float(unit_price) * (100.0 - to_float(discount)) / 100.0);

    price = unit_price * amount;
    if (!MONEY_D->player_pay(player, price * 100))
    {
        tell_object(player, "灵石不足，需要 " + price + " 灵石。\n");
        return 0;
    }

    for (i = 0; i < amount; i++)
    {
        object herb = new("/d/yueguo/obj/spirit_herb");
        if (!objectp(herb)) continue;
        herb->setup_herb(sc["name"], sc["id"], sc["unit"], sc["value"], sc["desc"]);
        if (!herb->move(player))
            herb->move(environment(player));
    }

    tell_object(player, HIG "你在坊市购得「" + sc["name"] + "」× " + amount +
                "，共 " + price + " 灵石。\n" NOR);
    return 1;
}

// ======== 每日修行/切磋奖励 ========

int practice(object player, string key)
{
    mapping cfg, reward;
    string err;
    int count, exp_gain, contrib_gain;

    cfg = facility_config[key];
    if (!mapp(cfg) || !mapp(cfg["daily_reward"]))
    {
        tell_object(player, "此处没有可进行的修行。\n");
        return 0;
    }

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    count = query_daily_count(player, key);
    if (count >= 1)
    {
        tell_object(player, "今日已在此修行过，明日再来。\n");
        return 0;
    }

    reward = cfg["daily_reward"];
    exp_gain = reward["exp"] + (cfg["level"] - 1) * SECT_SPAR_EXP_PER_LEVEL;
    contrib_gain = reward["contrib"];

    player->add("combat_exp", exp_gain);
    if (contrib_gain > 0)
        SECT_D->add_contribution(player, contrib_gain, "设施修行：" + cfg["name"]);

    add_daily_count(player, key);

    tell_object(player, HIG "你在「" + cfg["name"] + "」" + reward["verb"] +
                "一番，获得经验 " + exp_gain + "、门派贡献 +" + contrib_gain + "。\n" NOR);
    return 1;
}

// ======== 升级设施（门派共享，弟子捐献） ========

int upgrade_facility(object player, string key)
{
    mapping cfg, upgrade;
    string err;
    int lv, next, stone, contrib, rank;

    err = check_access(player, key);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    cfg = facility_config[key];
    lv = cfg["level"];
    if (lv >= cfg["max_level"])
    {
        tell_object(player, "「" + cfg["name"] + "」已升至最高等级。\n");
        return 0;
    }

    upgrade = cfg["upgrade"];
    next = lv + 1;
    if (!mapp(upgrade) || !arrayp(upgrade[next]))
    {
        tell_object(player, "该设施当前等级无法继续升级。\n");
        return 0;
    }

    stone = upgrade[next][0];
    contrib = upgrade[next][1];

    // 需内门弟子以上方可主持升级
    rank = SECT_D->query_rank(player);
    if (rank < 1)
    {
        tell_object(player, "只有内门弟子以上方可主持门派设施的升级。\n");
        return 0;
    }

    err = pay_cost(player, key, stone, contrib);
    if (stringp(err))
    {
        tell_object(player, err + "\n");
        return 0;
    }

    cfg["level"] = next;
    tell_object(player, HIG "你捐献灵石 " + stone + "、门派贡献 " + contrib +
                "，「" + cfg["name"] + "」升至 " + next + " 级！\n" NOR);
    return 1;
}

// ======== 状态展示 ========

string describe_facility(string key)
{
    mapping cfg, eff;
    string output;

    cfg = facility_config[key];
    if (!mapp(cfg)) return "未知设施";

    eff = cfg["effect"];
    output = sprintf("%s（%s，%d 级/%d 级）\n  %s\n",
                     cfg["name"], SECT_FACILITY_TYPE_NAME[cfg["type"]],
                     cfg["level"], cfg["max_level"], cfg["desc"]);

    if (cfg["type"] == SECT_FACILITY_PLANT)
    {
        output += sprintf("  灵田：%d 块地块，种植需灵石（种子价+%d），收获加成 +%d%%\n",
                          query_plot_count(key), SECT_PLANT_MAINTENANCE, eff_value(cfg));
    }
    else if (cfg["type"] == SECT_FACILITY_LIBRARY)
    {
        output += "  阅读免费（facility read <功法>）；抄录耗贡献（facility copy <功法>），升级每级抄录消耗 -" +
                  eff["per_level"] + "%\n";
    }
    else if (cfg["market"])
    {
        output += sprintf("  出售灵材：灵草/黄龙草/紫丹参（facility buy <货物>）；升级每级降价 %d%%\n",
                          eff["per_level"]);
    }
    else if (eff["type"] == SECT_BUFF_DEFENSE)
    {
        output += sprintf("  使用消耗：%d 灵石 + %d 贡献，效果：防御 +%d%%（+%d/级），持续 %d 分钟\n",
                          cfg["use_stone"], cfg["use_contrib"],
                          eff["base"], eff["per_level"], cfg["duration"] / 60);
    }
    else
    {
        output += sprintf("  使用消耗：%d 灵石 + %d 贡献，效果：%s +%d%%（+%d/级），持续 %d 分钟\n",
                          cfg["use_stone"], cfg["use_contrib"],
                          stringp(SECT_BUFF_NAME[eff["type"]]) ? SECT_BUFF_NAME[eff["type"]] : "效果",
                          eff["base"], eff["per_level"], cfg["duration"] / 60);
    }

    if (mapp(cfg["daily_reward"]))
        output += sprintf("  每日%s：经验 +%d，门派贡献 +%d（每日 1 次，facility practice）\n",
                          cfg["daily_reward"]["verb"], cfg["daily_reward"]["exp"],
                          cfg["daily_reward"]["contrib"]);

    if (cfg["level"] < cfg["max_level"] && mapp(cfg["upgrade"]) && arrayp(cfg["upgrade"][cfg["level"] + 1]))
    {
        int n = cfg["level"] + 1;
        output += sprintf("  升级（%d→%d）：%d 灵石 + %d 贡献（内门弟子以上可主持）\n",
                          cfg["level"], n, cfg["upgrade"][n][0], cfg["upgrade"][n][1]);
    }
    else
        output += "  已升至最高等级\n";

    return output;
}

// 灵种清单（灵田可种，坊市可购）
string describe_seeds()
{
    string output = "";
    string sid;

    foreach (sid in keys(seed_config))
    {
        mapping sc = seed_config[sid];
        output += sprintf("  %-8s 生长 %s，基础产量 %d，价 %d 灵石%s\n",
                          sc["name"], format_remaining(sc["growth"]), sc["yield"],
                          sc["cost"], sid == "zidanshen" ? "（筑基丹主药）" : "");
    }
    return output;
}
