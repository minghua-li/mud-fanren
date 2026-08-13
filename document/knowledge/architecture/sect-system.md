---
id: sect-system
claim: 门派系统由 SECT_D（adm/daemons/sect_d.c）承载，玩家门派数据存 "sect/" 路径，境界门槛用 tier（境界索引*3+小阶段）比较，realm 属性可能未设置需 exp 兜底
tags: [daemon, dbase, player, command]
modules: [adm-daemons, cmds, include]
cluster: architecture
kind: pattern
status: current
verified: "2026-08-12"
---

## Why

2026-08-12 实现 #57 门派系统（九宗入宗/晋升/贡献/功法）时建立的核心约定，后续 #58 地图、#59 任务链、#60 设施都要接这套接口：

- **玩家门派数据全部挂在玩家 DBASE 的 `sect/` 路径下**：`sect/id`（门派 ID）、`sect/rank`（阶位）、`sect/contribution`、`sect/join_time`、`sect/betrayed`（叛门记录 mapping）、`sect/learned`（已学功法 mapping）。SECT_D 本身只存 nosave 静态配置（九宗 ranks/promote/skills），无存档。
- **境界比较用 tier 模型**：`tier = 境界索引*3 + 小阶段(0初/1中/2后)`，境界索引对齐 quest_chain.h 的 REALM_NAMES。炼气层数折算 1-3 层=初期、4-6=中期、7+=后期（1D §三 内门=炼气7层）。晋升门槛存成 `({ tier, 贡献 })` 对，比较一次搞定。
- **`player->query("realm")` 可能未设置**：目前没有任何代码 `set("realm", ...)`（修炼系统尚未接玩家属性），quest_chain_d/activity_d/economy_bridge_d 都只在读。约定消费方必须兜底：realm 缺失时按 combat_exp 折算（阈值与 reputation_d.c query_daily_cap 一致：10w 炼气/100w 筑基/1000w 结丹/5000w 元婴/2亿 化神）。
- **功法学习只是"习得记录"**：`sect learn` 消耗贡献、按阶位解锁、写入 `sect/learned`，**不**直接灌 kungfu 技能（凡人功法 kungfu skill 文件尚不存在）。将来接 kungfu 时在 learn_skill 里加 set_skill 即可。

## How to apply

- 新功能需要"玩家属于哪个门派/什么阶位"时，统一走 `SECT_D->query_player_sect()` / `query_rank()` / `query_rank_name()`，不要直接读 `sect/` 路径。
- 加门派贡献用 `SECT_D->add_contribution(player, amount, reason)`（自动记 log）；任务链奖励（#59）应走此接口。
- 涉及境界判断的新代码：优先读 realm 字符串（parse 层数/阶段词），缺省时用 combat_exp 兜底，不要把"realm 一定存在"当默认。
- 九宗配置（ranks/promote/skills）改 sect_d.c 的 sect_config；功法解锁阶位改 skill 的 rank 字段；贡献阈值改 include/sect.h 的 SECT_CONTRIB_* 宏。
