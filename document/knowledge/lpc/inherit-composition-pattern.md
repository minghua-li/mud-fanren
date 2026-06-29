---
id: inherit-composition-pattern
claim: "LPC 对象通过多重继承 feature/ 获得能力（inherit F_DBASE + F_MOVE + F_COMBAT），而非深层次类继承"
tags: [inherit, lpc-syntax]
modules: [inherit, feature]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

LPC 不支持接口或 trait，通过多重继承 `feature/` 下的 mixin 文件（如 `F_DBASE`, `F_MOVE`, `F_COMBAT`）实现横切关注点的组合。这是 MUD 对象系统的核心架构模式。

## How to apply

- 所有角色类继承 `CHARACTER`（`/inherit/char/char.c`），它已组合了 22 个 F_ 特性
- 房间继承 `ROOM`（`/inherit/room/room.c`），组合了 `F_DBASE + F_CLEAN_UP`
- 自定义对象可继承 `/clone/` 下的模板（武器、盔甲等），它们已组合了所需特性
- 除非需要独特行为，否则不要直接 `inherit F_XXX`，而是 `inherit NPC/ROOM/ITEM` 等基类
