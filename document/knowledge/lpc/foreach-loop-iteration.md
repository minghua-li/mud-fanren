---
claim: LPC foreach 循环可遍历数组/mapping/字符串，mapping 用 foreach(k, v in m) 同时取键值，ref var
  可写回原数组
cluster: lpc
id: foreach-loop-iteration
kind: pattern
modules:
- kungfu
- feature
related:
- fluffos-v2019-syntax-limits
status: current
tags:
- lpc-syntax
verified: '2026-08-13'
---

## Why

LPC 没有 C 的下标式遍历习惯，`foreach` 是遍历数组和 mapping 的标准写法。项目里 `kungfu/music/shimian-maifu.c` 用 `foreach(listener in listeners)` 遍历房间内听众。

## How to apply

```lpc
// 遍历数组
foreach (ob in inv)
    tell_object(ob, "你收到了通知。\n");

// 遍历 mapping：同时取键值
foreach (key, value in skills)
    write(key + " = " + value + "\n");

// 遍历字符串（逐字符）
foreach (ch in str)
    if (ch == ' ')
        continue;

// ref 引用：直接修改原数组元素（注意：ref 前必须带类型，实测 v2019 驱动裸 `ref n` 编译失败）
foreach (int ref n in nums)
    n *= 2;              // nums 元素变为 2 倍

// 遍历字符串（逐 Unicode 码点，非字节）
foreach (int ch in "ab")
    write(ch);           // 输出 97 98
```

要点：
- mapping 用 `foreach(k, v in m)`，单变量形式 `foreach(k in m)` 只取键
- 普通循环变量是值拷贝，只有 `ref` 修饰才能写回原数组；**裸 `foreach (ref n ...)` 编译失败，必须写 `foreach (int ref n ...)`**
- foreach 遍历字符串按字形簇/码点，中文一个字符一个迭代（`foreach (int ch in "你好")` 两次迭代）
- 不要在多线程/时序场景里遍历中修改数组长度
