---
claim: LPC 铁律：所有局部变量必须在函数/代码块开头（任何语句之前）声明，同一作用域不可重复声明，与 C 不同
cluster: lpc
id: lpc-declare-vars-at-block-start
kind: pitfall
modules:
- inherit
- kungfu
related:
- fluffos-v2019-syntax-limits
status: current
tags:
- lpc-syntax
- pitfall
verified: '2026-08-13'
---

## Why

LPC 的变量声明规则比 C 严格：函数内所有局部变量必须在函数开头、任何可执行语句之前声明完毕；C 允许的"用到处声明"（如 for 循环里 `int i`）在 LPC 中编译报错。项目内全部 `.c` 文件都遵循"函数开头集中声明"的写法。

> 驱动版本差异（实测 v2019 驱动 20230604）：**必须开头声明**，中途声明直接 `syntax error`。新版 FluffOS（2026 基线）已支持 C99 风格任意位置声明，但本项目驱动不支持，升级驱动前不要依赖此特性。

## How to apply

```lpc
// 正确：变量全部在函数顶部声明
void do_something(object me)
{
    object target;
    int i, total;
    string name;

    total = 0;
    for (i = 0; i < 5; i++) {
        total += i;
    }
}

// 错误（C 习惯，LPC 编译报错）：
// void bad() { write("hi\n"); int x = 5; }
// int i; for (int i = 0; ...)  // 循环内声明不行
```

要点：
- 循环变量也要在函数开头声明（`int i;`），for 初始化处只做赋值
- 同一作用域内不可重复声明同名变量（C 允许内层遮蔽外层，LPC 不允许）
- 想中途定义新变量，可以开一个独立代码块 `{ ... }` 内声明
