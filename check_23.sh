#!/bin/bash
# Author_Check for #23 (声望互动系统) — 验收检查
# 验证子 ticket #34 (声望获取与等级体系) + #35 (阵营交互与商店) 代码已合入主干
set -e
errors=0

echo "=== #23 声望互动系统 — 验收检查 ==="
echo ""

# ============================================================
# 1. 文件存在性检查 (子 ticket #34 + #35 关键文件)
# ============================================================
echo "--- 1/6: 关键文件存在性 ---"
for f in \
  adm/daemons/reputation_d.c \
  adm/daemons/shop_d.c \
  cmds/usr/faction.c \
  include/reputation.h
do
  if [ -f "$f" ]; then
    echo "  OK: $f 存在"
  else
    echo "  FAIL: $f 不存在"
    errors=$((errors+1))
  fi
done
echo ""

# ============================================================
# 2. 声望常量完整性 (reputation.h)
# ============================================================
echo "--- 2/6: 声望常量完整性 (reputation.h) ---"

# 9 个声望等级
for level in DEADLY HOSTILE COLD NEUTRAL FRIENDLY TRUST RESPECT ADORE LEGENDARY; do
  if grep -q "REP_LEVEL_${level}" include/reputation.h; then
    echo "  OK: REP_LEVEL_${level} 已定义"
  else
    echo "  FAIL: REP_LEVEL_${level} 缺失"
    errors=$((errors+1))
  fi
done

# 10 个数值阈值
for val in DEADLY HOSTILE COLD NEUTRAL_LOW NEUTRAL_HIGH FRIENDLY TRUST RESPECT ADORE LEGENDARY; do
  if grep -q "REP_VALUE_${val}" include/reputation.h; then
    echo "  OK: REP_VALUE_${val} 已定义"
  else
    echo "  FAIL: REP_VALUE_${val} 缺失"
    errors=$((errors+1))
  fi
done

# 5 个商店层级
for tier in BASIC INTERMEDIATE ADVANCED CORE SECRET; do
  if grep -q "SHOP_TIER_${tier}" include/reputation.h; then
    echo "  OK: SHOP_TIER_${tier} 已定义"
  else
    echo "  FAIL: SHOP_TIER_${tier} 缺失"
    errors=$((errors+1))
  fi
done

# 6 个境界每日声望上限
for cap in QIYIN ZHUIJI JIEDAN YUANYING HUASHEN LIANXU; do
  if grep -q "REP_DAILY_CAP_${cap}" include/reputation.h; then
    echo "  OK: REP_DAILY_CAP_${cap} 已定义"
  else
    echo "  FAIL: REP_DAILY_CAP_${cap} 缺失"
    errors=$((errors+1))
  fi
done

# 9 个折扣率
for disc in DEADLY HOSTILE COLD NEUTRAL FRIENDLY TRUST RESPECT ADORE LEGENDARY; do
  if grep -q "REP_DISCOUNT_${disc}" include/reputation.h; then
    echo "  OK: REP_DISCOUNT_${disc} 已定义"
  else
    echo "  FAIL: REP_DISCOUNT_${disc} 缺失"
    errors=$((errors+1))
  fi
done
echo ""

# ============================================================
# 3. reputation_d.c API 完整性
# ============================================================
echo "--- 3/6: 声望守护进程 API 完整性 (reputation_d.c) ---"
R=adm/daemons/reputation_d.c

for func in \
  query_reputation_value \
  query_reputation_level \
  calculate_level \
  get_reputation_level_name \
  add_reputation \
  deduct_reputation \
  apply_mutex \
  query_daily_cap \
  query_discount \
  get_available_actions \
  set_faction_relation \
  query_faction_relation \
  get_race_info \
  query_race_relation_level \
  get_all_races \
  get_all_factions \
  get_faction_info \
  decay_reputation \
  format_reputation_bar
do
  if grep -q "$func" "$R"; then
    echo "  OK: $func() 存在"
  else
    echo "  FAIL: $func() 缺失"
    errors=$((errors+1))
  fi
done
echo ""

# ============================================================
# 4. shop_d.c API 完整性
# ============================================================
echo "--- 4/6: 声望商店 API 完整性 (shop_d.c) ---"
S=adm/daemons/shop_d.c

for func in \
  query_tier_items \
  query_available_items \
  get_item_by_id \
  query_stock \
  buy_item \
  restock_item \
  restock_all
do
  if grep -q "$func" "$S"; then
    echo "  OK: $func() 存在"
  else
    echo "  FAIL: $func() 缺失"
    errors=$((errors+1))
  fi
done
echo ""

# ============================================================
# 5. faction.c 命令完整性 (阵营交互)
# ============================================================
echo "--- 5/6: 阵营交互命令完整性 (faction.c) ---"
F=cmds/usr/faction.c

for cmd in list info shop buy race rank relation help; do
  if grep -q "\"$cmd\"" "$F"; then
    echo "  OK: subcommand '$cmd' 存在"
  else
    echo "  FAIL: subcommand '$cmd' 缺失"
    errors=$((errors+1))
  fi
done

if grep -q "show_summary\|main_default" "$F"; then
  echo "  OK: 默认总览 (show_summary/main_default) 存在"
else
  echo "  FAIL: 默认总览不存在"
  errors=$((errors+1))
fi
echo ""

# ============================================================
# 6. 集成与代码质量检查
# ============================================================
echo "--- 6/6: 集成与代码质量 ---"

# globals.h 宏
if grep -q "^#define REPUTATION_D" include/globals.h; then
  echo "  OK: REPUTATION_D 宏在 globals.h 中"
else
  echo "  FAIL: REPUTATION_D 宏缺失"
  errors=$((errors+1))
fi

if grep -q "^#define SHOP_D" include/globals.h; then
  echo "  OK: SHOP_D 宏在 globals.h 中"
else
  echo "  FAIL: SHOP_D 宏缺失"
  errors=$((errors+1))
fi

# 势力数量（>= 6 大势力）
faction_count=$(grep -c '^\s\+"' adm/daemons/reputation_d.c 2>/dev/null | head -1)
# 改用搜索 "name": 来估算势力数量
faction_entries=$(grep -c '"name":' adm/daemons/reputation_d.c 2>/dev/null || true)
if [ "$faction_entries" -ge 6 ]; then
  echo "  OK: 势力数量 = $faction_entries (>= 6)"
else
  echo "  FAIL: 势力数量 = $faction_entries (< 6)"
  errors=$((errors+1))
fi

# 灵界种族数量（>= 5）— race_info mapping 中的条目
race_entries=$(grep -c '^\s\+"' adm/daemons/reputation_d.c 2>/dev/null | head -1)
# 更精确：统计 race_info mapping 中双引号起始的键
race_entries=$(sed -n '/race_info = (\[/,/^\];/p' adm/daemons/reputation_d.c 2>/dev/null | grep -c '^\s*"' || true)
if [ "$race_entries" -ge 5 ]; then
  echo "  OK: 灵界种族数量 = $race_entries (>= 5)"
else
  echo "  FAIL: 灵界种族数量 = $race_entries (< 5)"
  errors=$((errors+1))
fi

# 花括号平衡检查（所有关键文件）
echo ""
echo "--- 花括号平衡 ---"
for file in adm/daemons/reputation_d.c adm/daemons/shop_d.c cmds/usr/faction.c; do
  if [ -f "$file" ]; then
    open_c=$(grep -o '{' "$file" | wc -l | tr -d ' ')
    close_c=$(grep -o '}' "$file" | wc -l | tr -d ' ')
    if [ "$open_c" -eq "$close_c" ]; then
      echo "  OK: $file 花括号平衡 ($open_c/$close_c)"
    else
      echo "  BRACE_MISMATCH: $file ($open_c/$close_c)"
      errors=$((errors+1))
    fi
  fi
done
echo ""

echo "=== 结果: $errors 个错误 ==="
exit $errors
