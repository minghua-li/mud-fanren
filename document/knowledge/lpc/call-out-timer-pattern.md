---
id: call-out-timer-pattern
claim: "LPC 的 call_out(func, delay, args) 在 delay 秒后单次调用，延迟单位为秒但有精度限制"
tags: [callout, lpc-syntax]
modules: [inherit, feature]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

MUD 是事件驱动而非帧驱动的，`call_out` 是唯一的内置延迟执行机制。驱动内部在每 tick 检查 call_out 队列。

## How to apply

- `call_out("greeting", 1, me)` 延迟 1 秒调用 `greeting(me)`
- `remove_call_out("greeting")` 取消尚未触发的 call_out
- `call_out("heart_beat", 0)` 的 0 延迟会在当前 tick 末尾执行（⚠️ 配置 `call_out(0) next level : 1000` 限制了单 tick 数量）
- 典型场景：房间进入提示（`/d/city/guangchang.c` 的 `greeting`）、NPC 定时行为、战斗延时
- 注意：`this_player()` 在 call_out 回调中可能为 0，除非配置了 `this player in call_out : 1`
