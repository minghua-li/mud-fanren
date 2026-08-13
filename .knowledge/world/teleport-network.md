---
id: teleport-network
title: 传送网络与九宗驻地（实现落地）
tags: [world, teleport, 区域]
updated: 2026-08-13
---

# 传送网络与九宗驻地（实现落地）

> 本文档记录传送网络系统（#33 框架）的节点落地现状与九宗驻地目录约定。
> 关联文档：[[1A-人界地理]] | [[WORLD_DESIGN]] | [[1D-门派种族声望]] | factions/sects/README

## 一、传送网络机制（#33）

- 节点在 `adm/daemons/teleport_d.c` 的 `init_teleport_nodes()` 中注册，节点宏定义于 `include/teleport.h`（`TP_NODE_*`）。
- 玩家在**注册节点 `TP_FIELD_ROOM` 指向的房间**内执行 `teleport` 命令：列出可达目标（`teleport list`）、查看详情（`teleport info <id>`）、传送（`teleport <id>`）。
- 入口匹配用 `base_name(environment(me))` 对照节点 ROOM（均不带 `.c` 后缀）；目的地用 `me->move(ROOM)` 传送。
- 境界门槛用 `combat_exp` 近似（`TP_REALM_MORTAL`=凡人 <60000，炼气=60000~500000…），见 `check_realm_requirement()`。费用走 `MONEY_D->player_pay`（基础费 `TP_BASE_INTERCITY`=100 灵石，费用由**源节点** `cost_base` 决定，见 `calculate_cost()`）。

## 二、已落地节点（#58 九宗 + #67 越国世俗区域）

| 节点 | 节点名 | ROOM（实际房间） | 落地票 | 可达 |
|------|--------|------------------|--------|------|
| `TP_NODE_MIRROR_LAKE` | 镜州江湖传送阵 | `/d/city/kedian`（客店） | #33 | yue_sects / tai_nan / jia_yuan / qing_niu |
| `TP_NODE_YUE_SECTS` | 越国七派传送阵 | `/d/yueguo/transmit`（步行通七派山门） | #58 | mirror_lake / tai_nan / huangfeng / jia_yuan / qing_niu / tianluo_sects |
| `TP_NODE_HUANGFENG` | 黄枫谷传送阵 | `/d/yueguo/huangfeng/shanmen` | #58 | yue_sects / tai_nan / ancient_portal |
| `TP_NODE_TIANLUO_SECTS` | 天罗国魔道传送阵 | `/d/tianluo/transmit` | #58 | mirror_lake / yue_sects |
| `TP_NODE_TAI_NAN` | 太南谷传送阵 | `/d/yueguo/tainan/fangshi`（太南谷坊市） | **#67**（原 #33 占位 `/d/wudang/wdroad9`） | mirror_lake / yue_sects / huangfeng / qing_niu |
| `TP_NODE_JIA_YUAN` | 嘉元城传送阵 | `/d/yueguo/jiayuan/dajie`（嘉元城大街） | **#67**（原 #33 占位 `/d/xinyang/kezhan`） | mirror_lake / yue_sects / qing_niu |
| `TP_NODE_QING_NIU` | 青牛镇传送阵 | `/d/yueguo/qingniu/zhenkou`（青牛镇镇口） | **#67** 新增 | mirror_lake / yue_sects / tai_nan / huangfeng / jia_yuan |

- **越国境内互通**：mirror_lake / yue_sects / huangfeng / tai_nan / jia_yuan / qing_niu / tianluo_sects 七节点互联（#58 三节点 + #67 落地三节点）。
- 落地节点的 `TP_FIELD_REALM_MIN` 均为 `TP_REALM_MORTAL`（新手出村即可达）；`tai_nan`/`jia_yuan` 保持 `TP_REALM_QI`（#33 原门槛，炼气期使用）。
- **出生链路（#67）**：`d/newbie/exit.c` 的 `leave` 把新玩家送到 `/d/yueguo/qingniu/zhenkou` 并设 `startroom`；新玩家从青牛镇步行可达七玄门、太南谷坊市，经传送可达九宗山门。

## 三、未落地占位节点（#33 遗留，建区时需同步）

`init_teleport_nodes()` 中以下节点的 ROOM 仍是示例占位路径，**指向不存在的房间**，直接 `teleport` 会失败（返回"无法到达目标地点"）：

- `TP_NODE_ANCIENT_PORTAL` → `/d/lingxiao/room2`（古传送阵）
- `TP_NODE_KUI_XING` / `TP_NODE_INNER_ISLANDS` → `/d/taohuadao/jieyin`；`TP_NODE_OUTER_ISLANDS` → `/d/taohuadao/haitan`（乱星海）
- `TP_NODE_TIAN_XING` → `/d/changan/kedian`（天星城）
- 灵界节点（tian_yuan / san_huang / barbarian / cross_portal / fu_jiao / yun_cheng）→ 各类示例路径

后续为这些区域建图时，应同步把对应节点 ROOM 改为真实房间路径（#67 已示范：tai_nan / jia_yuan 两个占位随建图落地）。

## 四、九宗驻地目录约定（#58）

- 越国七派：`d/yueguo/<宗门拼音>/`——yanyue（掩月宗）、huangfeng（黄枫谷）、lingshou（灵兽山）、qingxu（清虚门）、huadao（化刀坞）、tianque（天阙堡）、jujian（巨剑门）。
- 魔道两宗：`d/tianluo/<宗门拼音>/`——guiling（鬼灵门）、yuling（御灵宗）。
- 每宗标准房间：`shanmen.c`（山门）、`dadian.c`（大殿）、`chuangong.c`（传功阁），黄枫谷另有 `yuexudian.c`（岳麓殿）与 `fangshi.c`（坊市，太岳山脉东北边缘，#67）；NPC 统一放 `<宗>/npc/`。
- NPC 的 `set("sect", ...)` 与 `adm/daemons/sect_d.c` 的九宗 ID 对齐：yanyue_sect / huangfeng_valley / lingshou_mountain / qingxu_sect / huadao_dock / tianque_fort / jujian_gate / guiling_sect / yuling_sect。
- 各宗传送阵枢纽：`d/yueguo/transmit.c`（越国七派）、`d/tianluo/transmit.c`（天罗国两宗），山门与枢纽之间的 exits 双向互逆。
- 驻地 NPC 只做展示与台词（chat_msg），**不含拜师交互**——拜师逻辑由门派系统（#57，`adm/daemons/sect_d.c` + `cmds/usr/sect.c`）提供接口，驻地 NPC 预留 `sect` 属性供其接入。

## 五、越国世俗区域目录约定（#67）

- 世俗区域在 `d/yueguo/` 下按地名单建目录：`qingniu/`（青牛镇，出生地）、`qixuanmen/`（七玄门，镜州彩霞山）、`tainan/`（太南谷修仙坊市）、`jiayuan/`（嘉元城，岚州）、`jingzhou/`（镜州城）。
- **太南谷坊市买卖入口**（`d/yueguo/tainan/fangshi.c`）：`list`/`buy <id> [数量]`/`sell <id>` 交易材料（铁精/灵草/兽皮/黄龙草），定价与记账接 P5-4 经济接口——`ECONOMY_D->register_goods/query_current_price/record_purchase/record_sale`（商品 type 对齐 `include/region_economy.h` 的 `REGION_SPECIAL_PRODUCTS`：ore_basic/herb_basic/hide_basic），结算走 `MONEY_D->player_pay/pay_player`（1 灵石=100 文），境界标识取 `ECONOMY_BRIDGE_D->get_player_realm_code`；材料物件（`inherit ITEM`）带 `is_material`/`material_id` 属性供 `sell` 识别。炼制（FORGE_D/炼丹）不在坊市入口范围。
- 端到端链路：青牛镇（出生）→（步行）七玄门 →（山间驿道）太南谷坊市 →（传送 tai_nan→yue_sects）→ 越国七派传送阵 →（步行）九宗各派山门——全程无境界硬门槛，传送段按 #33 经济模型收费。
