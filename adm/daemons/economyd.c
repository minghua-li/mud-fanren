// economyd.c  动态定价系统
// by BCubed 团队 (#19)
// 基于供需关系自动调整物价，防止经济膨胀/紧缩
//
// 核心公式：
//   商品当前价格 = 基准价 × (1 + 需求系数 - 供给系数) × 区域物价修正
//   需求系数 = 最近 24h 该商品总购买量 / 该商品基准周转量
//   供给系数 = 最近 24h 该商品上架量 / 该商品基准周转量
//   价格波动边界: 基准价的 50% ~ 150%

inherit F_DBASE;
inherit F_SAVE;

#include <ansi.h>
#include <localtime.h>

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

        // 价格波动边界
        if (price_ratio < PRICE_FLOOR_RATIO)
                price_ratio = PRICE_FLOOR_RATIO;
        if (price_ratio > PRICE_CEIL_RATIO)
                price_ratio = PRICE_CEIL_RATIO;

        // 区域物价修正
        float region_mod = info["region_modifier"];
        price_ratio = price_ratio * region_mod;

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

// 强制保存
void force_save()
{
        save();
}
