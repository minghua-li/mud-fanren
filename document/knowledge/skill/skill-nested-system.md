---
id: skill-nested-system
claim: "F_SKILL 支持用 :: 分隔符的子技能（最多 3 层），如 music::gu_qin::gaoshan_liushui，set_skill/query_skill 递归处理嵌套 mapping"
tags: [skill, dbase, mapping]
modules: [kungfu, feature]
cluster: skill
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

`/feature/skill.c` 实现技能系统，支持根技能下多层子技能。例如 `music` → `music_theory` → `gu_qin` → `gaoshan_liushui`。`set_skill()` / `query_skill()` 内部用 `explode(skill, "::")` 解析层级。

## How to apply

```lpc
// 设置根技能
set_skill("sword", 100);

// 设置子技能（最多 3 层）
set_skill("music::gu_qin", 80);

// 查询（层级自动遍历）
query_skill("gu_qin");         // 通过子技能映射查找
query_skill("music::gu_qin");  // 精确路径查询

// 技能映射
query_skill_map();    // 当前启用的技能映射
query_skill_prepare(); // 当前准备的技能
```

- `skills mapping` 可能存 `int`（根技能等级）或嵌套 mapping（子技能树）
- `::` 分隔不超过 3 层
- `learned mapping` 记录每个技能的经验点数
- `skill_death_penalty()` 在玩家死亡时削减技能
