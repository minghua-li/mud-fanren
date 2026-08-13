---
id: combat-implementation-status
title: 战斗系统落地现状与缺口基准（#63 验收判定）
tags: [combat, implementation, acceptance]
updated: 2026-08-13
---

# 战斗系统落地现状与缺口基准（#63 验收判定）

> 本文记录凡人战斗系统（1F 规格）在代码中的**落地现状基准**：哪些已在 main 可复用、哪些是缺口。
> 设计权威见 [[1F-法术剑诀阵法战斗]]（LPC 详细实现）与 [[COMBAT_SYSTEM]]（总览）。
> 本基准是 #63 整体验收的判定产物，供战斗系统后续子票（#76 五行法术 / #77 剑诀神通 / #78 伤害接境界灵根）实施时对齐，避免重复落地既有资产或误以为已达标。

## 一、已在 main 的战斗基础设施（可复用资产）

### 五行相克系数（早期系统已合入）
- `include/element.h`：五行元素常量（金木水火土 + 雷冰风变异）、相克系数（克制 1.5x / 被克 0.7x）、相生关系、`query_character_element()` 辅助函数（从 spirit_root 读取五行属性，NPC 默认随机分配）
- `include/combat/damage.h`：`calc_damage()` 集成五行克制系数
- `feature/attack.c`：`fight_ob()` 记录双方五行元素到 combat/element
- `feature/damage.c`：相生减伤

### 阵法系统（早期系统已合入）
- `feature/formation.c`：阵法 mixin，7 种阵型——聚灵阵（辅助）、颠倒五行阵（防御+幻境）、天罡北斗阵（防御）、大庚剑阵（攻击）、三才阵（3人）、四象阵（4人）、五行阵（5人）；阵法激活/布阵进度/心跳处理/阵眼判定与转移/阵法效果（防御加成、五行减伤、伤害分摊、法力恢复、剑气伤害）
- 命令：`cmds/std/buzhen.c`（布阵）、`chezhen.c`（撤阵）、`zhenfa.c`（阵法查看）
- 集成：`adm/daemons/combatd.c` 冷却递减、阵法心跳（`formation_tick`）、阵法伤害修正（`formation_damage_modify`）；`include/globals.h` F_FORMATION 宏；`inherit/char/char.c` 继承

### 凡人功法 skill 文件（#62 已合入）
- `kungfu/skill/` 下 26 个九宗功法文件，其中战斗类为**标准 combat action 形态**（`query_action()` 返回 action 数组，含 dodge/parry/force/damage 数值，`valid_enable()` 声明武器槽），走北大侠客行标准 combatd 流程：
  - 剑类：`qingyuan-jianjue`（青元剑诀，sword 槽）、`jianxiu-chuancheng`（剑修传承，sword 槽）、`qingxu-jian-dian`（清虚剑典）
  - 重剑（体修方向）：`zhongjian-jianfa`（重剑剑法，sword 槽）
  - 术法类：`daomen-shufa`（道门术法）
- 学习门槛按档案境界成长线（SECT_TIER_* 宏），习得经 `sect_d.c` learn_skill → set_skill 入技能表

## 二、1F 落地缺口（#63 验收判定）

| 缺口 | 现状证据 | 对应 1F 章节 |
|---|---|---|
| 五系法术本体不存在 | `kungfu/skill/`（743 文件）下无火弹术/冰箭术/金刃术/土墙术/缠绕术/流沙术/地刺术等任何五行法术 `.c` 文件；全仓 `.c/.h` 命中仅 `include/combat/skill_combo.h`（#29 组合技表 desc 文本，见下「combo 断链」） | 1F §1.1 五行基础法术（等级分类/各属性法术详情） |
| 功法与五行属性无关联 | #62 的 26 个功法文件均未设置 `set("attr", ...)`（`kungfu/skill/` 全目录 508 文件 `set("attr")` 零命中）；五行相克系统（element.h）与技能体系互不相通 | 1F §1.1「attr」属性设计 |
| 剑诀进阶神通未实现 | `qingyuan-jianjue.c` 只有普通 combat action（「剑影分光」仅为攻击文本描述），`set("perform")` 零命中——1F §2.2 六门神通（剑芒/护体剑气/剑影分光/巨剑术/大庚剑阵/无形剑气）全未实现 | 1F §2.2 青元剑诀技能体系 |
| 伤害公式未接境界/灵根品质 | `adm/daemons/combatd.c`、`include/combat/damage.h`、`feature/damage.c`、`feature/attack.c` 中 realm / query_cultivation_tier 零命中——02-战斗机制与平衡.md:285 伤害公式的「境界系数」「灵根品质加成」项均未实现；注意区分：五行相克已用灵根**主属性**（element.h `query_character_element` 从 spirit_root 读主元素被 calc_damage 消费），缺口是 **realm 境界系数 + 灵根品质加成** | 1F §4.1 境界与战斗力关系、02-战斗机制与平衡.md §伤害公式 |

## 三、战斗系统断链点（c4「真实可达」的既有死点）

1. **combo 断链（#29 组合技永不可触发）**：`include/combat/skill_combo.h` combo_table 的 pre/post_skill 引用 7 个技能 id——freezing/goldblade/entangle/fireball/windwalk/swordflash/__cycle__——在 `kungfu/skill/` 下**全部 MISS**；而 `adm/daemons/combatd.c:23` include skill_combo.h、`:679` 调 clear_combo_temp（实际消费该表）→ 组合技永远无法触发。其中 fireball（火弹）/freezing（冰冻）/goldblade（金刃）/entangle（缠绕）正是 1F 五系法术 id——#29 已按 1F 设计预留法术 id 引用，法术本体从未落地。**新法术 id 必须与 combo_table 对齐**（#76 承接）
2. **realm_level 死检查**：`skill_combo.h:229` 连招境界检查读 `me->query("realm_level")`，但 `set("realm_level")` 全仓零写入方——战斗里唯一的境界检查是死检查（读无人写的字段），坐实 #61 realm 体系未被战斗消费（#78 承接）
3. **体修路径整体未落地**：#63 工作范围第 4 项（肉身搏击/淬体/体剑双修）无清单条款覆盖、无实现——zhongjian-jianfa（重剑剑法）仅是 #62 的普通 sword 槽功法，无淬体/肉身搏击机制（#79 承接）

## 四、边界与注意

- 门派场景护山大阵（#60 交付，`d/yueguo/*/shanmen.c` 出口到 `fac/hushan` 等）是**场景级阵法**，与 #29 的**战斗级阵法系统**（formation.c）是两条线，勿混淆；#63 范围指战斗中的阵法维持/攻防，承接 #29 的 formation.c
- 五行相克接入点已定（element.h → calc_damage），新法术/功法只需声明五行属性即可被克制体系覆盖，无需重复实现克制计算
- 北大侠客行既有 combatd 公式（attack/defense/force 数值）是伤害基底，1F 的境界/灵根加成应叠加在其上而非重写 combatd
