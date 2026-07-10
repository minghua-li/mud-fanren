// faction_economy_d.c  势力产业税收 + 声望经济 (A5)
// by BCubed 团队 (#48 P3 集成)
//
// 职责：
//   1. 势力产业税收系统（矿脉/坊市/拍卖行/灵脉税收）
//   2. 声望等级消费门槛验证与折扣计算
//   3. 势力任务灵石注入的经济约束
//   4. 势力金库管理与收入分配
//   5. 税收记录与通胀监控联动
//
// 设计文档：02-扩充内容/02-经济与资源.md §4
//          02-扩充内容/02-声望与互动玩法.md §3、§9.3
//          .knowledge/factions/1D-门派种族声望.md
//
// 调用关系：
//   商店NPC/拍卖行/矿点 → 调用本进程进行税收计算
//   本进程调用 MONEY_D 记录回收/消耗

#include <ansi.h>
#include <economy_lifecycle.h>
#include <globals.h>
#include <reputation.h>

inherit F_DBASE;
inherit F_SAVE;

// ---------- 数据结构 ----------

// 势力金库：([ faction_id: int ]) — 累计灵石收入
nosave mapping faction_treasury = ([]);

// 势力日税收记录：([ faction_id: int ])
nosave mapping faction_daily_tax = ([]);
nosave int last_reset_day;

// 税收统计（总览）
nosave mapping tax_stats = ([
  "total_collected":   0,     // 总税收（下品灵石）
  "market_tax":        0,     // 坊市税
  "auction_tax":       0,     // 拍卖行税  
  "mine_tax":          0,     // 矿脉税
  "lingmai_tax":       0,     // 灵脉税
  "faction_released":  0,     // 已分配给势力
  "system_recycled":   0,     // 系统回收部分
]);

// ---------- 初始化 ----------

void create()
{
  seteuid(getuid());
  set("name", "势力经济系统");
  set("id", "faction_economy");

  restore();

  if (!mapp(faction_treasury))
    faction_treasury = ([]);
  if (!mapp(faction_daily_tax))
    faction_daily_tax = ([]);
  if (!mapp(tax_stats))
    tax_stats = ([
      "total_collected":   0,
      "market_tax":        0,
      "auction_tax":       0,
      "mine_tax":          0,
      "lingmai_tax":       0,
      "faction_released":  0,
      "system_recycled":   0,
    ]);

  check_day_rollover();
}

string query_save_file()
{
  return "/data/faction_economy";
}

// ---------- 每日重置 ----------

void check_day_rollover()
{
  int today = time() / 86400;
  if (today != last_reset_day)
  {
    last_reset_day = today;
    faction_daily_tax = ([]);
    save();
  }
}

//=============================================================================
// 一、税收系统
//=============================================================================

// 获取当前适用的税率修正系数（基于通胀状态）
float query_tax_modifier()
{
  float tax_mod = TAX_MOD_HEALTHY;
  if (find_object(INFLATION_D))
  {
    string health = INFLATION_D->query_economy_health();
    switch (health)
    {
    case "healthy":  tax_mod = TAX_MOD_HEALTHY;  break;
    case "warning":  tax_mod = TAX_MOD_WARNING;  break;
    case "critical": tax_mod = TAX_MOD_CRITICAL; break;
    default:         tax_mod = TAX_MOD_HEALTHY;  break;
    }
  }
  return tax_mod;
}

// 计算某笔交易的税额
// 参数:
//   industry_type - 产业类型: "market" / "auction" / "mine" / "lingmai"
//   transaction_amount - 交易金额（下品灵石）
//   player - 玩家对象（可选）
// 返回: 税额（下品灵石）
int calculate_tax(string industry_type, int transaction_amount, object player)
{
  int base_tax;
  float tax_mod = query_tax_modifier();
  float rep_discount = 1.0;

  // 基础税率
  switch (industry_type)
  {
  case FACTION_INDUSTRY_MARKET:
    base_tax = to_int(to_float(transaction_amount) * TAX_MARKET_BASE / 1000.0);
    break;
  case FACTION_INDUSTRY_AUCTION:
    base_tax = to_int(to_float(transaction_amount) * TAX_AUCTION_BASE / 1000.0);
    break;
  case FACTION_INDUSTRY_MINE:
    base_tax = to_int(to_float(transaction_amount) * TAX_MINE_BASE / 1000.0);
    break;
  case FACTION_INDUSTRY_LINGMAI:
    base_tax = to_int(to_float(transaction_amount) * TAX_LINGMAI_BASE / 1000.0);
    break;
  default:
    return 0;
  }

  // 玩家声望折扣 （高声望玩家可以享受税收优惠）
  if (player && find_object(REPUTATION_D))
  {
    int rep_level = REPUTATION_D->query_reputation_level(player, "faction_" + industry_type);
    switch (rep_level)
    {
    case REP_LEVEL_ADORE:     rep_discount = 0.60; break;
    case REP_LEVEL_RESPECT:   rep_discount = 0.80; break;
    case REP_LEVEL_TRUST:     rep_discount = 0.90; break;
    case REP_LEVEL_FRIENDLY:  rep_discount = 0.95; break;
    default:                  rep_discount = 1.00; break;
    }
  }

  float final_tax = to_float(base_tax) * tax_mod * rep_discount;
  if (final_tax < 1.0 && transaction_amount > 0)
    final_tax = 1.0;  // 最低 1 灵石

  return to_int(final_tax);
}

// 收取税收并记录
// 参数:
//   faction_id - 势力ID
//   industry_type - 产业类型
//   transaction_amount - 交易金额
//   player - 玩家（可选，影响声望折扣）
// 返回: 实际收取的税额
int collect_tax(string faction_id, string industry_type, int transaction_amount, object player)
{
  check_day_rollover();

  int tax = calculate_tax(industry_type, transaction_amount, player);
  if (tax <= 0) return 0;

  // 记录到势力日税收
  if (!mapp(faction_daily_tax))
    faction_daily_tax = ([]);
  faction_daily_tax[faction_id] += tax;

  // 记录到总统计
  tax_stats["total_collected"] += tax;
  switch (industry_type)
  {
  case FACTION_INDUSTRY_MARKET:  tax_stats["market_tax"]  += tax; break;
  case FACTION_INDUSTRY_AUCTION: tax_stats["auction_tax"] += tax; break;
  case FACTION_INDUSTRY_MINE:    tax_stats["mine_tax"]    += tax; break;
  case FACTION_INDUSTRY_LINGMAI: tax_stats["lingmai_tax"] += tax; break;
  }

  // 收入分配
  int faction_share   = to_int(to_float(tax) * FACTION_INCOME_FACTION);
  int contribute_share = to_int(to_float(tax) * FACTION_INCOME_CONTRIBUTE);
  int maintenance_share = to_int(to_float(tax) * FACTION_INCOME_MAINTENANCE);
  int system_share    = to_int(to_float(tax) * FACTION_INCOME_SYSTEM);

  // 势力金库
  if (!mapp(faction_treasury))
    faction_treasury = ([]);
  faction_treasury[faction_id] += faction_share;
  tax_stats["faction_released"] += faction_share;

  // 系统回收（防通胀）
  tax_stats["system_recycled"] += system_share;

  // 记录灵石消耗到通胀监控
  if (find_object(MONEY_D))
  {
    MONEY_D->add_consumption("tax_" + industry_type, tax, faction_id);
  }

  save();
  return tax;
}

//=============================================================================
// 二、声望等级消费门槛
//=============================================================================

// 检查玩家是否达到指定消费层级的声望要求
// 参数:
//   player - 玩家对象
//   faction - 势力ID
//   shop_tier - 商店层级 (0-4, 对应 SHOP_TIER_BASIC ~ SHOP_TIER_SECRET)
// 返回: 1=通过, 0=未达门槛
int check_repurchase_threshold(object player, string faction, int shop_tier)
{
  if (!find_object(REPUTATION_D))
    return 0;  // REPUTATION_D 未加载属系统错误，拒绝通过

  int required_level;
  switch (shop_tier)
  {
  case SHOP_TIER_BASIC:       required_level = REP_THRESHOLD_BUY_BASIC;  break;
  case SHOP_TIER_INTERMEDIATE: required_level = REP_THRESHOLD_BUY_MID;   break;
  case SHOP_TIER_ADVANCED:    required_level = REP_THRESHOLD_BUY_ADV;    break;
  case SHOP_TIER_CORE:        required_level = REP_THRESHOLD_BUY_CORE;   break;
  case SHOP_TIER_SECRET:      required_level = REP_THRESHOLD_BUY_SECRET; break;
  default:                    required_level = 0;                         break;
  }

  int current_level = REPUTATION_D->query_reputation_level(player, faction);
  return current_level >= required_level ? 1 : 0;
}

// 获取玩家在指定势力的折扣率
// 返回: 折扣乘数 (0.6=6折, 1.0=原价, 3.0=3倍价)
float query_faction_discount(object player, string faction)
{
  if (find_object(REPUTATION_D))
    return REPUTATION_D->query_discount(faction, player);

  return 1.0;
}

//=============================================================================
// 三、势力任务灵石注入
//=============================================================================

// 计算势力任务的灵石奖励
// 势力任务灵石注入按经济产出增益调整
int calculate_faction_task_coin(object player, int base_coin, string faction)
{
  float econ_mod = 1.0;
  float rep_mod = 1.0;

  // 经济状态修正
  if (find_object(INFLATION_D))
  {
    float boost = INFLATION_D->query_output_boost();
    econ_mod = boost;
  }

  // 声望修正: 高声望玩家从势力任务获得更多灵石
  if (find_object(REPUTATION_D))
  {
    int rep_level = REPUTATION_D->query_reputation_level(player, faction);
    switch (rep_level)
    {
    case REP_LEVEL_TRUST:     rep_mod = 1.10; break;
    case REP_LEVEL_RESPECT:   rep_mod = 1.25; break;
    case REP_LEVEL_ADORE:     rep_mod = 1.40; break;
    case REP_LEVEL_LEGENDARY: rep_mod = 1.60; break;
    default:                  rep_mod = 1.00; break;
    }
  }

  float final_mod = econ_mod * rep_mod;
  if (final_mod < 0.3) final_mod = 0.3;
  if (final_mod > 2.5) final_mod = 2.5;

  int final_coin = to_int(to_float(base_coin) * final_mod);

  // 记录产出
  if (find_object(MONEY_D))
    MONEY_D->add_production("faction_task_" + faction, final_coin, "faction");

  return final_coin;
}

// 计算势力任务声望奖励
int calculate_faction_task_reputation(object player, int base_rep, string faction)
{
  float econ_mod = 1.0;

  if (find_object(INFLATION_D))
  {
    float boost = INFLATION_D->query_output_boost();
    econ_mod = boost;
  }

  return to_int(to_float(base_rep) * econ_mod);
}

//=============================================================================
// 四、势力金库接口
//=============================================================================

// 查询势力金库余额
int query_faction_balance(string faction_id)
{
  if (!mapp(faction_treasury))
    return 0;
  return faction_treasury[faction_id];
}

// 从势力金库支出（用于驻地维护、战争开销等）
// 返回: 1=成功, 0=余额不足
int spend_from_treasury(string faction_id, int amount, string reason)
{
  if (!mapp(faction_treasury))
    return 0;

  int balance = faction_treasury[faction_id];
  if (balance < amount)
    return 0;

  faction_treasury[faction_id] -= amount;

  // 记录消耗
  if (find_object(MONEY_D))
    MONEY_D->add_consumption("faction_" + reason, amount, faction_id);

  save();
  return 1;
}

// 向势力金库存入灵石
void deposit_to_treasury(string faction_id, int amount, string source)
{
  if (!mapp(faction_treasury))
    faction_treasury = ([]);

  faction_treasury[faction_id] += amount;

  // 记录产出
  if (find_object(MONEY_D))
    MONEY_D->add_production("faction_deposit_" + source, amount, faction_id);

  save();
}

//=============================================================================
// 五、查询与报告
//=============================================================================

// 获取全服税收报告
string query_tax_report()
{
  string msg = sprintf(
    HIC "╔════════════════════════════════╗\n" NOR
    HIC "║  " HIW "【势力经济税收报告】" HIC "              ║\n" NOR
    HIC "╚════════════════════════════════╝\n" NOR
    "\n"
    "【税收累计统计】\n"
    "  总税收:   %d 下品灵石\n"
    "  坊市税:   %d\n"
    "  拍卖行税: %d\n"
    "  矿脉税:   %d\n"
    "  灵脉税:   %d\n"
    "\n"
    "【分配情况】\n"
    "  势力金库: %d (%.0f%%)\n"
    "  系统回收: %d (%.0f%%)\n"
    "\n"
    "【当前税率修正】\n"
    "  税率系数: %.1f\n",
    tax_stats["total_collected"],
    tax_stats["market_tax"],
    tax_stats["auction_tax"],
    tax_stats["mine_tax"],
    tax_stats["lingmai_tax"],
    tax_stats["faction_released"],
    tax_stats["total_collected"] > 0 ?
      to_float(tax_stats["faction_released"]) / to_float(tax_stats["total_collected"]) * 100.0 : 0.0,
    tax_stats["system_recycled"],
    tax_stats["total_collected"] > 0 ?
      to_float(tax_stats["system_recycled"]) / to_float(tax_stats["total_collected"]) * 100.0 : 0.0,
    query_tax_modifier()
  );

  return msg;
}

// 获取所有势力的金库概览
string query_treasury_summary()
{
  string msg = "【势力金库概览】\n";
  if (!mapp(faction_treasury) || sizeof(faction_treasury) == 0)
  {
    msg += "  （暂无数据）\n";
    return msg;
  }

  foreach (string faction, int balance in faction_treasury)
  {
    msg += sprintf("  %-20s : %d 下品灵石\n", faction, balance);
  }

  return msg;
}

// 强制保存
void force_save()
{
  save();
}
