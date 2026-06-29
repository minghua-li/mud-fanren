---
id: type-check-functions
claim: "LPC 运行时类型检查用 objectp()/stringp()/mapp()/functionp() 等函数，而非 typeof(x) == 'object'"
tags: [lpc-syntax]
modules: [feature, inherit]
cluster: lpc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

LPC 的 `mixed` 类型变量可以持有任意类型，运行时需要显式类型检查。内建的 `*p()` 函数族是标准做法。

## How to apply

```lpc
mixed data = query("some_field");

if (objectp(data))     // 是对象?
    data->some_method();
else if (stringp(data)) // 是字符串?
    write(data);
else if (mapp(data))    // 是 mapping?
    keys(data);
else if (functionp(data)) // 是函数指针?
    evaluate(data, this_object());
else if (intp(data))    // 是整数?
    write(data + "\n");
else if (arrayp(data))  // 是数组?
    write(sizeof(data) + " elements\n");
```
