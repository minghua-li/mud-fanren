#!/bin/bash
# Author_Check for #20 (灵根养成系统, 含 #26, #27)
# 验证各子系统的文件是否就位、关键函数是否存在、与已有系统集成是否正确
set -e
errors=0

echo "=== #20 灵根养成系统（#26, #27）— 验收检查 ==="
echo ""

# --- 1. 文件存在性（在 origin/main 中） ---
echo "--- 1/5: 文件存在性 (origin/main) ---"
files=(
  adm/daemons/root_refine_d.c
  feature/attribute.c
  include/spirit_root.h
  kungfu/condition/root_debuff_dan_du.c
  cmds/usr/root_test.c
)
for f in "${files[@]}"; do
  if git ls-tree -r origin/main --name-only 2>/dev/null | grep -qx "$f"; then
    echo "  OK: $f 存在于 origin/main"
  else
    echo "  FAIL: $f 不在 origin/main"
    errors=$((errors+1))
  fi
done

combined_files=(
  include/globals.h
)
for f in "${combined_files[@]}"; do
  if git ls-tree -r origin/main --name-only 2>/dev/null | grep -qx "$f"; then
    echo "  OK: $f 存在于 origin/main"
  else
    echo "  FAIL: $f 不在 origin/main"
    errors=$((errors+1))
  fi
done
echo ""

# --- 2. 灵根基底常量（spirit_root.h） ---
echo "--- 2/5: 灵根常量与宏 ---"
constants=(
  SPIRIT_ROOT_NONE
  SPIRIT_ROOT_PSEUDO
  SPIRIT_ROOT_FAKE
  SPIRIT_ROOT_TRUE
  SPIRIT_ROOT_VARIANT
  SPIRIT_ROOT_HEAVENLY
  SPIRIT_ROOT_SPEED_FACTOR
  SPIRIT_ROOT_BASE_STRENGTH
  SPIRIT_ROOT_MAX_STRENGTH
  SPIRIT_ROOT_BREAKTHROUGH_QUALITY_FACTOR
)
src_content=$(git show origin/main:include/spirit_root.h 2>/dev/null) || {
  echo "  FAIL: 无法读取 include/spirit_root.h 从 origin/main"
  errors=$((errors+1))
  src_content=""
}
if [ -n "$src_content" ]; then
  for c in "${constants[@]}"; do
    if echo "$src_content" | grep -q "#define $c"; then
      echo "  OK: $c 已定义"
    else
      echo "  FAIL: $c 未定义"
      errors=$((errors+1))
    fi
  done
fi
echo ""

# --- 3. globals.h 集成 ---
echo "--- 3/5: 全局宏集成 ---"
globals=$(git show origin/main:include/globals.h 2>/dev/null) || {
  echo "  FAIL: 无法读取 include/globals.h"
  errors=$((errors+1))
  globals=""
}
if [ -n "$globals" ]; then
  if echo "$globals" | grep -q "ROOT_REFINE_D"; then
    echo "  OK: ROOT_REFINE_D 在 globals.h 中"
  else
    echo "  FAIL: ROOT_REFINE_D 不在 globals.h"
    errors=$((errors+1))
  fi
fi
echo ""

# --- 4. 核心函数与集成验证 ---
echo "--- 4/5: 核心函数与系统集成 ---"

# root_refine_d.c 核心函数
d_content=$(git show origin/main:adm/daemons/root_refine_d.c 2>/dev/null) || {
  echo "  FAIL: 无法读取 adm/daemons/root_refine_d.c"
  errors=$((errors+1))
  d_content=""
}
if [ -n "$d_content" ]; then
  funcs=(
    query_spirit_root_quality
    query_spirit_root_strength
    add_spirit_root_exp
    do_breakthrough
    do_refine
    refine_apply_result
    check_quality_upgrade
    do_quality_upgrade
  )
  for func in "${funcs[@]}"; do
    if echo "$d_content" | grep -q "$func"; then
      echo "  OK: 函数 $func() 在 root_refine_d.c 中存在"
    else
      echo "  FAIL: $func() 未在 root_refine_d.c 中找到"
      errors=$((errors+1))
    fi
  done
fi

# attribute.c 集成灵根
attr_content=$(git show origin/main:feature/attribute.c 2>/dev/null) || {
  echo "  FAIL: 无法读取 feature/attribute.c"
  errors=$((errors+1))
  attr_content=""
}
if [ -n "$attr_content" ]; then
  # 检查 #include <spirit_root.h>
  if echo "$attr_content" | grep -q '#include.*spirit_root'; then
    echo "  OK: attribute.c 引用了 spirit_root.h"
  else
    echo "  FAIL: attribute.c 未引用 spirit_root.h"
    errors=$((errors+1))
  fi
  # 检查 generate_spirit_root 函数
  if echo "$attr_content" | grep -q "generate_spirit_root"; then
    echo "  OK: attribute.c 有 generate_spirit_root() 函数"
  else
    echo "  FAIL: attribute.c 缺少 generate_spirit_root()"
    errors=$((errors+1))
  fi
  # 检查 query_spirit_root_speed
  if echo "$attr_content" | grep -q "query_spirit_root_speed"; then
    echo "  OK: attribute.c 有 query_spirit_root_speed() 函数"
  else
    echo "  FAIL: attribute.c 缺少 query_spirit_root_speed()"
    errors=$((errors+1))
  fi
  # 检查 query_spirit_root_display
  if echo "$attr_content" | grep -q "query_spirit_root_display"; then
    echo "  OK: attribute.c 有 query_spirit_root_display() 函数"
  else
    echo "  FAIL: attribute.c 缺少 query_spirit_root_display()"
    errors=$((errors+1))
  fi
fi

# cmds/usr/root_test.c 文件存在
if git ls-tree -r origin/main --name-only 2>/dev/null | grep -qx "cmds/usr/root_test.c"; then
  echo "  OK: 玩家命令 root_test.c 存在"
else
  echo "  FAIL: cmds/usr/root_test.c 不存在"
  errors=$((errors+1))
fi

# root_debuff_dan_du.c 桥接条件系统
debuff_content=$(git show origin/main:kungfu/condition/root_debuff_dan_du.c 2>/dev/null) || {
  echo "  FAIL: 无法读取 root_debuff_dan_du.c"
  errors=$((errors+1))
  debuff_content=""
}
if [ -n "$debuff_content" ]; then
  # 检查是否为有效的条件守护进程(含 create/dispel_message 等标准接口)
  # 标准 condition 接口: update_condition() + F_CLEAN_UP 继承
  if echo "$debuff_content" | grep -q "update_condition"; then
    echo "  OK: root_debuff_dan_du.c 有 update_condition() 标准接口"
  else
    echo "  FAIL: root_debuff_dan_du.c 缺少 update_condition()"
    errors=$((errors+1))
  fi
  if echo "$debuff_content" | grep -q "F_CLEAN_UP"; then
    echo "  OK: root_debuff_dan_du.c 继承 F_CLEAN_UP"
  else
    echo "  FAIL: root_debuff_dan_du.c 缺少 F_CLEAN_UP 继承"
    errors=$((errors+1))
  fi
fi
echo ""

# --- 5. 花括号平衡 ---
echo "--- 5/5: 花括号平衡 ---"
check_files=(adm/daemons/root_refine_d.c feature/attribute.c include/spirit_root.h)
for f in "${check_files[@]}"; do
  content=$(git show origin/main:"$f" 2>/dev/null) || {
    echo "  FAIL: 无法读取 $f"
    errors=$((errors+1))
    continue
  }
  open=$(echo "$content" | grep -o '{' | wc -l | tr -d ' ')
  close=$(echo "$content" | grep -o '}' | wc -l | tr -d ' ')
  if [ "$open" -eq "$close" ]; then
    echo "  OK: $f 花括号平衡 ($open/$close)"
  else
    echo "  BRACE_MISMATCH: $f ($open/$close)"
    errors=$((errors+1))
  fi
done
echo ""

# 总结
echo "=== 结果: $errors 个错误 ==="
exit $errors
