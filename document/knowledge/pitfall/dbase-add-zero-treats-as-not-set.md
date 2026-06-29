---
id: dbase-add-zero-treats-as-not-set
claim: "F_DBASE 的 add() 中用 !old 检查键是否存在，属性值为 0 时误判为未设置而调用 set() 覆盖"
tags: [pitfall, dbase, mapping]
modules: [feature]
cluster: dbase
kind: pitfall
status: current
verified: "2026-06-29"
verify_freq: yearly
---

## Why

`/feature/dbase.c:72`：
```lpc
if( !mapp(dbase) || !(old = query(prop, 1)) )
    return set(prop, data);
```
`!(old = query(...))` 在 `old == 0` 时成立。如果属性值被显式设为 0（如 `set("count", 0)`），`add("count", 5)` 不会得到 5，而是再次调用 `set("count", 5)`，最终结果是 5 而非实际应有的 5 + 0。

## How to apply

修复：应使用 `undefinedp()` 替代 `!`：
```lpc
old = query(prop, 1);
if( !mapp(dbase) || undefinedp(old) )
    return set(prop, data);
```
