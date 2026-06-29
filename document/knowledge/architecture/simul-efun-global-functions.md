---
id: simul-efun-global-functions
claim: "simul_efun.c 中定义的函数对所有对象全局可见（无需 inherit），包括 message_vision/find_player/wizhood/utf8_to_gb 等核心函数"
tags: [lpc-syntax, command, daemon]
modules: [adm-single]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

`/adm/single/simul_efun.c` 通过继承聚合多个子文件（`message.c`, `object.c`, `player.c`, `chinese.c`, `wizard.c` 等）。其中的函数在整个 MUD 中自动可用，类似全局工具函数。

## How to apply

关键全局函数：

| 函数 | 来源 | 用途 |
|---|---|---|
| `message_vision(msg, me, ...)` | message.c | 房间消息，`$N`/`$n` 自动替换 |
| `find_player(id)` | player.c | `users()` 封装，按 UID 找玩家 |
| `wizhood(ob)` / `wiz_level(ob)` | wizard.c | 巫师权限检查 |
| `getoid(ob)` | object.c | 从 `file_name(ob)` 提取数字实例ID |
| `file_owner(file)` | object.c | 从 `/u/` 路径提取巫师所有者 |
| `chinese_number(n)` | chinese.c | 数字转中文 |
| `gb_to_utf8()` / `utf8_to_gb()` | chinese.c | 编码转换（当前为空操作！） |

⚠️ 注意：simul_efun 污染全局命名空间，命名要谨慎，避免覆盖 efun 或与其他模块冲突。
