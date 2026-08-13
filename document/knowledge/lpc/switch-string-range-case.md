---
id: switch-string-range-case
claim: "LPC switch 的 case 支持字符串常量匹配和数值范围匹配（case x..y），同一 switch 内可混用不同类型 case"
tags: [lpc-syntax]
modules: [kungfu, inherit]
cluster: lpc
kind: pattern
status: current
verified: "2026-08-13"
---

## Why

LPC 的 switch 比 C 更强：case 可以是字符串或数值范围。项目里 `kungfu/skill/huagong-dafa.c`、`kungfu/skill/zixia-shengong.c` 都用 `case "TYPE_RECOVER":` 这类字符串分支区分特殊攻击效果类型。

## How to apply

```lpc
// 字符串匹配：不需要 if/else if 链
switch (effect_type) {
    case "TYPE_RECOVER":  handle_recover();  break;
    case "TYPE_DEC_SPD":  handle_slow();     break;
    default:              handle_other();    break;
}

// 数值范围匹配（LPC 扩展，C 不支持）
switch (level) {
    case 0..10:   write("初级\n"); break;
    case 11..50:  write("中级\n"); break;
    case 51..:    write("高级\n"); break;   // 51 及以上
}

// 同一 switch 混用字符串与整型 case 是允许的
```

要点：
- 范围 `case x..y:` 是闭区间；`case ..x:` 是 ≤x，`case x..:` 是 ≥x
- 每个 case 结尾务必 `break`，LPC 不做自动 fallthrough 的语义与 C 相同
- 字符串 case 非常适合处理技能效果分派，避免长 if/else if 链
