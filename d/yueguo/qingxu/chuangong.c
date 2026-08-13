// d/yueguo/qingxu/chuangong.c
// 清虚门 - 传功阁（藏经阁）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "清虚门传功阁（藏经阁）");
        set("long", @LONG
传功阁内藏道门经卷，道门术法、清虚剑典等典籍陈列于架，尘埃不染。窗外论道
台上偶有修士切磋论道，剑符齐飞。清虚门清净无为，以道法自然为要，弟子在此
静心修习，不为外物所动。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/qingxu/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/zhangglao" : 1,
        ]));

        setup();
}
