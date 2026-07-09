#!/bin/sh
# scripts/verify-ticket-18.sh
# 验证 #18 的清理工作已在 origin/main 生效
# 清理内容：删除 antirobot/log.txt、antirobot/log.txt.1、antirobot/123、antirobot/test、www/ 目录
#
# commits: 5362d41b (antiroot 非UTF-8文件), f7d1f16e (移除 www/)

set -e

PASS=0
FAIL=0

check() {
    local desc="$1"
    shift
    if "$@"; then
        echo "✅ PASS: $desc"
        PASS=$((PASS + 1))
    else
        echo "❌ FAIL: $desc"
        FAIL=$((FAIL + 1))
    fi
}

echo "=========================================="
echo "验证 Ticket #18 — 清理非UTF-8文件与www目录"
echo "=========================================="
echo ""

# ---- 1. 验证 commits 是 origin/main 的祖先 ----
echo "--- 1/3: commit ancestry ---"

check "5362d41b 是 origin/main 的祖先" \
    git merge-base --is-ancestor 5362d41b origin/main

check "f7d1f16e 是 origin/main 的祖先" \
    git merge-base --is-ancestor f7d1f16e origin/main

echo ""

# ---- 2. 验证 commit 5362d41b 的改动内容 ----
echo "--- 2/3: commit 5362d41b 改动内容 ---"

# 确认该 commit 删除了 antiroot/ 下的文件
DELETED=$(git diff-tree --no-commit-id --name-status -r 5362d41b | grep -c '^D')
check "5362d41b 包含 $DELETED 个删除操作 (应 >= 4)" \
    test "$DELETED" -ge 4

echo ""

# ---- 3. 验证文件已从工作树和 origin/main 中移除 ----
echo "--- 3/3: 文件已不存在 ---"

check "antirobot/log.txt 不存在" \
    test ! -f antirobot/log.txt

check "antirobot/log.txt.1 不存在" \
    test ! -f antirobot/log.txt.1

check "antirobot/123 不存在" \
    test ! -f antirobot/123

check "antirobot/test 不存在" \
    test ! -f antirobot/test

check "www/ 目录不存在" \
    test ! -d www

echo ""
echo "=========================================="
echo "结果：$PASS 通过，$FAIL 失败"
echo "=========================================="

# 如果有失败，非零退出
[ "$FAIL" -eq 0 ] || exit 1
