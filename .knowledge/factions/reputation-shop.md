---
id: reputation-shop
title: 声望商店（shop_d）商品目录与购买交付
tags: [shop, reputation, economy, faction]
updated: 2026-11-01
---

# 声望商店（shop_d）商品目录与购买交付

> 02-扩充内容/02-声望与互动玩法.md 第 3 章的 LPC 落地（#35）；#64/#97 修复悬空引用并补上购买交付。
> 入口：`cmds/usr/faction.c` `faction shop <势力ID>` / `faction buy <势力ID> <物品ID> [数量]`；数据与逻辑在 `adm/daemons/shop_d.c`。

## 目录不变式（item_catalog）

- 行格式固定 11 字段：`({ id, name, type, path, price, rep_req, rep_cost, tier, faction, desc, stock })`，stock `-1` 无限。
- **path 必须是真实存在的 .c**：`/obj/weapon`、`/obj/armor`、`/obj/item`、`/obj/skill` 四个目录本仓不存在——历史 33 条悬空引用已在 #64/#97 逐条判定：21 条改指既有实体（武器 8 → `/clone/weapon/*`、护甲 4 → `/clone/armor/*`、法宝杂物 9 → `/d/yueguo/obj/treasure` 参数化法宝基类；其中庚精后在 #64 收口轮再改指真实材料实体 `/d/yueguo/tainan/obj/gengjing`——treasure 基类无 `is_material`，不能入 FORGE_D 炼器配方，同名挂接即货不对板），13 条移除（技能书 7：对应功法内容未落地；符箓/空间袋/入场券 5：本仓无对应实体；天玄石 1：无对应真实材料实体，收口轮移除）。新增商品先确认实体存在。
- `tools/check/shop_verify.py` 守卫零 MISS（解析全部路径断言存在 + 购买交付原文守卫 + 突变实证）；改目录前后重跑。

## 购买交付契约（buy_item）

- 顺序固定：等级/敌对/库存/声望/灵石五查 → **`new(path)` 预生成**（生成失败返回 -6，声望/灵石/库存一律不扣）→ 扣费 → **逐个 move 给玩家**（move 失败整体回滚扣减，返回 -7，无净扣费；其中声望回补走退款专用接口，见下节）。
- 参数化法宝基类 `/d/yueguo/obj/treasure`（#74 产物，FORGE_D 炼成时 set 属性）走商店交付时由商品行补 `name`/`id`，`value=price`（value 即交易接线，与 #74 口径一致）；未设 attack/defense，故不可御使/佩戴，只是可持有珍宝。
- `query_available_items` 用 `file_exists` 过滤，源文件不存在的条目永不展示（幽灵商品防线）。
- faction.c 对两类失败分开提示（面向用户的非技术措辞）：-6 商品数据异常、-7 背包无法容纳该物品，两类均明示「未扣除任何费用」。

## 声望扣费与回退不对称（reputation_d）

- 扣费路径 `deduct_reputation`（3 参）不查每日上限、不触发势力互斥、不留日志；正常获取路径 `add_reputation`（4 参、带 reason）受每日上限钳制、触发互斥扣减（STRONG 30% / WEAK 10%，下限死敌值）、仅 reason 非空才写日志。两路径不对称是既有设计：获取带上限+互斥，扣减不带。
- 购买失败回滚的回补**必须**用退款专用接口 `refund_reputation(player, faction, amount, reason)`：裸回补——不查上限、不触发互斥（与扣费侧对称，玩家与互斥势力均无单向净损）、强制带 reason 写审计日志（reputation 日志，reason 缺失则拒绝并返回 0、不落日志不改动）。若回补走 `add_reputation`，会被每日上限钳制（极端情形一分不回）并连带压低互斥势力 30%/10%——一次没成交的购买反而伤了其他势力关系。
- `tools/check/shop_verify.py` 第六节守卫本契约（四参签名 / 不触每日上限 / 不触互斥 / 强制写日志 / 强制 reason / buy_item 回滚不得残留 add_reputation）+ 第七节突变实证（回滚改回 add_reputation 守卫红）。
- 剩余缺口：扣费侧 `deduct_reputation` 仍不留日志（对称缺口，待补；#105 只修了回补侧）。

## 已知边界

- 移除的 12 条商品无对应实体；后续阶段要补齐时按 #97 路径②（1E §1.1/§1.3）补最小可用对象。技能书 7 条还依赖对应功法内容落地——`/clone/book` 既有书籍（金蛇秘笈/冰蚕残篇/江湖兵器谱）教授的是无关旧技能，改指会货不对板。
- 库存 `runtime_stock` 是 nosave 内存 mapping（守护进程重启后 init_stock 按目录重初始化），补货接口 `restock_item`/`restock_all`。
