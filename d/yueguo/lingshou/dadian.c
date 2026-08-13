// d/yueguo/lingshou/dadian.c
// 灵兽山 - 大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "灵兽山大殿");
        set("long", @LONG
大殿内以兽皮为饰、兽骨为柱，处处透着一股野性气息。灵兽山以驱兽役虫立派，
殿中供奉着历代驭兽先辈的画像。弟子们在此听候差遣，腰间灵兽袋鼓鼓囊囊，
金翅蚕吐丝可炼宝衣，噬金虫吞噬万物近乎不灭。
LONG );
        set("exits", ([
                "down" : "/d/yueguo/lingshou/shanmen",
                "north" : "/d/yueguo/lingshou/chuangong",
                "east" : "/d/yueguo/lingshou/fac/shoulan",
                "west" : "/d/yueguo/lingshou/fac/chongfang",
        ]));
        set("objects", ([
                __DIR__"npc/yunlu" : 1,
        ]));

        setup();
}
