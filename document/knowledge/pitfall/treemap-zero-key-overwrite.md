---
id: treemap-zero-key-overwrite
claim: "F_TREEMAP 的 _set 中 if(!map[parts[0]]) 将值为 0 的键视为不存在并被覆盖，与 F_DBASE 其他处的 undefinedp 检查不一致"
tags: [pitfall, dbase, mapping]
modules: [feature]
cluster: dbase
kind: pitfall
status: current
verified: "2026-06-29"
verify_freq: yearly
---

## Why

`/feature/treemap.c:44`：
```lpc
if( !map[parts[0]] || !mapp( map[parts[0]] ) )
```
使用 `!map[key]` 判断键是否存在。当值为 0 时，`!0` 为真，导致旧值被覆盖。而同文件的 `_query` 和 `F_DBASE` 其他地方正确使用了 `undefinedp`。

影响：`query("a/b")` 返回 0 后，再 `set("a/b/c", val)` 会覆盖 `a/b` 的值。

## How to apply

修复：使用 `undefinedp` 替代 `!`：
```lpc
if( undefinedp(map[parts[0]]) || !mapp( map[parts[0]] ) )
```
