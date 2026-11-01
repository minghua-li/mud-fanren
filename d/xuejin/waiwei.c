// d/xuejin/waiwei.c
// 血色禁地 - 外围迷瘴森林（第 1 层）
// Created for ticket #108

inherit ROOM;

void create()
{
        set("short", "迷瘴森林");
        set("long", @LONG
越入禁地，血色迷雾骤然浓了三倍，树冠遮天蔽日，腐叶层里渗出丝丝毒雾。
蛇虫鼠蚁的窸窣声从四面八方的灌木里传出来，偶尔有巨蚊成群掠过，嗡嗡如雷。
这是血色禁地的外围——迷瘴森林，1-2 级妖兽的巢穴，也是禁地里灵草最多的地方。
LONG );
        set("exits", ([
                "north" : "/d/xuejin/zhongceng",
                "south" : "/d/xuejin/rukou",
        ]));
        set("sr_layer", 1);
        // 本层妖兽（#108 c2，1G §3.2.1 怪物表·外层）：真实 NPC 实体，探索时由守护进程生成
        set("sr_mobs", ([
                "/d/xuejin/npc/dushe": 2,
                "/d/xuejin/npc/juwen": 2,
                "/d/xuejin/npc/dushi": 1,
        ]));

        setup();
}
