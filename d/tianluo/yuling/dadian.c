// d/tianluo/yuling/dadian.c
// 御灵宗 - 大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "御灵宗大殿");
        set("long", @LONG
大殿之中，兽皮为帐、兽牙为饰，粗犷而森严。魔道讲究弱肉强食、强权真理，
御灵宗高层在此裁决宗务。曾有御灵宗修士与化刀坞修士斗法，威势惊人，
方圆数里天翻地覆，可见此宗底蕴。
LONG );
        set("exits", ([
                "south" : "/d/tianluo/yuling/shanmen",
                "north" : "/d/tianluo/yuling/chuangong",
                "west" : "/d/tianluo/yuling/fac/wanshouyuan",
        ]));
        set("objects", ([
                __DIR__"npc/quhun" : 1,
        ]));

        setup();
}
