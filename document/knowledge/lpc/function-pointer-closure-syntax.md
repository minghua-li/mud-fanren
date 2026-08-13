---
claim: 'LPC 函数指针用 (: call_other, path, ''func'', args :) 语法，post_action、call_out 等回调大量依赖它，用
  evaluate() 调用'
cluster: lpc
id: function-pointer-closure-syntax
kind: pattern
modules:
- kungfu
- adm-daemons
related:
- fluffos-v2019-syntax-limits
status: current
tags:
- lpc-syntax
verified: '2026-08-13'
---

## Why

LPC 没有 C 语言的函数指针类型，用 `(: ... :)` 闭包语法表示"待调用的函数"作为值传递。项目里 `kungfu/skill/mantian-xing.c` 的 `post_action`、各技能的 `start_call_out` 都依赖此机制，理解它才能看懂武功绝招如何把回调挂到战斗流程上。

## How to apply

```lpc
// 最常见的三种形式
(: function_name :)                       // 直接引用本对象函数
(: call_other, "/adm/daemons/weapond", "throw_weapon" :)  // 跨对象调用（技能 post_action 典型写法）
(: $1 + $2 :)                             // 表达式闭包，$1/$2 是调用时的参数占位符

// 存入 dbase 供战斗系统回调
set("post_action", (: call_other, "/adm/daemons/weapond", "throw_weapon" :));

// 调用：evaluate() 执行，或 (*fp)(args) 语法
evaluate(fp, this_player(), target);
```

注意：
- 函数指针带上参数写法 `(: call_other, path, "func", arg1, arg2 :)` 会把参数一并封装进去，回调时只需传入剩余参数
- 驱动版本差异（实测 v2019 驱动）：**不支持**新版 FluffOS 的简化赋值 `function f = add;` 和 `f(args)` 直接调用，必须用 `(: add :)` + `evaluate()`/`(*fp)()`
- 匿名函数 `function(int x) { ... }` 在 v2019 支持（实测通过）

关键文件：`kungfu/skill/mantian-xing.c`、`kungfu/skill/duanjia-xinfa/powerup.c`（`start_call_out((:call_other, __FILE__, "remove_effect", me, skill/4 :), skill)`）。
