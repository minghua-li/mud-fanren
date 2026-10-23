---
claim: calc_damage 在五行克制之后按 realm_power_modifier_permille 施加境界压制、对法术攻击按 spirit_root_damage_info
  施加灵根倍率
cluster: combat
id: combat-realm-spirit-root-damage
kind: architecture
modules:
- adm-daemons
- include
related:
- combatd-attack-flow
- spirit-root-system
- character-attribute-system
status: current
tags:
- combat
- daemon
- dbase
verified: '2026-10-23'
verify_freq: quarterly
---

## Why

1F §4.1 与 02-战斗机制与平衡.md §2.1/§2.2 要求境界差影响伤害、灵根影响法术威力，而 #61 交付的
`realm_index`/`realm_sub` 只在门派/任务判定里被消费，`adm/daemons/combatd.c` 的伤害结算
（`include/combat/damage.h` 的 `calc_damage`）完全没有读境界或灵根——战斗数值与境界成长线脱节。

实现时踩到两个非 obvious 的坑：

1. **不能按 combat_exp 猜 NPC 境界**。若对未声明境界的 NPC 走
   `SECT_D->query_cultivation_tier()`（其 realm 缺失时回退 `exp_to_tier(combat_exp)`），
   历史 NPC（大量 combat_exp 数十万）会被判成筑基期，玩家打它们伤害一律 ×0.5，等于静默改写全服 PVE 平衡。
   因此 `realm_power_index()` 只读 `realm_index` / `realm` 两个显式属性，任一方取不到境界就返回中性 1.0。
2. **灵根倍率只对法术生效**。§2.2 的「天灵根 1.5×」是全法术基础倍率，若套到物理/剑诀伤害上，
   现有武学数值会被整体放大 50%，所以灵根块由「本回合招式槽是 spells」或「perform 预先写入
   `combat/spell_elem`」门禁；物理攻击永远不吃灵根倍率。

另有一个口径分歧：§2.2 写「变异灵根：变异属性 +25%（常规属性 -10%）」，而 `spirit_root["elements"]`
里变异灵根也存了构成它的基础五行（雷=金+水），若按 `member_array` 判同系，雷灵根的金系法术会被
误判成同系而 +25%。`spirit_root_damage_info()` 因此对 `quality_idx == 1` 走单独分支：同系只认
`variant`，基础五行按常规属性吃 -10%。

## How to apply

- 新战斗数值接线一律放 `include/combat/realm_power.h`（纯函数、无 daemon 依赖），由
  `include/combat/damage.h` 在五行克制块之后、`special_armor_effect()` 之前调用；顺序不可颠倒，
  否则五行系数与境界系数的乘算上下文（护甲减免）不一致。
- 境界读 `realm_index`（1=炼气 … 8=大乘，0=凡人）→ 内部换成 0=炼气 的序号；小阶段读
  `realm_sub`（初期/中期/后期/巅峰/大圆满），缺失按「中期」中性处理。凡人（`realm_index == 0`）
  与未声明一律中性。
- 系数：同境界内每差 1 小阶段 1.2 / 0.85（累计上限 1.44）；大境界差 1 档 2.0 / 0.5；越级 ≥2 档 0.25 下限。
- 灵根表（`realm_root_base_pm` / `realm_root_same_pm` / `realm_root_penalty_pm`，千分比）：
  天 1500/+300/-150、变异 1300/+250/-100、真 1000/+150/-50、假 800/+100/-30（文档未列，按真伪插值）、
  伪 600/+50/0；同系加成再按 `spirit_root["strength"]`（0-100）折算。
- #76 法术 perform 出手前写 `me->set_temp("combat/spell_elem", ELE_*)`（并可用
  `combat/attack_type` = "spell"）即可让灵根倍率与法术元素生效；不写时回退角色主属性
  `query_character_element()`。
- NPC 若要参与境界压制，显式 `set("realm_index", n)` + `set("realm_sub", "初期")`
  （或 `set("realm", "筑基初期")`）；不要指望从 combat_exp 自动推断。
- 自检：`python3 tools/check/combat_realm_verify.py`（静态契约 + LPC 原文守卫 + 突变实证 + 同构模拟）。
