---
id: new-id-rate-limit
claim: "logind.c/logind2.c 的 check_new_id_per_ip() 按 IP 统计每日新账号数（create_new_id_from_ip mapping），超过 5 个则拒绝注册"
tags: [pitfall, player, dbase]
modules: [adm-daemons]
cluster: pitfall
kind: pitfall
status: current
verified: "2026-06-29"
---

## Why

`check_new_id_per_ip(ob)` 在 `logind.c:553` 和 `logind2.c:255`。它用 `create_new_id_from_ip` mapping 按 `query_ip_number(ob)` 累加计数，超过 5 时输出"今天这个IP上已经创建太多的新账号了"并 return 0。

该映射定义于文件顶部 `nosave mapping create_new_id_from_ip`，仅在重启后重置（无跨重启持久化）。

## How to apply

取消限流只需让函数直接 `return 1`：

```lpc
int check_new_id_per_ip(object ob)
{
    return 1;
}
```

若需调整阈值，改 `> 5` 为更大值即可，如 `> 100`。
