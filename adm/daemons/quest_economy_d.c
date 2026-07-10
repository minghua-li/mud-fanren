// quest_economy_d.c  任务奖励经济约束系统 (A4)
// by BCubed 团队 (#48 P3 集成)
//
// 职责：
//   1. 任务灵石奖励的经济约束（总量控制 + 活跃度修正）
//   2. 每日任务奖励预算管理（按境界分层）
//   3. 玩家每日任务灵石获取上限（防刷）
//   4. 任务奖励与 MONEY_D 产出记录联动
//   5. 通胀状态下的奖励增益/衰减
//   6. 任务失败惩罚经济影响
//
// 设计文档：02-扩充内容/02-经济与资源.md §5、§7
//          02-扩充内容/02-任务链与奖励曲线.md §6.3、§7.3
//
// 调用关系：
//   QUEST_D / MAIN_QUEST_D / DAILY_TASK_D 调用本进程计算奖项
//   本进程调用 MONEY_D 记录产出/消耗
//   本进程查询 INFLATION_D 获取产出增益系数

#include <ansi.h>
#include <economy_lifecycle.h>
#include <globals.h>

inherit F_DBASE;
inherit F_SAVE;

// ---------- 数据结构 ----------

// 境界索引映射表：字符串 → 索引值
nosave mapping realm_index_map = ([
  "qige":     0,
  "zhuji":    1,
  "jiedan":   2,
  "yuanying": 3,
  "huashen":  4,
]);

// 每日预算余额（动态跟踪）
// mapping: ([ "qige": int, "zhuji": int, ... ])
nosave mapping daily_budget_balance = ([]);
nosave int    last_reset_day;          // 上次重置的天数

// 玩家每日奖励记录
// 存储在玩家对象的 quest_economy/daily/ 路径下

// ---------- 初始化 ----------

void create()
{
  seteuid(getuid());
  set("name", "任务奖励经济系统");
  set("id", "quest_economy");

  restore();

  // 初始化每日预算
  if (!mapp(daily_budget_balance))
    daily_budget_balance = ([]);

  check_day_rollover();

  // 启动自动调节循环
  remove_call_out("auto_adjust_loop");
  call_out("auto_adjust_loop", AUTO_ADJUST_INTERVAL);
}

string query_save_file()
{
  return "/data/quest_economy";
}

// ---------- 每日重置 ----------

void check_day_rollover()
{
  int today = time() / 86400;
  if (today != last_reset_day)
  {
    last_reset_day = today;
    reset_daily_budgets();
  }
}

void reset_daily_budgets()
{
  // 基于当前在线玩家数重新计算每日预算
  int player_count = MONEY_D->query_daily_production_cap();
  // 预算 = 每日产出上限 × 任务灵石占比上限
  int total_budget = to_int(to_float(player_count) * QUEST_COIN_PCT_CAP);

  daily_budget_balance = ([
    "qige":     to_int(to_float(total_budget) * 0.25),
    "zhuji":    to_int(to_float(total_budget) * 0.30),
    "jiedan":   to_int(to_float(total_budget) * 0.25),
    "yuanying": to_int(to_float(total_budget) * 0.15),
    "huashen":  to_int(to_float(total_budget) * 0.05),
  ]);

  save();
}

// ---------- 奖励计算 ----------

// 计算任务灵石奖励
// 参数:
//   player    - 玩家对象
//   quest_type - 任务类型: "daily" / "main" / "side" / "weekly"
//   base_coin  - 基准灵石奖励（未经经济约束的原始值）
// 返回:
//   经过经济约束调整后的实际灵石奖励
int calculate_quest_coin_reward(object player, string quest_type, int base_coin)
{
  int realm_idx;
  float econ_mod = 1.0;
  float activity_mod = 1.0;
  float budget_mod = 1.0;
  float final_mod;

  // 1. 检查每日滚动
  check_day_rollover();

  // 2. 获取玩家境界索引
  realm_idx = get_player_realm_index(player);

  // 3. 经济状态修正（从通胀系统获取）
  if (find_object(INFLATION_D))
  {
    float boost = INFLATION_D->query_output_boost();
    econ_mod = boost;
  }

  // 4. 活跃度修正
  int online_time = player->query("online_time");
  activity_mod = query_activity_modifier(online_time);

  // 5. 预算余额修正（确保不超每日总预算）
  string realm_key = get_realm_key(realm_idx);
  int budget_remaining = daily_budget_balance[realm_key];
  if (budget_remaining <= 0)
    return 0;  // 该境界预算已耗尽

  // 如果奖励超过剩余预算，裁剪到剩余预算
  if (base_coin > budget_remaining)
    base_coin = budget_remaining;

  // 6. 计算综合修正系数
  final_mod = econ_mod * activity_mod;
  // 确保修正系数不低于 0.3（设下限防归零）
  if (final_mod < 0.3) final_mod = 0.3;
  if (final_mod > 2.0) final_mod = 2.0;

  int final_coin = to_int(to_float(base_coin) * final_mod);

  // 7. 检查玩家个人日上限
  int player_cap = QUEST_DAILY_PLAYER_CAP(realm_idx);
  int player_today = player->query("quest_economy/daily/coin_earned");
  int today_key = time() / 86400;
  int player_day = player->query("quest_economy/daily/day");

  if (player_day != today_key)
  {
    // 新的一天，重置
    player->set("quest_economy/daily/day", today_key);
    player->set("quest_economy/daily/coin_earned", 0);
    player_today = 0;
  }

  int remaining_cap = player_cap - player_today;
  if (remaining_cap <= 0)
    return 0;  // 已达个人日上限

  if (final_coin > remaining_cap)
    final_coin = remaining_cap;

  // 8. 扣除预算
  daily_budget_balance[realm_key] -= final_coin;

  // 9. 记录到玩家数据
  player->add("quest_economy/daily/coin_earned", final_coin);

  // 10. 记录产出到经济监控（通过 MONEY_D）
  if (find_object(MONEY_D))
    MONEY_D->add_production("quest_" + quest_type, final_coin, realm_key);

  // 11. 保存
  save();

  return final_coin;
}

// 计算任务失败惩罚（灵石扣减）
// 返回实际扣减灵石数量
int calculate_quest_fail_penalty(object player, int quest_coin)
{
  int penalty;

  if (quest_coin <= 0)
    return 0;

  penalty = to_int(to_float(quest_coin) * QUEST_FAIL_COIN_PENALTY);
  if (penalty < 1) penalty = 1;

  // 记录消耗到经济监控
  if (find_object(MONEY_D))
    MONEY_D->add_consumption("quest_fail_penalty", penalty, "all");

  return penalty;
}

// 计算任务声望奖励（带经济约束）
int calculate_quest_reputation(object player, int base_rep, string faction)
{
  // 基础声望计算,带经济约束
  float econ_mod = 1.0;

  // 通胀状态下声望产出调整
  if (find_object(INFLATION_D))
  {
    float boost = INFLATION_D->query_output_boost();
    // 声望奖励也受经济产出增益影响（同向调整）
    econ_mod = boost;
  }

  int final = to_int(to_float(base_rep) * econ_mod);
  if (final < 1) final = 1;

  return final;
}

// ---------- 查询接口 ----------

// 获取玩家可用的每日灵石配额
int query_player_daily_quota(object player)
{
  int realm_idx = get_player_realm_index(player);
  int today_key = time() / 86400;
  int player_day = player->query("quest_economy/daily/day");

  if (player_day != today_key)
    return QUEST_DAILY_PLAYER_CAP(realm_idx);

  int earned = player->query("quest_economy/daily/coin_earned");
  int cap = QUEST_DAILY_PLAYER_CAP(realm_idx);
  int remaining = cap - earned;
  return remaining > 0 ? remaining : 0;
}

// 获取该境界今日剩余预算
int query_realm_remaining_budget(string realm)
{
  check_day_rollover();
  return daily_budget_balance[realm];
}

// 获取全服今日已用任务奖励预算
int query_total_daily_budget_used()
{
  check_day_rollover();
  int total = 0;
  foreach (string realm, int balance in daily_budget_balance)
  {
    total += balance;
  }
  return total;
}

// 活跃度修正系数
float query_activity_modifier(int online_minutes)
{
  if (online_minutes < 30)  return ACTIVITY_MOD_MIN;
  if (online_minutes < 120) return ACTIVITY_MOD_LOW;
  if (online_minutes < 240) return ACTIVITY_MOD_NORMAL;
  return ACTIVITY_MOD_HIGH;
}

// 获取玩家境界索引
int get_player_realm_index(object player)
{
  int exp = player->query("combat_exp");

  // 按经验估算境界
  if (exp < 100000)      return 0;  // 炼气
  if (exp < 1000000)     return 1;  // 筑基
  if (exp < 10000000)    return 2;  // 结丹
  if (exp < 50000000)    return 3;  // 元婴
  return 4;                          // 化神+
}

// 境界key字符串转换
string get_realm_key(int realm_idx)
{
  switch (realm_idx)
  {
  case 0:  return "qige";
  case 1:  return "zhuji";
  case 2:  return "jiedan";
  case 3:  return "yuanying";
  default: return "huashen";
  }
}

// ---------- 自动调节循环 ----------

void auto_adjust_loop()
{
  // 每过一段时间自动调整预算分配
  // 如果某个境界的预算长期用不完,可以重新分配给其他境界
  check_day_rollover();

  // 重新安排循环
  remove_call_out("auto_adjust_loop");
  call_out("auto_adjust_loop", AUTO_ADJUST_INTERVAL);
}

// ---------- 报告接口 ----------

// 获取任务经济报告
string query_report()
{
  check_day_rollover();

  string msg = sprintf(
    HIC "╔════════════════════════════════╗\n" NOR
    HIC "║  " HIW "【任务经济系统报告】" HIC "              ║\n" NOR
    HIC "╚════════════════════════════════╝\n" NOR
    "\n"
    "【每日预算余额（下品灵石）】\n"
  );

  foreach (string realm, int balance in daily_budget_balance)
  {
    string realm_name;
    switch (realm)
    {
    case "qige":     realm_name = "炼气期"; break;
    case "zhuji":    realm_name = "筑基期"; break;
    case "jiedan":   realm_name = "结丹期"; break;
    case "yuanying": realm_name = "元婴期"; break;
    case "huashen":  realm_name = "化神期+"; break;
    default:         realm_name = realm; break;
    }
    msg += sprintf("  %-8s : %d\n", realm_name, balance);
  }

  if (find_object(INFLATION_D))
  {
    msg += sprintf(
      "\n【经济状态参考】\n"
      "  产出增益系数: %.2f\n"
      "  产出/消耗比:  %.2f\n",
      INFLATION_D->query_output_boost(),
      INFLATION_D->query_output_consumption_ratio()
    );
  }

  return msg;
}

// 强制保存
void force_save()
{
  save();
}
