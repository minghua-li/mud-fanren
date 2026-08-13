// d/tianluo/guiling/shanmen.c
// 鬼灵门 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "鬼灵门山门");
        set("long", @LONG
鬼灵门山门阴气森森，门前鬼火点点，隐隐有低泣之声随风传来。鬼灵门乃魔道
六宗之一，驱鬼役妖、毒术暗术，虽列六宗最弱，实力仍远超越国最强的掩月宗。
门中弟子面色阴冷，腰间挂着驭鬼法器，鬼罗幡在门楼之上无风自动。
LONG );
        set("exits", ([
                "southeast" : "/d/tianluo/transmit",
                "north" : "/d/tianluo/guiling/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
