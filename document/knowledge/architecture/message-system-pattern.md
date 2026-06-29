---
id: message-system-pattern
claim: "LPC 消息系统分三级：write() 发给当前玩家、tell_object() 发给指定对象、message_vision() 发给房间所有人（$N 自动替换为玩家名）"
tags: [message, command]
modules: [feature, cmds]
cluster: architecture
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

LPMUD 有三种不同的消息发送机制，用途不同。`message_vision` 中 `$N`/`$n` 等占位符会被自动替换为玩家/目标的中文名，并处理颜色代码。

## How to apply

```lpc
// 只发给自己
write("你感到一阵寒意。\n");

// 发给指定对象
tell_object(target, "你被打了一拳！\n");

// 发给房间所有人（除了自己）
message("vision", me->name() + "被谁打了一拳。\n", environment(me), ({me}));

// 发给自己 + 房间其他人，自动替换 $N
message_vision("$N大喝一声，冲了上去。\n", me);

// 带目标对象
message_vision("$N对$n使了一招。\n", me, target);
```
