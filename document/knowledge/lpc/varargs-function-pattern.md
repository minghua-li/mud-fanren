---
id: varargs-function-pattern
claim: "LPC 函数用 varargs 关键字声明可选参数，调用时未传的参数为 0，常用于 query()、do_attack() 等核心函数"
tags: [lpc-syntax, inherit]
modules: [feature, adm-daemons]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

LPC 不支持函数重载，`varargs` 是唯一实现可选参数的方式。`COMBAT_D` 的 `do_attack()` 有 6 个参数不过 4 个是 `varargs`。

## How to apply

```lpc
// 声明
varargs mixed query(string prop, int raw) {
    // 如果 raw 未传，为 0
    // ...
}

varargs int do_attack(object me, object victim, object weapon, int attack_type, string attack_msg) {
    // 调用者可以只传前 2 或 3 个参数
}

// 调用
do_attack(me, victim);                    // 用默认武器
do_attack(me, victim, weapon);            // 指定武器
do_attack(me, victim, weapon, 1);         // 指定攻击类型
```
