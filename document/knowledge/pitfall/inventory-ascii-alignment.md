---
id: inventory-ascii-alignment
claim: "cmds/usr/inventory.c 的装备界面 ASCII 人形图各部位 body art 宽度必须一致（9 字符），否则右侧装备标签逐行错位"
tags: [pitfall, command, message]
modules: [cmds]
cluster: pitfall
kind: pitfall
status: current
verified: "2026-06-29"
---

## Why

`inventory.c` 的 `main()` 函数用 7 行 `sprintf` 拼接出人形图，每行格式为：
```
│%29s [左标签]__<body art>__[右标签] %-29s│
```

`%29s` 和 `%-29s` 是固定宽度字段。如果各行 `<body art>` 可见字符数不同，则右标签 `__[xxx]` 的起始位置逐行偏移，整个人形图看起来"歪"。

原代码 7 行 body art 宽度分别为 9/11/8/7/10/9/9，错位最大达 4 字符。

## How to apply

修改时必须保证所有 7 行的 body art 可见字符宽度相同（当前为 9）。

验证方法：用脚本提取每行 `]__` 和 `__[` 之间的内容，确认长度一致。注意 `bra` 和 `lip` 是运行时变量（各 1 字符），不计入字符串字面量。

相关常量：`skin_color`（男性=YEL/女性=WHT）、`skin_b`（背景色）、`bra`（男性="█"/女性="▂"）、`lip`（男性="﹀"/女性="一"）。
