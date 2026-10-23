---
claim: 五行法术走 SPELL_D 的 cast_spell 与 shifa 命令施展，法术元数据由各 skill 的 query_spell_info()
  单真源提供
cluster: combat
id: elemental-spell-casting
kind: architecture
modules:
- adm-daemons
- cmds
- kungfu
- include
related:
- combat-realm-spirit-root-damage
- sect-skill-learning-chain
- combatd-attack-flow
status: current
tags:
- combat
- skill
- command
- daemon
verified: '2026-10-23'
verify_freq: quarterly
---

## Why

1F §1.1 的五行基础法术此前完全没有落地：`kungfu/skill/` 下没有任何五系法术文件，`combatd.c` 也没有
法术施展入口。法术 skill 虽然能 `valid_enable("spells")`，但那个招式槽只有「武器 `skill_type` 为
spells」才会被 `choose_attack_skill()` 选中，实际战斗中永远不会被选中——等于「有功法、无法术」。

实现时踩到三个非 obvious 的坑：

1. **skill 对象没有 dbase**。`inherit SKILL` 走 `F_SSERVER`，不继承 `F_DBASE`，所以 1F §5.1 范例里的
   `set("attr", ...)` / `set("type", "法术")` 在法术文件里根本无法执行（`set()` 未定义）。法术元数据
   因此统一由各 skill 文件暴露 `mapping query_spell_info()` 返回，SPELL_D 只做别名索引与流程，
   不在 daemon 里复制数值——单真源，改数值只改法术文件。
2. **不能用 perform 施展法术**。`cmds/skill/perform.c` 的 `perform <招式槽>.<绝招>` 要求玩家已
   `enable` 该槽、绝招文件经 `perform_action_file()` 解析，并且 spells 槽被归入空手功夫分支后还要
   `prepare` 映射命中该技能；法术本质是「按技能等级自由施展」，套进 perform 只会得到一堆前置校验。
   因此新增独立命令 `shifa`（cmds/usr/shifa.c）+ SPELL_D 结算，绕开招式槽与兵器校验。
3. **mapping 取不到的键返回 0，而 `undefinedp(0)` 为假**。别名索引最初写成
   `if (!undefinedp(spell_alias[arg])) return spell_alias[arg];`，未命中的别名会被当成「存在」而
   返回 0，中文名兜底分支永远走不到。必须用 `stringp(spell_alias[arg])` 判断（与 dbase 的
   「0 值被当不存在」同源的坑）。

## How to apply

- 施法链路：`shifa <法术> [目标]` → 目标/场景校验（no_fight、uni_target、living）→
  `SPELL_D->cast_spell()`：学会校验 → 境界门槛（`spell_tier_need(level)` vs
  `SECT_D->query_cultivation_tier()`）→ 冷却（`spell/cooldown/<id>` temp）→ 法力
  （`neili`，按 tier 递减：筑基 85%、结丹 70%）→ 扣法力 + `start_busy(SPELL_CAST_BUSY)` →
  伤害/附加效果 → 未开战则 `fight_ob()`。
- 伤害口径全部整数千分比：`power × (SPELL_COEF_BASE + lv×SPELL_COEF_PER_LEVEL)` →
  `combat_power_permille(me, victim, 1, elem)`（#78 的境界压制×灵根加成，与普攻同一套原语）→
  `calc_element_modifier()`（#28 五行相克）→ 破甲（`SPELL_PIERCE_RATIO`）。不要再写第二套境界/灵根公式。
- 新增一个法术 = 4 处：①`kungfu/skill/<id>.c` 提供 `query_spell_info()`/`valid_enable("spells")`/
  `valid_learn()`（境界门槛用 `spell_tier_need()`）；②`adm/daemons/spell_d.c` 的 `spell_ids` 与
  `spell_alias` 登记；③`adm/daemons/sect_d.c` 的 `sect_config[sect]["skills"]` 加条目（贡献学习 +
  `sect skills` 可见）；④该宗传功 NPC `set_skill("<id>", ...)`（`learn` 请教路径）。
- 数值旋钮集中在 `include/spell.h`（等级系数、灼烧跳数/占比、减速/束缚/护体数值、破甲比例、硬直）；
  法术本身的威力/法力/冷却写在各自 skill 的 `query_spell_info()`。
- 注意：往 `sect_config` 加技能会打破 #62 的功法清点守卫（26 唯一 id / 28 条目），
  `tools/check/sect_skill_verify.py` 现按 SPELL_D 的 `spell_ids` 把法术排除在「#62 功法树」口径之外，
  法术清点归 `tools/check/spell_verify.py`。
- 自检：`python3 tools/check/spell_verify.py`（静态契约 + 文档品阶对齐 + 原文守卫 + 7 组突变实证 +
  同构模拟：等级系数四档、法力递减、五行相克、境界×灵根叠加、灼烧总量、施法拒绝分支）。
