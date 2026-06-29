---
id: init-add-action-registration
claim: "玩家进入房间时触发 init()，房间在此处通过 add_action() 注册命令动词，回调需返回 0（继续匹配）或 1（消费输入）"
tags: [init, command, room]
modules: [inherit, cmds]
cluster: architecture
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

`init()` 是 LPMUD 的核心机制：当玩家进入房间（或被 move）、房间内出现新物体时，驱动自动调用 `init()`。房间在该函数中通过 `add_action("do_enter", "enter")` 注册命令处理器。

## How to apply

- `add_action("do_enter", "enter")` 注册后，玩家输入 "enter" 时调用 `do_enter(arg)`
- `do_enter` 返回 `1` 表示已处理，返回 `0` 表示不匹配（让其他命令继续匹配）
- `notify_fail("消息")` 在返回 `0` 前设置失败消息
- `valid_leave(me, dir)` 在玩家离开前触发，可阻止离开
- 典型结构：
```lpc
void init() {
    add_action("do_push", "push");
}
int do_push(string arg) {
    if (arg != "door") return 0;
    // 处理推门逻辑
    return 1;
}
```
