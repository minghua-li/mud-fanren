// d/yueguo/lingshou/chuangong.c
// 灵兽山 - 传功阁（兽栏）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "灵兽山传功阁（兽栏）");
        set("long", @LONG
传功阁一侧连着宽阔的兽栏，灵兽栖息训练于此。金翅蚕吐丝结茧，噬金虫在玉盒
中嗡嗡作响，寒冰蟾偶尔喷出一缕寒气。弟子在此习练御兽术、役虫术，越国七派
中灵兽山的御兽体系独树一帜，傀儡术亦较他派突出。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/lingshou/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/luosaidizi" : 1,
        ]));

        setup();
}
