#!/bin/bash
# Author_Check for #21 (战斗机制增强) — 综合验收检查
# 验证三个子系统的文件存在性与集成完整性：
#   #28 五行克制系统
#   #29 技能组合与阵法系统
#   #30 PVP基础框架
set -e
errors=0

echo "=== #21 战斗机制增强 — 综合验收检查 ==="
echo ""

# ============================================================
# 第一部分：文件存在性检查（3个子系统的关键文件）
# ============================================================
echo "--- 1/6: 关键文件存在性 ---"

# #28 五行克制系统
for f in include/element.h; do
  if [ -f "$f" ]; then echo "  [OK] #28: $f 存在"; else echo "  [FAIL] #28: $f 不存在"; errors=$((errors+1)); fi
done

# #29 技能组合与阵法系统
for f in include/combat/skill_combo.h feature/formation.c cmds/std/buzhen.c cmds/std/chezhen.c cmds/std/zhenfa.c; do
  if [ -f "$f" ]; then echo "  [OK] #29: $f 存在"; else echo "  [FAIL] #29: $f 不存在"; errors=$((errors+1)); fi
done

# #30 PVP基础框架
for f in adm/daemons/pvpd.c; do
  if [ -f "$f" ]; then echo "  [OK] #30: $f 存在"; else echo "  [FAIL] #30: $f 不存在"; errors=$((errors+1)); fi
done

# 基础战斗文件
for f in adm/daemons/combatd.c feature/attack.c feature/damage.c; do
  if [ -f "$f" ]; then echo "  [OK] 基础: $f 存在"; else echo "  [FAIL] 基础: $f 不存在"; errors=$((errors+1)); fi
done

echo ""

# ============================================================
# 第二部分：globals.h 宏定义检查
# ============================================================
echo "--- 2/6: globals.h 宏定义 ---"

for macro in "COMBAT_D" "PVP_D" "F_ATTACK" "F_FORMATION"; do
  if grep -q "^#define $macro" include/globals.h; then
    echo "  [OK] globals.h: $macro 已定义"
  else
    echo "  [FAIL] globals.h: $macro 未定义"
    errors=$((errors+1))
  fi
done

echo ""

# ============================================================
# 第三部分：#28 五行克制系统 — 功能验证
# ============================================================
echo "--- 3/6: #28 五行克制系统验证 ---"

# 3a. 五行元素常量
ele_count=$(grep -c "^#define ELE_" include/element.h 2>/dev/null || echo 0)
if [ "$ele_count" -ge 5 ]; then
  echo "  [OK] #28: $ele_count 个元素常量 (≥5)"
else
  echo "  [FAIL] #28: 元素常量不足 ($ele_count)"
  errors=$((errors+1))
fi

# 3b. 核心五行常量 (金木水火土)
for ele in "ELE_GOLD" "ELE_WOOD" "ELE_WATER" "ELE_FIRE" "ELE_EARTH"; do
  if grep -q "^#define $ele" include/element.h; then
    echo "  [OK] #28: $ele 已定义"
  else
    echo "  [FAIL] #28: $ele 未定义"
    errors=$((errors+1))
  fi
done

# 3c. 变异属性 (雷冰风)
for ele in "ELE_THUNDER" "ELE_ICE" "ELE_WIND"; do
  if grep -q "^#define $ele" include/element.h; then
    echo "  [OK] #28: $ele 变异属性已定义"
  else
    echo "  [WARN] #28: $ele 变异属性未定义 (可能已重命名)"
  fi
done

# 3d. query_character_element 函数
if grep -q "query_character_element" include/element.h; then
  echo "  [OK] #28: query_character_element 函数存在"
else
  echo "  [FAIL] #28: query_character_element 函数缺失"
  errors=$((errors+1))
fi

# 3e. element.h 被 combatd.c 包含
if grep -q '#include.*element\.h' adm/daemons/combatd.c; then
  echo "  [OK] #28: combatd.c 包含 element.h"
else
  echo "  [FAIL] #28: combatd.c 未包含 element.h"
  errors=$((errors+1))
fi

# 3f. damage.c 包含 element.h
if grep -q '#include.*element\.h' feature/damage.c; then
  echo "  [OK] #28: damage.c 包含 element.h"
else
  echo "  [FAIL] #28: damage.c 未包含 element.h"
  errors=$((errors+1))
fi

# 3g. damage.c 五行相生减伤逻辑
if grep -q "query_element_generates\|reduced" feature/damage.c; then
  echo "  [OK] #28: damage.c 含相生减伤逻辑"
else
  echo "  [WARN] #28: damage.c 未见相生减伤关键词"
fi

echo ""

# ============================================================
# 第四部分：#29 技能组合与阵法系统 — 功能验证
# ============================================================
echo "--- 4/6: #29 技能组合与阵法系统验证 ---"

# 4a. char.c 继承 F_FORMATION
if grep -q "inherit F_FORMATION" inherit/char/char.c; then
  echo "  [OK] #29: char.c 继承 F_FORMATION"
else
  echo "  [FAIL] #29: char.c 未继承 F_FORMATION"
  errors=$((errors+1))
fi

# 4b. combatd.c 包含 skill_combo.h
if grep -q '#include.*skill_combo\.h' adm/daemons/combatd.c; then
  echo "  [OK] #29: combatd.c 包含 skill_combo.h"
else
  echo "  [FAIL] #29: combatd.c 未包含 skill_combo.h"
  errors=$((errors+1))
fi

# 4c. 组合技数量 (≥3)
combo_count=$(grep -c '"pre_skill"' include/combat/skill_combo.h 2>/dev/null || echo 0)
if [ "$combo_count" -ge 3 ]; then
  echo "  [OK] #29: $combo_count 种组合技 (≥3)"
else
  echo "  [FAIL] #29: 组合技不足 ($combo_count < 3)"
  errors=$((errors+1))
fi

# 4d. 冷却管理常量
for c in COOLDOWN_PUBLIC COOLDOWN_INDIE COOLDOWN_COMBO; do
  if grep -q "$c" include/combat/skill_combo.h; then
    echo "  [OK] #29: $c 已定义"
  else
    echo "  [FAIL] #29: $c 未定义"
    errors=$((errors+1))
  fi
done

# 4e. combatd 集成冷却/阵法函数
for func in tick_cooldowns formation_damage_modify; do
  if grep -q "$func" adm/daemons/combatd.c; then
    echo "  [OK] #29: combatd.c 含 $func()"
  else
    echo "  [FAIL] #29: combatd.c 缺 $func()"
    errors=$((errors+1))
  fi
done

# 4f. 阵型常量数量 (至少5种)
formation_count=$(grep -c "^#define FORMATION_" feature/formation.c 2>/dev/null || echo 0)
if [ "$formation_count" -ge 5 ]; then
  echo "  [OK] #29: $formation_count 种阵型 (≥5)"
else
  echo "  [FAIL] #29: 阵型不足 ($formation_count < 5)"
  errors=$((errors+1))
fi

echo ""

# ============================================================
# 第五部分：#30 PVP基础框架 — 功能验证
# ============================================================
echo "--- 5/6: #30 PVP 基础框架验证 ---"

# 5a. pvpd.c 基本结构
if grep -q "calc_rank_by_score\|match_queue\|process_matchmaking" adm/daemons/pvpd.c; then
  echo "  [OK] #30: pvpd.c 含匹配/段位逻辑"
else
  echo "  [FAIL] #30: pvpd.c 缺匹配或段位逻辑"
  errors=$((errors+1))
fi

# 5b. pvpd.c 赛季系统
if grep -q "SEASON_\|season_end\|PVP_SEASON_" adm/daemons/pvpd.c; then
  echo "  [OK] #30: pvpd.c 含赛季系统"
else
  echo "  [FAIL] #30: pvpd.c 缺赛季系统"
  errors=$((errors+1))
fi

# 5c. PVP_D 宏引用正确
if grep -q "PVP_D" adm/daemons/combatd.c; then
  echo "  [OK] #30: combatd.c 引用 PVP_D"
else
  echo "  [WARN] #30: combatd.c 未直接引用 PVP_D (可能通过其他方式集成)"
fi

echo ""

# ============================================================
# 第六部分：语法完整性 — 花括号平衡
# ============================================================
echo "--- 6/6: 花括号平衡检查 ---"

for f in include/element.h include/combat/skill_combo.h feature/formation.c feature/damage.c; do
  if [ -f "$f" ]; then
    open=$(grep -o '{' "$f" | wc -l | tr -d ' ')
    close=$(grep -o '}' "$f" | wc -l | tr -d ' ')
    if [ "$open" -eq "$close" ]; then
      echo "  [OK] $f 花括号平衡 ($open/$close)"
    else
      echo "  [FAIL] $f 花括号不平衡 ($open/$close)"
      errors=$((errors+1))
    fi
  fi
done

# 对 cmd 文件和 pvpd.c 也检查
for f in cmds/std/buzhen.c cmds/std/chezhen.c cmds/std/zhenfa.c adm/daemons/pvpd.c; do
  if [ -f "$f" ]; then
    open=$(grep -o '{' "$f" | wc -l | tr -d ' ')
    close=$(grep -o '}' "$f" | wc -l | tr -d ' ')
    if [ "$open" -eq "$close" ]; then
      echo "  [OK] $f 花括号平衡 ($open/$close)"
    else
      echo "  [FAIL] $f 花括号不平衡 ($open/$close)"
      errors=$((errors+1))
    fi
  fi
done

echo ""
echo "=== 结果: $errors 个错误 ==="
exit $errors
