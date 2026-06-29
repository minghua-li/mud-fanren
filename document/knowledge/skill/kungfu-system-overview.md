---
id: kungfu-system-overview
claim: "武功系统在 /kungfu/ 下分 skill/（技能）、class/（门派）、special/（绝招）、condition/（内功心法）、music/（音乐）、profession/（职业）六个模块"
tags: [skill, dbase, mapping]
modules: [kungfu]
cluster: skill
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

北大侠客行有 717 种武功技能，门派 30+。武功系统的模块化设计支撑了整个战斗、学习、练习体系。

## How to apply

- 技能文件在 `/kungfu/skill/`，每个技能一个 `.c` 文件，继承 `SKILL`（`/inherit/skill/skill.c`）
- 门派文件在 `/kungfu/class/`，通过 `CLASS_D(x)` 宏引用
- 绝招（perform）在 `/kungfu/special/`，通过技能文件的 `setup_special()` 注册到技能对象
- 宏 `SKILL_D("sword")` 展开为 `"/kungfu/skill/sword"`
- 玩家/ NPC 通过 `set_skill("sword", N)` 获得技能，`query_skill("sword")` 查询
