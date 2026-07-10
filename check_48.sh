#!/bin/bash
# check_48.sh — P3 集成 A4-A6 经济循环生命周期验证
# 验证:
#   A4: 经济→任务循环 (quest_economy_d.c)
#   A5: 经济→声望循环 (faction_economy_d.c)
#   A6: 经济自身循环跑通 (economyd.c 增强 + inflationd.c 增强)

set -e

BASE_DIR="$(dirname "$0")"
PASS=0
FAIL=0

check_file() {
    local file="$1"
    local desc="$2"
    if [ -f "$BASE_DIR/$file" ]; then
        echo "  ✓ $desc ($file)"
        PASS=$((PASS + 1))
    else
        echo "  ✗ $desc ($file 不存在)"
        FAIL=$((FAIL + 1))
    fi
}

check_define() {
    local file="$1"
    local define="$2"
    local desc="$3"
    if grep -q "$define" "$BASE_DIR/$file" 2>/dev/null; then
        echo "  ✓ $desc ($define 已定义)"
        PASS=$((PASS + 1))
    else
        echo "  ✗ $desc ($define 未定义)"
        FAIL=$((FAIL + 1))
    fi
}

check_grep() {
    local file="$1"
    local pattern="$2"
    local desc="$3"
    if grep -q "$pattern" "$BASE_DIR/$file" 2>/dev/null; then
        echo "  ✓ $desc"
        PASS=$((PASS + 1))
    else
        echo "  ✗ $desc"
        FAIL=$((FAIL + 1))
    fi
}

check_lpc_syntax() {
    local file="$1"
    local desc="$2"
    local path="$BASE_DIR/$file"
    if [ ! -f "$path" ]; then
        echo "  ✗ $desc (文件不存在)"
        FAIL=$((FAIL + 1))
        return
    fi
    # 检查括号配对
    local open_parens
    local close_parens
    open_parens=$(grep -c '{' "$path" 2>/dev/null || echo 0)
    close_parens=$(grep -c '}' "$path" 2>/dev/null || echo 0)
    if [ "$open_parens" -eq "$close_parens" ] && [ "$open_parens" -gt 0 ]; then
        echo "  ✓ $desc (花括号配对: $open_parens/$close_parens)"
        PASS=$((PASS + 1))
    else
        echo "  ✗ $desc (花括号不配对: {$open_parens, }$close_parens)"
        FAIL=$((FAIL + 1))
    fi
}

echo "============================================"
echo " P3 集成 A4-A6 经济循环生命周期验证"
echo "============================================"
echo ""

echo "【A4: 经济→任务循环】"
check_file "include/economy_lifecycle.h" "经济生命周期常量定义"
check_file "adm/daemons/quest_economy_d.c" "任务奖励经济约束守护进程"
check_define "include/economy_lifecycle.h" "QUEST_COIN_PCT_CAP" "任务灵石产出占比上限"
check_define "include/economy_lifecycle.h" "QUEST_DAILY_BUDGET_QIGE" "炼气期每日任务预算"
check_define "include/economy_lifecycle.h" "QUEST_DAILY_PLAYER_CAP_QIGE" "玩家每日灵石上限"
check_define "include/economy_lifecycle.h" "ACTIVITY_MOD_NORMAL" "活跃度修正系数"
check_define "include/economy_lifecycle.h" "QUEST_FAIL_COIN_PENALTY" "任务失败惩罚比例"
check_grep "adm/daemons/quest_economy_d.c" "calculate_quest_coin_reward" "quest_economy_d: 灵石奖励计算函数"
check_grep "adm/daemons/quest_economy_d.c" "calculate_quest_fail_penalty" "quest_economy_d: 失败惩罚函数"
check_grep "adm/daemons/quest_economy_d.c" "MONEY_D->add_production" "quest_economy_d: 与 MONEY_D 联动"
check_grep "adm/daemons/quest_economy_d.c" "INFLATION_D->query_output_boost" "quest_economy_d: 与 INFLATION_D 联动"
check_lpc_syntax "adm/daemons/quest_economy_d.c" "quest_economy_d.c 语法检查"

echo ""
echo "【A5: 经济→声望循环】"
check_file "adm/daemons/faction_economy_d.c" "势力产业税收守护进程"
check_define "include/economy_lifecycle.h" "TAX_MARKET_BASE" "坊市交易税基准"
check_define "include/economy_lifecycle.h" "TAX_AUCTION_BASE" "拍卖行手续费基准"
check_define "include/economy_lifecycle.h" "FACTION_INCOME_FACTION" "势力收入分配比例"
check_define "include/economy_lifecycle.h" "REP_THRESHOLD_BUY_CORE" "核心商店声望门槛"
check_grep "adm/daemons/faction_economy_d.c" "collect_tax" "faction_economy_d: 税收收取函数"
check_grep "adm/daemons/faction_economy_d.c" "check_repurchase_threshold" "faction_economy_d: 消费门槛检查"
check_grep "adm/daemons/faction_economy_d.c" "query_faction_discount" "faction_economy_d: 折扣计算"
check_grep "adm/daemons/faction_economy_d.c" "calculate_faction_task_coin" "faction_economy_d: 势力任务灵石注入"
check_grep "adm/daemons/faction_economy_d.c" "MONEY_D->add_consumption" "faction_economy_d: 与 MONEY_D 联动"
check_lpc_syntax "adm/daemons/faction_economy_d.c" "faction_economy_d.c 语法检查"

echo ""
echo "【A6: 经济自身循环跑通】"
check_define "include/economy_lifecycle.h" "ECON_HEALTHY_MAX" "经济健康阈值"
check_define "include/economy_lifecycle.h" "RECOVERY_TARGET_MIN" "回收/产出比目标下限"
check_define "include/economy_lifecycle.h" "SINK_TAX_WEIGHT" "回收通道权重"
check_define "include/economy_lifecycle.h" "EVENT_COOLDOWN_CRISIS" "经济事件冷却期"
check_grep "adm/daemons/economyd.c" "verify_economy_lifecycle" "economyd: 经济生命周期验证"
check_grep "adm/daemons/economyd.c" "query_lifecycle_report" "economyd: 生命周期报告"
check_grep "adm/daemons/economyd.c" "query_inflation_adjusted_price" "economyd: 通胀感知定价"
check_grep "adm/daemons/inflationd.c" "query_system_recovery_ratio" "inflationd: 系统回收率"
check_grep "adm/daemons/inflationd.c" "query_economy_health_score" "inflationd: 健康评分"
check_grep "adm/daemons/inflationd.c" "record_sink" "inflationd: 回收通道记录"
check_grep "adm/daemons/inflationd.c" "query_lifecycle_deep_report" "inflationd: 深度报告"
check_grep "adm/daemons/inflationd.c" "init_sink_stats" "inflationd: 回收通道初始化"
check_lpc_syntax "adm/daemons/economyd.c" "economyd.c 语法检查"
check_lpc_syntax "adm/daemons/inflationd.c" "inflationd.c 语法检查"

echo ""
echo "【跨文件集成验证】"
check_define "include/globals.h" "QUEST_ECONOMY_D" "QUEST_ECONOMY_D 全局宏定义"
check_define "include/globals.h" "FACTION_ECONOMY_D" "FACTION_ECONOMY_D 全局宏定义"
check_grep "include/globals.h" "quest_economy_d" "quest_economy_d 路径注册"
check_grep "include/globals.h" "faction_economy_d" "faction_economy_d 路径注册"

echo ""
echo "============================================"
echo " 验证完成：通过 $PASS 项 / 失败 $FAIL 项"
echo "============================================"

exit $FAIL
