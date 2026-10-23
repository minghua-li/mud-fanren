---
claim: 剑诀神通走 perform <招式槽>.<action> 经 perform_action_file 指向 kungfu/skill/<skill>/<action>.c，层数按
  (lv-1)/10+1 换算
cluster: skill
id: sword-art-perform-unlock
kind: pattern
modules:
- kungfu
- cmds
related:
- elemental-spell-casting
- combat-realm-spirit-root-damage
- sect-skill-learning-chain
status: current
tags:
- skill
- combat
- lpc-syntax
verified: '2026-10-23'
verify_freq: quarterly
---

## Why

1F §2.2 要求青元剑诀按层解锁神通（剑芒/护体剑气/剑影分光术/巨剑术/大庚剑阵），而 #62 交付的
`kungfu/skill/qingyuan-jianjue.c` 只有普通 combat action 数组——「剑影分光」只是攻击文本，
没有任何可主动施展的神通。落地时踩到四个非 obvious 的坑：

1. **perform 的 arg 第一段是「招式槽」不是技能 id**。`perform sword.jianmang` 的第一段必须是被
   `enable` 的基础功夫槽（sword），`cmds/skill/perform.c` 用
   `me->query_skill_mapped("sword")` 拿到高级技能（qingyuan-jianjue），再调
   `SKILL_D(skill)->perform_action_file(action)` 找 `kungfu/skill/qingyuan-jianjue/jianmang.c`。
   所以技能文件必须提供 `perform_action_file()`，否则 `perform` 只会回「没有这个武功招式」。
2. **拿不到「自身增益类」的 perform**。`perform.c` 的 `old_main()` 在 `perform_notarget` 白名单之外
   一律要求 `target_ob` 与 `me->is_fighting(target_ob)`，且 `target == me` 会被当成「你要自杀吗」拒绝。
   护体剑气这类只作用于自己的神通，只能要求「战斗中 + 对手为目标」，不能做成脱战自施，
   除非去改全服共用的 `perform_notarget` 列表（代价大于收益）。
3. **伤害要吃剑诀本身而不是基础剑法**。不设 `me->set_temp("attack_val", ([ "base": ([ "weapon": "sword" ]),
   "skill": ([ "weapon": "qingyuan-jianjue" ]) ]))`，`calc_damage()` 会回退
   `query_skill_mapped(attack_skill)`——虽然结果常常相同，但一旦玩家把 sword 换成别的剑法
   （或临时改了映射），伤害口径就与神通不符。同理走 `COMBAT_D->do_attack(..., TYPE_SPECIAL)`
   才能自动吃 #78 的境界压制与五行系数。
4. **层数换算差一层**。青元剑诀 valid_learn 的分段是 `lv<=30` 炼气（层1-3）、`lv<=60` 筑基（层4-6）、
   其余结丹（层7-9），所以层数必须按 `(lv - 1) / 10 + 1` 换算；写成 `lv / 10 + 1` 时 lv=30 会落到
   第 4 层（筑基段），与「前三层=炼气」矛盾，剑芒强化/剑影分光门槛全部偏一层。

## How to apply

- 新增一个剑诀神通 = ①`kungfu/skill/<skill>/<action>.c`（`int perform(object me, object target)` +
  `valid_target_arg()`，`version` 返回 2 表示支持新格式）；②技能文件加
  `string perform_action_file(string action) { return __DIR__ "<skill>/" + action; }`。
- 公共校验放进该技能的 `jianjue.h` 一类头文件：层数换算（`jianjue_layer`）、持剑+映射校验
  （`jianjue_ready`）、战斗对手校验（`jianjue_target`）、`attack_val` 构造（`jianjue_attack_val`）、
  单次剑气结算（`jianjue_strike` 内部 `COMBAT_D->do_attack(me, target, weapon, TYPE_SPECIAL)`）。
- 门槛两层把关：层数（1F §2.2 文档层段下界：剑芒/护体 1、剑影分光/巨剑术 6、大庚剑阵 10）+
  境界（`SECT_D->query_cultivation_tier`，筑基 `SECT_TIER_ZHU`、结丹 `SECT_TIER_JIE`）+ 内力消耗。
- 自身增益型神通（护体剑气）用 `active_buffer/<skill>.<action>` 命名空间记状态，`add_temp` 与
  `remove_effect` 里的 `add_temp(-值)` 必须成对（week/flat 都要撤销），并用
  `me->start_call_out(__FILE__, "remove_effect", 秒数, me)` 定时收敛——把加成写进 temp 却忘记撤销
  会变成永久属性，是最容易漏的平衡事故。
- 单次爆发型（巨剑术）如果临时 `add_temp("apply/damage", bonus)`，命中后必须立刻减回去，
  且加减必须成对出现（校验脚本有专门守卫）。
- 自检：`python3 tools/check/jianjue_verify.py`（1F 层数表对齐 / 门槛 / buff 与 apply/damage 加减对称 /
  7 组突变实证 / 同构模拟：层数换算与 #62 三段分段一致性、道数曲线、数值封顶、施展拒绝分支）。
