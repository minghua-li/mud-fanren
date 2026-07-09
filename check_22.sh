#!/bin/bash
# Ticket #22 区域玩法实现验收检查
# 验证子任务 #31(秘境副本系统)、#32(洞府经营系统)、#33(传送网络) 代码就位与集成正确性
set -e

PASS=0
FAIL=0
ERRORS=""

check_file() {
    local desc="$1" path="$2"
    if [ -f "$path" ]; then
        lines=$(wc -l < "$path")
        echo "  ✓ $desc ($path, ${lines}行)"
        PASS=$((PASS+1))
    else
        echo "  ✗ $desc ($path MISSING)"
        FAIL=$((FAIL+1))
        ERRORS="$ERRORS\n  - 缺失文件: $path ($desc)"
    fi
}

check_grep() {
    local desc="$1" file="$2" pattern="$3"
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo "  ✓ $desc"
        PASS=$((PASS+1))
    else
        echo "  ✗ $desc (模式 '$pattern' 未在 $file 中找到)"
        FAIL=$((FAIL+1))
        ERRORS="$ERRORS\n  - $desc: 模式 '$pattern' 未在 $file 中找到"
    fi
}

check_function() {
    local desc="$1" file="$2" func="$3"
    if grep -q "^[[:space:]]*\(static\|public\|protected\|private\|nomask\|int\|string\|void\|mapping\|object\|mixed\)[[:space:]*]*$func(" "$file" 2>/dev/null; then
        echo "  ✓ $desc (函数 $func 存在)"
        PASS=$((PASS+1))
    else
        echo "  ✗ $desc (函数 $func 未在 $file 中找到)"
        FAIL=$((FAIL+1))
        ERRORS="$ERRORS\n  - $desc: 函数 $func 未在 $file 中找到"
    fi
}

echo "=========================================="
echo " 验收检查 — #22 区域玩法实现"
echo "=========================================="
echo ""

echo "--- 1. 秘境副本系统 (#31) ---"
check_file "秘境守护进程" "adm/daemons/secret_realm_d.c"
check_file "秘境头文件" "include/secret_realm.h"
check_function "秘境注册函数" "adm/daemons/secret_realm_d.c" "register_realm"
check_function "进入条件校验" "adm/daemons/secret_realm_d.c" "check_entry_condition"
check_function "冷却管理" "adm/daemons/secret_realm_d.c" "set_cooldown"
check_function "探索逻辑" "adm/daemons/secret_realm_d.c" "do_explore"
check_function "过期清理" "adm/daemons/secret_realm_d.c" "cleanup_expired_instances"
check_function "计时秘境调度" "adm/daemons/secret_realm_d.c" "timed_realm_check"
check_grep "内置秘境定义（血色禁地）" "adm/daemons/secret_realm_d.c" "血色禁地"
check_grep "内置秘境定义（虚天殿）" "adm/daemons/secret_realm_d.c" "虚天殿"
check_grep "秘境头文件引用" "adm/daemons/mapd.c" "secret_realm.h"

echo ""
echo "--- 2. 洞府经营系统 (#32) ---"
check_file "洞府守护进程" "adm/daemons/mansion_d.c"
check_file "洞府头文件" "include/mansion.h"
check_function "洞府创建函数" "adm/daemons/mansion_d.c" "create_mansion"
check_function "洞府升级函数" "adm/daemons/mansion_d.c" "upgrade_mansion"
check_function "建筑建造函数" "adm/daemons/mansion_d.c" "build_building"
check_function "建筑升级函数" "adm/daemons/mansion_d.c" "upgrade_building"
check_function "药圃种植函数" "adm/daemons/mansion_d.c" "plant_seed"
check_function "药圃收获函数" "adm/daemons/mansion_d.c" "harvest_plot"
check_function "生长检查函数" "adm/daemons/mansion_d.c" "check_garden_growth"
check_function "洞府降级函数" "adm/daemons/mansion_d.c" "degrade_mansion"
check_function "维护费缴纳" "adm/daemons/mansion_d.c" "pay_maintenance"
check_grep "MANSION_D 宏定义" "include/globals.h" "MANSION_D"
check_grep "洞府入口注册痕迹" "adm/daemons/mapd.c" "mansion_entries"

echo ""
echo "--- 3. 传送网络系统 (#33) ---"
check_file "传送守护进程" "adm/daemons/teleport_d.c"
check_file "传送头文件" "include/teleport.h"
check_file "传送用户命令" "cmds/usr/teleport.c"
check_function "节点注册函数" "adm/daemons/teleport_d.c" "register_node"
check_function "传送执行函数" "adm/daemons/teleport_d.c" "do_teleport"
check_function "费用计算函数" "adm/daemons/teleport_d.c" "calculate_cost"
check_function "冷却设置函数" "adm/daemons/teleport_d.c" "set_cooldown"
check_function "冷却查询函数" "adm/daemons/teleport_d.c" "query_cooldown"
check_function "解锁校验函数" "adm/daemons/teleport_d.c" "is_node_unlocked"
check_function "解锁尝试函数" "adm/daemons/teleport_d.c" "try_unlock_node"
check_function "节点查询函数" "adm/daemons/teleport_d.c" "query_all_nodes"
check_grep "TELEPORT_D 宏定义" "include/globals.h" "TELEPORT_D"
check_grep "传送阵入口注册痕迹" "adm/daemons/mapd.c" "teleport_entries"
check_grep "teleport_d preload" "adm/etc/preload" "teleport_d"
check_grep "14个预定义传送节点" "adm/daemons/teleport_d.c" "init_teleport_nodes"

echo ""
echo "=========================================="
echo " 结果: $PASS 通过 / $FAIL 失败"
echo "=========================================="

if [ $FAIL -gt 0 ]; then
    echo -e "失败详情:$ERRORS"
    exit 1
else
    echo "所有检查通过！三个子系统均已就位且集成正确。"
    exit 0
fi
