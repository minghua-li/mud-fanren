// d/xuejin/zhongceng.c
// 血色禁地 - 中层遗迹广场（第 2 层）
// Created for ticket #108

inherit ROOM;

void create()
{
        set("short", "遗迹广场");
        set("long", @LONG
密林尽头是一片上古遗迹广场，断裂的石柱环列成圈，柱身上刻满了无人识得
的文字。血雾在石柱间缓缓流动，石像的眼窝深处偶尔闪过一点幽光。各派弟子
常在此相遇——按越国七派的旧约，禁地之内弟子互不干涉，但谁也没有真正
遵守过。2-3 级妖兽盘踞在遗迹深处，石傀儡与吸血蝠群是它们中最凶的。
LONG );
        set("exits", ([
                "north" : "/d/xuejin/heixin",
                "south" : "/d/xuejin/waiwei",
        ]));
        set("sr_layer", 2);
        // 本层妖兽（#108 c2，1G §3.2.1 怪物表·中层）：真实 NPC 实体，探索时由守护进程生成
        set("sr_mobs", ([
                "/d/xuejin/npc/shikulei": 2,
                "/d/xuejin/npc/xixuefu": 2,
        ]));

        setup();
}
