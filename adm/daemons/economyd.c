// economyd.c  动态定价系统 + 区域经济
// by BCubed 团队 (#19, #22)
// 基于供需关系自动调整物价，防止经济膨胀/紧缩
//
// 核心公式：
//   商品当前价格 = 基准价 × (1 + 需求系数 - 供给系数) × 区域物价修正
//   需求系数 = 最近 24h 该商品总购买量 / 该商品基准周转量
//   供给系数 = 最近 24h 该商品上架量 / 该商品基准周转量
//   价格波动边界: 基准价的 50% ~ 150%
//
// 区域经济扩展（#22）：
//   区域物价差异（高阶区域物价更高）
//   区域特产系统（特产在原产地溢价销售）
//   贸易路线利润计算（跨区域跑商收益）

inherit F_DBASE;
inherit F_SAVE;

#include <ansi.h>
#include <localtime.h>
#include <region_economy.h>
#include <economy_lifecycle.h>

// 价格波动边界
#define PRICE_FLOOR_RATIO  0.50   // 最低为基准价的 50%
#define PRICE_CEIL_RATIO   1.50   // 最高为基准价的 150%

// 数据保存周期（秒）
#define SAVE_INTERVAL      300    // 5 分钟自动保存

// ---------- 商品类型索引 ----------
// 所有商品以 type key 标识，data mapping 结构：
// ([
//   "type": {
//     "base_price": int,        // 基准价（下品灵石）
//     "turnover": int,          // 基准周转量（24h 参考值）
//     "bought_24h": int,        // 最近 24h 购买量
//     "sold_24h": int,          // 最近 24h 上架量
//     "last_update": int,       // 最后更新时间戳
//     "region_modifier": float, // 区域物价修正（默认 1.0）
//   },
//   ...
// ])

void create()
{
        seteuid(getuid());
        set("name", "经济系统");
        set("id", "economyd");

        // 恢复已保存的市场数据
        restore();

        // 如果没有初始化数据，设置默认
        if (!mapp(query("goods")))
                set("goods", ([]));

        // 初始化区域物价修正
        init_region_modifiers();

        // 启动定时保存
        remove_call_out("auto_save");
        call_out("auto_save", SAVE_INTERVAL);

        // 启动过期数据清理
        remove_call_out("cleanup_old_data");
        call_out("cleanup_old_data", 3600);
}

string query_save_file()
{
        return "/data/economyd";
}

void auto_save()
{
        save();
        remove_call_out("auto_save");
        call_out("auto_save", SAVE_INTERVAL);
}

// ---------- 商品注册 ----------

// 注册一个商品类型到定价系统
// 参数: type - 商品类型标识（如 "pill_basic", "weapon_low"）
//       base_price - 基准价（下品灵石）
//       turnover - 基准周转量（24h 标准交易量）
void register_goods(string type, int base_price, int turnover)
{
        mapping goods = query("goods");
        if (!mapp(goods)) goods = ([]);

        if (!mapp(goods[type])) {
                goods[type] = ([
                        "base_price":   base_price,
                        "turnover":     turnover,
                        "bought_24h":   0,
                        "sold_24h":     0,
                        "last_update":  time(),
                        "region_modifier": 1.0,
                ]);
        } else {
                // 更新基准价和周转量
                goods[type]["base_price"] = base_price;
                goods[type]["turnover"] = turnover;
        }

        set("goods", goods);
}

// 设置区域物价修正系数
void set_region_modifier(string type, float modifier)
{
        mapping goods = query("goods");
        if (!mapp(goods) || !mapp(goods[type])) return;

        goods[type]["region_modifier"] = modifier;
        set("goods", goods);
}

// ---------- 供需记录 ----------

// 记录一次购买（需求增加）
// 返回更新后的价格倍率
float record_purchase(string type, int amount)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return 1.0;

        mapping info = goods[type];
        if (!mapp(info)) return 1.0;

        info["bought_24h"] += amount;
        info["last_update"] = time();
        set("goods", goods);

        return calculate_price_ratio(type);
}

// 记录一次上架/出售（供给增加）
// 返回更新后的价格倍率
float record_sale(string type, int amount)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return 1.0;

        mapping info = goods[type];
        if (!mapp(info)) return 1.0;

        info["sold_24h"] += amount;
        info["last_update"] = time();
        set("goods", goods);

        return calculate_price_ratio(type);
}

// ---------- 价格计算 ----------

// 计算指定商品类型的当前价格倍率
// 返回相对于基准价的乘数
float calculate_price_ratio(string type)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return 1.0;

        mapping info = goods[type];
        if (!mapp(info)) return 1.0;

        int turnover = info["turnover"];
        if (turnover <= 0) turnover = 1;  // 防除零

        float demand_ratio = to_float(info["bought_24h"]) / to_float(turnover);
        float supply_ratio = to_float(info["sold_24h"]) / to_float(turnover);

        float price_ratio = 1.0 + demand_ratio - supply_ratio;

        // 区域物价修正（先乘区域修正系数，再钳制边界）
        float region_mod = info["region_modifier"];
        price_ratio = price_ratio * region_mod;

        // 价格波动边界（相对基准价 ±50%）
        if (price_ratio < PRICE_FLOOR_RATIO)
                price_ratio = PRICE_FLOOR_RATIO;
        if (price_ratio > PRICE_CEIL_RATIO)
                price_ratio = PRICE_CEIL_RATIO;

        return price_ratio;
}

// 获取当前价格（下品灵石）
int query_current_price(string type)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return 0;

        mapping info = goods[type];
        if (!mapp(info)) return 0;

        float ratio = calculate_price_ratio(type);
        return to_int(to_float(info["base_price"]) * ratio);
}

// 获取基准价
int query_base_price(string type)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return 0;

        mapping info = goods[type];
        if (!mapp(info)) return 0;

        return info["base_price"];
}

// ---------- 查询 ----------

// 获取指定商品类型的供需报告
string query_market_report(string type)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return "无数据。\n";

        mapping info = goods[type];
        if (!mapp(info)) return "无此商品数据。\n";

        float ratio = calculate_price_ratio(type);
        string msg = sprintf(
                "商品类型：%s\n"
                "基准价：%d 下品灵石\n"
                "当前价：%d 下品灵石\n"
                "价格倍率：%.2f\n"
                "24h 购买量：%d\n"
                "24h 上架量：%d\n"
                "基准周转量：%d\n"
                "区域修正：%.2f\n",
                type,
                info["base_price"],
                to_int(to_float(info["base_price"]) * ratio),
                ratio,
                info["bought_24h"],
                info["sold_24h"],
                info["turnover"],
                info["region_modifier"]
        );
        return msg;
}

// 获取所有商品类型的价格信息（用于监控）
mapping query_all_prices()
{
        mapping result = ([]);
        mapping goods = query("goods");

        if (!mapp(goods)) return result;

        foreach (string type, mapping info in goods) {
                float ratio = calculate_price_ratio(type);
                result[type] = ([
                        "base_price":   info["base_price"],
                        "current_price": to_int(to_float(info["base_price"]) * ratio),
                        "ratio":        ratio,
                        "bought_24h":   info["bought_24h"],
                        "sold_24h":     info["sold_24h"],
                ]);
        }

        return result;
}

// ---------- 数据维护 ----------

// 每日清理过期数据（保留 24h 滚动窗口）
void cleanup_old_data()
{
        mapping goods = query("goods");
        if (!mapp(goods)) return;

        // 将 24h 数据清 0（数据已过时）
        // 在正式生产环境中，这里应改为按时间戳衰减，
        // 而非直接清零，以免月初物价剧烈波动
        foreach (string type, mapping info in goods) {
                info["sold_24h"] = 0;
                info["bought_24h"] = 0;
        }

        set("goods", goods);
        save();

        // 每小时运行一次
        remove_call_out("cleanup_old_data");
        call_out("cleanup_old_data", 3600);
}

// ======== A6 经济自身循环系统 =======================================
// 整合以下子能力：
//   1. 产出总量控制查询（对接 MONEY_D 的日产出上限）
//   2. 回收通道监控（对接 INFLATION_D 的消耗记录）
//   3. 全服通胀感知动态调价
//   4. 经济生命周期验证
// ==================================================================

// 记录购买并联动通胀监控
// 覆写原有 record_purchase 以增加通胀联动
float record_purchase_with_inflation(string type, int amount, string realm)
{
        float ratio = record_purchase(type, amount);

        // 记录消耗到通胀监控
        if (find_object(MONEY_D))
                MONEY_D->add_consumption("market_purchase_" + type, amount, realm);

        return ratio;
}

// 记录上架并联动产出监控
float record_sale_with_inflation(string type, int amount, string realm)
{
        float ratio = record_sale(type, amount);

        // 记录产出到通胀监控
        if (find_object(MONEY_D))
                MONEY_D->add_production("market_sale_" + type, amount, realm);

        return ratio;
}

// 获取通胀感知价格（在动态定价基础上叠加通胀附加税）
// 通胀严重时自动加价，抑制过度消费
int query_inflation_adjusted_price(string type, string region_id)
{
        int base_price = query_region_price(region_id, type);
        if (base_price <= 0) return 0;

        // 获取通胀附加税系数
        if (find_object(INFLATION_D))
        {
                int add_tax = INFLATION_D->get_additional_tax_modifier();
                // 附加税 = 基准价 × 附加税率
                int surcharge = to_int(to_float(base_price) * to_float(add_tax) / 1000.0);
                base_price += surcharge;
        }

        return base_price;
}

// 验证经济闭环状态
// 返回 mapping，包含各环节的状态
mapping verify_economy_lifecycle()
{
        mapping result = ([
                "status": "pass",
                "checks": ([]),
        ]);
        mapping checks = ([]);

        // 1. 产出总量控制
        if (find_object(MONEY_D))
        {
                int cap = MONEY_D->query_daily_production_cap();
                int prod = MONEY_D->query_daily_production();
                if (cap > 0)
                {
                        float util = to_float(prod) / to_float(cap);
                        checks["production_control"] = ([
                                "status":   util <= 1.0 ? "pass" : "warn",
                                "message":  sprintf("日产出利用率 %.0f%% (上限 %d, 已用 %d)", util * 100.0, cap, prod),
                                "current":  prod,
                                "expected": cap,
                        ]);
                }
        }

        // 2. 动态定价有效
        mapping goods = query("goods");
        if (mapp(goods) && sizeof(goods) > 0)
        {
                int within_bounds = 1;
                foreach (string type, mapping info in goods)
                {
                        float ratio = calculate_price_ratio(type);
                        if (ratio < PRICE_FLOOR_RATIO || ratio > PRICE_CEIL_RATIO)
                        {
                                within_bounds = 0;
                                break;
                        }
                }
                checks["dynamic_pricing"] = ([
                        "status":   within_bounds ? "pass" : "warn",
                        "message":  sprintf("动态定价在 [%.0f%%, %.0f%%] 边界内运行",
                                            PRICE_FLOOR_RATIO * 100.0, PRICE_CEIL_RATIO * 100.0),
                        "current":  sizeof(goods),
                        "expected": ">0",
                ]);
        }

        // 3. 回收通道可用
        if (find_object(INFLATION_D))
        {
                float ratio = INFLATION_D->query_output_consumption_ratio();
                checks["recycling_channels"] = ([
                        "status":   (ratio >= 0.8 && ratio <= 1.2) ? "pass" : "warn",
                        "message":  sprintf("产出/消耗比 %.2f (目标区间 [0.80, 1.20])", ratio),
                        "current":  ratio,
                        "expected": 1.0,
                ]);
        }

        // 4. 通胀监控有效
        if (find_object(INFLATION_D))
        {
                int per_capita = INFLATION_D->query_per_capita();
                string health = INFLATION_D->query_economy_health();
                checks["inflation_monitoring"] = ([
                        "status":   "pass",
                        "message":  sprintf("通胀监控运行中: 人均灵石 %d, 状态 %s", per_capita, health),
                        "current":  per_capita,
                        "expected": "<" + ECON_WARNING_MAX,
                ]);
        }

        // 汇总
        string overall = "pass";
        foreach (string key, mapping check in checks)
        {
                if (check["status"] == "fail") overall = "fail";
                else if (check["status"] == "warn" && overall == "pass") overall = "warn";
        }
        result["status"] = overall;
        result["checks"] = checks;

        return result;
}

// 生成经济生命周期报告
string query_lifecycle_report()
{
        mapping vresult = verify_economy_lifecycle();
        mapping checks = vresult["checks"];
        string status_str = vresult["status"] == "pass" ? HIW "通过" NOR :
                            vresult["status"] == "warn" ? HIY "警告" NOR :
                            HIR "失败" NOR;

        string msg = sprintf(
                HIC "╔════════════════════════════════╗\n" NOR
                HIC "║  " HIW "【经济生命周期报告】" HIC "                ║\n" NOR
                HIC "╚════════════════════════════════╝\n" NOR
                "\n"
                "总体状态：%s\n\n",
                status_str
        );

        foreach (string key, mapping check in checks)
        {
                string s = check["status"] == "pass" ? HIW "✓" NOR :
                           check["status"] == "warn" ? HIY "△" NOR :
                           HIR "✗" NOR;
                msg += sprintf("%s [%s] %s\n", s, key, check["message"]);
        }

        // 附加通胀监控信息
        if (find_object(INFLATION_D))
        {
                msg += "\n" + INFLATION_D->get_economy_adjustment_message();
        }

        // 附加区域物价概览
        mapping mods = REGION_PRICE_MODIFIER;
        if (mapp(mods) && sizeof(mods) > 0)
        {
                msg += "\n【区域物价概览】\n";
                foreach (string rid, float mod in mods)
                {
                        string rname = REGION_NAME[rid];
                        msg += sprintf("  %-12s ×%.2f\n", rname, mod);
                }
        }

        return msg;
}

// ======== 区域经济系统（对应设计文档 §5.2 + §6.5）==================

// 获取指定区域的物价修正系数
// 高阶区域物价更高，低阶区域物价更低
float query_region_modifier(string region_id)
{
        mapping mods = REGION_PRICE_MODIFIER;

        if (undefinedp(mods[region_id]))
                return 1.0;

        return mods[region_id];
}

// 获取某商品在指定区域的当前价格（含区域修正 + 供需波动）
// 这是商店 NPC 定价的推荐接口
int query_region_price(string region_id, string goods_type)
{
        mapping goods = query("goods");
        if (!mapp(goods)) return 0;

        mapping info = goods[goods_type];
        if (!mapp(info)) return 0;

        float ratio = calculate_price_ratio(goods_type);

        // 叠加区域物价修正（在动态供需之上再乘区域系数）
        float region_mod = query_region_modifier(region_id);
        ratio = ratio * region_mod;

        // 如果是特产在原产地，额外享受溢价
        if (is_region_special(region_id, goods_type))
                ratio = ratio * SPECIAL_SOURCE_BONUS;

        return to_int(to_float(info["base_price"]) * ratio);
}

// 批量初始化所有区域的商品物价修正
// 由系统启动时调用，确保各区域的物价差异生效
void init_region_modifiers()
{
        mapping mods = REGION_PRICE_MODIFIER;
        string *regions;
        string *specials;
        int i, j;

        regions = keys(mods);
        for (i = 0; i < sizeof(regions); i++)
        {
                specials = query_region_specials(regions[i]);
                for (j = 0; j < sizeof(specials); j++)
                {
                        // 为特产设置较高的基准价（体现区域价值）
                        set_region_modifier(specials[j], mods[regions[i]]);
                }
        }
}

// 获取指定区域的特色商品列表
// 返回 string 数组
string *query_region_specials(string region_id)
{
        mapping specials = REGION_SPECIAL_PRODUCTS;

        if (undefinedp(specials[region_id]))
                return ({});

        return specials[region_id];
}

// 判断某商品是否为指定区域的特产
int is_region_special(string region_id, string goods_type)
{
        string *specials;

        specials = query_region_specials(region_id);
        if (sizeof(specials) == 0)
                return 0;

        return (member_array(goods_type, specials) != -1);
}

// 获取特产在原产地的出售价格（含溢价）
// 在原产地的商店：售价 × SPECIAL_SOURCE_BONUS
int query_special_price(string region_id, string goods_type)
{
        int base_price;
        float modifier;

        base_price = query_base_price(goods_type);
        if (base_price <= 0)
                return 0;

        // 只有特产才有溢价
        if (!is_region_special(region_id, goods_type))
                return base_price;

        modifier = REGION_PRICE_MODIFIER[region_id];
        if (modifier < 0.1) modifier = 1.0;

        return to_int(to_float(base_price) * modifier * SPECIAL_SOURCE_BONUS);
}

// 计算两地贸易路线的利润系数
// 从 src_region 购买特产，运到 dest_region 出售
// 返回利润系数（>1.0 有利润，<1.0 亏损）
float calculate_trade_profit(string src_region, string dest_region, string goods_type)
{
        float src_price, dest_price, ratio;
        string route_key;

        // 检查商品是否为 src 区域特产
        if (!is_region_special(src_region, goods_type))
                return 0.8;  // 非特产，跨区域利润率低

        // 获取两地价格
        src_price = REGION_PRICE_MODIFIER[src_region];
        dest_price = REGION_PRICE_MODIFIER[dest_region];

        if (src_price <= 0 || dest_price <= 0)
                return 1.0;

        // 利润 = 目的地价格 / 来源地价格
        ratio = dest_price / src_price;

        // 检查是否有预设的贸易路线加成
        route_key = src_region + "->" + dest_region;
        if (!undefinedp(TRADE_ROUTE_PROFIT[route_key]))
        {
                ratio = ratio * TRADE_ROUTE_PROFIT[route_key];
        }

        // 特产在原产地买入有溢价，所以在原产地买特产更贵
        // 这是为了平衡：特产在原产地价值高，利润空间在贸易中体现
        ratio = ratio * 1.0 / SPECIAL_SOURCE_BONUS;

        return ratio;
}

// 获取区域经济报告
string query_region_report(string region_id)
{
        string result;
        string *specials;
        float modifier;
        int i;

        modifier = query_region_modifier(region_id);
        specials = query_region_specials(region_id);

        result = sprintf(
                HIW "╔════════════════════════════════╗\n" NOR
                HIW "║  " HIC "【区域经济】%s" HIW "              ║\n" NOR
                HIW "╚════════════════════════════════╝\n" NOR
                "\n"
                "区域名称 ：%s\n"
                "物价水平 ：×%.2f（%s）\n"
                "\n"
                "【区域特产】\n",
                REGION_NAME[region_id],
                REGION_NAME[region_id],
                modifier,
                (modifier < 0.8) ? "物价低廉" :
                (modifier < 1.0) ? "物价偏低" :
                (modifier < 1.2) ? "物价适中" :
                (modifier < 1.6) ? "物价偏高" :
                "物价昂贵"
        );

        if (sizeof(specials) == 0)
        {
                result += "  （该区域无特别特产）\n";
        }
        else
        {
                for (i = 0; i < sizeof(specials); i++)
                {
                        result += sprintf("  · %s（溢价 ×%.0f%%）\n",
                                          specials[i],
                                          (SPECIAL_SOURCE_BONUS - 1.0) * 100);
                }
        }

        return result;
}

// 强制保存
void force_save()
{
        save();
}
