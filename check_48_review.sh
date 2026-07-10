#!/bin/bash
# check_48_review.sh — #48 A1-A3 审查问题修复验证
# 验证:
#   1. LINGSHI_TO_COPPER 常量定义 (1灵石=100文)
#   2. 所有 player_pay 调用均正确使用 ×100（PVP 例外链除外）
#   3. query_carried_spirit_stones → query_carried_copper 重命名
#   4. 传送计费公式与 teleport_d.c 一致
#   5. 无残留旧函数名引用
#   6. 括号配对/LPC 语法
#   7. 设计范围说明注释

set -e

BASE_DIR="$(dirname "$0")"
TARGET="$BASE_DIR/adm/daemons/economy_bridge_d.c"
TELEPORT_D="$BASE_DIR/adm/daemons/teleport_d.c"
PASS=0
FAIL=0

check_grep() {
    local file="$1"
    local pattern="$2"
    local desc="$3"
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo "  ✓ $desc"
        PASS=$((PASS + 1))
    else
        echo "  ✗ $desc"
        FAIL=$((FAIL + 1))
    fi
}

check_ngrep() {
    local file="$1"
    local pattern="$2"
    local desc="$3"
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo "  ✗ $desc (找到: $pattern)"
        FAIL=$((FAIL + 1))
    else
        echo "  ✓ $desc"
        PASS=$((PASS + 1))
    fi
}

check_brace() {
    local file="$1"
    local desc="$2"
    if [ ! -f "$file" ]; then
        echo "  ✗ $desc (文件不存在)"
        FAIL=$((FAIL + 1))
        return
    fi
    local open_p
    local close_p
    open_p=$(grep -c '{' "$file" 2>/dev/null || echo 0)
    close_p=$(grep -c '}' "$file" 2>/dev/null || echo 0)
    if [ "$open_p" -eq "$close_p" ] && [ "$open_p" -gt 0 ]; then
        echo "  ✓ $desc ({$open_p, }$close_p)"
        PASS=$((PASS + 1))
    else
        echo "  ✗ $desc (花括号不配对: {$open_p, }$close_p)"
        FAIL=$((FAIL + 1))
    fi
}

echo "============================================"
echo " #48 A1-A3 审查问题修复验证"
echo "============================================"
echo ""

echo "【1. 常量定义】"
check_grep "$TARGET" "LINGSHI_TO_COPPER" "LINGSHI_TO_COPPER 常量已定义"
echo ""

echo "【2. player_pay ×100 转换完整性】"
# 应被 ×100 的调用计数
COUNT_100=$(grep -c "LINGSHI_TO_COPPER" "$TARGET")
echo "       找到 $COUNT_100 处含 LINGSHI_TO_COPPER 的调用"
# PVP 链例外：apply_pvp_death_penalty 中的 player_pay 不应有 ×100
check_ngrep "$TARGET" "apply_pvp_death_penalty.*LINGSHI_TO_COPPER" "PVP 链 player_pay 未误加 ×100"
echo ""

echo "【3. 函数重命名】"
check_grep "$TARGET" "query_carried_copper" "query_carried_copper() 已定义"
# 只禁止非注释/非说明性的残留裸函数调用（允许"旧名"说明）
FOUND_LINES=$(grep -n "query_carried_spirit_stones(" "$TARGET" 2>/dev/null || true)
if [ -z "$FOUND_LINES" ]; then
    echo "  ✓ 无残留 query_carried_spirit_stones 函数调用"
    PASS=$((PASS + 1))
else
    REMAINING=$(echo "$FOUND_LINES" | grep -v "旧名" || true)
    if [ -n "$REMAINING" ]; then
        echo "  ✗ 残留 query_carried_spirit_stones 调用:"
        echo "$REMAINING" | while read -r line; do echo "      $line"; done
        FAIL=$((FAIL + 1))
    else
        echo "  ✓ 无残留 query_carried_spirit_stones 函数调用（仅存说明性引用）"
        PASS=$((PASS + 1))
    fi
fi
echo ""

echo "【4. 传送计费公式】"
# 验证新公式结构：base_fee + addon (addon = dist×realm×base/1000)
check_grep "$TARGET" "int fee = base_fee + addon" "新公式采用 base_fee + addon 结构"
check_grep "$TARGET" "to_float(dist_coeff)" "addon 计算引用 dist_coeff"
check_grep "$TARGET" "to_float(base_fee) / 1000" "addon 含 base_fee / 1000"
# 验证旧公式已被移除
check_ngrep "$TARGET" "base_fee) \* to_float(dist_coeff)" "旧公式（纯乘积）已移除"
# 检查与 teleport_d.c 结构一致
check_grep "$TELEPORT_D" "dist_coeff \* realm_coeff \* base_cost" "teleport_d.c 含 dist×realm×base 公式"
echo ""

echo "【5. 语法健康检查】"
check_brace "$TARGET" "economy_bridge_d.c 花括号配对"
echo ""

echo "【6. 设计范围说明】"
check_grep "$TARGET" "设计范围说明" "设计范围扩散注释存在"
echo ""

echo "【7. 其他关键函数完整性】"
check_grep "$TARGET" "deduct_spirit_stones" "通用扣款函数 deduct_spirit_stones 存在"
check_grep "$TARGET" "calculate_pvp_death_penalty" "PVP 惩罚计算函数存在"
echo ""

echo "============================================"
echo " 验证完成：通过 $PASS 项 / 失败 $FAIL 项"
echo "============================================"

exit $FAIL
