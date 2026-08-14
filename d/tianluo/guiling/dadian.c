// d/tianluo/guiling/dadian.c
// 鬼灵门 - 议事大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "鬼灵门议事大殿");
        set("long", @LONG
大殿幽暗，墙壁上绘着鬼道符文，角落里的鬼罗幡无风自动，隐隐有魂影掠过。
元婴长老王天古在此主事，心机深沉。鬼灵门曾以《万灵真经》副册和副门主之职
拉拢燕家，结姻亲共修血灵大法，谋划之事便多出于此殿。
LONG );
        set("exits", ([
                "south" : "/d/tianluo/guiling/shanmen",
                "north" : "/d/tianluo/guiling/chuangong",
                "west" : "/d/tianluo/guiling/fac/lianshifang",
        ]));
        set("objects", ([
                __DIR__"npc/wangtiangu" : 1,
        ]));

        setup();
}
