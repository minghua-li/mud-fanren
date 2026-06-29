---
id: macro-path-reference-system
claim: "/include/globals.h 定义了所有关键路径宏（ROOM/NPC/COMBAT_D/SKILL_D 等），被驱动自动 include，修改后需重启游戏"
tags: [inherit, dbase, daemon]
modules: [include, adm-daemons]
cluster: architecture
kind: invariant
status: current
verified: "2026-06-29"
---

## Why

`config.ini` 中 `global include file : <globals.h>` 指定了自动包含的头文件。所有对象的路径引用都通过宏定义，而不是硬编码字符串。这是一个关键架构约束。

## How to apply

- 引用房间基类：用 `ROOM` 而非 `"/inherit/room/room"`
- 引用 NPC：用 `NPC` 而非 `"/inherit/char/npc"`
- 引用技能：用 `SKILL_D("sword")` 而非 `"/kungfu/skill/sword"`
- 引用守护进程：用 `COMBAT_D` 而非 `"/adm/daemons/combatd"`
- ⚠️ 如果 `globals.h` 中没有定义新路径，必须添加对应宏，否则代码可读性和可维护性会下降
- ⚠️ 修改 `globals.h` 后需要重启驱动才能生效（不会热加载）
