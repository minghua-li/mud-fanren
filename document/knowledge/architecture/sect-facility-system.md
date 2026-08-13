---
id: sect-facility-system
claim: 门派设施系统由 SECT_FACILITY_D（adm/daemons/sect_facility_d.c）配置驱动承载，18 个设施条目按 9 宗配置，消耗走 SECT_D->add_contribution 与 MONEY_D->player_pay，房间匹配用 base_name(environment(player)) 对照配置 room 字段
tags: [daemon, dbase, command, room, mapping, lpc-syntax]
modules: [adm-daemons, cmds, include, d-areas]
cluster: architecture
kind: pattern
status: current
verified: "2026-08-13"
---

## Why

#60 落地门派设施系统（P4-4：通用设施框架 + 九宗特色设施）时确立的架构约定：设施不是为每个房间手写逻辑，而是由一个配置驱动的守护进程统一承载——facility_config 声明 18 个设施（9 宗 × 通用/特色），玩家在设施房间内执行 `facility` 命令，逻辑全部走 SECT_FACILITY_D。这样新增设施只需加一条配置 + 一个房间，命令层零改动。另有关键接缝：设施消耗（贡献）必须走 #57 门派系统交付的 SECT_D->add_contribution（含扣减与奖励双向），灵石走 MONEY_D->player_pay（单位：文，1 灵石 = 100 文）；设施房间识别不靠房间数据字段，而靠 base_name(environment(player)) 与配置 room 字段精确匹配。

## How to apply

- 设施配置在 `adm/daemons/sect_facility_d.c` 的 `facility_config` mapping 中：每条含 sect（对齐 sect_d.c 九宗 ID）/ type（SECT_FACILITY_PLANT/ALCHEMY/LIBRARY/DEFENSE/TRAINING/SPECIAL）/ room（房间路径不含 .c）/ level+max_level / effect（type+base+per_level）/ duration / use_stone+use_contrib / upgrade（目标等级→({灵石,贡献})，宏 SECT_UPGRADE_COMMON/DEFENSE/SPECIAL/PLANT 来自 include/sect_facility.h）/ daily_limit / daily_reward / market。
- 玩家命令 `cmds/usr/facility.c`：`facility list` 任何位置可用（须本派弟子）；`facility use/upgrade/practice/plant/harvest/read/copy/buy` 需身处对应设施房间；房间内 `set("sect_facility", "<key>")` 仅作可读标注，真正识别是 daemon 的 base_name 匹配。
- **命令面参数口径（c6 约定）**：帮助/列表给玩家展示中文名（灵草/黄龙草/紫丹参、功法名如长春功），子命令参数支持「中文名或拼音 id」双向解析——daemon 提供 `resolve_seed_id`（seed_config 键 ↔ sc["name"]）与 `resolve_skill_id`（SECT_D 技能键 ↔ info["name"]）两个解析函数，命令层参数原样透传、解析在 daemon 侧完成（对齐 cmds/usr/sect.c sect learn 的既有规范；harvest 取地块编号数字，不经名称解析）。改命令面时保持「帮助文本中文名 + 中文名可用」双向一致，勿只改一边。
- 消耗铁律：**先查贡献（SECT_D->query_contribution）后扣灵石（MONEY_D->player_pay），最后 SECT_D->add_contribution(负值)**——顺序颠倒会让贡献不足的玩家白扣灵石；且贡献扣减/奖励**必须走 SECT_D->add_contribution 接口**（不可直接写 sect/contribution，否则绕过 #57 门派系统的日志与互斥链）。该两条性质由 path_verify.py 的「LPC 原文守卫」直接解析 sect_facility_d.c 函数体做机器验证（pay_cost 扣费顺序、base_name 房间匹配、三处 add_contribution 调用——pay_cost/transcribe_skill/practice、plant 灵石扣费 MONEY_D->player_pay、use_facility buff 加成经 grant_buff 授予、resolve_seed_id/resolve_skill_id 中文名双向匹配、命令层 facility.c 参数透传），含 LPC 原文突变实证（颠倒顺序→红；add_contribution 换 player->add→红；plant 扣费删→红；grant_buff 移除→红；中文名分支删→红），不仅停留在 Python 翻译层。贡献奖励（演武场切磋/每日修行）用 add_contribution(正值, "设施修行：<设施名>")。
- buff 存玩家属性 `sect_facility/buffs`（key→({expire,value})），过期查询返回 0；对外钩子 query_danfang_bonus / query_forge_bonus / query_training_bonus / query_defense_bonus 供炼丹/炼器/打坐/战斗系统接入（当前全库无消费者，属后续票接线点），同类型多设施取最大值（query_effect_value）。
- 灵田种植/收获状态存玩家属性 `sect_facility/plots`（key→idx→plot），地块数随设施等级增长（Lv1=2/Lv2=5/Lv3=8，对齐 mansion.h LAND_MAX_PLOTS）；生长时间随等级缩短（speed_factor=100+(lv-1)*25）；收获产出 = 种子基础产量 × (1+药园效率加成/100) + 30% 随机 +1。
- 升级设施是"弟子捐献"模式：内门弟子以上（rank>=1）在设施房间内 `facility upgrade`，扣灵石+贡献，设施等级为全门共享（daemon 内存态，重启丢失——与 mansion_d/sect_hq_d 一致的有意取舍）。
- 数值对齐来源：02-扩充内容/02-区域游戏玩法.md §4.4（炼丹/炼器 +10%、护山大阵 30000/15000、100000/50000、500000/200000、坊市 15000/8000、灵药园贡献 5000）与 mansion.h（PLOT_*/GARDEN_MAINTENANCE/LAND_*）。
- 验证（双层，均入库可复跑）：① `python3 tools/facility/static_check.py`（71 断言静态验收：括号状态机/字符串截断/exits 双向/接口签名/设施配置↔房间一致）；② `python3 tools/facility/path_verify.py`（80 断言：Python 翻译层端到端 + 场景 10 命令层中文名→id 解析全链路 + LPC 原文守卫——pay_cost 扣费顺序、base_name 房间匹配、三处 add_contribution 接口、plant 灵石扣费、use_facility grant_buff、resolve 中文名匹配、命令层 facility.c 透传，含 6 组 LPC 原文/命令层突变实证（颠倒顺序→红；add_contribution 换 player->add→红；plant 扣费删→红；grant_buff 移除→红；中文名分支删→红；命令层硬编码 id→红）。注意分层语义：Python 翻译层验证的是翻译镜像对 LPC 配置字面量的忠实性；LPC 原文守卫验证的是原文关键逻辑顺序——两者都不等于执行 LPC（环境无 fluffos driver），运行时行为仍需人工装驱动复验。
