#!/bin/bash
# Author_Check for #29 (技能组合与阵法系统)
set -e
errors=0

echo "=== #29 技能组合与阵法系统 — 验收检查 ==="

# 文件存在性
for f in include/combat/skill_combo.h feature/formation.c cmds/std/buzhen.c cmds/std/chezhen.c cmds/std/zhenfa.c; do
  if [ ! -f "$f" ]; then echo "MISSING: $f"; errors=$((errors+1)); else echo "OK: $f exists"; fi
done

# globals.h 宏 (F_FORMATION)
if grep -q "^#define F_FORMATION" include/globals.h; then echo "OK: F_FORMATION in globals.h"; else echo "FAIL: F_FORMATION missing"; errors=$((errors+1)); fi

# char.c 继承 F_FORMATION
if grep -q "inherit F_FORMATION" inherit/char/char.c; then echo "OK: char.c inherits F_FORMATION"; else echo "FAIL: char.c missing inherit"; errors=$((errors+1)); fi

# combatd.c include skill_combo.h
if grep -q "#include.*skill_combo.h" adm/daemons/combatd.c; then echo "OK: combatd.c includes skill_combo.h"; else echo "FAIL: combatd.c missing include"; errors=$((errors+1)); fi

# 验收标准1: 3种以上组合技 (combo_table)
combo_count=$(grep -c '"pre_skill"' include/combat/skill_combo.h 2>/dev/null || true)
if [ "$combo_count" -ge 3 ]; then echo "OK: $combo_count combos defined (>=3)"; else echo "FAIL: only $combo_count combos (<3)"; errors=$((errors+1)); fi

# 验收标准2: 冷却管理
for c in COOLDOWN_PUBLIC COOLDOWN_INDIE COOLDOWN_COMBO; do
  if grep -q "$c" include/combat/skill_combo.h; then echo "OK: $c defined"; else echo "FAIL: $c missing"; errors=$((errors+1)); fi
done

# 验收标准2: 冷却常量
for c in CD_PUBLIC_DEFAULT CD_BASIC_SPELL CD_MID_SPELL CD_HIGH_SPELL CD_ULTIMATE_SPELL; do
  if grep -q "$c" include/combat/skill_combo.h; then echo "OK: $c defined"; else echo "FAIL: $c missing"; errors=$((errors+1)); fi
done

# 验收标准2: combatd 集成 (冷却递减 + combo清除)
for func in tick_cooldowns clear_combo_temp; do
  if grep -q "$func" adm/daemons/combatd.c; then echo "OK: combatd.c function $func() exists"; else echo "FAIL: combatd.c function $func() missing"; errors=$((errors+1)); fi
done

# 验收标准3: 7种阵型
for f in FORMATION_NONE FORMATION_JULING FORMATION_DIANDAO FORMATION_TIANGANG FORMATION_DAGENG FORMATION_SANCAI FORMATION_SIXIANG FORMATION_WUXING; do
  if grep -q "$f" feature/formation.c; then echo "OK: $f defined"; else echo "FAIL: $f missing"; errors=$((errors+1)); fi
done

# 验收标准3: 阵法效果 (阵型数据表)
for e in defense_bonus damage_share mana_regen sword_damage front_share back_bonus; do
  if grep -q "$e" feature/formation.c; then echo "OK: formation effect $e exists"; else echo "WARN: $e not found in formation.c (may have different naming)"; fi
done

# 验收标准3: 阵眼
if grep -q "formation_eye\|阵眼\|eye.*formation" feature/formation.c; then echo "OK: formation eye mechanism"; else echo "WARN: formation eye not found (may be named differently)"; fi

# 验收标准3: combatd 阵法集成 (formation_damage_modify)
if grep -q "formation_damage_modify" adm/daemons/combatd.c; then echo "OK: combatd integrates formation damage modify"; else echo "FAIL: formation_damage_modify not in combatd.c"; errors=$((errors+1)); fi
if grep -q "query_formation_type\|formation_tick" adm/daemons/combatd.c; then echo "OK: combatd integrates formation tick"; else echo "FAIL: formation tick not in combatd.c"; errors=$((errors+1)); fi

# 验收标准4: 3个命令文件存在
for cmd in buzhen chezhen zhenfa; do
  if [ -f "cmds/std/${cmd}.c" ]; then echo "OK: command $cmd exists"; else echo "FAIL: cmds/std/${cmd}.c missing"; errors=$((errors+1)); fi
done

# 花括号平衡
for f in feature/formation.c cmds/std/buzhen.c cmds/std/chezhen.c cmds/std/zhenfa.c; do
  if [ -f "$f" ]; then
    open=$(grep -o '{' "$f" | wc -l)
    close=$(grep -o '}' "$f" | wc -l)
    if [ "$open" -eq "$close" ]; then echo "OK: $f brace balance ($open/$close)"; else echo "BRACE_MISMATCH: $f ($open/$close)"; errors=$((errors+1)); fi
  fi
done

echo "=== 结果: $errors 个错误 ==="
exit $errors
