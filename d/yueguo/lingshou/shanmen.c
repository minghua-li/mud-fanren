// d/yueguo/lingshou/shanmen.c
// 灵兽山 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "灵兽山山门");
        set("long", @LONG
灵兽山山门处处可见警戒虫网，彩蛾等灵虫漫天飞舞，侦测着进出之人。门中弟子
服饰花哨，皮囊口袋众多，隐隐有活物鼓动。灵兽山实力在越国七派中仅次于掩月
宗，擅长驱兽役虫，山间兽鸣虫啸之声此起彼伏。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/transmit",
                "up" : "/d/yueguo/lingshou/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/shaonvdizi" : 1,
        ]));

        setup();
}
