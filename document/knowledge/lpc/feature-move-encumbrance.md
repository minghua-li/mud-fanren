---
id: feature-move-encumbrance
claim: "F_MOVE 的 move(dest) 处理负重递归传播（add_encumbrance 沿 container 链向上累加），并检查 TotalLimit(10000)/PlayerHoldLimit(400)/killer 防躲"
tags: [move, dbase, mapping]
modules: [feature]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

`/feature/move.c` 实现了对象的移动、负重和容器层级管理。`move_object(ob)` 是驱动级函数负责实际位置变换，`F_MOVE` 在其上封装了所有游戏逻辑检查。

## How to apply

- `ob->move(dest)` 最常用，内部按顺序：取下装备 → 解析目标 → 负重检查 → 移除自身 → `move_object()` → 添加到目标 → 自动 look
- 负重向上递归：`add_encumbrance(n)` 调 `environment()->add_encumbrance(n)` 直到顶层
- 硬限制：容器总物品 `TotalLimit=10000`，单层 `CurrentLayerLimit=1000`，玩家携带 `PlayerHoldLimit=400`
- `move_or_destruct(dest)` 尝试移动，失败则自动 `destruct`
- 杀手防躲：15 分钟内杀过人的玩家无法进入 `no_fight` 或 `is_xinfang` 房间
- `silently` 参数可抑制移动时的自动 look 消息
