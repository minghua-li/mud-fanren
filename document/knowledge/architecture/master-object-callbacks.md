---
id: master-object-callbacks
claim: "master.c 是驱动入口对象，定义 connect/compile_object/crash/epilog/preload/log_error 等驱动回调，修改后需重启"
tags: [daemon, lpc-syntax]
modules: [adm-single]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

`/adm/single/master.c` 是 FluffOS 驱动启动时加载的第一个对象，驱动在特定事件发生时调用 master 上的固定回调方法。这是 MUD 的"内核"。

## How to apply

关键回调：

| 回调 | 触发时机 | 职责 |
|---|---|---|
| `connect(port)` | 新连接 | 创建 `LOGIN_OB`，根据端口设置编码（5555=GBK, 6666=UTF-8） |
| `compile_object(file)` | 驱动找不到对应.c | 委托 `VIRTUAL_D` 生成虚拟对象 |
| `crash(error, cmd_giver, current)` | 驱动崩溃 | 广播通知、记录日志到 `/log/static/CRASHES` |
| `epilog(load_empty)` | 启动时 | 返回预加载文件列表（读取 `/adm/etc/preload`） |
| `preload(file)` | 逐个预加载 | 预加载文件，失败则记录错误 |
| `log_error(file, msg)` | 编译/运行时错误 | 按文件所有者路由错误日志 |

- 使用 `efun::xxx` 调用原始 efun 避免递归（如 `efun::shout`）
- `update /adm/single/master` 后立即生效（无需完全重启）
- 修改 `connect()` 中的端口/编码逻辑要小心影响所有新连接
