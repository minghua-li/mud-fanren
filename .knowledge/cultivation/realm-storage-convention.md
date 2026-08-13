---
id: realm-storage-convention
claim: 玩家境界统一存 DBASE "realm"（炼气N层/筑基初期等，层数为 ASCII 数字）+ "realm_index" + "realm_sub" + "xiuwei"（修为），读写统一走 ROOT_REFINE_D；sect_d.extract_layer 只认 0-9，中文数字层数会解析失败
tags: [daemon, dbase, player, realm]
modules: [adm-daemons, cmds, include]
cluster: cultivation
kind: pattern
status: current
verified: "2026-08-13"
---

## Why

2026-08-13 实现 #61 修炼系统落地（P5-1：realm 接线 + 打坐修炼 + 突破流程）时建立的境界存储约定。此前没有任何代码 `set("realm", ...)`，sect_d/quest_chain_d/activity_d/achievement_d/economy_bridge_d 都在读 `query("realm")` 且各自兜底（见 [[sect-system]]）。本条目固化写入方与格式，避免各模块自造格式互相不兼容。

## 存储约定（玩家 DBASE）

| 属性 | 格式 | 示例 | 说明 |
|------|------|------|------|
| `realm` | 中文境界字符串 | `"炼气1层"`、`"筑基初期"`、`"结丹后期"` | **炼气层数必须用 ASCII 数字（1~13）**；`sect_d.extract_layer` 逐字节检查 0-9，中文数字（如"七"）提取不到层数，会退化为默认中期档 |
| `realm_sub` | 子阶段名 | `"初期"`/`"中期"`/`"后期"`/`"巅峰"`/`"大圆满"` | 供 achievement_d 等按子阶段比较的系统 |
| `realm_index` | 大境界索引 | `1` | 0=凡人 1=炼气 2=筑基 3=结丹 4=元婴 5=化神 6=炼虚 7=合体 8=大乘 |
| `xiuwei` | 修为值 int | `0` | 打坐/灵石灌注获得，突破消耗 |

- **写入统一走 `ROOT_REFINE_D->set_player_realm(ob, index, sub)`**，不要在别处手拼 realm 字符串。
- **新玩家初始境界**：`adm/daemons/race/human.c` `setup_human` 中 `userp && undefinedp(realm)` 时置 `"炼气1层"`/`"初期"`/1/0（与灵根生成并列，登录即补，老玩家也会获得初始境界）。

## 修炼系统接口（ROOT_REFINE_D）

- 境界读取：`query_player_realm_index` / `query_player_realm_layer` / `query_player_realm` / `query_player_realm_sub` / `query_player_sub_stage`
- 修为：`query_xiuwei` / `add_xiuwei` / `spend_xiuwei`
- 修炼：`do_heartbeat_cultivation(ob)`（单次心跳修为 = 境界基准 × 灵根速度系数，含炼气层数自动提升）、`check_qi_layer_up(ob)`（升层检查，灵石灌注后单独调它避免重复加修为）
- 突破：`do_major_breakthrough(ob, method)`（返回 1 成功/2 失败/0 条件不满足；概率用 `query_major_breakthrough_probability`，失败回退门槛 50% 修为 + 冷却 + 连续失败保底）、`query_major_break_need` / `query_next_layer_need` / `query_next_sub_need` / `query_break_cooldown_remaining`
- 玩家命令：`dazuo`（打坐 / `dazuo lingshi <N>` 灵石灌注，接 `ECONOMY_BRIDGE_D->perform_spirit_stone_cultivation` A1 循环）、`xiuwei`（查询）、`tupo`（大境界突破）、`root`（灵根查看，转正）

## 数值门槛（实现层，可调）

- 炼气 N→N+1 层：修为 N×100（自动升层，新人保护无概率）；炼气→筑基：10000；筑基→结丹：30000；结丹→元婴：100000；元婴→化神：500000
- 打坐心跳修为：炼气 10 / 筑基 30 / 结丹 100 / 元婴 400 / 化神 1500（× 灵根速度系数 0.3~2.5）
- 突破失败冷却：炼气/筑基档 1 现实天，结丹及以上 3 现实天（对齐 [[realm-breakthrough-failure-penalty]] 的 15/45 游戏天）

## 消费端兼容性

- `sect_d`：`query_cultivation_tier` 用 `parse_realm`（"炼气N层" → index0+层数 → stage；"筑基初期" → index1+阶段词），promote 门槛 `SECT_TIER_*` 与 tier 对齐（内门=炼气后期 tier2、筑基 tier3）——**realm 真实写入后 promote 即可按真实境界卡门槛**
- `economy_bridge_d`：`chinese_to_realm_code` 按子串（"炼气"→qige）→ A1 灵石灌注比率
- `achievement_d`：`ach_check_realm` 用 `member_array` **精确匹配** realm（realm_order 无"炼气1层"→ri=-1→境界成就静默失败），且其成就定义目标子阶段为"1层"/"13层"（与 realm_sub 存的"初期"/"大圆满"语义不符）——**与 `"炼气1层"` 格式不兼容**。此为既有缺陷（基线无 realm 时成就同样失败），非 #61 引入；要修复需改 ach_realm_compare 的匹配方式（子串/索引比较），属 achievement 系统自身改造，未做。
- `activity_d`：`realm_index_map` 的 key 是 `"炼气期"` 格式（带"期"），与 `"炼气1层"` 不命中 → 兜底为凡人档（安全但奖励保守）；要精确需 activity_d 改子串匹配（未做，属其自身改造）

## How to apply

- 新功能需要境界/修为判断：读 `query("realm")`/`query("xiuwei")` 或走 ROOT_REFINE_D 接口，**不要**假定 realm 一定存在（老数据兜底用 combat_exp，阈值见 [[sect-system]]）。
- 写 realm 一律经 `set_player_realm`；炼气层数用 ASCII 数字。
- 灵石灌注后调 `check_qi_layer_up` 而非 `do_heartbeat_cultivation`（后者会重复加一次心跳修为）。
