---
id: secret-realm-entry-standard
title: 血色禁地进入境界口径：筑基中期起（realm_index=2 + realm_sub 中期及以上）
tags: [realm, secret-realm, design]
updated: 2026-11-01
---

# 血色禁地进入境界口径：筑基中期起

## 口径

血色禁地（blood_forbidden）进入境界 = **筑基中期起**：`realm_index >= 2` 且同大境界内子阶段 >= 中期（`realm_sub` 中期/后期/巅峰/大圆满；`realm_index > 2` 的高境界直接放行）。炼气期一律不可进入。

## 依据（三方一致）

- 主线 mq_1_9「血色禁地试炼」：`main_quest_d.c` 节点 `realm_range {1,2}` + 描述「筑基中期，你获得进入血色禁地的资格」。
- 1G-任务副本奇遇 §3.2.1：推荐境界「筑基中期—筑基后期」。
- 宗门事件「血禁试炼」：境界条件 (1,1)（仅筑基期，见 `sect_quest_d.c` check_event_conditions 的 realm_min/realm_max 语义注释）。

## 判旧的口径（不采用）

- `02-扩充内容/02-区域游戏玩法.md` §2.2.1「血色禁地（炼气期）/进入门槛：炼气期」——与主线/1G/宗门事件三方冲突，判旧，不采用。
- 守护进程旧值 `MIN_LEVEL=5`/`MAX_LEVEL=15`（读玩家 `level` 数值属性）——#31 产物，`level` 属性不是 #61 境界存储（见 [[realm-storage-convention]]），不是口径依据。

## 实施接口

- 境界读取统一走 `ROOT_REFINE_D->query_player_realm_index(ob)` / `query_player_sub_stage(ob)`（子阶段 0=初期 1=中期 2=后期），不要自造解析（约定见 [[realm-storage-convention]]）。
- 门槛判定：`realm_index > min_idx` 直接放行；`realm_index == min_idx` 时要求 `sub_stage >= min_sub`；低于则拒。血色禁地即 min_idx=2 / min_sub=1。
- LPC 注意：mapping 缺键取 0 且 `intp(0)` 为真，判「是否注册了境界门槛字段」必须用 `!undefinedp(...)`，不能用 intp 真值判断。
