---
id: reputation-shop
title: 声望商店（shop_d）商品目录与购买交付
tags: [shop, reputation, economy, faction]
updated: 2026-10-31
---

# 声望商店（shop_d）商品目录与购买交付

> 02-扩充内容/02-声望与互动玩法.md 第 3 章的 LPC 落地（#35）；#64/#97 修复悬空引用并补上购买交付。
> 入口：`cmds/usr/faction.c` `faction shop <势力ID>` / `faction buy <势力ID> <物品ID> [数量]`；数据与逻辑在 `adm/daemons/shop_d.c`。

## 目录不变式（item_catalog）

- 行格式固定 11 字段：`({ id, name, type, path, price, rep_req, rep_cost, tier, faction, desc, stock })`，stock `-1` 无限。
- **path 必须是真实存在的 .c**：`/obj/weapon`、`/obj/armor`、`/obj/item`、`/obj/skill` 四个目录本仓不存在——历史 33 条悬空引用已在 #64/#97 逐条判定：21 条改指既有实体（武器 8 → `/clone/weapon/*`、护甲 4 → `/clone/armor/*`、法宝杂物 9 → `/d/yueguo/obj/treasure` 参数化法宝基类），12 条移除（技能书 7：对应功法内容未落地；符箓/空间袋/入场券 5：本仓无对应实体）。新增商品先确认实体存在。
- `tools/check/shop_verify.py` 守卫零 MISS（解析全部路径断言存在 + 购买交付原文守卫 + 突变实证）；改目录前后重跑。

## 购买交付契约（buy_item）

- 顺序固定：等级/敌对/库存/声望/灵石五查 → **`new(path)` 预生成**（生成失败返回 -6，声望/灵石/库存一律不扣）→ 扣费 → **逐个 move 给玩家**（move 失败整体回滚扣减，仍 -6）。
- 参数化法宝基类 `/d/yueguo/obj/treasure`（#74 产物，FORGE_D 炼成时 set 属性）走商店交付时由商品行补 `name`/`id`，`value=price`（value 即交易接线，与 #74 口径一致）；未设 attack/defense，故不可御使/佩戴，只是可持有珍宝。
- `query_available_items` 用 `file_exists` 过滤，源文件不存在的条目永不展示（幽灵商品防线）。
- faction.c 对 -6 的提示是「物品数据缺失，未扣除任何费用」（面向用户的非技术措辞）。

## 已知边界

- 移除的 12 条商品无对应实体；后续阶段要补齐时按 #97 路径②（1E §1.1/§1.3）补最小可用对象。技能书 7 条还依赖对应功法内容落地——`/clone/book` 既有书籍（金蛇秘笈/冰蚕残篇/江湖兵器谱）教授的是无关旧技能，改指会货不对板。
- 库存 `runtime_stock` 是 nosave 内存 mapping（守护进程重启后 init_stock 按目录重初始化），补货接口 `restock_item`/`restock_all`。
