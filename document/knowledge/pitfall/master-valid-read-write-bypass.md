---
id: master-valid-read-write-bypass
claim: "master.c 的 valid_write()/valid_read() 首行 return 1 绕过所有文件安全校验，SECURITY_D 相关代码为死代码"
tags: [pitfall, daemon, lpc-syntax]
modules: [adm-single]
cluster: architecture
kind: pitfall
status: current
verified: "2026-06-29"
verify_freq: yearly
---

## Why

`/adm/single/master.c:461-463` 和 `478-479`：
```lpc
int valid_write( string file, mixed user, string func )
{
    object ob;
    return 1;   // 所有写操作直接放行
    // 以下 SECURITY_D 检查为死代码
}
```
同理 `valid_read` 也直接 `return 1`。任何用户可读写任何文件，安全系统形同虚设。

## How to apply

这不是 bug，可能是开发期调试遗留下来的。生产环境应启用 `SECURITY_D` 检查：
```lpc
int valid_write(string file, mixed user, string func) {
    object ob = previous_object();
    if (ob->is_root()) return 1;
    return SECURITY_D->valid_write(file, user, func);
}
```
