// pill_d.c
// 丹药炼制守护进程（#73：#64 子单 丹药炼制链路，P5-4 法宝丹药炼制系统 1E 落地）
//
// 职责：
//   1. 丹方数据（药材组合/基准成功率/所需炼丹术等级/基准品级）
//   2. 成功率计算：基准 ×（1 + 炼丹术等级×0.02 + 丹房加成）− 品级难度罚值
//      （02-丹药体系详解 §4.3；丹房加成接 #60 SECT_FACILITY_D->query_danfang_bonus）
//   3. 品质判定：炼丹术等级越高，产出高品级概率越大
//   4. 炼丹术等级（玩家 DBASE pill_refine_exp → 等级换算，02 §4.4 分段）
//   5. 炼制执行：校验材料→扣除→概率判定→成丹/失败
//
// 炼丹命令：cmds/usr/liandan.c
// 药材来源：#67 坊市（d/yueguo/tainan/obj/lingcao.c 灵草 / huanglongcao.c 黄龙草，
//           material_id 对齐丹方 ingredients 键）
// 成品丹药：clone/pill/*.c（继承 DAN_BASE，1E §4.1 六属性）

#include <ansi.h>
#include <globals.h>
#include <pill.h>
#include <spirit_root.h>

inherit F_DBASE;

// 丹方数据：
//   ([ id: ([
//       "name"        : 丹方名（中文）
//       "pill"        : 成品丹药对象路径
//       "ingredients" : ([ 材料id: 数量 ])，材料id 对齐坊市 material_id
//       "base_rate"   : 基准成功率（0-100）
//       "refine_level": 所需炼丹术等级
//       "stage"       : 对应境界阶段（REALM_*，用于展示）
//       "quality"     : 基准品级（PILL_QUALITY_*）
//   ]) ])
nosave mapping danfang = ([
    // ================= 炼气期 =================
    "lianqisan" : ([
        "name" : "炼气散",
        "pill" : "/clone/pill/lianqisan",
        "ingredients" : ([ "lingcao" : 2 ]),
        "base_rate" : 70,
        "refine_level" : 1,
        "stage" : REALM_QI_REFINERY,
        "quality" : PILL_QUALITY_FAN,
    ]),
    "huanglongdan" : ([
        "name" : "黄龙丹",
        "pill" : "/clone/pill/huanglongdan",
        "ingredients" : ([ "huanglongcao" : 2 ]),
        "base_rate" : 60,
        "refine_level" : 3,
        "stage" : REALM_QI_REFINERY,
        "quality" : PILL_QUALITY_FAN,
    ]),
    "juqi" : ([
        "name" : "聚气丹",
        "pill" : "/clone/pill/juqi",
        "ingredients" : ([ "lingcao" : 3 ]),
        "base_rate" : 75,
        "refine_level" : 2,
        "stage" : REALM_QI_REFINERY,
        "quality" : PILL_QUALITY_FAN,
    ]),
    // ================= 筑基期 =================
    "zhuji" : ([
        "name" : "筑基丹",
        "pill" : "/clone/pill/zhuji",
        "ingredients" : ([ "huanglongcao" : 3, "lingcao" : 5 ]),
        "base_rate" : 40,
        "refine_level" : 8,
        "stage" : REALM_FOUNDATION,
        "quality" : PILL_QUALITY_LIANG,
    ]),
    "ningdan" : ([
        "name" : "凝丹丸",
        "pill" : "/clone/pill/ningdan",
        "ingredients" : ([ "huanglongcao" : 4, "lingcao" : 8 ]),
        "base_rate" : 45,
        "refine_level" : 12,
        "stage" : REALM_FOUNDATION,
        "quality" : PILL_QUALITY_LIANG,
    ]),
    // ================= 结丹期 =================
    "jiejindan" : ([
        "name" : "结金丹",
        "pill" : "/clone/pill/jiejindan",
        "ingredients" : ([ "huanglongcao" : 6, "lingcao" : 12 ]),
        "base_rate" : 30,
        "refine_level" : 18,
        "stage" : REALM_CORE_FORMATION,
        "quality" : PILL_QUALITY_SHANG,
    ]),
]);

void create()
{
    seteuid(getuid());

    // 成品丹药注册到经济系统（单位：下品灵石 = value/100 文），
    // 使 refine_pill 的 record_sale 产出记账真实生效（幂等注册）
    if (find_object(ECONOMY_D))
    {
        string *ids = keys(danfang);
        int i;
        for (i = 0; i < sizeof(ids); i++)
            ECONOMY_D->register_goods("pill_" + ids[i],
                    query_pill_value(ids[i]), 50);
    }
}

// 丹药成品的灵石基准价（value 文 / 100）
int query_pill_value(string id)
{
    mapping df = danfang[id];
    if (!mapp(df)) return 1;
    return load_object(df["pill"])->query("value") / 100;
}

// ============ 丹方查询 ============

mapping query_danfang(string id)
{
    return danfang[id];
}

// 中文名 → 丹方 id（命令层支持中文输入）
string query_danfang_id(string name)
{
    string *ids;
    int i;

    ids = keys(danfang);
    for (i = 0; i < sizeof(ids); i++)
        if (danfang[ids[i]]["name"] == name)
            return ids[i];
    return 0;
}

string *query_danfang_ids()
{
    return keys(danfang);
}

// ============ 炼丹术等级（02 §4.4 分段升级）============

// 各级所需炼制次数：1-20 级每级 5 次，21-40 每级 8 次，41-60 每级 12 次，
// 61-80 每级 20 次，81-100 每级 50 次
int refine_need(int level)
{
    if (level <= 20) return 5;
    if (level <= 40) return 8;
    if (level <= 60) return 12;
    if (level <= 80) return 20;
    return 50;
}

// 从累计经验换算炼丹术等级（exp = 成功炼制次数）
int query_refine_level(object player)
{
    int exp, level, need;

    if (!objectp(player)) return 0;
    exp = player->query(PILL_REFINE_EXP);
    level = 1;
    while (level < 100)
    {
        need = refine_need(level);
        if (exp < need) break;
        exp -= need;
        level++;
    }
    return level;
}

// 升级提示（返回是否升了级）
int add_refine_exp(object player, int n)
{
    int before, after;

    before = query_refine_level(player);
    player->add(PILL_REFINE_EXP, n);
    after = query_refine_level(player);
    if (after > before)
    {
        tell_object(player, HIC "你对丹道的领悟加深了！炼丹术提升至 " + after + " 级！\n" NOR);
        return 1;
    }
    return 0;
}

// ============ 成功率（02 §4.3 简化版）============

// 最终成功率 = 基准 ×（1 + 炼丹术×0.02 + 丹房加成）− 品级难度罚值
// 简化实现（无丹炉/火候/材料品质维度）：直接累加，钳制 [5,95]
int query_success_rate(object player, string id)
{
    mapping df;
    int rate, refine, danfang_bonus, penalty;

    df = danfang[id];
    if (!mapp(df)) return 0;

    rate = df["base_rate"];
    refine = query_refine_level(player);
    rate += to_int(refine * 2);          // 炼丹术每级 +2%

    danfang_bonus = 0;
    if (find_object(SECT_FACILITY_D))
        danfang_bonus = SECT_FACILITY_D->query_danfang_bonus(player);
    rate += danfang_bonus;               // 丹房加成（#60 钩子，激活丹房 buff 时生效）

    penalty = (df["quality"] - PILL_QUALITY_FAN) * 10;  // 品级每高一品 −10%
    rate -= penalty;

    if (rate < 5) rate = 5;
    if (rate > 95) rate = 95;
    return rate;
}

// ============ 品质判定 ============

// 基础品质来自丹方；炼丹术等级提高后有小概率产出高品级
int roll_quality(object player, string id)
{
    mapping df;
    int quality, refine, roll;

    df = danfang[id];
    if (!mapp(df)) return PILL_QUALITY_FAN;

    quality = df["quality"];
    refine = query_refine_level(player);

    // 炼丹术 ≥15：20% 概率品质 +1；≥30：额外 20% 概率再 +1（上限上品）
    if (quality < PILL_QUALITY_SHANG && refine >= 15)
    {
        roll = random(100);
        if (roll < 20)
        {
            quality++;
            if (quality < PILL_QUALITY_SHANG && refine >= 30 && random(100) < 20)
                quality++;
        }
    }
    if (quality > PILL_QUALITY_SHANG) quality = PILL_QUALITY_SHANG;
    return quality;
}

// ============ 炼制执行 ============

// 校验并扣除材料（从玩家身上按 material_id 找材料对象）
// 返回：0=材料不足（已提示） 1=材料扣除完成
int consume_ingredients(object player, string id)
{
    mapping ing, have;
    string *mats;
    object *inv;
    int i, j, need, count;

    ing = danfang[id]["ingredients"];
    if (!mapp(ing)) return 1;

    have = ([]);
    mats = keys(ing);
    inv = all_inventory(player);
    for (i = 0; i < sizeof(mats); i++)
    {
        need = ing[mats[i]];
        count = 0;
        for (j = 0; j < sizeof(inv); j++)
        {
            if (inv[j]->query("material_id") == mats[i])
                count++;
            if (count >= need) break;
        }
        have[mats[i]] = count;
        if (count < need)
        {
            tell_object(player, sprintf("材料不足：%s 还缺 %d 份（已备 %d 份）。\n",
                        mats[i], need - count, count));
            return 0;
        }
    }

    // 扣除
    for (i = 0; i < sizeof(mats); i++)
    {
        need = ing[mats[i]];
        count = 0;
        inv = all_inventory(player);
        for (j = 0; j < sizeof(inv); j++)
        {
            if (count >= need) break;
            if (inv[j]->query("material_id") == mats[i])
            {
                destruct(inv[j]);
                count++;
            }
        }
    }
    return 1;
}

// 炼制（返回：1=成丹 0=失败/条件不满足）
// 调用方：cmds/usr/liandan.c
int refine_pill(object player, string id)
{
    mapping df;
    object pill;
    int rate, quality, roll;

    if (!objectp(player)) return 0;
    df = danfang[id];
    if (!mapp(df))
    {
        tell_object(player, "没有这种丹方。\n");
        return 0;
    }

    // 炼丹术等级门槛
    if (query_refine_level(player) < df["refine_level"])
    {
        tell_object(player, sprintf("你炼丹术等级不足：炼制%s需炼丹术 %d 级（当前 %d 级）。\n",
                    df["name"], df["refine_level"], query_refine_level(player)));
        return 0;
    }
    if (!consume_ingredients(player, id))
        return 0;

    rate = query_success_rate(player, id);
    tell_object(player, sprintf(HIC "你生火起炉，按「%s」丹方投放药材，凝神控火……成功率约 %d%%\n" NOR,
                df["name"], rate));

    roll = random(100);
    if (roll >= rate)
    {
        message_vision(HIR "$N开炉一看，炉中焦黑一片——炼废了。\n" NOR, player);
        return 0;
    }

    // 成丹
    quality = roll_quality(player, id);
    pill = new(df["pill"]);
    if (!objectp(pill))
    {
        tell_object(player, "丹方成品缺失，炼制未能产出丹药（请联系管理员）。\n");
        return 0;
    }
    pill->set("quality", quality);
    pill->move(player);

    message_vision(HIG "$N开炉取丹，一枚" + pill->name() + HIG "静静躺在炉底，药香扑鼻！\n" NOR, player);

    // 炼丹术经验
    add_refine_exp(player, 1);

    // 经济记账：成品产出（单位灵石 = value/100，走 ECONOMY_D 产出监控）
    if (find_object(ECONOMY_D) && find_object(ECONOMY_BRIDGE_D))
        ECONOMY_D->record_sale("pill_" + id, pill->query("value") / 100,
                ECONOMY_BRIDGE_D->get_player_realm_code(player));

    return 1;
}
