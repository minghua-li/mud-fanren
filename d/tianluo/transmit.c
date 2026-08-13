// d/tianluo/transmit.c
// 天罗国魔道传送阵（枢纽）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "天罗国魔道传送阵");
        set("long", @LONG
天罗国是魔道六宗的老巢所在，这座传送阵由鬼灵门与御灵宗共同维护，阵纹
幽暗，泛着森森魔气。魔道讲究弱肉强食、强权真理，阵台旁并无闲杂守卫，
敢来此地者，自有其底气。阵外两条魔气弥漫的山道，分别通向鬼灵门与御灵宗。
LONG );
        set("exits", ([
                "northwest" : "/d/tianluo/guiling/shanmen",
                "southwest" : "/d/tianluo/yuling/shanmen",
        ]));

        setup();
}
