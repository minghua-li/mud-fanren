// inflationd.c  通胀控制与经济健康监控 + 六大循环监控
// by BCubed 团队 (#19, #48)
//
// 职责：
//   1. 监控全服灵石总量，计算人均持有量
//   2. 动态调整税率（交易税/拍卖手续费）
//   3. 在超阈值时触发经济调整事件
//   4. 维护产出/消耗比，确保经济健康
//
// 阈值系统：
//   安全线：人均 ≤ 500 下品灵石 → 基准税率
//   警戒线：人均 500-800 → 动态上调税率
//   危险线：人均 > 800 → 触发经济事件

inherit F_DBASE;
inherit F_SAVE;

#include <ansi.h>
#include <localtime.h>
#include <economy_circulation.h>

// ---------- 经济阈值（下品灵石） ----------
#define THRESHOLD_SAFE     300   // 基准税率
#define THRESHOLD_WARN     500   // 轻度上调
#define THRESHOLD_ALARM    800   // 重度干预

// ---------- 基础税率（千分比） ----------
#define TAX_BASE           50    // 基准 5%
#define TAX_WARN           80    // 轻度 8%
#define TAX_ALARM          120   // 中度 12%
#define TAX_CRISIS         200   // 重度 20%

// ---------- 监控周期 ----------
#define MONITOR_INTERVAL   300   // 5 分钟
#define SAVE_INTERVAL      600   // 10 分钟

// ---------- 产出/消耗比健康范围 ----------
#define RATIO_MIN          0.95
#define RATIO_MAX          1.05

void create()
{
        seteuid(getuid());
        set("name", "通胀监控系统");
        set("id", "inflationd");

        restore();

        // 初始化统计数据结构
        if (!mapp(query("stats")))
                set("stats", ([]));
        if (!mapp(query("production")))
                set("production", ([]));
        if (!mapp(query("consumption")))
                set("consumption", ([]));
        if (!mapp(query("events")))
                set("events", ([]));

        // 启动监控循环
        remove_call_out("monitor_loop");
        call_out("monitor_loop", MONITOR_INTERVAL);

        // 启动定时保存
        remove_call_out("auto_save");
        call_out("auto_save", SAVE_INTERVAL);
}

string query_save_file()
{
        return "/data/inflationd";
}

void auto_save()
{
        save();
        remove_call_out("auto_save");
        call_out("auto_save", SAVE_INTERVAL);
}

// ---------- 经济状态查询 ----------

// 查询当前全服人均灵石持有量
// 返回：人均持有下品灵石数量
int query_per_capita()
{
        mapping stats = query("stats");
        if (!mapp(stats)) return 0;

        int total = stats["total_spirit_stones"];
        int players = stats["active_players"];

        if (players <= 0) return 0;
        return total / players;
}

// 查询当前适用税率（千分比）
// 返回值 = 实际税率（‰），如 50 = 5%
int query_tax_rate()
{
        int per_capita = query_per_capita();

        if (per_capita <= THRESHOLD_SAFE)
                return TAX_BASE;
        if (per_capita <= THRESHOLD_WARN)
                return TAX_WARN;
        if (per_capita <= THRESHOLD_ALARM)
                return TAX_ALARM;

        // 超过危险线
        return TAX_CRISIS;
}

// 查询经济健康状态
// 返回值: "healthy" | "warning" | "critical"
string query_economy_health()
{
        int per_capita = query_per_capita();

        if (per_capita <= THRESHOLD_SAFE)
                return "healthy";
        if (per_capita <= THRESHOLD_ALARM)
                return "warning";
        return "critical";
}

// 查询产出/消耗比
// 返回值：产出/消耗 比率，如 1.02 表示产出略高于消耗
float query_output_consumption_ratio()
{
        mapping stats = query("stats");
        if (!mapp(stats)) return 1.0;

        int output = stats["total_production"];
        int consumption = stats["total_consumption"];

        if (consumption <= 0) return 1.0;
        return to_float(output) / to_float(consumption);
}

// ---------- 数据采集接口 ----------

// 由 moneyd.c 调用：记录灵石产出
void on_production(int amount, string realm)
{
        mapping production = query("production");
        if (!mapp(production)) production = ([]);

        production["total"] += amount;
        production[realm] += amount;
        set("production", production);

        // 更新总统计
        add("stats/total_production", amount);
        add("stats/total_spirit_stones", amount);
}

// 由 moneyd.c 调用：记录灵石消耗
void on_consumption(int amount, string realm)
{
        mapping consumption = query("consumption");
        if (!mapp(consumption)) consumption = ([]);

        string key = "consumed_" + realm;
        consumption["consumed_total"] += amount;
        consumption[key] += amount;
        set("consumption", consumption);

        // 更新总统计
        add("stats/total_consumption", amount);
        add("stats/total_spirit_stones", -amount);
        if (query("stats/total_spirit_stones") < 0)
                set("stats/total_spirit_stones", 0);
}

// 更新在线玩家数（由 moneyd.c 或 logind 调用）
void update_active_players(int count)
{
        set("stats/active_players", count);
}

// 记录税收回收（由 moneyd.c 的市场税/拍卖手续费接口调用）
void on_tax_collected(int amount)
{
        if (amount <= 0) return;

        mapping stats = query("stats");
        if (!mapp(stats)) return;

        add("stats/tax_collected", amount);

        // 税收本身就是消耗（灵石从玩家流向系统）
        // 注意：此处不能重复扣除 total_spirit_stones（已在 moneyd 层扣除）
}

// ---------- 经济事件检查 ----------

// 检查是否需要触发经济事件
// 返回: 事件类型字符串，无事件返回 ""
string check_economic_event()
{
        int per_capita = query_per_capita();

        // 危险线 → 触发重度事件
        if (per_capita > THRESHOLD_ALARM) {
                // 检查冷却期（7 天内不重复触发同类型事件）
                int last_event = query("events/last_crisis_time");
                if (time() - last_event > 7 * 86400) {
                        set("events/last_crisis_time", time());
                        return "crisis_event";
                }
        }

        // 警戒线且持续超过 7 天 → 触发中度事件
        if (per_capita > THRESHOLD_WARN) {
                int warn_start = query("events/warn_start_time");
                if (warn_start == 0)
                        set("events/warn_start_time", time());
                else if (time() - warn_start > 7 * 86400) {
                        int last_event = query("events/last_warn_event_time");
                        if (time() - last_event > 7 * 86400) {
                                set("events/last_warn_event_time", time());
                                return "warn_event";
                        }
                }
        } else {
                // 回到安全线以下，重置警戒计时
                set("events/warn_start_time", 0);
        }

        // 产出/消耗比失衡检查
        float ratio = query_output_consumption_ratio();
        if (ratio < RATIO_MIN) {
                // 产出不足 → 加速产出
                return "output_shortage";
        }
        if (ratio > RATIO_MAX) {
                // 产出过剩 → 加速消耗
                return "output_surplus";
        }

        return "";
}

// ---------- 经济调整行动 ----------

// 获取针对指定商品的附加税率调整（千分比）
// 用于在基础税率之上叠加通胀调整
int get_additional_tax_modifier()
{
        int per_capita = query_per_capita();

        if (per_capita <= THRESHOLD_SAFE)
                return 0;       // 无附加
        if (per_capita <= THRESHOLD_WARN)
                return 10;      // +1%
        if (per_capita <= THRESHOLD_ALARM)
                return 30;      // +3%

        return 50;              // +5% 重度附加
}

// 获取经济调整建议信息
// 返回给 MONITOR_D 用于广播的消息
string get_economy_adjustment_message()
{
        int per_capita = query_per_capita();
        int tax = query_tax_rate();
        string health = query_economy_health();
        float ratio = query_output_consumption_ratio();

        string msg = sprintf(
                HIC "【经济监控】%s\n" NOR
                "全服人均灵石：%d 下品灵石\n"
                "当前税率：%d‰（%.1f%%）\n"
                "经济状态：%s\n"
                "产出/消耗比：%.2f\n",
                health == "healthy" ? HIW "经济健康" NOR :
                health == "warning" ? HIY "经济预警" NOR :
                HIR "经济危机" NOR,
                per_capita,
                tax, to_float(tax) / 10.0,
                health,
                ratio
        );

        return msg;
}

// ---------- 监控主循环 ----------

void monitor_loop()
{
        // 1. 检查是否需要触发经济事件
        string event = check_economic_event();
        if (event != "") {
                string msg;
                switch (event) {
                case "crisis_event":
                        msg = HIR "【经济事件】全服灵石存量过高，触发「妖兽潮」！"
                              "装备损耗+50%，修炼消耗+25%，持续 3 天。\n" NOR;
                        // 设置全局状态标记
                        set("events/active", "beast_tide");
                        set("events/active_until", time() + 3 * 86400);
                        break;
                case "warn_event":
                        msg = HIY "【经济事件】灵石持续充裕，触发「灵气潮汐」！"
                              "全服修炼消耗+25%，持续 3 天。\n" NOR;
                        set("events/active", "spirit_tide");
                        set("events/active_until", time() + 3 * 86400);
                        break;
                case "output_shortage":
                        msg = HIW "【经济调整】产出不足，系统将临时提升任务灵石奖励。\n" NOR;
                        break;
                case "output_surplus":
                        msg = HIC "【经济调整】产出过剩，系统商店收购价下调 20%。\n" NOR;
                        break;
                }

                // 广播经济事件消息
                if (msg != "" && find_object(CHANNEL_D))
                        CHANNEL_D->do_channel(this_object(), "sys_misc", msg);
        }

        // 2. 产出/消耗比自动调节
        float ratio = query_output_consumption_ratio();
        if (ratio < 0.9) {
                // 产出严重不足，提升产出奖励
                set("stats/output_boost", 1.2);     // +20%
        } else if (ratio > 1.1) {
                // 产出过剩，降低产出奖励
                set("stats/output_boost", 0.85);    // -15%
        } else {
                set("stats/output_boost", 1.0);     // 正常
        }

        save();

        // 继续监控循环
        remove_call_out("monitor_loop");
        call_out("monitor_loop", MONITOR_INTERVAL);
}

// ---------- 查询接口 ----------

// 获取当前活跃的经济事件（用于其他系统查询）
mapping query_active_event()
{
        int until = query("events/active_until");
        if (until > time()) {
                return ([
                        "type":  query("events/active"),
                        "until": until,
                        "remaining": until - time(),
                ]);
        }
        return ([]);
}

// 获取产出增益系数（供任务奖励系统参考）
float query_output_boost()
{
        return query("stats/output_boost");
}

// 获取完整经济报告
string query_full_report()
{
        mapping stats = query("stats");
        if (!mapp(stats))
                return "经济数据未初始化。\n";

        string msg = sprintf(
                "=== 经济系统报告 ===\n"
                "全服灵石总量：%d\n"
                "活跃玩家数：%d\n"
                "人均灵石：%d\n"
                "当前税率：%d‰\n"
                "经济状态：%s\n"
                "产出/消耗比：%.2f\n"
                "产出增益系数：%.2f\n"
                "总产出：%d\n"
                "总消耗：%d\n",
                stats["total_spirit_stones"],
                stats["active_players"],
                query_per_capita(),
                query_tax_rate(),
                query_economy_health(),
                query_output_consumption_ratio(),
                query_output_boost(),
                stats["total_production"],
                stats["total_consumption"]
        );

        return msg;
}

// ---------- 全局收支平衡验证 ----------

// 验证经济系统中的关键健康指标
// 返回包含所有验证项的 mapping
//   ([
//     "status": "pass" | "warn" | "fail",
//     "checks": ([
//       "check_name": ([
//         "status": "pass" | "warn" | "fail",
//         "message": "详细描述",
//         "current": 当前值,
//         "expected": 期望值
//       ]),
//       ...
//     ]),
//   ])
mapping verify_economy_balance()
{
        mapping result = ([
                "status": "pass",
                "checks": ([]),
        ]);
        mapping checks = ([]);
        mapping stats = query("stats");

        // 1. 产出/消耗比
        float ratio = query_output_consumption_ratio();
        if (ratio >= RATIO_MIN && ratio <= RATIO_MAX) {
                checks["output_consumption_ratio"] = ([
                        "status": "pass",
                        "message": sprintf("产出/消耗比 %.2f 在健康区间 [%.2f, %.2f] 内", ratio, RATIO_MIN, RATIO_MAX),
                        "current": ratio,
                        "expected": 1.0,
                ]);
        } else {
                checks["output_consumption_ratio"] = ([
                        "status": "warn",
                        "message": sprintf("产出/消耗比 %.2f 超出健康区间 [%.2f, %.2f]", ratio, RATIO_MIN, RATIO_MAX),
                        "current": ratio,
                        "expected": 1.0,
                ]);
        }

        // 2. 人均灵石存量
        int per_capita = query_per_capita();
        if (per_capita <= THRESHOLD_SAFE) {
                checks["per_capita_holdings"] = ([
                        "status": "pass",
                        "message": sprintf("人均灵石 %d 在安全线 %d 以下", per_capita, THRESHOLD_SAFE),
                        "current": per_capita,
                        "expected": sprintf("<%d", THRESHOLD_SAFE),
                ]);
        } else if (per_capita <= THRESHOLD_WARN) {
                checks["per_capita_holdings"] = ([
                        "status": "warn",
                        "message": sprintf("人均灵石 %d 在警戒区 [%d, %d)", per_capita, THRESHOLD_SAFE, THRESHOLD_WARN),
                        "current": per_capita,
                        "expected": sprintf("<%d", THRESHOLD_SAFE),
                ]);
        } else {
                checks["per_capita_holdings"] = ([
                        "status": "warn",
                        "message": sprintf("人均灵石 %d 超过警戒线 %d，需干预", per_capita, THRESHOLD_WARN),
                        "current": per_capita,
                        "expected": sprintf("<%d", THRESHOLD_WARN),
                ]);
        }

        // 3. 系统回收/产出比（目标 40-60%）
        int total_production = stats["total_production"];
        int total_consumption = stats["total_consumption"];
        float recovery_ratio = 0.0;
        if (total_production > 0) {
                // 消耗本身就是回收的一种形式，但真正的系统回收还包括税收
                int tax_recovery = query("stats/tax_collected");
                if (tax_recovery > 0)
                        recovery_ratio = to_float(tax_recovery) / to_float(total_production);
        }
        if (recovery_ratio >= 0.40 && recovery_ratio <= 0.60) {
                checks["recovery_ratio"] = ([
                        "status": "pass",
                        "message": sprintf("系统回收/产出比 %.0f%% 在目标区间 [40%%, 60%%]", recovery_ratio * 100.0),
                        "current": recovery_ratio,
                        "expected": 0.50,
                ]);
        } else {
                checks["recovery_ratio"] = ([
                        "status": "warn",
                        "message": sprintf("系统回收/产出比 %.0f%% 不在目标区间 [40%%, 60%%]", recovery_ratio * 100.0),
                        "current": recovery_ratio,
                        "expected": 0.50,
                ]);
        }

        // 4. 境界间灵石倍率差（目标 ≤ 100 倍）
        // 此处简化为检查高境界产出系数与低境界的比值
        float realm_gap = MONEY_D->query_realm_coefficient("huashen") / MONEY_D->query_realm_coefficient("qige");
        if (realm_gap <= 100.0) {
                checks["realm_wealth_gap"] = ([
                        "status": "pass",
                        "message": sprintf("境界产出系数倍率差 %.0f 倍，在目标 ≤ 100 倍范围内", realm_gap),
                        "current": realm_gap,
                        "expected": 100.0,
                ]);
        } else {
                checks["realm_wealth_gap"] = ([
                        "status": "warn",
                        "message": sprintf("境界产出系数倍率差 %.0f 倍，超出目标 ≤ 100 倍", realm_gap),
                        "current": realm_gap,
                        "expected": 100.0,
                ]);
        }

        // 5. 全服灵石总存量
        int total = stats["total_spirit_stones"];
        if (total >= 0) {
                checks["total_supply"] = ([
                        "status": "pass",
                        "message": sprintf("全服灵石总存量 %d，系统运行正常", total),
                        "current": total,
                        "expected": ">0",
                ]);
        } else {
                checks["total_supply"] = ([
                        "status": "fail",
                        "message": sprintf("全服灵石总存量 %d 出现负值，数据异常", total),
                        "current": total,
                        "expected": "≥0",
                ]);
        }

        // 汇总状态
        string overall = "pass";
        foreach (string key, mapping check in checks) {
                if (check["status"] == "fail") overall = "fail";
                else if (check["status"] == "warn" && overall == "pass") overall = "warn";
        }
        result["status"] = overall;
        result["checks"] = checks;

        return result;
}

// 生成可读的平衡验证报告
string query_balance_report()
{
        mapping vresult = verify_economy_balance();
        mapping checks = vresult["checks"];
        string status_str = vresult["status"] == "pass" ? HIW "通过" NOR :
                            vresult["status"] == "warn" ? HIY "警告" NOR :
                            HIR "失败" NOR;

        string msg = sprintf(
                "=== 全局收支平衡验证报告 ===\n"
                "总体状态：%s\n\n",
                status_str
        );

        foreach (string key, mapping check in checks) {
                string s = check["status"] == "pass" ? HIW "✓" NOR :
                           check["status"] == "warn" ? HIY "△" NOR :
                           HIR "✗" NOR;
                msg += sprintf("%s [%s] %s\n", s, key, check["message"]);
        }

        msg += "\n" + query_full_report();

        return msg;
}

// ======== P3 集成 A：六大循环监控 ===========================================

// 获取六大循环的整体健康评分（0-100）
// 根据产出/消耗比、各循环均衡度等指标综合计算
int query_circulation_health_score()
{
        float ratio = query_output_consumption_ratio();
        mapping stats = query("stats");

        // 产出/消耗比（权重40%）
        float ratio_score;
        if (ratio >= 0.95 && ratio <= 1.05)
                ratio_score = 100.0;
        else if (ratio >= 0.90 && ratio <= 1.10)
                ratio_score = 70.0;
        else if (ratio >= 0.80 && ratio <= 1.20)
                ratio_score = 40.0;
        else
                ratio_score = 10.0;

        // 人均灵石存量（权重30%）
        int per_capita = query_per_capita();
        float capita_score;
        if (per_capita <= 300)
                capita_score = 100.0;
        else if (per_capita <= 500)
                capita_score = 70.0;
        else if (per_capita <= 800)
                capita_score = 40.0;
        else
                capita_score = 10.0;

        // 系统回收/产出比（权重30%）
        int total_production = stats["total_production"];
        int tax_collected = stats["tax_collected"];
        float recovery_score = 50.0;  // 默认中等
        if (total_production > 0 && tax_collected > 0) {
                float recovery_ratio = to_float(tax_collected) / to_float(total_production);
                if (recovery_ratio >= 0.40 && recovery_ratio <= 0.60)
                        recovery_score = 100.0;
                else if (recovery_ratio >= 0.20 && recovery_ratio <= 0.80)
                        recovery_score = 60.0;
                else
                        recovery_score = 20.0;
        }

        int score = to_int(ratio_score * 0.4 + capita_score * 0.3 + recovery_score * 0.3);
        if (score < 0) score = 0;
        if (score > 100) score = 100;

        return score;
}

// 查询六大循环中消耗占比最高的类型
// 返回: ({ "sink_type", 消耗量, 占比 }) 或 ({}) 无数据
mixed *query_top_circulation_consumption()
{
        // 预留接口：待 moneyd.c 的 consumption_log 按类型分类后挂接
        // 当前返回空数组表示尚未实现详细分类
        return ({});
}

// 查询六大循环通胀风险报告
string query_circulation_risk_report()
{
        string msg = HIY "═══ 六大经济循环风险报告 ═══\n" NOR;
        int score = query_circulation_health_score();
        float ratio = query_output_consumption_ratio();
        int per_capita = query_per_capita();

        msg += sprintf("经济循环健康评分：%d/100\n", score);
        msg += sprintf("产出/消耗比：%.2f（健康区间 0.95-1.05）\n", ratio);
        msg += sprintf("人均灵石存量：%d（安全线 ≤300，警戒线 ≤500，危险线 ≥800）\n", per_capita);

        // 各循环消耗占比简要评估
        msg += "\n【六大循环运行状态】\n";
        msg += "  A1 修炼循环：灵石灌注 + 聚灵阵消耗 — 基础消耗通道\n";
        msg += "  A2 战斗循环：阵法维持 + 装备维修 + PVP + 比武 — 弹性消耗\n";
        msg += "  A3 区域循环：传送 + 秘境 + 洞府 + 宗门 — 刚性+弹性混合\n";
        msg += "  A4 任务循环：任务奖励注入（产出）+ 失败惩罚（回收）— 主产出通道\n";
        msg += "  A5 声望循环：势力任务 + 声望门槛 + 产业税收 — 高阶产出/回收\n";
        msg += "  A6 自身循环：动态定价 + 税率调节 + 回收通道 — 自动稳定器\n";

        // 风险提示
        if (score >= 80)
                msg += HIG "\n✓ 经济循环健康，各通道运行正常。\n" NOR;
        else if (score >= 60)
                msg += HIY "\n△ 经济循环轻度预警，建议关注各通道消耗平衡。\n" NOR;
        else
                msg += HIR "\n✗ 经济循环风险高，需检查产出/消耗比和回收效率！\n" NOR;

        return msg;
}

// 强制保存
void force_save()
{
        save();
}
