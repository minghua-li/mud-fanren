---
id: npc-create-pattern
claim: "NPC 的 create() 中设置属性后用 setup() 初始化，carry_object(path)->wear() 穿戴装备"
tags: [npc, dbase, inherit]
modules: [inherit, d-areas]
cluster: npc
kind: pattern
status: current
verified: "2026-06-29"
---

## Why

NPC 继承自 `/inherit/char/npc.c`，后者继承 `CHARACTER`。`setup()` 驱动调用 `CHAR_D->setup_char()` 根据设置的属性生成实际战力。

## How to apply

标准 NPC 结构：
```lpc
inherit NPC;

void create() {
    set_name("流氓", ({ "liu mang", "liu" }));
    set("gender", "男性");
    set("age", 19);
    set("long", "一个游手好闲的流氓。\n");
    set("combat_exp", 1000);
    set("shen_type", -1);
    set("attitude", "peaceful");
    set_skill("unarmed", 20);
    set_skill("dodge", 20);
    set_temp("apply/attack", 10);
    set_temp("apply/defense", 10);
    set("inquiry", ([
        "name" : "大爷我...",
    ]));
    setup();
    carry_object("/clone/misc/cloth")->wear();
}
```
- `set_name(中文名, 英文ID数组)` — 第一个元素是英文 id，其余是别名
- `set_skill("sword", 100)` — 设置技能等级（数字越大越强）
- `carry_object(path)` — 在 NPC 背包中生成物品，返回物品对象
- `->wear()` / `->wield()` — 穿戴/握持物品
