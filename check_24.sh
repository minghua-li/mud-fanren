#!/bin/bash
# 验收检查：Ticket #24 任务链系统
# 验证所有子 ticket 的关键文件/函数是否已合入 origin/main
# 用法：bash check_24.sh

set -e
errors=0

GITBASE="origin/main"

red()   { echo -e "\033[31mFAIL\033[0m $1"; }
green() { echo -e "\033[32mPASS\033[0m $1"; }
check_file() {
    if git -C /Users/apple/playground/games/mud-fanren ls-tree -r "$GITBASE" --name-only | grep -q "^$1$"; then
        green "文件存在: $1"
    else
        red "文件缺失: $1"
        errors=$((errors + 1))
    fi
}
check_pattern() {
    local file=$1 pattern=$2 desc=$3
    if git -C /Users/apple/playground/games/mud-fanren show "$GITBASE:$file" 2>/dev/null | grep -q "$pattern"; then
        green "模式 '$pattern' 存在于 $file — $desc"
    else
        red "模式 '$pattern' 未在 $file 中找到 — $desc"
        errors=$((errors + 1))
    fi
}
check_define() {
    local macro=$1
    if git -C /Users/apple/playground/games/mud-fanren show "$GITBASE:include/globals.h" 2>/dev/null | grep -q "#define $macro"; then
        green "宏 $macro 在 globals.h 中定义"
    else
        red "宏 $macro 未在 globals.h 中定义"
        errors=$((errors + 1))
    fi
}

echo "==================================="
echo " Ticket #24 验收检查 — 任务链系统"
echo " 基线: $GITBASE"
echo " 时间: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
echo "==================================="
echo ""

# ─── 1. 任务链框架 (#36) ───
echo "--- [1] 任务链框架与奖励曲线 (#36) ---"
check_file "adm/daemons/quest_chain_d.c"
check_file "include/quest_chain.h"
check_define "QUEST_CHAIN_D"
check_pattern "adm/daemons/quest_chain_d.c" "register_chain" "任务链注册函数"
check_pattern "adm/daemons/quest_chain_d.c" "calc_exp_reward" "经验奖励曲线计算"
check_pattern "adm/daemons/quest_chain_d.c" "calc_coin_reward" "灵石奖励曲线计算"
check_pattern "adm/daemons/quest_chain_d.c" "calc_reputation_reward" "声望奖励曲线计算"
check_pattern "adm/daemons/quest_chain_d.c" "get_next_chain_quest" "串行链推进"
check_pattern "adm/daemons/quest_chain_d.c" "get_branch_options" "分支链选项"
echo ""

# ─── 2. 每日任务系统 (#39) ───
echo "--- [2] 日常任务系统 (#39) ---"
check_file "adm/daemons/daily_task_d.c"
check_file "cmds/usr/daily_task.c"
check_file "include/daily_task.h"
check_define "DAILY_TASK_D"
check_pattern "adm/daemons/daily_task_d.c" "refresh_player_tasks" "刷新玩家日常任务池"
check_pattern "adm/daemons/daily_task_d.c" "update_progress" "日常任务进度更新"
check_pattern "cmds/usr/daily_task.c" "daily_task" "日常任务玩家命令"
echo ""

# ─── 3. 主线任务框架 (#40) ───
echo "--- [3] 主线任务框架 (#40) ---"
check_file "adm/daemons/main_quest_d.c"
check_file "cmds/usr/main_quest.c"
check_file "include/main_quest.h"
check_define "MAIN_QUEST_D"
check_pattern "adm/daemons/main_quest_d.c" "chapter" "主线章节检测"
check_pattern "cmds/usr/main_quest.c" "main_quest" "主线任务玩家命令"
echo ""

# ─── 4. 活跃度系统 (#37) ───
echo "--- [4] 活跃度与日常任务系统 (#37) ---"
check_file "adm/daemons/activity_d.c"
check_file "cmds/usr/activity.c"
check_file "include/activity.h"
check_define "ACTIVITY_D"
check_pattern "adm/daemons/activity_d.c" "add_activity" "活跃度增加值函数"
check_pattern "adm/daemons/activity_d.c" "claim_daily_reward" "每日奖励领取"
check_pattern "adm/daemons/activity_d.c" "claim_weekly_reward" "每周奖励领取"
check_pattern "cmds/usr/activity.c" "activity" "活跃度玩家命令"
echo ""

# ─── 5. 成就系统 (#38) ───
echo "--- [5] 成就系统 (#38) ---"
check_file "adm/daemons/achievement_d.c"
check_file "cmds/usr/achievement.c"
check_file "include/achievement.h"
check_define "ACHIEVEMENT_D"
check_pattern "adm/daemons/achievement_d.c" "unlock_achievement" "成就解锁函数"
check_pattern "adm/daemons/achievement_d.c" "query_achievements_by_category" "成就分类查询"
check_pattern "cmds/usr/achievement.c" "achievement" "成就玩家命令"
echo ""

# ─── 6. 预加载注册 ───
echo "--- [6] 预加载注册 ---"
check_pattern "adm/etc/preload" "daily_task_d" "DAILY_TASK_D 预加载"
check_pattern "adm/etc/preload" "main_quest_d" "MAIN_QUEST_D 预加载"
check_pattern "adm/etc/preload" "activity_d" "ACTIVITY_D 预加载"
echo ""

# ─── 汇总 ───
echo "==================================="
if [ $errors -eq 0 ]; then
    echo -e "\033[32m全部通过 — $errors 个错误\033[0m"
else
    echo -e "\033[31m$errors 个检查未通过\033[0m"
fi
echo "==================================="
exit $errors
