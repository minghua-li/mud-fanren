---
id: character-attribute-system
claim: "F_ATTRIBUTE 的 query_str/int/con/dex() 组合了先天基础值、临时装备加成和技能等级加成（最低 10），并支持 Chem/HitRate/PoisonRate 等战斗修饰符"
tags: [combat, dbase, player]
modules: [feature, adm-daemons]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

`/feature/attribute.c` 计算角色的四维属性（膂力、悟性、根骨、身法）。每个属性值由三层叠加：先天 base、临时 apply、技能加成。

## How to apply

属性计算公式：
```lpc
query_str() = max(10, query("str") + query_temp("apply/strength")
    + (最高空手技能/10) + ... );
query_int() = max(10, query("int") + query_temp("apply/intelligence")
    + query_skill("literate", 1)/10 + ... );
query_con() = max(10, query("con") + query_temp("apply/constitution")
    + query_skill("force", 1)/10 + ... );
query_dex() = max(10, query("dex") + query_temp("apply/dexterity")
    + query_skill("dodge", 1)/10 + ... );
```

- 每个属性最低 10，不会低于此值
- 经脉系统（`F_VEIN->AttrBonus()`）提供额外加成
- 战斗修饰符见 `query_enhance()`（Chem/HitRate/PoisonRate）和 `query_proof()`（AntiChem/AntiBusy/AntiHitRate）
- 版本号 `query_attr_version()` 返回 141125，用于公式迁移
