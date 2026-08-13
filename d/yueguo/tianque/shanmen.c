// d/yueguo/tianque/shanmen.c
// 天阙堡 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "天阙堡山门");
        set("long", @LONG
天阙堡依山筑堡，城墙高耸，箭楼林立。堡门上方悬着一方黄色巨印图腾，那是
天阙堡的标志。护山大阵笼罩全堡，泛着微微灵光。天阙堡以筑堡建州、阵法
擅场，堡中弟子服饰上皆绣有黄色巨印为记。
LONG );
        set("exits", ([
                "northwest" : "/d/yueguo/transmit",
                "north" : "/d/yueguo/tianque/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
