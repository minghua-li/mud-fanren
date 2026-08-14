// d/yueguo/tianque/chuangong.c
// 天阙堡 - 传功阁（阵法演练场）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "天阙堡传功阁（阵法演练场）");
        set("long", @LONG
传功阁外是宽阔的演武场，弟子在此演练阵法，黄色大印砸落时声如风雷。
阵法师们在阁中推演布阵，护山大阵的纹路图样挂满四壁。天阙堡阵法在越国
七派中最为突出，城防体系完善，易守难攻。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/tianque/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/lanyiren" : 1,
        ]));

        setup();
}
