# 主线任务系统（LPC 实现版）

> 落地：ticket #65（P5-5）。主线任务内容填充——第零章凡人篇 4 节点 + 第一章越国篇 13 节点，注册到 #59 quest_chain_d 框架。
> 设计源：`.knowledge/quests/1G-任务副本奇遇.md`（§二 主线任务链）、`02-扩充内容/02-任务链与奖励曲线.md`（§2.1/2.2/2.3）。
> 依赖：#59 任务链框架（quest_chain_d.c 基线）、#61 修炼系统（realm 属性→境界索引）、#67 越国世俗区域（青牛镇/七玄门/嘉元城/太南谷）、#58 九宗驻地（黄枫谷/掩月宗）。

## 一、架构（与 #59 宗门任务并列，同走 quest_chain_d）

| 文件 | 职责 |
|---|---|
| `adm/daemons/main_quest_d.c` | 主线任务注册器 + 玩家接口：17 个任务模板（mq_0_1..mq_1_13）+ 两条串行链（chain_main_0 / chain_main_1）；start_quest / accept_node / complete_node / query_progress / quest_progress |
| `include/main_quest.h` | 章节常量（CHAPTER_MORTAL..FEISHENG）+ 奖励系数（CHAPTER_N_BASE / MULTIPLIER） |
| `cmds/usr/main_quest.c` | 玩家命令：main_quest / accept / submit / help |
| `tools/check/main_quest_verify.py` | 静态校验 + 同构模拟 + 突变验证（exit 0 全绿） |

数据流：`main_quest accept` → `main_quest_d.start_quest` → `quest_chain_d.assign_quest`（is_quest_available 境界/前置过滤）→ 玩家到目标房间 → `main_quest submit` → `complete_node`（quest_progress 位置判定 → complete_quest 结算 + 章节完成检测 → 章节奖励）。

**与宗门任务（sectquest）的关系**：两者都注册进 quest_chain_d，用 `type` 区分（主线 QUEST_TYPE_MAIN 难度系数 2.5 / 宗门 QUEST_TYPE_SIDE 1.5）；主线无宗门归属/驻地要求，任意地点可接（目标 OBJ_REACH 引导到场景）。

## 二、任务模板字段约定（主线特有）

- `chapter`：所属章节（complete_node 用它做章节完成检测）。
- `realm_range`：境界门槛（quest_chain 索引 0 炼气 1 筑基 2 结丹 3 元婴…）。第零章/第一章炼气段 `({0,1})`、筑基段 `({1,2})`、结丹段 `({2,3})`——**max 比 min 宽一个大境界**（如筑基段任务允许结丹玩家回流做，`max=2`），用于吸收小幅超境界回流。**max 不能无限放宽**：`quest_chain_d.calc_realm_reward_scale` 用 `quest_mid=(min+max)/2` 做奖励缩放中点，max 放到 7 会让 mid 漂到化神，正常推进的筑基玩家（realm 1 < mid）触发 0.4 倍奖励惩罚——奖励曲线塌掉。极端超境界回流（元婴回头做第一章筑基任务）会被 max 拦截，属已知边缘（主线 2.5× 是最大经验源，正常玩家不会出现），后续若需支持需先改框架奖励缩放。
- `prerequisites.quests`：显式前置任务（与串行链顺序双保险；is_quest_available 只认 completed 表）。
- `objectives`：`OBJ_REACH` 目标=真实房间路径（#67/#58 场景挂接），提交时按 `base_name(environment(player))` 前缀匹配判定。
- 奖励走 `grant_quest_rewards` 六渠道（同 #59）：exp/coin/reputation/contribution/items/skills。
- **剧情入宗（c4，审查第 2 轮修复）**：`complete_node` 完成 `mq_1_6`（拜入黄枫谷）时，若玩家未入任何门派，自动调 `SECT_D->join_sect(player, "huangfeng_valley")`——默认分支（黄枫谷）剧情落地，入宗后贡献/功法奖励（mq_1_7/1_8/1_10/1_11）真实可达。`join_sect` 自带条件校验（炼气三层/已入他派/叛门记录），不满足时拒绝并提示（如炼气 1-2 层完成 mq_1_6 的玩家收到「修为不足」提示，需修炼后手动 `sect join`）；玩家已入他派时不强行改派（尊重分支选择，贡献发到实际门派）。

## 三、境界门槛与跨章解锁（c3）

- 跨章条件：第零章→第一章 `realm_min=0`（炼气即可）+ 前置章节 completed；第一章→第二章 `realm_min=2`（结丹）。第二章起 chain_id 为空 = 未落地占位（另开子 ticket），`is_chapter_unlocked` 对空 chain_id 返回 0。
- **粒度边界**：quest_chain 境界索引只到大境界（炼气=0），无法表达「炼气≥7 层」这类细分门槛——1G 节点表的「炼气≥7/≥9/13 层」在机制上降级为大境界（炼气即可），细分提示写在 desc 文本。与 #57 exp_to_tier 粒度同源，属框架能力边界（非本票缺陷）。
- 串行链自动接续受境界门槛拦截：炼气玩家完成 mq_1_6 后，下一环 mq_1_7（筑基 `({1,2})`）不会自动接——修炼突破筑基后手动 `main_quest accept` 接续。这是「主线卡修炼」的既定设计（#61 衔接点），不是缺陷。

## 四、奖励曲线（c4/c6）

- 主线难度系数 2.5（quest_chain_d.c calc_exp_reward QUEST_TYPE_MAIN 分支）——实际发放 = 模板 exp × 2.5 × realm_scale(1.0) × chain_bonus(≤2.0)。模板值按「实际/2.5」反推设计，对齐 02-任务链与奖励曲线 §2.3（越国篇任务平均 1000~8000 经验）。
- coin 直接发模板值（calc_coin_reward 有 coin 就返回，不乘系数），1 灵石 = 100 文。
- 章节完成额外发里程碑：title（初入修仙/越国风云）+ item（baicao-dan/lingzhi）+ 章节基础×3 经验。
- skills 渠道（mq_1_8 青元剑诀）：玩家已入黄枫谷时走 grant_skill 本门分支（写 sect/learned 免贡献），**不要求 kungfu/skill/qingyuan-jianjue.c 存在**（#62 功法实体化合入前也安全）。

## 五、踩坑与约定

- **重写主线时废弃旧实现**：旧 main_quest_d.c 是自成一体的简化框架（quest_chapters/quest_nodes + 自己的 accept/complete/reward + `get_player_realm_index` 读 level/combat_exp 兜底，未接 #61 真实 realm），c1 要求与 #59 框架挂接 → 整文件重写为「注册器 + 接口」形态。**旧玩家数据（main_quest/* 属性）作废**，进度迁移到 quest_chain/*（游戏未正式运营，无真实存量玩家）。
- **主线任务 ID 前缀 `mq_`**：避免与宗门任务（yanyue_quest_1 等）在 quest_chain_d 全局模板表冲突。
- **chapter 字段必须显式写**：register_quest 不校验多余键但原样存储；complete_node 读 `quest_defs[node_id]["chapter"]` 做章节完成检测，缺键会 LPC 取 0 误判为第零章。
- **award_chapter_reward 用真实物品路径**：旧实现引用 `/clone/pill/huang_long_dan`、`zhu_ji_dan` 等**不存在路径**（clone/pill 目录整个没有）——改为 `/clone/drug/baicao-dan`、`/clone/drug/lingzhi`（存在）。后续做主线章节奖励/奇遇奖励一律先 `ls clone/drug/` 核实。
- 主线任务不驱动 streak（只读 streak 加成），与宗门任务（report 后 update_daily_streak）区分——主线是剧情推进，不参与每日活跃度。

## 六、验证

`python3 tools/check/main_quest_verify.py`：44 断言，四部分（静态：17 任务定义/房间路径/realm_range/前置无环/括号配对/键一致性守卫；行为模拟：第零章全走→跨章解锁→第一章炼气段→境界门槛拦截→突破后续接→全 17 节点→剧情入宗后贡献/功法可达；LPC 原文守卫；真实突变 4 组：改坏房间路径/删任务定义/放宽境界门槛/删 mq_1_6 入宗接线各转红）。可复跑，供采纳 check 复用（架构师登记时直接 `python3 tools/check/main_quest_verify.py`）。
