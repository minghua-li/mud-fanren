---
id: pill-system
title: 丹药炼制系统（LPC 实现版）
tags: [pill, danfang, economy, breakthrough]
updated: 2026-08-13
---

# 丹药炼制系统（LPC 实现版）

> 1E 法宝丹药经济 §2 丹药体系 + 02-丹药体系详解 的 LPC 落地实现（#73，#64 子单）。
> 覆盖：丹药实体、丹方与药材、炼丹 daemon（PILL_D）、炼丹命令、筑基丹接突破、成品入经济。

## 一、丹药实体（clone/pill/）

- 基类 `inherit/item/dan.c`（DAN_BASE）：继承 ITEM（F_DBASE/F_MOVE/F_NAME），获得/持有/交易/消耗走既有物品背包接线；自带 `init()` 注册 `eat` 命令，通用 `do_eat` 按 `pill_type` 分发。
- 1E §4.1 六属性（每个丹药必设）：`pill_type` / `stage` / `effect` / `quality` / `side_effect` / `refine_level`。
- 三分类（pill.h PILL_TYPE_*）：
  - `xiuwei` 修为丹：`ROOT_REFINE_D->add_xiuwei` 加修为 + 丹毒累积（PILL_TOXIN，超阈值提示，不做惩罚层）+ 炼气期 `check_qi_layer_up` 自动升层。
  - `breakthrough` 突破丹：境界校验（筑基丹须炼气期服用，stage=目标境界）→ `add_temp("breakthrough/pill_bonus", effect)` 叠加，上限 `PILL_BREAK_MAX_STACK`=3 颗。
  - `heal` 疗伤丹：`receive_curing` 回 qi。
- 品质系数：凡品 ×1.0 / 良品 ×1.5 / 上品 ×2.0（quality_factor，作用于 effect）。

## 二、丹方与药材（PILL_D 数据）

- 丹方结构：`name`（中文）/ `pill`（成品路径）/ `ingredients`（材料 id→数量，id 对齐坊市 `material_id`）/ `base_rate` / `refine_level` / `stage` / `quality`。
- 六个丹方：炼气散（灵草×2）、黄龙丹（黄龙草×2）、聚气丹（灵草×3）、筑基丹（黄龙草×3+灵草×5）、凝丹丸（黄龙草×4+灵草×8）、结金丹（黄龙草×6+灵草×12）。
- 药材来源：#67 坊市（太南谷 `d/yueguo/tainan/obj/lingcao.c|huanglongcao.c`，herb_basic/herb_huanglong）与 #60 灵田种植；炼制时从玩家背包按 `material_id` 扣除。

## 三、成功率与品质（02 §4.3 简化）

- `最终成功率 = base_rate + 炼丹术等级×2 + 丹房加成 − 品级罚值((quality−1)×10)`，钳制 [5,95]。
- 炼丹术等级：玩家 DBASE `pill_refine_exp`（成功炼制 +1），分段换算（1-20 级每级 5 次、21-40 每 8 次、41-60 每 12 次、61-80 每 20 次、81-100 每 50 次）。
- 丹房加成：`SECT_FACILITY_D->query_danfang_bonus`（#60 设施钩子，激活丹房 buff 时生效；炼丹命令任何地点可用，丹房只加成功率）。
- 品质：基准来自丹方，炼丹术 ≥15 有 20% 概率 +1 品、≥30 再 20% 概率（上限上品）。

## 四、筑基丹接突破（#61 接线）

- 服用突破丹写 `breakthrough/pill_bonus` temp；`root_refine_d.c query_major_breakthrough_probability` 补读（与既有 `aux_bonus` 并列）；`tupo.c` 突破成功后清理（失败保留药力）。
- 既有小境界 `query_breakthrough_probability` 也读同一 pill_bonus（#61 预留端），两处共用该 temp。
- 效果示例：伪灵根炼气→筑基 基础 3%，服 1/2/3 颗筑基丹 → 28/53/78%。

## 五、成品入经济

- 炼制成功 `new(pill_path)` move 到玩家（获得）；ITEM 继承天然支持背包/交易/服用消耗。
- PILL_D create() 将成品注册到 ECONOMY_D（`pill_<id>` 类型，base=value/100 灵石），refine_pill 成功时 `record_sale` 记账。
- **shop_d 断链修复**：#35 商品表 13 个丹药引用 `/obj/remedy/*`、`/obj/nicheng/shouwan` 全 MISS → 改指 `/clone/pill/*`（16 个丹药实体在 clone/pill/）。

## 已知边界（checklist_gaps）

- shop_d 购买链路「扣款后待 NPC 领物」未实现（#35 框架既有缺口，所有商品通用，非本票）。
- 丹毒只累积与提示，未做修炼效率惩罚层（02 §5 完整机制待后续）。
- 无丹炉/火候/灵药年份维度（1E §2.3 三要素中仅炼丹术+丹房落地）。
