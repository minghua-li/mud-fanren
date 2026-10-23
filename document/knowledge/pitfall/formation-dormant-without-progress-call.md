---
claim: feature/formation.c 的 formation_active 只由 progress_formation() 置位，缺调用者时阵法永远停在布置中、全部效果休眠
cluster: pitfall
id: formation-dormant-without-progress-call
kind: pitfall
modules:
- feature
- cmds
related:
- formation-system
- combatd-attack-flow
status: current
tags:
- pitfall
- combat
verified: '2026-10-23'
verify_freq: quarterly
---

## Why

`buzhen` 布阵只调用 `start_formation()`，它把 `formation_type`/`formation_setup_ticks` 设好、
`formation_active = 0`；真正把阵法「点亮」的是 `progress_formation()`（setup tick 递减到 0 时
置 `formation_active = 1` 并 `activate_formation_effects()`）。

问题在于 `progress_formation()` 当时**没有任何调用者**：`formation_tick()` 第一行就是
`if (!formation_active) return;`，其余效果函数（`formation_damage_modify()`、聚灵阵回法力、
`formation_spirit_act()`、阵眼分摊、`summon_formation_spirit()`）也都以 `formation_active` 为前提。
结果是：玩家能布阵、能撤阵、战斗日志正常，但阵法效果一项都不生效——**静默失效**，没有任何报错。

修复方式：在 `formation_tick()` 里先推进进度，未激活时调用 `progress_formation()` 并直接返回，
把「每回合推进一回合布阵」交给唯一被战斗流程调用的入口（`adm/daemons/combatd.c` 每次出手会调
`me->formation_tick(me)`）。

## How to apply

- 任何「先进入状态、再靠 tick 生效」的机制，都要检查状态标志的**唯一写入点**有没有被调用：
  `grep -rn "progress_formation" --include=*.c .` 这类反查比读单个文件可靠得多。
- 修 tick 类问题时，把进度推进放在「效果结算之前」，且未激活分支要 `return`，避免进度与效果同
  tick 双重推进。
- 回归守卫：`tools/check/combat_system_verify.py` 断言 `formation_tick` 未激活时先
  `progress_formation()`，并带突变实证（删掉该调用 → 守卫红）；同构模拟断言 3 回合布阵在
  第 4 tick 生效。
- 顺带注意维持成本：`formation_data[*]["cost_per_tick"]` 从玩家 `balance`（文）扣，余额不足时
  阵法会当场消散——测试阵法效果时先确认角色 `balance` 足够。
