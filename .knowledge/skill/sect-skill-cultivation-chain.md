---
id: sect-skill-cultivation-chain
claim: 九宗 force/knowledge 类功法修炼链路：#62 习得（sect learn）→ learn 传功 NPC（1→N，recognize_apprentice 门派检查）→ force 100+ 用 xiulian（valid_xiulian 按 SECT_D 动态查本门 + valid_enable("force") 过滤）；exercise 内力上限依赖 basic force 等级（learn force from 传功 NPC 获取）
tags: [skill, sect, cultivation, npc]
modules: [kungfu, cmds-skill, d-yueguo, d-tianluo]
cluster: skill
kind: pattern
status: current
verified: "2026-08-13"
---

## Why

2026-08-13 实现 #72（#62 子单）时建立。`#62 凡人功法实体化` 交付的 26 个功法中，18 个（8 force 内功 + 10 knowledge/profession 类）只能经 `sect learn`「习得」（`set_skill(id, 1)` 入技能表），**不能真正提升等级**：force 类走 `xiulian` 时 `feature/kungfu.c valid_xiulian` 的 `family_force` 映射是北侠旧门派表（不含九宗）；knowledge 类无 enable 槽不可 `practice` 也无读书/传授渠道。本条固化修炼链路的接法与传功 NPC 契约。

## 修炼链路（三类功法）

| 功法类 | 习得（#62 已通） | 1→N 提升渠道 | 100+ 后期 | 效果 |
|--------|------------------|---------------|-----------|------|
| force 内功（8） | `sect learn` → `set_skill(id,1)` | **learn 传功 NPC**（潜能消耗） | **xiulian**（`valid_xiulian` sect 分支放行本门内功，门槛 100 级） | exercise 内力上限 = basic force × 10；exert 运功走 `/kungfu/skill/force/` 基础 exert 文件兜底 |
| knowledge/profession（10） | `sect learn` → `set_skill(id,1)` | **learn 传功 NPC**（无 max_skill，上限=NPC skill 等级） | 无（NPC 等级即上限） | 等级可见；毒/暗/役虫/傀儡的被动/辅助效果机制属后续票（炼丹/炼器接技能） |
| martial（8，剑法/刀法等） | `sect learn` → `set_skill(id,1)` | practice（#62 已通，enable 后 practice） | — | 战斗招式 |

## valid_xiulian 九宗分支（feature/kungfu.c）

- 九宗玩家门派信息存 **`sect/id`**（#57 门派系统），**不在** 北侠 `family/family_name` → 原 `family_force` 表对九宗玩家恒空，`xiulian` 被拒。
- 新增分支（在 `family/family_name` 判断**之前**，sect 是门派权威来源）：
  - `SECT_D->query_player_sect(me)` 非空 → 查 `SECT_D->query_sect_skills(sect_id)` 得本门功法清单 → `member_array(skill, sect_skills) >= 0 && SKILL_D(skill)->valid_enable("force")` 才放行（返回 0）；否则拒绝。
  - **DRY 关键**：本门功法清单动态取自 SECT_D（sect_config），**不复制**「门派→内功」映射表（那会与 sect_d.c 双份数据漂移）。
  - `valid_enable("force")` 过滤保证只有 force 槽功法可 `xiulian`（剑法/刀法虽在本门清单内但不能 xiulian）。
- 北侠门派玩家（有 family）与散修逻辑不变（散修仅可练 `family_force["百姓"]/["公共武学"]`）。

## 传功 NPC 契约（#58 chuangong 房间 NPC 改造）

9 宗各 1 个传功 NPC（掩月宗 qionglao / 黄枫谷 lihuayuan / 灵兽山 luosaidizi / 清虚门 zhangglao / 化刀坞 hantianya / 天阙堡 lanyiren / 巨剑门 gaoren / 鬼灵门 wangchan / 御灵宗 zhanglao），改造三件事：

1. **`int recognize_apprentice(object ob)`**：`SECT_D->query_player_sect(ob) != "<本宗 sect_id>"` 时返回 0（他派/散修不得偷学），否则 1。这是 `learn` 命令（`cmds/skill/learn.c`）的门派闸门——NPC 无此函数时 `learn` 命令的 `ob->recognize_apprentice(me)` 调用返回 0 会拒所有人（见下 FluffOS 行为）。
2. **`set_skill("<本门功法>", N)`**：NPC 会本门全部功法（含 martial）。**N 即玩家可从该 NPC 学到的等级上限**（`learn.c` 的 `my_skill >= master_skill` 判断，之后进切磋模式）。**传功 NPC 的 force 类功法等级必须 ≥ 100**（否则玩家到不了 `xiulian` 的 100 级门槛）。
3. **basic skills**：`set_skill("force"/"dodge"/"parry", N)`——**`exercise` 涨内力（max_neili 上限）依赖 basic force 等级**（`feature/attribute.c query_max_neili = query_skill("force") × TYPE_NEILI(10)`），玩家必须先从传功 NPC `learn force`（basic）拿到 basic force 等级，内力链路才通（basic force=0 时 max_neili 上限 0，exercise 卡「瓶颈」）。

## 关键坑

- **FluffOS 对对象未定义函数的调用返回 0（不抛 error）**：`learn.c` 直接调 `ob->prevent_learn(me, skill)`（多数 NPC 无此函数）、`xiulian.c` 全房间调 `environment(me)->get_xiulian_bonus(me)`（仅个别房间定义）——都依赖该行为。写可选回调时可放心直接调，不必 `function_exists` 包裹（`study.c` 用 `function_exists` 是特例非必需）。
- **NPC 加 `set_skill` 不会重算属性**：`chard.c setup_char` 只在 `userp(ob)` 时重算 `max_neili`/`max_jingli`，NPC（非 userp）显式 `set` 的属性保留——加技能安全，不影响 #58 的 NPC 数值。
- **`learn` 的潜能/精力消耗**：`learned_points`（潜能）与 `jing` 双消耗，knowledge 类不受经验限制（`type()=="knowledge"` 跳过 `query_skill_limit` 检查），force/martial 类（`type()=="martial"`）受经验限制（`my_skill >= query_skill_limit()` 时停止）。
- **习得入口仍在 `sect learn`**：传功 NPC 只教已习得功法（`learn.c` 要求 `my_skill >= master_skill` 才切磋，且 NPC 不会把 0 级玩家从 0 教会——`learn` 可教会（my_skill=0 时正常学），但**惯例上玩家先 `sect learn` 花贡献习得**；未习得玩家也可直接 learn（若 NPC 会且 valid_learn 过），这是北侠 learn 语义，不视为绕过门派贡献体系）。

## 跨票衔接

- 习得入口 = #62 `sect_d.learn_skill`（`set_skill(id,1)` 入技能表 + 贡献扣费）；任务奖励路径 = #59 `grant_skill`（写 `sect/learned` 免贡献，`learn_skill` 补灌入技能表）。
- 传功 NPC 是「传授 NPC」不是「拜师 NPC」——#57 无拜师交互（`sect join` 纯命令入宗），learn 的门派闸门全靠 `recognize_apprentice`。
- knowledge 类的「被动/辅助效果」（毒术伤害、傀儡召唤等）**机制未实现**——仅修炼链路通（等级可提升），效果系统属后续票（#73 丹药 / #74 法宝炼制接技能、战斗系统挂接）。
- 北侠 basic skills（force/dodge/parry 等）仍无新手村教学点（修仙新手走 #61 打坐修为体系），依赖 basic force 的战斗资源（neili）链路 = learn force from 传功 NPC。
