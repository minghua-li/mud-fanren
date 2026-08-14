// d/yueguo/tianque/dadian.c
// 天阙堡 - 大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "天阙堡大殿");
        set("long", @LONG
大殿以巨石垒成，坚固厚重。天阙堡以建筑、阵法擅场，殿中陈列着城防图纸与
阵法图谱。黄色大印是天阙堡的标志性法宝，砸下时涨如小山、爆发出风雷之声，
声势最为浩大，堡中高手持之可撼山裂石。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/tianque/shanmen",
                "north" : "/d/yueguo/tianque/chuangong",
                "west" : "/d/yueguo/tianque/fac/chengbang",
        ]));
        set("objects", ([
                __DIR__"npc/gaoshou" : 1,
        ]));

        setup();
}
