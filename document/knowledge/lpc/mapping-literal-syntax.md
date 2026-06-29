---
id: mapping-literal-syntax
claim: "LPC mapping 字面量用 ([]) 而非 {}，键值之间用冒号而非 =>，如 ([ 'key': value, 'key2': value2 ])"
tags: [lpc-syntax, mapping]
modules: [include, feature]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

LPC 的 mapping 语法与 JSON 或 Python 不同，初学者容易写错。数组用 `({ })`，mapping 用 `([ ])`，之间没有 `=>`。

## How to apply

```lpc
// mapping 字面量
mapping m = ([
    "name": "张三",
    "age": 30,
    "skills": ({ "sword", "dodge" }),
]);

// 数组字面量
string *arr = ({ "a", "b", "c" });

// 空 mapping
mapping empty = ([]);

// 空数组
string *empty_arr = ({});

// 访问
m["name"] → "张三"
```

常见陷阱：在 mapping 中用 `=>` 或 json 风格的 `:` 不带空格——LPC 实际只支持 `key: value`（冒号 + 空格）。
