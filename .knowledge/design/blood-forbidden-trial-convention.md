---
id: blood-forbidden-trial-convention
title: 血色禁地试炼实现约定：三层房间/存活标记/开放机制（#108 沉淀）
tags: [secret-realm, blood-forbidden, design, quest, sect]
updated: 2026-11-01
---

# 血色禁地试炼实现约定：三层房间 / 存活标记 / 开放机制（#108 沉淀）

## 三层内容与房间持有怪物表

- 血色禁地为三层环形结构（1G §3.2.1）：`d/xuejin/rukou`（入口，sr_layer=0 表示不挂试炼路线）→ `waiwei`（外围，1 层）→ `zhongceng`（中层，2 层）→ `heixin`（核心，3 层）→ `chonguo`（墨蛟巢窟，3 层）。exits 逐层可走回入口，成环。
- 怪物表**由房间持有**：每个试炼房间 `set("sr_mobs", ([ "/d/xuejin/npc/<实体>": 数量 ]))`——与既有 `set("objects", ...)` 房间重生表同一约定，不放在守护进程侧（#31 的 daemon 侧 `generate_layer_mobs` 程序化占位生成器已删）。
- `do_explore`（secret_realm_d.c）按玩家所在房间的 `sr_mobs` 补齐缺失实体（`new(path)->move(room)`），再对房间内全部 `sr_realm` 匹配的妖兽 `me->fight_ob(mob)` 接真实战斗（#63 战斗系统，reward.h 战胜判定回调）。
- 掉落走真实物品实体：各 NPC `set("sr_drop", ([ "/d/yueguo/tainan/obj/huanglongcao": 百分比, ... ]))`，`winner_reward(killer, victim)` 按百分比 `new` 真物品 move 到房间，并回调 `SECRET_REALM_D->on_mob_defeated`。

## 「试炼存活」标记（主线与晋升共用的持久状态）

- **写入端**：BOSS 墨蛟（`set("sr_boss", 1)`）被击杀时，`secret_realm_d.mark_trial_survivors` 给击杀者 + 同室存活玩家持久写入 `secret_realm/blood_forbidden/survived` = time()（玩家对象属性，跨会话持久）。
- **形参顺序契约（#108 审查第 1 轮踩坑）**：调用链 reward.h `victim->winner_reward(killer, victim)`（killer=玩家、victim=NPC）→ NPC 内回调 `on_mob_defeated(killer, victim)` → 守护进程签名必须为 `on_mob_defeated(object killer, object victim)`（形参 1=玩家、形参 2=NPC）。sr_realm/sr_score/sr_boss 全挂在 NPC 上：形参顺序写反会让结算在第一个检查处静默 return，而文本层面（调用点字符串在位）静态全绿——`secret_realm_verify.py` 已断言「守护进程在第二个形参上查 sr_realm」+「NPC 调用实参与 winner_reward 形参同序」，并配对应突变实证。
- **读取端**：
  - 主线 mq_1_9 目标 = `OBJ_REACH "/d/xuejin"`（区域前缀判定）+ `OBJ_SURVIVE "secret_realm/blood_forbidden/survived"`（新目标类型，`include/quest_chain.h` `OBJ_SURVIVE 10`；`main_quest_d.quest_progress` 读 `(int)player->query(target) > 0`）。
  - 九宗晋升：`sect_d.c` 的 `xuejin_trial_gate`（键=当前阶位索引，值=晋升事件名，依九宗档案「仕途晋升」表）+ `promote()` 读取存活标记做门槛。
- 约定：存活标记的键路径 `secret_realm/<realm_id>/survived` 是写入端与读取端的**唯一契约**，两端改动必须同步（`tools/check/secret_realm_verify.py` 十、节断言两端均在位）。

## 开放状态机制

- `register_realm()` 注册后**强制置 `SR_STATUS_CLOSED`**（守护进程统一行为，勿改）；要转正开放，注册块写 `SR_FIELD_STATUS: SR_STATUS_OPEN` 之后，在 `load_default_realms()` 里补一次既有机制调用 `set_realm_status("<id>", SR_STATUS_OPEN)`。
- 进入走玩家命令（`cmds/usr/jindi.c`：`jindi enter/exit/explore/info`），门槛校验 = `check_entry_condition`（开放状态 + 境界门槛，口径见 [[secret-realm-entry-standard]]）。

## 冷却口径

- 血色禁地冷却为 `SR_CD_DAILY`（#108 由周改日）：一次性剧情试炼，死亡/退出后当日可重试；周冷却会把主线 mq_1_9 卡死一周。
