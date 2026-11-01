// d/xuejin/chonguo.c
// 血色禁地 - 核心 BOSS 巢窟（第 3 层·最深处）
// Created for ticket #108

inherit ROOM;

void create()
{
        set("short", "墨蛟巢窟");
        set("long", @LONG
洞窟的最深处是一个巨大的穹顶水窟，黑水如墨，腥气冲天。水底盘踞着一条
巨大的蛟形妖兽，鳞甲乌黑发亮，双目如两盏血灯。两头护卫妖兽分立水窟两侧，
见生人闯入，齐齐发出嘶鸣。越国七派五十年一破禁，年年有人死在这里，也
年年有人从这里带着筑基丹主药活着出去——传闻，只有活着离开这里的人，
才有资格被各派破格擢升。
LONG );
        set("exits", ([
                "south" : "/d/xuejin/heixin",
        ]));
        set("sr_layer", 3);
        // 核心 BOSS 与护卫（#108 c2，1G §3.2.1 怪物表·核心）：真实 NPC 实体，探索时由守护进程生成
        set("sr_mobs", ([
                "/d/xuejin/npc/mojiao": 1,
                "/d/xuejin/npc/huwei": 2,
        ]));

        setup();
}
