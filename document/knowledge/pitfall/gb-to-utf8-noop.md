---
id: gb-to-utf8-noop
claim: "simul_efun/chinese.c 的 gb_to_utf8()/utf8_to_gb() 为空操作直接 return input，6666 端口玩家收到 GBK 编码乱码"
tags: [pitfall, setup, lpc-syntax]
modules: [adm-single]
cluster: architecture
kind: pitfall
status: current
verified: "2026-06-29"
verify_freq: yearly
---

## Why

`/adm/simul_efun/chinese.c:20-29` 中两个编码转换函数是 stub：
```lpc
string gb_to_utf8(string input) { return input; }
string utf8_to_gb(string input) { return input; }
```
且 `master.c:connect()` 中 `set_encoding("UTF8")` 在 LOGIN_OB 创建后才调用，导致初始 I/O 使用 GBK。

## How to apply

- 如需修正需实现真正的 `iconv` 转换（LPC 无内建编码转换，需通过 efun 或 external）
- 临时方案：在 `master.c` 中提前 `set_encoding` 再创建对象
- 所有 `receive_message` 中调用 `gb_to_utf8` 的地方目前都无效
