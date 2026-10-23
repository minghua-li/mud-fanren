---
claim: 玩家境界只有 realm_index（1=炼气…8=大乘）与 realm/realm_sub 三个字段，query("realm_level") 恒为
  0
cluster: pitfall
id: realm-level-field-missing
kind: pitfall
modules:
- cmds
- adm-daemons
- include
related:
- combat-realm-spirit-root-damage
- sect-system
status: current
tags:
- pitfall
- dbase
- combat
verified: '2026-10-23'
verify_freq: quarterly
---

## Why

#61 定下的境界存储约定是 `realm`（中文串，如「炼气7层」）+ `realm_index`（大境界索引，
0=凡人 1=炼气 … 8=大乘）+ `realm_sub`（初期/中期/后期…），读取统一走
`SECT_D->query_cultivation_tier()`（tier = (realm_index-1)×3 + 小阶段）或
`ROOT_REFINE_D->query_player_realm_index()`。

但 `cmds/std/buzhen.c` 与 `feature/formation.c` 早期代码查的是 `query("realm_level")`——
这个字段**从来不存在**，取值恒为 0：

- `buzhen` 里 `if (!realm) realm = 1;` 兜底成 1，于是「颠倒五行阵需炼气后期（原码 realm<3）」
  与「大庚剑阵需筑基（原码 realm<4）」**永远布不出来**（玩家看着修为够了，却被拒）；
- `summon_formation_spirit()` 用 `total_realm += ob->query("realm_level")` 求平均修为，
  结果阵灵永远按 0 修为生成，强度不随修为成长。

## How to apply

- 判断境界一律用 `SECT_D->query_cultivation_tier(me)`（高等级判断用 `include/sect.h` 的
  `SECT_TIER_QI_LATE`(2) / `SECT_TIER_ZHU`(3) / `SECT_TIER_JIE`(6) / `SECT_TIER_YING`(9) 常量），
  或 `ROOT_REFINE_D->query_player_realm_index()` 拿大境界索引；不要直接读未定义的 dbase 键。
- 新增「境界/修为」判定前，先在 `include/globals.h` 与 `adm/daemons/root_refine_d.c` 的境界接线
  注释里确认字段名；`query("realm_level")`、`query("level")` 之类都是历史残留。
- 排查技巧：`grep -rn 'query("realm_level")' --include=*.c .` 一次查全仓（本次仅 2 处，均已修）。
- 回归守卫：`tools/check/combat_system_verify.py` 断言 `buzhen` 与阵灵用 tier、且不再出现
  `query("realm_level")`，并带突变实证（改回 `realm_level` → 守卫红）。
