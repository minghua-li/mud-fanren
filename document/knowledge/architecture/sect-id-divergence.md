---
id: sect-id-divergence
claim: 门派 ID 存在设计文档（tianque_fort/huadao_dock/jujian_gate/guiling_sect）与代码 reputation_d.c（tianque_sect/qianyuan_sect 等）两套不一致命名
tags: [daemon, dbase]
modules: [adm-daemons, include]
cluster: architecture
kind: architecture
status: current
verified: "2026-08-12"
---

## Why

2026-08-12 在 `.knowledge/factions/sects/` 新建九个开局宗门档案时，对照代码发现 `adm/daemons/reputation_d.c` 的 `faction_info` 门派 ID 与设计文档（`.knowledge/factions/1D-门派种族声望.md`、新档案）不一致：

- 天阙堡：设计文档/新档案用 `tianque_fort`，代码用 `tianque_sect`
- 鬼灵门：新档案用 `guiling_sect`，代码用 `ghost_spirit_sect`
- 代码还出现了 `qianyuan_sect`（千元派）、`biling_sect`（碧灵派）、`huayang_sect`（化阳派）、`six_pulse_sword`（六脉剑宗）等——这些名字不在原著越国七派（掩月宗/黄枫谷/灵兽山/清虚门/化刀坞/天阙堡/巨剑门）中

设计文档（1D）的 `faction_relations` 结构示例中用的是 `tianque_fort`/`huadao_dock`/`jujian_gate`/`qingxu_sect`，与我的新档案一致；但实现代码走了另一套命名。这是「设计文档与代码现实脱节」的典型案例，后续接门派系统/声望系统时极易因 ID 对不上而出 bug。

## How to apply

- 设计/内容侧（`.knowledge/`）：以 1D 文档与 sects/ 档案的命名为准（`huangfeng_valley`/`yanyue_sect`/`lingshou_mountain`/`qingxu_sect`/`huadao_dock`/`tianque_fort`/`jujian_gate`/`guiling_sect`/`yuling_sect`）。
- 代码侧（`reputation_d.c` 等）：将来实现门派功能时需决定以哪套为权威，将代码 `faction_info` 与设计对齐（尤其天阙堡 `tianque_sect` vs `tianque_fort`、鬼灵门 `ghost_spirit_sect` vs `guiling_sect`，以及代码中四个非原著门派名）。
- 改代码前先 `grep` 全部引用，避免遗漏。
