---
id: lpc-region-data-acceptance
title: LPC 区域数据静态验收方法（无 driver 环境）
tags: [lpc, 静态校验, author-check, 区域, 验收]
updated: 2026-08-13
---

# LPC 区域数据静态验收方法（无 driver 环境）

> 本文档记录**环境无 fluffos driver** 时，LPC 区域数据（纯房间/NPC 数据）的 author_check 写法与 check 脚本踩坑。此路线经 #58（九宗山门地图，62 个新增文件）probe 实测通过，作为后续「建区域类」ticket 的验收基线。
> 关联文档：[[teleport-network]] | [[1D-门派种族声望]] | factions/sects/README

## 背景与适用边界

- 本仓库环境**未安装 fluffos v2019 driver**，LPC 无法运行时编译；项目无测试框架（AGENTS.md 明确「修改后需人工验证」）。
- 因此对**纯区域数据**（房间 `.c` 只含 `create()`/`set()`、NPC 只含 `set()`/`chat_msg` 之类，无运行逻辑）采用**严格静态校验路线**：对每一类机械错误**非零退出**，作为合入主干的客观硬闸门（author_check / check_cmd）。
- **适用边界**：仅限「纯数据」文件。含复杂逻辑的守护进程（`adm/daemons/`）、命令（`cmds/`）仍需人工静态审查 + 后续装 driver 后运行时编译验证，不能只靠本路线。

## 一、author_check 静态校验维度（#58 实测通过）

六类机械错误检查（5 项必查 + 第 6 项增强项），每类独立非零退出，全部通过才算绿：

### 1. 括号配对状态机（LPC 语法配对）

对每个 `.c` 文件，先**屏蔽**字符串、行注释、块注释、LPC heredoc（`@LONG ... LONG` / `@HELP ... HELP` 长文本块），再统计剩余结构字符的 `{} () []` 配对数量。

- **必须先屏蔽再统计**：字符串/注释/heredoc 内可含任意引号、括号、花括号，不屏蔽会误报配对失衡。
- heredoc 是 LPC 特有结构，房间描述/NPC 台词常用 `@LONG` 块，校验脚本容易漏掉——漏掉它，长文本里的括号就会污染统计。
- 引号检查：屏蔽后双引号/单引号数量应为偶数（未被屏蔽的裸引号必为语法错误）。

### 2. `set("exits")` 引用完整性

房间 `set("exits", ([ ... ]))` 中的每个出口值都必须是**真实存在**的房间文件路径（按本票文件集 + 既有 `d/` 目录解析到存在的 `.c` 文件）。

### 3. `set("objects")` 引用完整性

房间 `set("objects", ([ ... ]))` 中的每个 NPC/物件引用（`"/d/xxx/npc/yyy.c"`）都指向真实存在的文件，防止房间引用到不存在的 NPC。

### 4. NPC `set("sect")` 对齐注册表

NPC 的 `set("sect", ...)` 值必须存在于门派系统注册表（`adm/daemons/sect_d.c` 的九宗 ID：yanyue_sect / huangfeng_valley / lingshou_mountain / qingxu_sect / huadao_dock / tianque_fort / jujian_gate / guiling_sect / yuling_sect）。防止脏数据/拼写错误流入门派系统（#57 依赖该字段）。

### 5. teleport 节点一致性

- `include/teleport.h` 的 `TP_NODE_*` 宏与 `adm/daemons/teleport_d.c` `init_teleport_nodes()` 登记一致（无重复、无悬空宏）。
- 每个节点 `TP_FIELD_ROOM` 指向的房间**真实存在**（#33 遗留的大量占位节点 ROOM 指向不存在的路径，见 [[teleport-network]] §三——建区时同步落地）。

### 6.（增强项）exits 双向互逆

A 房间出口指向 B，则 B 也应有回到 A 的出口（单向出口在 MUD 里玩家会「进去出不来」）。#58 全量校验覆盖此项。

## 二、check 脚本踩坑：LPC 路径字面量提取

从 `set("exits")` / `set("objects")` / `set("sect")` 中正则提取路径字面量后，**必须同时去引号并转相对路径**再校验存在性。两个经典错误：

### 坑 1：只去左引号（`p[1:]`）→ 前导斜杠残留，误判成绝对路径

`"d/yueguo/huangfeng/shanmen"` 这类带前导 `/` 的字面量，若只 `p[1:]` 去掉左引号，得到的仍是 `/d/yueguo/...`——前导斜杠还在，与 `ROOM` 宏/base_name 的相对路径比对时会**误判成绝对路径**（`os.path.join` 遇绝对路径还会丢弃前缀，校验结果直接失真）。必须同时 `strip` 两端引号（或 `[1:-1]`）。

### 坑 2：`__DIR__"xxx"` 用 `p[8:]` 切前缀 → 尾部引号残留

LPC 里常用 `__DIR__"dadian"` 拼当前目录。若用 `p[8:]`（切掉 `__DIR__` 的 8 个字符）处理，得到的是 `"dadian"`——**尾部引号被带出来**，路径含引号字符，存在性校验必然失败且报错信息极具误导性（看着像路径不存在，其实是引号没去干净）。

### 正确做法

提取字面量后：① 先同时去掉两端引号（`strip('"')` 或 `[1:-1]`）；② 若前缀是 `__DIR__`，去引号后**拼上当前文件所在目录**再转相对路径；③ 才用拼好的相对路径做存在性校验。顺序不能反。

## 三、复用要点（给后续建区域 / 验收单的 checklist）

- 建区域类 ticket 的 author_check 直接按 §一 六维度写，模板可从 #58 已登记 check 的维度描述复制。
- 校验脚本产出**落盘到临时目录**、不进交付；check 本身作为客观硬闸门登记在平台（check_cmd），实现方不需要（也不应）知道脚本原文。
- 无 driver 的静态校验**不等于**运行时验证：验收结论应如实注明「静态校验通过，运行时待装 driver 后验证」——#58 票面结论即按此口径声明。
- 项目目前未在 AGENTS.md 声明标准验收命令，每张单临时出 check；若后续票量增大，可推动项目级声明（#58 架构验收评论已向用户建议）。
