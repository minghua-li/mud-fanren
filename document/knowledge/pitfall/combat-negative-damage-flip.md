---
id: combat-negative-damage-flip
claim: "COMBAT_D 中 damage < 0 时取反（damage = 0 - damage），如果吸收超过伤害量则受害者反而受到正伤害"
tags: [pitfall, combat, dbase]
modules: [adm-daemons]
cluster: combat
kind: pitfall
status: current
verified: "2026-06-29"
verify_freq: yearly
---

## Why

`/adm/daemons/combatd.c:475`：
```lpc
if ( damage<0 ) damage = 0 - damage;
```
当吸收技能吸收量大于原始伤害时（如 damage=100, absorb=150, 结果=-50），`0 - (-50) = 50`，受害者反而受到正伤害。正确的行为应该是 `damage = 0`。

## How to apply

修复：将取反改为归零
```lpc
if ( damage<0 ) damage = 0;
```
