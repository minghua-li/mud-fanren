#!/bin/bash
# Objective acceptance check for #37 (活跃度与日常任务系统)
# Verifies: file existence, integration points, bracket balance
set -euo pipefail

REF="${1:-origin/main}"
errors=0

echo "=== #37 客观检查 ==="
echo "基准: $REF"
echo ""

# 1. 检查文件存在性
echo "--- 1/4: 文件存在性 ---"
for f in adm/daemons/activity_d.c cmds/usr/activity.c include/activity.h; do
  if git ls-tree "$REF" -- "$f" >/dev/null 2>&1; then
    echo "  ✅ $f"
  else
    echo "  ❌ $f — 不存在"
    errors=$((errors+1))
  fi
done
echo ""

# 2. 检查集成点
echo "--- 2/4: 集成点 ---"
if git grep -q "ACTIVITY_D" "$REF" -- include/globals.h; then
  echo "  ✅ include/globals.h 含 ACTIVITY_D 宏"
else
  echo "  ❌ include/globals.h 缺 ACTIVITY_D 宏"
  errors=$((errors+1))
fi
for pre in adm/etc/preload adm/etc/preload_bak; do
  if git grep -q "activity_d" "$REF" -- "$pre" 2>/dev/null; then
    echo "  ✅ $pre 含 activity_d 预加载"
  else
    echo "  ❌ $pre 缺 activity_d 预加载"
    errors=$((errors+1))
  fi
done
echo ""

# 3. 检查括号平衡
echo "--- 3/4: 括号平衡 ---"
for f in adm/daemons/activity_d.c cmds/usr/activity.c include/activity.h; do
  content=$(git show "$REF:$f" 2>/dev/null) || { echo "  ❌ $f — 无法读取"; errors=$((errors+1)); continue; }
  b_o=$(echo "$content" | awk '{for(i=1;i<=length;i++){c=substr($0,i,1);if(c=="{")n++}}END{print n}')
  b_c=$(echo "$content" | awk '{for(i=1;i<=length;i++){c=substr($0,i,1);if(c=="}")n++}}END{print n}')
  p_o=$(echo "$content" | awk '{for(i=1;i<=length;i++){c=substr($0,i,1);if(c=="(")n++}}END{print n}')
  p_c=$(echo "$content" | awk '{for(i=1;i<=length;i++){c=substr($0,i,1);if(c==")")n++}}END{print n}')
  s_o=$(echo "$content" | awk '{for(i=1;i<=length;i++){c=substr($0,i,1);if(c=="[")n++}}END{print n}')
  s_c=$(echo "$content" | awk '{for(i=1;i<=length;i++){c=substr($0,i,1);if(c=="]")n++}}END{print n}')
  
  ok=true
  [ "$b_o" = "$b_c" ] || { echo "  ❌ $f 花括号 ${b_o}/${b_c} 不平衡"; ok=false; }
  [ "$p_o" = "$p_c" ] || { echo "  ❌ $f 圆括号 ${p_o}/${p_c} 不平衡"; ok=false; }
  [ "$s_o" = "$s_c" ] || { echo "  ❌ $f 方括号 ${s_o}/${s_c} 不平衡"; ok=false; }
  $ok && echo "  ✅ $f — 括号平衡"
  $ok || errors=$((errors+1))
done
echo ""

# 4. 检查头文件常量完整性
echo "--- 4/4: activity.h 常量完整性 ---"
content=$(git show "$REF:include/activity.h" 2>/dev/null) || { echo "  ❌ 无法读取 activity.h"; errors=$((errors+1)); exit 1; }
expected_defines=(
  "ACT_TYPE_HERB" "ACT_TYPE_HUNT" "ACT_TYPE_SECT_CHORE" "ACT_TYPE_ALCHEMY"
  "ACT_TYPE_TRADE" "ACT_TYPE_COURIER" "ACT_TYPE_MINE" "ACT_TYPE_PATROL"
  "ACT_DAILY_THRESHOLD_1" "ACT_DAILY_THRESHOLD_2" "ACT_DAILY_THRESHOLD_3" "ACT_DAILY_THRESHOLD_4"
  "ACT_WEEKLY_THRESHOLD_1" "ACT_WEEKLY_THRESHOLD_2" "ACT_WEEKLY_THRESHOLD_3" "ACT_WEEKLY_THRESHOLD_4"
  "ACT_DAILY_MAX" "ACT_WEEKLY_MAX"
)
for def in "${expected_defines[@]}"; do
  if echo "$content" | grep -q "#define $def"; then
    echo "  ✅ #$def"
  else
    echo "  ❌ 缺 #$def"
    errors=$((errors+1))
  fi
done

echo ""
echo "=== 结果 ==="
if [ $errors -eq 0 ]; then
  echo "✅ 全部通过 (0 errors)"
  exit 0
else
  echo "❌ $errors 个错误"
  exit 1
fi
