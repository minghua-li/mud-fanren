// d/tianluo/yuling/shanmen.c
// 御灵宗 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "御灵宗山门");
        set("long", @LONG
御灵宗山门以兽骨堆叠为饰，灵兽嘶鸣之声不绝于耳。宗门以『万灵归宗』为号，
虫兽双修，是魔道六宗中御兽之术的翘楚。灵兽山数千年前便是御灵宗的一个分支，
埋入越国为暗桩，两宗同源，兽道一脉相承。
LONG );
        set("exits", ([
                "northeast" : "/d/tianluo/transmit",
                "north" : "/d/tianluo/yuling/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
