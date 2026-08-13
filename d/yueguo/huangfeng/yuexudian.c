// d/yueguo/huangfeng/yuexudian.c
// 黄枫谷 - 岳麓殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "黄枫谷岳麓殿");
        set("long", @LONG
岳麓殿是黄枫谷收藏法器、丹药配方与密术之所，殿内禁制繁多，层层设防。
谷中大部分丹药配方已然失传，仅存的配方皆藏于此，筑基丹主药出自血色禁地，
是谷内最核心的资源。殿角丹炉之中，偶有青烟袅袅升起。
LONG );
        set("exits", ([
                "east" : "/d/yueguo/huangfeng/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/hongfu" : 1,
        ]));

        setup();
}
