---
id: dbase-path-access-pattern
claim: "F_DBASE 的 query/set 支持路径式访问（query('apply/attack')），通过 '/' 分割 mapping 层级"
tags: [dbase, mapping, lpc-syntax]
modules: [feature]
cluster: dbase
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

`F_DBASE` 的 `query()`/`set()` 方法内部用 `strsrch(prop, '/')` 检测路径分隔符，然后调用 `_set/_query` 在嵌套 mapping 中递归存取。这让属性可以按命名空间组织（如 `"apply/attack"`、`"skill/sword"`）。

## How to apply

- `set("apply/attack", 20)` 等价于 `dbase["apply"]["attack"] = 20`
- `set_temp("apply/attack", 10)` 操作 `tmp_dbase`，不持久化到磁盘
- 在 NPC 武器配置中常用 `set_skill("sword", 100)` 和 `set_temp("apply/attack", 20)`
- `add("qi", -10)` 可以直接做数值增减
