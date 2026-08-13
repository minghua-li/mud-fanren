// d/tianluo/guiling/chuangong.c
// 鬼灵门 - 传功阁（阴火炼魂场）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "鬼灵门传功阁（阴火炼魂场）");
        set("long", @LONG
传功阁连着一片阴火炼魂场，幽蓝阴火灼烧着魂体，鬼物在火中哀嚎。炼尸地里，
一具具尸傀静静矗立。鬼灵门弟子在此修习鬼道功法、毒术与暗术，血灵大法是
《万灵真经》第一魔功，需天灵根与暗灵根双修方可驾驭。
LONG );
        set("exits", ([
                "south" : "/d/tianluo/guiling/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/wangchan" : 1,
        ]));

        setup();
}
