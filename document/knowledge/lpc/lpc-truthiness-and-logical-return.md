---
id: lpc-truthiness-and-logical-return
claim: "LPC 中空字符串和空数组是真值，&& 返回最后非零值、|| 返回第一个非零值，与 C 的 0/1 语义不同"
tags: [lpc-syntax, pitfall]
modules: [inherit, feature]
cluster: lpc
kind: pitfall
status: current
verified: "2026-08-13"
---

## Why

LPC 的真值判断和逻辑运算返回值与 C 完全不同，容易把"空数组/空字符串应该为假"的直觉带到 LPC 里踩坑。判断对象是否存在的惯用写法是 `objectp()` / `stringp()` 等类型检查，而不是真值判断。

## How to apply

```lpc
// 真假判断：只有 0、0.0、'\0' 是假；空字符串 ""、空数组 ({}) 都是真
if (empty_array)          // 恒真！空数组不等于 0
    write("这里总会执行\n");

// 正确做法：用 sizeof() 判断是否为空
if (sizeof(items) == 0)
    write("数组为空\n");

// && 返回"最后评估的值"而非 0/1
int x = 0 && 42;          // x == 0
int y = 3 && 42;          // y == 42（返回最后一个非零值）

// || 返回"第一个非零值"而非 0/1
int a = 0 || 42;          // a == 42
int b = 3 || 42;          // b == 3（返回第一个非零值）

// 实用模式：返回值默认可直接在 || 中提供 fallback 语义
int rate = query("rate") || default_rate;
```

注意：`""` 是真值这点尤其隐蔽——判断"是否有消息/文本"时要用 `str == ""` 或 `sizeof(str)`，不能 `if (str)`。
