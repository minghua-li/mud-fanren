// d/yueguo/jujian/shanmen.c
// 巨剑门 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "巨剑门山门");
        set("long", @LONG
巨剑峰拔地而起，巨剑门山门由两块巨石天然拱成，形如巨剑劈开。门中弟子
人人一身黑衣，后背一把一人高的无鞘巨剑，神色冷酷、煞气冲天。巨剑门全为
男子，重剑碾压、体剑双修，是越国七派中最为刚猛的一脉。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/transmit",
                "up" : "/d/yueguo/jujian/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
