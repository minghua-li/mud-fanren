---
id: error-handling-crash-logging
claim: "master.c 的 log_error() 按文件所有者路由编译/运行时错误到对应巫师家目录，crash() 广播并记录日志到 /log/static/CRASHES"
tags: [daemon, lpc-syntax, pitfall]
modules: [adm-single]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

FluffOS 没有 IDE 或调试器，错误处理完全依赖 master 对象回调。正确配置 `mudlib error handler : 1` 后，运行时错误会作为 mapping 传给 `error_handler()`。

## How to apply

- `log_error(file, msg)` 被驱动在编译/运行时错误时调用
  - 区分错误（error）和警告（warning）
  - 通过 `file_owner(file)` 将错误路由到对应巫师的家目录 `/log/wiz/<name>`
  - 警告进入通用 `/log/debug.log`
- `crash(error, cmd_giver, current)` 在驱动崩溃时调用
  - 向所有在线玩家广播
  - 记录到 `/log/static/CRASHES`
  - 包含 `command_giver` 和 `current_object` 用于调试
- `config.ini` 中 `mudlib error handler : 1` 启用增强错误处理
- 关键日志文件：
  - `log/debug.log` — 驱动级日志
  - `log/log` — 游戏日志
  - `log/static/CRASHES` — 崩溃记录
