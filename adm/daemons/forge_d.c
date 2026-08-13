// adm/daemons/forge_d.c
// 炼器守护进程（FORGE_D）—— 法宝炼制链路
// 设计文档: .knowledge/items/1E-法宝丹药经济.md §1.5（炼器流程/材料表/五步）
//          .knowledge/02-扩充内容/02-法宝与武器图鉴.md §4（炼制条件/进阶）
// 依赖:
//   SECT_D->query_cultivation_tier      —— 境界判定（#61 修炼系统）
//   SECT_FACILITY_D->query_forge_bonus  —— 炼器工坊设施加成（#60 门派设施系统）
//   MONEY_D                             —— 灵石结算（材料购置走坊市）
// 流程（1E §1.5）：材料采集 → 精炼提纯 → 器胚锻造 → 禁制铭刻 → 通灵开光
//   材料采集为确定性步骤（背包材料齐备即通过）；工坊内四步（FORGE_STEPS）
//   每步独立判定，步概率 = 综合成功率^(1/4)，合成概率即综合成功率。
// Created for ticket #74

#include <ansi.h>
#include <forge.h>
#include <sect.h>
#include <globals.h>

inherit F_DBASE;

// -------- 配方配置 --------
// formula_id -> ([
//   "name"          : 法宝名（炼成后显示名 = name +（品质））,
//   "desc"          : 配方描述,
//   "treasure_type" : 法器/法宝（1E §1.1）,
//   "element"       : 属性（1E §1.3）,
//   "forbidden_count": 禁制基数（品质越高加成越多，FORGE_QUALITY_BAN）,
//   "attack"/"defense": 基础攻防（品质倍率乘算，FORGE_QUALITY_RATE）,
//   "special"       : 特殊效果描述,
//   "require_tier"  : 炼制与使用所需境界 tier（SECT_TIER_*，1E §1.1 适用境界）,
//   "materials"     : ([ 材料id: 数量 ])（材料对象 material_id 对齐）,
//   "min_skill"     : 炼器术等级要求（02 图鉴 §4.3：法器≥10、法宝≥50）,
//   "base_rate"     : 基准成功率 %（02 图鉴 §4.3 档位）,
//   "value"         : 法宝价值（文，1 灵石 = 100 文，交易与经济接线用）,
// ])
nosave mapping forge_formula = ([
  "qinggangjian": ([
    "name": "青钢剑",
    "desc": "以铁精为胚、银精淬锋的制式法器飞剑，金行锐气内蕴，炼气期修士即可御使。",
    "treasure_type": "法器",
    "element": "金",
    "forbidden_count": 2,
    "attack": 20,
    "defense": 0,
    "special": "锋锐：剑芒携带金行锐气",
    "require_tier": SECT_TIER_QI_LATE,
    "materials": ([ "tiejing": 2, "yinjing": 1 ]),
    "min_skill": 10,
    "base_rate": 75,
    "value": 1500,
  ]),
  "chitongdun": ([
    "name": "赤铜盾",
    "desc": "铁精为骨、金精为面熔铸而成的护身法器，土行灵光可化盾护体。",
    "treasure_type": "法器",
    "element": "土",
    "forbidden_count": 3,
    "attack": 0,
    "defense": 25,
    "special": "护御：土行灵光护体",
    "require_tier": SECT_TIER_QI_LATE,
    "materials": ([ "tiejing": 3, "jinjing": 1 ]),
    "min_skill": 20,
    "base_rate": 70,
    "value": 3200,
  ]),
  "xuantiezhongjian": ([
    "name": "玄铁重剑",
    "desc": "以金精为骨、玄铁为刃的重型法宝飞剑，势大力沉，筑基期修士方能御使。",
    "treasure_type": "法宝",
    "element": "金",
    "forbidden_count": 5,
    "attack": 45,
    "defense": 0,
    "special": "重压：剑势沉重，势大力沉",
    "require_tier": SECT_TIER_ZHU,
    "materials": ([ "jinjing": 2, "xuantie": 1 ]),
    "min_skill": 50,
    "base_rate": 60,
    "value": 9000,
  ]),
  "gengjingfeijian": ([
    "name": "庚精飞剑",
    "desc": "以玄铁为胎、庚精为刃的极品法宝飞剑，锋锐无匹，结丹期修士方可御使。",
    "treasure_type": "法宝",
    "element": "金",
    "forbidden_count": 7,
    "attack": 70,
    "defense": 0,
    "special": "庚金锋锐：可破甲伤敌",
    "require_tier": SECT_TIER_JIE,
    "materials": ([ "xuantie": 2, "gengjing": 1 ]),
    "min_skill": 60,
    "base_rate": 50,
    "value": 20000,
  ]),
]);

// 配方 id → 中文名解析（命令面双向解析，对齐 #60 resolve_seed_id 先例）
string resolve_formula(string arg)
{
    string *ids;
    string id;
    int i;

    if (!stringp(arg) || arg == "") return 0;
    ids = keys(forge_formula);
    for (i = 0; i < sizeof(ids); i++)
        if (ids[i] == arg || forge_formula[ids[i]]["name"] == arg)
            return ids[i];
    return 0;
}

// -------- 配方查询（供命令层/验证用） --------

string *query_formula_ids()
{
    return keys(forge_formula);
}

mapping query_formula(string id)
{
    return forge_formula[id];
}

// 配方列表文本（lianqi list）
string describe_formula_list()
{
    string *ids;
    string msg;
    string *mkeys;
    int i, j;

    ids = keys(forge_formula);
    msg = HIC "化刀坞炼器工坊可炼制的法宝配方：\n" NOR;
    for (i = 0; i < sizeof(ids); i++)
    {
        mapping f = forge_formula[ids[i]];
        string need = "";

        mkeys = keys(f["materials"]);
        for (j = 0; j < sizeof(mkeys); j++)
            need += (j ? "、" : "") + mkeys[j] + "×" + f["materials"][mkeys[j]];
        msg += sprintf("  %-16s %s（%s·%s）\n",
            ids[i], f["name"], f["treasure_type"], f["element"]);
        msg += "      材料：" + need + "｜炼器术" + f["min_skill"] + "级｜境界" +
               SECT_D->tier_name(f["require_tier"]) + "｜成功率约" + f["base_rate"] + "%\n";
    }
    msg += "用法：lianqi <配方id>（如 lianqi qinggangjian）\n";
    return msg;
}

// -------- 材料检查/扣减（背包中按 material_id 计数，材料不堆叠、每对象一份） --------

int count_material(object player, string mid)
{
    object *inv;
    int i, c;

    if (!objectp(player)) return 0;
    inv = all_inventory(player);
    c = 0;
    for (i = 0; i < sizeof(inv); i++)
        if (inv[i]->id(mid) && inv[i]->query("is_material"))
            c++;
    return c;
}

void consume_material(object player, string mid, int n)
{
    object *inv;
    int i, left;

    inv = all_inventory(player);
    left = n;
    for (i = 0; i < sizeof(inv) && left > 0; i++)
        if (inv[i]->id(mid) && inv[i]->query("is_material"))
        {
            destruct(inv[i]);
            left--;
        }
}

// -------- 成功率计算 --------
// base_rate + 炼器术加成（每 2 级 +1%，100 级 +50%）+ 炼器工坊设施加成（query_forge_bonus）
int query_success_rate(object player, mapping f)
{
    int rate, skill;

    rate = f["base_rate"];
    skill = player->query_skill("lianqi-shu");
    rate += skill / 2;
    if (find_object(SECT_FACILITY_D))
        rate += SECT_FACILITY_D->query_forge_bonus(player);

    if (rate < FORGE_MIN_RATE) rate = FORGE_MIN_RATE;
    if (rate > FORGE_MAX_RATE) rate = FORGE_MAX_RATE;
    return rate;
}

// -------- 品质判定（成功率越高，越易出高品；倍率 FORGE_QUALITY_RATE） --------
string roll_quality(int rate)
{
    int roll;

    roll = random(100) + 1;
    if (roll < rate * 25 / 100) return FORGE_QUALITY_TOP;
    if (roll < rate * 50 / 100) return FORGE_QUALITY_HIGH;
    if (roll < rate * 80 / 100) return FORGE_QUALITY_MID;
    return FORGE_QUALITY_LOW;
}

// -------- 炼制主流程 --------
// 返回 1=成功（法宝已 move 给玩家），0=失败（材料已耗毁）
int forge(object player, string arg)
{
    string fid, quality;
    mapping f, mats;
    string *mkeys;
    object ob;
    int i, rate, per_rate, tier_need, need, missing;

    if (!objectp(player)) return 0;
    fid = resolve_formula(arg);
    if (!stringp(fid))
    {
        tell_object(player, "没有这种炼器配方，可用 lianqi list 查看。\n");
        return 0;
    }
    f = forge_formula[fid];

    // 境界检查（1E §1.1 法宝适用境界；接 #61 境界判定）
    tier_need = f["require_tier"];
    if (SECT_D->query_cultivation_tier(player) < tier_need)
    {
        tell_object(player, "你境界不足，无法炼制「" + f["name"] + "」（需" +
                    SECT_D->tier_name(tier_need) + "）。\n");
        return 0;
    }

    // 炼器术等级检查（02 图鉴 §4.3）
    if (player->query_skill("lianqi-shu") < f["min_skill"])
    {
        tell_object(player, "你的炼器术修为不足，无法炼制「" + f["name"] + "」（需" +
                    f["min_skill"] + "级）。\n");
        return 0;
    }

    // 材料检查（1E §1.5 材料表；材料采集为流程第一步）
    mats = f["materials"];
    mkeys = keys(mats);
    missing = 0;
    for (i = 0; i < sizeof(mkeys); i++)
    {
        need = mats[mkeys[i]];
        if (count_material(player, mkeys[i]) < need)
            missing++;
    }
    if (missing > 0)
    {
        string miss = "";
        for (i = 0; i < sizeof(mkeys); i++)
        {
            need = mats[mkeys[i]];
            if (count_material(player, mkeys[i]) < need)
                miss += (miss == "" ? "" : "、") + mkeys[i] + "×" + need;
        }
        tell_object(player, "炼器材料不足：还缺 " + miss + "。可往太南谷坊市购置。\n");
        return 0;
    }

    // 扣减材料（材料采集通过即消耗）
    for (i = 0; i < sizeof(mkeys); i++)
        consume_material(player, mkeys[i], mats[mkeys[i]]);

    // 成功率
    rate = query_success_rate(player, f);

    // 五步流程：工坊内四步（精炼提纯/器胚锻造/禁制铭刻/通灵开光）每步独立判定，
    // 步概率 = 综合成功率^(1/4)，合成概率即综合成功率
    per_rate = to_int(pow(to_float(rate) / 100.0, 1.0 / 4.0) * 100.0);
    if (per_rate > 99) per_rate = 99;

    tell_object(player, HIC "你取出材料，开始炼制「" + f["name"] + "」……\n" NOR);
    for (i = 0; i < sizeof(FORGE_STEPS); i++)
    {
        if (random(100) >= per_rate)
        {
            tell_object(player, HIR "「" + FORGE_STEPS[i] + "」失败，材料尽数耗毁！\n" NOR);
            return 0;
        }
        tell_object(player, "「" + FORGE_STEPS[i] + "」成功。\n");
    }

    // 品质判定（1E §1.3 品质维度）
    quality = roll_quality(rate);

    // 生成法宝成品（1E §4.1 数据结构）
    ob = new("/d/yueguo/obj/treasure");
    ob->set_name(f["name"] + "（" + quality + "）", ({ fid, "fabao", "treasure" }));
    ob->set("treasure_type", f["treasure_type"]);
    ob->set("level", quality);
    ob->set("element", f["element"]);
    ob->set("forbidden_count", f["forbidden_count"] + FORGE_QUALITY_BAN[quality]);
    ob->set("attack", to_int(f["attack"] * FORGE_QUALITY_RATE[quality]));
    ob->set("defense", to_int(f["defense"] * FORGE_QUALITY_RATE[quality]));
    ob->set("special", f["special"]);
    ob->set("require_level", f["require_tier"]);
    ob->set("unit", "件");
    ob->set("value", f["value"]);
    ob->setup();
    if (!ob->move(player))
    {
        ob->move(environment(player));
        tell_object(player, "你身上已放不下法宝，它落到了地上。\n");
    }

    tell_object(player, HIY "炼成！" + ob->query("name") + "灵光流转，法宝成形！\n" NOR);
    return 1;
}
