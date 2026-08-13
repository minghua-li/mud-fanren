---
claim: LPC 字符串/数组支持范围索引 str[n1..n2]、右索引 str[<n] 和切片 str[n..]，越界自动截断
cluster: lpc
id: string-array-range-indexing
kind: pattern
modules:
- kungfu
- inherit
related:
- fluffos-v2019-syntax-limits
status: current
tags:
- lpc-syntax
verified: '2026-08-13'
---

## Why

LPC 的下标语法比 C 灵活：既能范围切片也能从右数。项目里少林 `kungfu/class/shaolin/qing-fa.c`、`xuan-tong.c` 用 `name[0..0]` 取名字首字来判定辈分字，避免写 C 风格的手动循环拼接。

## How to apply

```lpc
string s = "abcd";

s[0..1]      // "ab"，范围切片
s[<1]        // "d"，从右数第 1 个（右索引）
s[1..]       // "bcd"，从 1 到末尾
s[..2]       // "abc"，从头到 2
s[2..5]      // "cd"，越界自动截断到边界（不会报错）

// 数组同样支持
string *arr = ({ "a", "b", "c" });
arr[1..2]    // ({ "b", "c" })
arr[<1]      // "c"

// 实际项目用法：取名字首字判断辈分
if (name[0..0] == "澄")
    new_name = "澄" + name[1..];
```

要点：
- `str[<n]` 等价于 `str[sizeof(str)-n]`，`<1` 是最后一个
- 越界范围索引自动截断而非报错，但单索引 `str[99]` 越界会报错
- **按字形簇/Unicode 码点索引，不是字节**（实测 v2019：`strlen("你好")==2`、`"你好"[0..0]=="你"`、`strlen("👍🏽")==1`），中文首字直接用 `str[0..0]` 即可，无需按 UTF-8 字节数切片
- `strlen()` 返回字形簇数，`strwidth()` 返回显示宽度（全角占 2），对齐输出用 `strwidth` + `sprintf`
