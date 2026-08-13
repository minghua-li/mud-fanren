---
claim: 门派 ID 以 .knowledge/factions/sects/ 九宗档案命名为权威（tianque_fort/guiling_sect 等），reputation_d.c
  的 faction_info 已于 2026-08-12 对齐
cluster: architecture
id: sect-id-divergence
kind: architecture
modules:
- adm-daemons
- include
status: current
tags:
- daemon
- dbase
verified: '2026-08-12'
---

## Why

2026-08-12 新建九个开局宗门档案（`.knowledge/factions/sects/`）时，对照代码发现 `adm/daemons/reputation_d.c` 的 `faction_info` 门派 ID 与设计文档（`.knowledge/factions/1D-门派种族声望.md`、九宗档案）不一致：

- 天阙堡：设计用 `tianque_fort`，代码当时用 `tianque_sect`
- 鬼灵门：设计用 `guiling_sect`，代码当时用 `ghost_spirit_sect`
- 代码曾出现 `qianyuan_sect`（千元派）、`biling_sect`（碧灵派）、`huayang_sect`（化阳派）、`six_pulse_sword`（六脉剑宗）等非原著门派名；魔道侧则混入 `blood_reincarnation`（血影宗）、`heavenly_corpse`（天尸宗）、`yin_sect`（阴煞宗）、`soul_refining`（炼魂宗）等编造名，与 1D 文档的魔道六宗（合欢宗/天煞宗/鬼灵门/御灵宗/天魔宗/阴罗宗）对不上。

同日已将代码 `faction_info` 与 `mutex_relations` 全面对齐设计文档：改名 2 处（`tianque_fort`/`guiling_sect`），删除 8 个非原著门派，补齐 `qingxu_sect`/`huadao_dock`/`jujian_gate`（越国七派）与 `hehuan_sect`/`tiansha_sect`/`yuling_sect`/`tianmo_sect`/`yinluo_sect`（魔道六宗）。当时所有旧 ID 仅在 `reputation_d.c` 一处硬编码，其余系统（`faction.c` 命令、`faction_economy_d.c` 等）均通过 `REPUTATION_D->get_faction_info()` 动态取用，无玩家存档需迁移。

## How to apply

- 门派 ID 的权威命名源是 `.knowledge/factions/sects/` 九宗档案与 1D 文档；新增门派时先查档案，代码与设计冲突以设计为准。
- 修改 `reputation_d.c` 的门派 ID 前，先 `grep` 全部引用；新增门派需同步考虑 `mutex_relations`（如灵兽山叛出与越国六派敌对）与 `faction_prosperity` 初始化。
- 九宗开局档案与 `faction_info` 的 desc 字段可互相印证；desc 以档案定位为准（如掩月宗=法修/双修魁首）。
