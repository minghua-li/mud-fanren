---
id: combatd-attack-flow
claim: "COMBAT_D 的 do_attack() 通过 ap（攻击力与技能计算）、dp（防御与闪避）、pp（招架）三方判定命中，支持四种攻击类型"
tags: [combat, daemon, dbase]
modules: [adm-daemons, include]
cluster: combat
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

`/adm/daemons/combatd.c` 是战斗系统的核心引擎。`do_attack()` 计算攻击流程、伤害类型、命中判定、招架和反击。战斗数据通过 8 个独立的 include 文件组织（`message.h`, `damage.h`, `probable.h` 等）。

## How to apply

- `do_attack(me, victim, weapon, attack_type)` 执行一次攻击
- 攻击类型 `attack_type`：`TYPE_REGULAR`（普攻）、`TYPE_QUICK`（反击）、`TYPE_RIPOSTE`（招架反击）、`TYPE_SPECIAL`（绝招）
- ap = 攻击者技能 + 攻击加值；dp = 防御者技能 + 防御加值；pp = 招架技能 + 招架加值
- 命中判定：`ap > dp` 命中，否则判定是否被 `pp` 招架
- 伤害类型在 `combat/damage.h` 定义，支持 `combat/damage_xxx.h` 细分
- 奖励在 `combat/reward.h` 中处理（经验、潜能、声望等）
