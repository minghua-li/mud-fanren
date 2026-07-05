---
id: spirit-root-system
claim: ROOT_REFINE_D（/adm/daemons/root_refine_d.c）是灵根洗练/品质提升/境界突破/debuff管理的中央守护进程，通过玩家 dbase 属性 spirit_root/* 持久化状态
tags: [daemon, dbase]
modules: [adm-daemons, include]
cluster: architecture
kind: architecture
status: current
verified: "2026-07-05"
---

## Why

基于设计文档 `02-扩充内容/02-灵根养成与突破.md` 实现灵根养成系统时，需要一处中央逻辑管理品质（伪/假/真/变异/天）、五行属性、洗练成本曲线、突破概率公式和 debuff 生命周期。因涉及属性跨多个子系统（修炼/战斗/境界），且状态全由单个玩家维护，LPC 中典型的守护进程 + dbase 模式最合适——F_DBASE 继承者用 set/query 路径式访问，无需额外数据库。

## How to apply

- 调用 `ROOT_REFINE_D` 路径宏（已在 globals.h 中定义）获取守护进程引用
- 玩家灵根状态存于 `spirit_root/*` dbase 属性树：quality/strength/purity/elements/main_element/exp/level/debuff 等
- 查询系数：`ROOT_REFINE_D->query_cultivation_speed_factor(ob)` 获取修炼速度倍率，`ROOT_REFINE_D->query_magic_damage_bonus(ob, element)` 获取法术伤害加成
- 洗练流程：调用 `refine_cost_check()` 检查消耗 → `refine_calculate_result()` 获得概率结果 → `refine_apply_result()` 执行
- 突破流程：`query_breakthrough_probability()` 获取概率 → `do_breakthrough()` 执行（含15%随机事件）
- debuff 通过 `apply_root_debuff()` / `remove_root_debuff()` 管理，自动过期清理
- 品质提升用 `do_quality_upgrade(ob, item_type)`，item_type 为 "净灵莲" 或 "补天丹"
