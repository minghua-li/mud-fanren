#!/bin/bash
# Author_Check for #43 — GATE_PROBE 探针文件内容校验
set -e
errors=0

echo "=== #43 GATE_PROBE 探针文件 — 验收检查 ==="
echo ""

FILE="doc/GATE_PROBE.md"

# 1. 文件存在性
echo "--- 1/2: 文件存在性 ---"
if [ -f "$FILE" ]; then
  echo "  OK: $FILE 存在"
else
  echo "  FAIL: $FILE 不存在"
  errors=$((errors+1))
fi
echo ""

# 2. 内容精确匹配
echo "--- 2/2: 内容精确匹配 ---"
if [ ! -f "$FILE" ]; then
  errors=$((errors+1))
else
  # 检查行数恰为2
  line_count=$(wc -l < "$FILE" | tr -d ' ')
  if [ "$line_count" -eq 2 ]; then
    echo "  OK: 行数 = 2"
  else
    echo "  FAIL: 行数 = $line_count (期望 2)"
    errors=$((errors+1))
  fi

  # 检查第1行
  line1=$(sed -n '1p' "$FILE")
  if [ "$line1" = "probe: GATE-9M4T" ]; then
    echo "  OK: 第1行 = 'probe: GATE-9M4T'"
  else
    echo "  FAIL: 第1行 = '$line1' (期望 'probe: GATE-9M4T')"
    errors=$((errors+1))
  fi

  # 检查第2行
  line2=$(sed -n '2p' "$FILE")
  if [ "$line2" = "round: 3" ]; then
    echo "  OK: 第2行 = 'round: 3'"
  else
    echo "  FAIL: 第2行 = '$line2' (期望 'round: 3')"
    errors=$((errors+1))
  fi

  # 检查尾随空格/多余行 (确保文件尾部干净)
  # 总字节应为: len("probe: GATE-9M4T\nround: 3\n") = 26
  byte_count=$(wc -c < "$FILE" | tr -d ' ')
  if [ "$byte_count" -eq 26 ]; then
    echo "  OK: 文件大小 = 26 字节 (无多余尾随内容)"
  else
    echo "  FAIL: 文件大小 = $byte_count 字节 (期望 26, 有多余尾随内容)"
    errors=$((errors+1))
  fi
fi
echo ""

echo "=== 结果: $errors 个错误 ==="
exit $errors
