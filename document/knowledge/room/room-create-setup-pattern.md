---
id: room-create-setup-pattern
claim: "ROOM 的 create() 中必须在 setup() 之前依次调用 set('short', ...)、set('long', ...)、set('exits', ...)、set('objects', ...)"
tags: [room, dbase]
modules: [inherit, d-areas]
cluster: room
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

房间的 `create()` 在对象加载时由驱动调用，`setup()` 是最后的初始化步骤。`set()` 设置属性到 `F_DBASE` 的 `dbase mapping`，`setup()` 内部调用 `::setup()` 处理对象生成等工作。

## How to apply

标准房间结构：
```lpc
inherit ROOM;

void create() {
    set("short", "中央广场");
    set("long", @LONG
这里是城市中心...
LONG);
    set("outdoors", "city");
    set("exits", ([
        "east"  : __DIR__"dongdajie1",
        "south" : __DIR__"nandajie1",
    ]));
    set("objects", ([
        "/clone/npc/camel1" : 1,
    ]));
    setup();
}
```
- `__DIR__` 是预处理器宏，自动展开为当前文件所在目录路径
- `@LONG ... LONG` 是 heredoc 语法，用于多行字符串
- `setup()` 必须在所有 `set()` 之后调用
