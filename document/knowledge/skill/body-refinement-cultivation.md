---
claim: 淬体点数存 body_refine/points 并由 BODY_D 按阶段换算成 body_refine/qi|defense|fist，战斗热路径直接读
  dbase 不调 daemon
cluster: skill
id: body-refinement-cultivation
kind: architecture
modules:
- adm-daemons
- include
- feature
- kungfu
- cmds
related:
- combat-realm-spirit-root-damage
- sword-art-perform-unlock
- character-attribute-system
- sect-skill-learning-chain
status: current
tags:
- combat
- skill
- dbase
verified: '2026-10-23'
verify_freq: quarterly
---

## Why

COMBAT_SYSTEM §三 的体修体系（基础炼体→法体双修→极限炼体→毒体修炼）此前只是设计表：没有淬体
机制、没有肉身搏击、巨剑门重剑也只是普通 sword 功法，体剑双修无从体现。落地时踩到三个坑：

1. **`max_qi` 是派生值，不能 `add("max_qi", n)`**。`feature/attribute.c` 的 `query_max_qi()` 每次按
   年龄/根骨/内功重算并覆盖 `max_qi`，直接加会被下一次重算抹掉。淬体加成必须接进
   `query_max_qi()` 内部（读 dbase `body_refine/qi` 累加），且要排在该函数末尾的
   `query_temp("apply/max_qi")` 之前，才能与临时增益叠加。
2. **战斗热路径不能调 daemon**。`calc_damage()` 每次出手都跑，若在里面调
   `BODY_D->query_xxx()` 会引入每回合的 daemon call_other；因此淬体只把派生值
   （`body_refine/qi|defense|fist|sword`）落进 dbase，`calc_damage()`/`query_max_qi()`/
   `poison.c` 直接读字段，`BODY_D->refresh()` 只在淬体或数据变更时重算一次。
3. **阶段必须逐段上锁**。只按「学过哪门功法」判阶段，玩家可以跳过象甲功直接学梵圣真魔功，
   淬体上限会跳到 450。`query_stage()` 因此对每段都要求上一段功法 ≥ `BODY_PREREQ_SKILL`(50)，
   与各 skill 文件 `valid_learn()` 里的同名门槛成双保险。

另外：体修养成路线必须和既有槽位对齐——象甲功/涅槃圣体/万毒混元身走 `iron-cloth` 槽（可
`enable iron-cloth` 接管护体判定、可 `practice` 打熬筋骨），梵圣真魔功是**内功**必须继承 `FORCE`
（否则 `max_neili`/`hit_ob` 等内功接口缺失），三头六臂走 `exert` 而非 `perform`（force 槽的
perform 会卡兵器类型校验）。

## How to apply

- 玩家侧只有一条命令 `cuina`（cmds/usr/cuina.c）：无参看面板，`cuina lian` 淬体（耗内力 60 +
  精神 40，收益 `1 + 最高体修功法等级/20`（上限 8）点，点数夹在当前阶段上限内）。
- 阶段与上限（`include/body.h`）：未淬体 0 / 基础炼体 200 / 法体双修 450 / 极限炼体 800 /
  毒体 1200；每点收益 `+3 max_qi`、每 3 点 `+1 固定减伤`、每 3 点 `+1 空手伤害`、每 6 点
  `+1 重剑伤害`。
- 战斗接线（`include/combat/damage.h`）：空手 += `body_refine/fist`；`martial_skill ==
  "zhongjian-jianfa"`（巨剑门重剑）+= `body_refine/sword`（体剑双修，半额）；`defatk` 叠加
  `body_refine/defense`（后面本来就有 `defatk > damage/2` 的钳制，安全）。
- 毒体（`body_refine/stage >= BODY_STAGE_POISON`）：`feature/poison.c` 的 `ObtainProof()`
  抗毒 +`BODY_POISON_RESIST`；`feature/attack.c` 的 `fight_ob()` 开战时
  `F_POISON->empoison(me, "wudu_poison", ..., "unarmed")`，空手出手自带五毒。
- 法体双修神通：`exert santou_liubi`（kungfu/skill/fansheng-zhenmo-gong/santou_liubi.c），
  临时 `apply/attack`/`apply/damage` 加成，`start_call_out(__FILE__, "remove_effect", ...)` 到期
  成对收回——临时加成必须加减对称，否则变永久属性。
- 新增一门体修功法 = `kungfu/skill/<id>.c`（`valid_enable` 声明槽位、`valid_learn` 卡境界 +
  上一段功法）→ `include/body.h` 里登记 phase 的 `BODY_SKILL_*` 与上限 → `sect_d.c` 的
  `sect_config[sect]["skills"]` → 该宗传功 NPC 的 `set_skill`。
- 注意：往 `sect_config` 加技能会打破 #62 的功法清点守卫，
  `tools/check/sect_skill_verify.py` 现按 `include/body.h` 的 `BODY_SKILL_*` 与 SPELL_D 注册表
  把 #76/#79 的新增排除在「#62 功法树」口径外；体修清点归 `tools/check/body_refine_verify.py`。
- 自检：`python3 tools/check/body_refine_verify.py`（文档对齐 + 原文守卫 + 8 组突变实证 +
  同构模拟：阶段阶梯、上限与收益夹取、空手/重剑/普通兵器差异、固定减伤钳制、法相曲线、毒体抗毒）。
