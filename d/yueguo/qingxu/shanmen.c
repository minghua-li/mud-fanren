// d/yueguo/qingxu/shanmen.c
// 清虚门 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "清虚门山门");
        set("long", @LONG
清虚山道观建筑风格，清净素雅。山门为青石所筑，门楣上书『清虚』二字，
隐有仙气缭绕。偶闻钟声悠扬，回荡山间。清虚门为越国七派之一，道门传承，
清心寡欲、道法自然，弟子多为道士装束，以术法为主。
LONG );
        set("exits", ([
                "southwest" : "/d/yueguo/transmit",
                "north" : "/d/yueguo/qingxu/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/daotong" : 1,
        ]));

        setup();
}
