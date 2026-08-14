---
claim: 九宗功法学习链路=SECT_D->learn_skill 写 sect/learned 习得记录+set_skill 入技能表（kungfu/skill/<id>.c
  须先实体化）；任务奖励经
cluster: skill
id: sect-skill-learning-chain
kind: pattern
modules:
- kungfu
- adm-daemons
related:
- sect-system
- kungfu-system-overview
status: current
tags:
- skill
- daemon
- player
verified: '2026-08-13'
---

## Why

#57 门派系统最初只写 `sect/learned` 习得记录（与 #62 衔接约定），不灌技能表——因为 F_SKILL 的 `set_skill()` 会校验 `kungfu/skill/<id>.c` 文件存在（`error("F_SKILL: No such skill")`），功法未实体化前调用即报错。#62 补齐 26 个功法文件后，`learn_skill` 才可接 `set_skill` 实现「习得即入技能表」。

踩坑点：习得记录（`sect/learned` mapping）与技能表（F_SKILL `skills` mapping）是**两套数据**。任务奖励渠道（#59 `grant_skill`）对本门功法只写 learned 不写技能表——玩家需再执行一次 `sect learn <功法>` 触发补灌分支才会把技能灌入技能表（不重复扣贡献）。若 learn_skill 没有补灌分支，任务奖励功法永远只存在于习得记录、技能表为空，玩家无法练习。

## How to apply

新功法接入须三步齐备：

1. **实体化**：`kungfu/skill/<skill_id>.c` 存在（继承 SKILL），否则 `set_skill` 报错。
2. **接线**：`sect_d.c learn_skill` 扣贡献、写 `sect/learned` 之后调 `player->set_skill(skill_id, 1)`；并保留「已学过（learned 有）但技能表空 → 补灌（不扣贡献）」分支。
3. **门槛**：境界门槛写在功法文件的 `valid_learn(object me)`（调 `SECT_D->query_cultivation_tier`），**一律按各档案「境界成长线」逐功法对齐**（如毒术/暗术/役虫术/重剑剑法/阵法术=筑基、玄月吸阴功=结丹、傀儡术=元婴）；青元剑诀按技能等级分段（≤30 炼气、≤60 筑基、>60 结丹，对应残本 9 层 1-3/4-6/7-9）；血灵大法需结丹 + 天灵根（`SR_QUALITY_IDX==ROOT_QUALITY_T0`）或暗灵根（`SR_VARIANT==ROOT_VAR_DARK`）；长春功炼气可修但需灵根存在。

修炼提升路径分三类：sword/blade 类可 `practice`（需 enable + 武器）；force 类走 `xiulian`（但 F_KUNGFU 的 family_force 门派映射不含九宗，需先适配，遗留）；knowledge/profession 类只能 study/learn。

中文名显示：`sect_d.c create()` 调 `CHINESE_D->add_translate(skill_id, 中文名)` 注册，skills 面板 `to_chinese()` 才有中文名。

## force 槽功法必须继承 FORCE（#62 审查 P1 教训）

force 槽功法不能继承裸 SKILL：仓库既有 60 个 force 功法全部 `inherit FORCE`（/inherit/skill/force，链 SKILL→FORCE）。force.c 基类提供 force_character/hit_ob/recover_speed/backup_neili/restore_neili，attribute.c（max_neili/max_jingli/内力恢复）与 combat 加力路径直接调用——裸 SKILL 时宽松语义系数=0 使 max_neili=0、严格语义驱动报错。另需自带 `string exert_function_file(string func)` 返回 `__DIR__"<skill>/" + func`（exert.c 会在本功法 exert 子目录缺失时回退基本内功 force，基本 exert qi/jing 可直接用）。

判别：valid_enable 返回 `usage == "force"` 的功法 → 必须 inherit FORCE + exert_function_file。验证脚本已内置此守卫（FORCE_SKILLS 清单 + 红→绿突变实证）。
