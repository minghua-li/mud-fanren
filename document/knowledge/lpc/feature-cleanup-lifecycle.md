---
id: feature-cleanup-lifecycle
claim: "F_CLEAN_UP 在 clean_up() 中检查 interactive() 玩家、environment() 父对象和 no_clean_up 标志，决定是否销毁自身释放内存"
tags: [reset, lpc-syntax, inherit]
modules: [feature]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

驱动按 config.ini 的 `time to clean up`（默认 600 秒）调用对象的 `clean_up()`。未正确实现清理的对象会永久占用内存。

## How to apply

标准 clean_up 返回约定：
- 返回 `1` = 我还活着，不要移除
- 返回 `0` = 可以销毁了

标准检查顺序：
```lpc
int clean_up() {
    if (!clonep() && query("no_clean_up")) return 1;       // 守护进程/重要NPC
    if (interactive(this_object())) return 1;                // 玩家
    if (environment()) return 1;                             // 被携带时由父对象管理
    // 检查子对象中是否有玩家
    foreach (ob in all_inventory())
        if (interactive(ob)) return 1;
    // 清理子对象
    foreach (ob in all_inventory())
        if (!userp(ob)) destruct(ob);
    destruct(this_object());
    return 0;
}
```
- `no_clean_up` 设置在长生命期守护进程和重要 NPC 上
- 房间清理前必须确保无玩家在线
- `destruct()` 在清理中安全调用
