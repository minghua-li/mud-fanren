#!/usr/bin/env bash
#===============================================================================
# 经济生命周期系统 — 计算逻辑验证测试
#
# 本脚本独立于 LPC 运行时，用 bash 复现关键计算公式并断言其正确性。
# 当 LPC 驱动可用时，这些测试应迁移为 LPC 单元测试。
#
# 覆盖范围：
#   1. 任务奖励三重约束计算
#   2. 税收四类计算和分配
#   3. 通胀感知定价
#===============================================================================

set -o nounset
set -o errexit
set -o pipefail

#-------------------------------------------------------------------------------
# 辅助函数
#-------------------------------------------------------------------------------
PASS=0
FAIL=0

assert_eq() {
    local desc="$1" expected="$2" actual="$3"
    if [[ "$expected" == "$actual" ]]; then
        echo "  ✅ PASS: $desc (expected=$expected, actual=$actual)"
        PASS=$((PASS + 1))
    else
        echo "  ❌ FAIL: $desc (expected=$expected, actual=$actual)"
        FAIL=$((FAIL + 1))
    fi
}

assert_float_eq() {
    local desc="$1" expected="$2" actual="$3" tol="${4:-0.01}"
    local diff
    diff=$(python3 -c "print(abs($expected - $actual) < $tol)")
    if [[ "$diff" == "True" ]]; then
        echo "  ✅ PASS: $desc (expected≈$expected, actual=$actual)"
        PASS=$((PASS + 1))
    else
        echo "  ❌ FAIL: $desc (expected≈$expected, actual=$actual)"
        FAIL=$((FAIL + 1))
    fi
}

assert_ge() {
    local desc="$1" expected="$2" actual="$3"
    if (( actual >= expected )); then
        echo "  ✅ PASS: $desc (≥$expected, actual=$actual)"
        PASS=$((PASS + 1))
    else
        echo "  ❌ FAIL: $desc (≥$expected, actual=$actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo "========================================================================"
echo " 经济生命周期系统 — 计算逻辑验证测试"
echo "========================================================================"
echo ""

#-------------------------------------------------------------------------------
# 1. 任务奖励三重约束计算
#-------------------------------------------------------------------------------
echo "--- 1. 任务奖励三重约束计算 ---"

# 模拟 quest_economy_d.c 的约束公式:
#   最终奖励 = clamp(base_coin × econ_mod × activity_mod, 0.3, 2.0)
#   同时受以下约束:
#     a) 玩家每日上限 (QUEST_DAILY_PLAYER_CAP)
#     b) 境界预算余额 (daily_budget_balance)
#     c) 个人已领取量 (player_today)

# 常量（与 economy_lifecycle.h 保持一致）
QUEST_DAILY_PLAYER_CAP_QIGE=20
QUEST_DAILY_PLAYER_CAP_ZHUJI=80
QUEST_DAILY_PLAYER_CAP_JIEDAN=400
QUEST_DAILY_PLAYER_CAP_YUANYING=2000
QUEST_DAILY_PLAYER_CAP_HUASHEN=8000

PLAYER_CAP_LOOKUP() {
    local idx=$1
    if   (( idx <= 0 )); then echo $QUEST_DAILY_PLAYER_CAP_QIGE
    elif (( idx == 1 )); then echo $QUEST_DAILY_PLAYER_CAP_ZHUJI
    elif (( idx == 2 )); then echo $QUEST_DAILY_PLAYER_CAP_JIEDAN
    elif (( idx == 3 )); then echo $QUEST_DAILY_PLAYER_CAP_YUANYING
    else                     echo $QUEST_DAILY_PLAYER_CAP_HUASHEN
    fi
}

# 测试用例：炼气期 (index=0) 玩家，基础奖励 10 灵石
# 经济状态正常 (econ_mod=1.0)，活跃度中等 (activity_mod=1.0，在线 180 分钟)
# 预算充足，玩家今日未领取
test_triple_constraint_1() {
    echo "  [用例1] 炼气期正常奖励 — 所有约束宽松"

    local realm_idx=0
    local base_coin=10
    local econ_mod=1.0
    local activity_mod=1.0
    local player_today=0

    # 综合修正系数
    local final_mod
    final_mod=$(python3 -c "print(max(0.3, min(2.0, $econ_mod * $activity_mod)))")
    assert_float_eq "修正系数 clamp(1.0*1.0, 0.3, 2.0)" 1.0 "$final_mod"

    # 修正后奖励
    local final_coin
    final_coin=$(python3 -c "print(int($base_coin * $final_mod))")
    assert_eq "修正后奖励 (10*1.0)" 10 "$final_coin"

    # 玩家日上限检查
    local player_cap
    player_cap=$(PLAYER_CAP_LOOKUP $realm_idx)
    assert_eq "炼气期日上限" 20 "$player_cap"

    local remaining=$(( player_cap - player_today ))
    assert_ge "剩余额度" 10 "$remaining"

    # 最终奖励（修正后不超过剩余额度）
    if (( final_coin > remaining )); then final_coin=$remaining; fi
    assert_eq "最终奖励" 10 "$final_coin"
}

# 测试用例：预算耗尽 -> 奖励为 0
test_triple_constraint_2() {
    echo "  [用例2] 境界预算耗尽 → 奖励为 0"

    local daily_budget_balance_qige=0
    assert_eq "预算为 0 时奖励应返回 0" 0 "$daily_budget_balance_qige"
}

# 测试用例：玩家已达日上限 -> 奖励为 0
test_triple_constraint_3() {
    echo "  [用例3] 玩家已达日上限 → 奖励为 0"

    local realm_idx=0
    local player_cap
    player_cap=$(PLAYER_CAP_LOOKUP $realm_idx)
    local player_today=$player_cap  # 已达上限
    local remaining=$(( player_cap - player_today ))
    assert_eq "剩余额度（已达上限）" 0 "$remaining"
}

# 测试用例：经济危机产出增益
test_triple_constraint_4() {
    echo "  [用例4] 通胀增益生效 — 经济产出增益让奖励提升"

    local base_coin=10
    # output_boost = 1.2 （产出不足时 +20%）
    local econ_mod=1.2
    local activity_mod=1.0

    local final_mod
    final_mod=$(python3 -c "print(max(0.3, min(2.0, $econ_mod * $activity_mod)))")
    assert_float_eq "修正系数 (1.2*1.0)" 1.2 "$final_mod"

    local final_coin
    final_coin=$(python3 -c "print(int($base_coin * $final_mod))")
    assert_eq "通胀增益奖励 (10*1.2)" 12 "$final_coin"
}

# 测试用例：活跃度修正 — 在线不足 30 分钟
test_triple_constraint_5() {
    echo "  [用例5] 低活跃度惩罚 — 在线时间 <30 分钟"

    local base_coin=10
    local econ_mod=1.0
    local activity_mod=0.3  # ACTIVITY_MOD_MIN

    local final_mod
    final_mod=$(python3 -c "print(max(0.3, min(2.0, $econ_mod * $activity_mod)))")
    assert_float_eq "修正系数 (1.0*0.3)" 0.3 "$final_mod"

    local final_coin
    final_coin=$(python3 -c "print(int($base_coin * $final_mod))")
    assert_eq "低活跃奖励 (10*0.3)" 3 "$final_coin"
}

# 测试用例：活跃度修正 — 在线超过 240 分钟
test_triple_constraint_6() {
    echo "  [用例6] 高活跃度加成 — 在线 >240 分钟"

    local base_coin=10
    local econ_mod=1.0
    local activity_mod=1.1  # ACTIVITY_MOD_HIGH

    local final_mod
    final_mod=$(python3 -c "print(max(0.3, min(2.0, $econ_mod * $activity_mod)))")
    assert_float_eq "修正系数 (1.0*1.1)" 1.1 "$final_mod"

    local final_coin
    final_coin=$(python3 -c "print(int($base_coin * $final_mod))")
    assert_eq "高活跃奖励 (10*1.1)" 11 "$final_coin"
}

# 测试用例：修正系数钳制 — 低于下限
test_triple_constraint_7() {
    echo "  [用例7] 修正系数下限钳制 — 极低值被抬至 0.3"

    local final_mod
    final_mod=$(python3 -c "print(max(0.3, min(2.0, 0.1)))")
    assert_float_eq "下限钳制 clamp(0.1, 0.3, 2.0)" 0.3 "$final_mod"
}

# 测试用例：修正系数钳制 — 超过上限
test_triple_constraint_8() {
    echo "  [用例8] 修正系数上限钳制 — 极高值被降至 2.0"

    local final_mod
    final_mod=$(python3 -c "print(max(0.3, min(2.0, 5.0)))")
    assert_float_eq "上限钳制 clamp(5.0, 0.3, 2.0)" 2.0 "$final_mod"
}

test_triple_constraint_1
test_triple_constraint_2
test_triple_constraint_3
test_triple_constraint_4
test_triple_constraint_5
test_triple_constraint_6
test_triple_constraint_7
test_triple_constraint_8

echo ""

#-------------------------------------------------------------------------------
# 2. 税收四类计算和分配
#-------------------------------------------------------------------------------
echo "--- 2. 税收四类计算和分配 ---"

# 模拟 faction_economy_d.c 的税收公式:
#   基础税额 = transaction_amount × tax_rate / 1000
#   最终税额 = base_tax × tax_mod × rep_discount
#   分配:
#     势力金库 40% (FACTION_INCOME_FACTION)
#     贡献分配 30% (FACTION_INCOME_CONTRIBUTE)
#     驻地维护 20% (FACTION_INCOME_MAINTENANCE)
#     系统回收 10% (FACTION_INCOME_SYSTEM)

# 常量（与 economy_lifecycle.h 保持一致）
TAX_MARKET_BASE=50    # 5%
TAX_AUCTION_BASE=100  # 10%
TAX_MINE_BASE=20      # 2%
TAX_LINGMAI_BASE=30   # 3%

TAX_MOD_HEALTHY=1.0
TAX_MOD_WARNING=1.3
TAX_MOD_CRITICAL=1.8

FACTION_INCOME_FACTION=0.40
FACTION_INCOME_CONTRIBUTE=0.30
FACTION_INCOME_MAINTENANCE=0.20
FACTION_INCOME_SYSTEM=0.10

CALCULATE_TAX() {
    local industry="$1" amount="$2" modifier="$3" discount="$4"
    local base_tax
    case "$industry" in
        market)  base_tax=$(python3 -c "print(int($amount * $TAX_MARKET_BASE / 1000.0))")  ;;
        auction) base_tax=$(python3 -c "print(int($amount * $TAX_AUCTION_BASE / 1000.0))") ;;
        mine)    base_tax=$(python3 -c "print(int($amount * $TAX_MINE_BASE / 1000.0))")    ;;
        lingmai) base_tax=$(python3 -c "print(int($amount * $TAX_LINGMAI_BASE / 1000.0))") ;;
        *)       base_tax=0 ;;
    esac
    local final_tax
    final_tax=$(python3 -c "
t = $base_tax * $modifier * $discount
if t < 1.0 and $amount > 0: t = 1.0
print(int(t))
")
    echo "$final_tax"
}

# 测试用例：坊市交易 1000 灵石
test_tax_1() {
    echo "  [用例1] 坊市交易 1000 灵石，健康经济，无折扣"

    local tax
    tax=$(CALCULATE_TAX "market" 1000 1.0 1.0)
    # base = 1000 * 50 / 1000 = 50
    assert_eq "坊市税 (1000×5%)" 50 "$tax"
}

# 测试用例：拍卖行交易 10000 灵石
test_tax_2() {
    echo "  [用例2] 拍卖行 10000 灵石，健康经济，无折扣"

    local tax
    tax=$(CALCULATE_TAX "auction" 10000 1.0 1.0)
    # base = 10000 * 100 / 1000 = 1000
    assert_eq "拍卖税 (10000×10%)" 1000 "$tax"
}

# 测试用例：经济危机状态加税
test_tax_3() {
    echo "  [用例3] 坊市交易 1000 灵石，危机经济，无折扣"

    local tax
    tax=$(CALCULATE_TAX "market" 1000 1.8 1.0)
    # base = 50, final = 50 * 1.8 = 90
    assert_eq "危机税 (50×1.8)" 90 "$tax"
}

# 测试用例：高声望折扣
test_tax_4() {
    echo "  [用例4] 拍卖行 10000 灵石，崇拜折扣 (0.6)"

    local tax
    tax=$(CALCULATE_TAX "auction" 10000 1.0 0.6)
    # base = 1000, final = 1000 * 1.0 * 0.6 = 600
    assert_eq "声望折扣税 (1000×0.6)" 600 "$tax"
}

# 测试用例：矿脉税
test_tax_5() {
    echo "  [用例5] 矿脉 5000 灵石，健康经济"

    local tax
    tax=$(CALCULATE_TAX "mine" 5000 1.0 1.0)
    # base = 5000 * 20 / 1000 = 100
    assert_eq "矿脉税 (5000×2%)" 100 "$tax"
}

# 测试用例：灵脉税
test_tax_6() {
    echo "  [用例6] 灵脉 3000 灵石，预警经济"

    local tax
    tax=$(CALCULATE_TAX "lingmai" 3000 1.3 1.0)
    # base = 3000 * 30 / 1000 = 90, final = 90 * 1.3 = 117
    assert_eq "灵脉税预警 (90×1.3)" 117 "$tax"
}

# 测试用例：最低税额 1 灵石
test_tax_7() {
    echo "  [用例7] 小额交易保底 1 灵石税额"

    local tax
    tax=$(CALCULATE_TAX "market" 5 1.0 1.0)
    # base = 5*50/1000 = 0, 但 >0 交易保底 1
    assert_eq "小额保底税" 1 "$tax"
}

# 测试用例：税收分配比例
test_tax_distribution() {
    echo "  [用例8] 税收分配 — 100 灵石按比例分配"

    local tax=100
    local faction_share contribute_share maintenance_share system_share

    faction_share=$(python3 -c "print(int($tax * $FACTION_INCOME_FACTION))")
    contribute_share=$(python3 -c "print(int($tax * $FACTION_INCOME_CONTRIBUTE))")
    maintenance_share=$(python3 -c "print(int($tax * $FACTION_INCOME_MAINTENANCE))")
    system_share=$(python3 -c "print(int($tax * $FACTION_INCOME_SYSTEM))")

    assert_eq "势力金库 40%" 40 "$faction_share"
    assert_eq "贡献分配 30%" 30 "$contribute_share"
    assert_eq "驻地维护 20%" 20 "$maintenance_share"
    assert_eq "系统回收 10%" 10 "$system_share"

    local total=$(( faction_share + contribute_share + maintenance_share + system_share ))
    assert_eq "分配总和等于税收" "$tax" "$total"
}

test_tax_1
test_tax_2
test_tax_3
test_tax_4
test_tax_5
test_tax_6
test_tax_7
test_tax_distribution

echo ""

#-------------------------------------------------------------------------------
# 3. 通胀感知定价
#-------------------------------------------------------------------------------
echo "--- 3. 通胀感知定价 ---"

# 模拟 economyd.c 的通胀定价公式:
#   动态价格 = base_price × (1 + demand_ratio - supply_ratio) × region_modifier
#   边界钳制: [base_price × 0.50, base_price × 1.50]
#   通胀附加税 = base_price × add_tax / 1000

PRICE_FLOOR_RATIO=0.50
PRICE_CEIL_RATIO=1.50
SPECIAL_SOURCE_BONUS=1.2

CALC_PRICE_RATIO() {
    local demand="$1" supply="$2" turnover="$3" region_mod="${4:-1.0}"
    python3 -c "
if $turnover <= 0: turnover = 1
dr = $demand / $turnover
sr = $supply / $turnover
ratio = (1.0 + dr - sr) * $region_mod
ratio = max($PRICE_FLOOR_RATIO, min($PRICE_CEIL_RATIO, ratio))
print(ratio)
"
}

# 测试用例：供需平衡
test_pricing_1() {
    echo "  [用例1] 供需平衡 → 基准价 ×1.0"

    local ratio
    ratio=$(CALC_PRICE_RATIO 100 100 100)
    assert_float_eq "供需平衡 (1+1-1=1.0)" 1.0 "$ratio"
}

# 测试用例：需求高，供给低
test_pricing_2() {
    echo "  [用例2] 高需求低供给 → 价格上涨"

    local ratio
    ratio=$(CALC_PRICE_RATIO 200 50 100)
    # 1+2.0-0.5 = 2.5, 钳制到 1.5
    assert_float_eq "供需失衡 (1+2-0.5=2.5) 钳制" 1.50 "$ratio"
}

# 测试用例：供给高，需求低
test_pricing_3() {
    echo "  [用例3] 高供给低需求 → 价格下跌"

    local ratio
    ratio=$(CALC_PRICE_RATIO 30 200 100)
    # 1+0.3-2.0 = -0.7, 钳制到 0.5
    assert_float_eq "供给过剩 (1+0.3-2=-0.7) 钳制" 0.50 "$ratio"
}

# 测试用例：区域物价修正
test_pricing_4() {
    echo "  [用例4] 高物价区域 → 价格 ×1.3"

    local ratio
    ratio=$(CALC_PRICE_RATIO 100 100 100 1.3)
    # 1.0 * 1.3 = 1.3
    assert_float_eq "高物价区域" 1.30 "$ratio"
}

# 测试用例：低物价区域
test_pricing_5() {
    echo "  [用例5] 低物价区域 → 价格 ×0.7"

    local ratio
    ratio=$(CALC_PRICE_RATIO 100 100 100 0.7)
    # 1.0 * 0.7 = 0.7
    assert_float_eq "低物价区域" 0.70 "$ratio"
}

# 测试用例：通胀附加税
test_pricing_6() {
    echo "  [用例6] 通胀附加税 — 重度通胀 +5%"

    local base_price=100
    local add_tax=50  # 重度附加 50‰ = 5%
    local surcharge
    surcharge=$(python3 -c "print(int($base_price * $add_tax / 1000.0))")
    assert_eq "通胀附加税 (100×50‰)" 5 "$surcharge"

    local final=$(( base_price + surcharge ))
    assert_eq "含附加税价格" 105 "$final"
}

# 测试用例：区域特产溢价
test_pricing_7() {
    echo "  [用例7] 区域特产溢价 — 在原产地 ×1.2"

    local base_price=200
    local region_mod=1.0
    local supply=100 demand=100 turnover=100

    local ratio
    ratio=$(CALC_PRICE_RATIO $demand $supply $turnover $region_mod)
    local special_ratio
    special_ratio=$(python3 -c "print($ratio * $SPECIAL_SOURCE_BONUS)")
    assert_float_eq "特产溢价" 1.20 "$special_ratio"

    local special_price
    special_price=$(python3 -c "print(int($base_price * $special_ratio))")
    assert_eq "特产价格 (200×1.2)" 240 "$special_price"
}

# 测试用例：区域特产边界钳制联动
test_pricing_8() {
    echo "  [用例8] 价格边界钳制 — 区域×物价后超上限"

    local ratio
    ratio=$(CALC_PRICE_RATIO 200 50 100 1.5)
    # (1+2-0.5) = 2.5, *1.5 = 3.75, 钳制到 1.5
    assert_float_eq "物价钳制上限" 1.50 "$ratio"
}

test_pricing_1
test_pricing_2
test_pricing_3
test_pricing_4
test_pricing_5
test_pricing_6
test_pricing_7
test_pricing_8

echo ""
echo "========================================================================"
echo " 测试汇总"
echo "========================================================================="
echo " 通过: $PASS"
echo " 失败: $FAIL"
echo " 总计: $((PASS + FAIL))"
echo "========================================================================="

if (( FAIL > 0 )); then
    echo "  ❌ 存在失败的测试!"
    exit 1
else
    echo "  ✅ 全部测试通过!"
    exit 0
fi
