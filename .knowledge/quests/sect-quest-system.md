# 宗门任务链与宗门事件（LPC 实现版）

> 落地：ticket #59（P4-3）。九宗档案「宗门事件与任务链」节的 LPC 实现。
> 设计源：`.knowledge/factions/sects/` 九宗档案、`02-扩充内容/02-任务链与奖励曲线.md`、`.knowledge/quests/1G-任务副本奇遇.md`。
> 依赖：#57 门派系统（sect_d.c add_contribution）、#58 九宗山门地图（d/yueguo、d/tianluo 驻地目录）、quest_chain_d.c（任务链框架基线）。

## 一、架构（三件套）

| 文件 | 职责 |
|---|---|
| `adm/daemons/sect_quest_d.c` | 宗门侧数据与接口：9 宗 × 3 任务链模板 + 9 宗 × 3 事件定义；接取/交任务/触发事件；驻地场景判定 |
| `adm/daemons/quest_chain_d.c` | 任务链框架（基线已有）+ **奖励结算 grant_quest_rewards / grant_skill（#59 新增）** + 活跃度 streak 按天计算（#59 修订） |
| `cmds/usr/sectquest.c` | 玩家命令：sectquest / accept / info / progress / report / event / help |
| `include/sect_quest.h` | 事件条件键（EV_COND_*）、事件奖励键（EV_REWARD_*）、活跃度参数、数据路径宏 |

数据流：`sectquest` 命令 → `sect_quest_d` 接口（校验宗门归属/驻地场景/事件条件）→ `quest_chain_d` 框架（assign/complete/奖励结算）→ 奖励渠道落账。

## 二、奖励渠道（c4，四类真实落账）

| 奖励键 | 落账渠道 | 说明 |
|---|---|---|
| `exp` | `player->add("combat_exp", ...)` | 修为经验；经 `calc_exp_reward`（境界缩放 × 难度系数 × streak 加成 × 链长加成） |
| `coin` | `MONEY_D->pay_player(player, coin * 100)` | **1 灵石 = 100 文**（对齐传送费口径 105 灵石 = 10500 文） |
| `reputation` | `REPUTATION_D->add_reputation(player, faction, value, reason)` | 含正魔互斥（apply_mutex）；faction 用 sect_id（如 `yanyue_sect`） |
| `contribution` | `SECT_D->add_contribution(player, value, reason)` | 玩家必须已入宗，否则返回 0（宗门任务天然满足） |
| `items` | `new(path)->move(player)` | 真实物品路径（如 `/clone/drug/baicao-dan`） |
| `skills` | 本门功法→写 `sect/learned` 免贡献解锁；通用功法→`player->set_skill(id, 1)` | 对齐 #57 learn_skill 数据结构（sect/learned mapping），任务奖励功法**不扣贡献** |

## 三、活跃度梯度（c6）

- `quest_chain_d.complete_quest` 中 streak 按**自然日**计算：`today = time()/86400`；同日多次完成不重复计、昨日活跃则 +1、断档（间隔 >1 天）重置为 1。
- 奖励加成 `calc_daily_bonus(streak)`：每连续一天 +5%，7 天封顶 ×1.5（对齐 quest_chain.h DAILY_STREAK_*）。
- 宗门任务/事件奖励**同样吃 streak 加成**（事件侧 `grant_event_rewards` 对 exp 乘 streak_bonus）→ 连续活跃奖励递增、断档回落，等价满足 02-任务链与奖励曲线.md 活跃度梯度。
- 链长加成 `calc_chain_length_bonus`：链内每完成 1 个前置任务后续奖励 +5%，最高 ×2（连续推进任务链递增）。

## 四、场景挂接与任务目标（c5）

- 驻地判定 `in_sect_area`：`base_name(environment(player))` 前缀匹配 sect_areas 映射（sect_id → `/d/yueguo/<sect>` 或 `/d/tianluo/<sect>`，目录名与 #58 落地一致）。
- **接取/交任务要求已入本宗 + 位于本宗驻地场景**；查看列表不要求场景。
- 任务目标 `OBJ_REACH`/`OBJ_TALK`：report 时按玩家所在房间路径前缀匹配 target 判定达成（无需 NPC 交互改造，避免动 #58 的 62 个文件）。
- 串行链自动接续受境界门槛拦截：链任务 `realm_range` 不满足时 `assign_quest` 返回 0（如炼气玩家完成入门任务后，筑基门槛的下一环不会自动接）——境界达标后可**手动 `sectquest accept`**（前置已完成 + 境界达标即通过 `is_quest_available`）。这是合理设计，不是缺陷。

## 五、事件触发

- 事件条件：`realm_min/realm_max`（境界索引 0 炼气 1 筑基 2 结丹…）、`rep_min`（本门声望）、`contrib_min`、`quest`（前置任务已完成）、`male_only`。
- 触发境界与九宗档案「宗门事件与任务链」节一致（炼气期/筑基期/结丹+ 等）。
- 已触发事件记录 `sect_quest/triggered`（事件 id → time），不可重复触发。
- 大额事件声望（如暗桩抉择、正魔大战）走 add_reputation 自动触发正魔互斥。

## 六、踩坑与约定

- **境界索引兜底**：quest_chain_d.get_player_realm_index 优先读 `player->query("realm")`；缺失时按 combat_exp 兜底（阈值对齐 sect_d.exp_to_tier：10 万/百万/千万/5 千万/2 亿）——#61 未合入 main 时新玩家无 realm 属性，兜底保证奖励曲线不塌。
- **quest_chain_d 基线是死代码**（#59 前无任何调用者），#59 通过 sect_quest_d.create → register_quest/register_chain 首次接线。
- 任务模板额外带 `"sect"` 归属键（register_quest 不校验多余键，原样存储）。
- 灵石奖励的 `coin` 以灵石为单位（quest_chain.h COIN_FLOOR=10 语义），落账乘 100 转文。
- `query_sect_skill_info` 返回 mapping，取功法名要声明 mapping 变量（不能赋给 string）。

## 七、相关

- `.knowledge/world/teleport-network.md`（九宗驻地目录与 sect_id 对齐约定）
- `.knowledge/quests/1G-任务副本奇遇.md`（任务框架设计）
- `02-扩充内容/02-任务链与奖励曲线.md`（奖励曲线/活跃度设计）
- `.knowledge/architecture/sect-system.md`（#57 门派系统实现）
