---
id: channel-system
claim: "CHANNEL_D 通过 channels mapping 集中管理所有聊天频道，每条频道配置消息格式、权限、颜色和匿名选项"
tags: [daemon, message, player]
modules: [adm-daemons]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

`/adm/daemons/channeld.c` 是游戏内所有聊天的中枢。频道定义在一个 mapping 中，每个频道独立配置消息模板、权限控制、匿名策略。

## How to apply

频道配置结构：
```lpc
mapping channels = ([
    "chat": ([
        "msg_speak": HIC "【闲聊】%s"+HIC": %s\n" NOR,
        "msg_emote": HIC "【闲聊】%s\n\r" NOR,
        "prefix": HIC,
    ]),
    "rumor": ([
        "msg_speak": HIM "【谣言】%s"+HIM": %s\n" NOR,
        "anonymous": "某人",         // 强制匿名
        "prefix": HIM,
    ]),
]);
```

关键安全检查（在 `do_channel()` 中）：
- 反刷屏：`last_chat_record_time` + 6条/2分钟 冷却
- 长度限制：公共频道最多 180 字符
- 重复检测：比较 `last_channel_msg`
- 匿名频道秘密记录造谣者到 sys 频道
- `wiz_only` / `npc_only` 等权限标志
