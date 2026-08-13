// d/yueguo/qixuanmen/shenshougu.c
// 七玄门 - 神手谷（墨大夫住所）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "神手谷");
        set("long", @LONG
神手谷是七玄门墨大夫的住所。谷中草木葱茏，建有几间青瓦石屋，屋前有一
片晒药场，晾着各种草药。墨大夫医术高明，素有"神手"之称，谷中人迹不多，
却常有人远道而来求医问药。石屋后有一间练功静室，门窗紧闭，据说墨大夫
常在其中闭门不出。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/qixuanmen/shanmen",
        ]));
        set("objects", ([
                __DIR__"npc/modafu" : 1,
        ]));

        setup();
}
