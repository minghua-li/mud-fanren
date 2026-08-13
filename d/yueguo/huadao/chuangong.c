// d/yueguo/huadao/chuangong.c
// 化刀坞 - 传功阁（炼器坊）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "化刀坞传功阁（炼器坊）");
        set("long", @LONG
传功阁连着一座炼器坊，炉火熊熊，叮当之声不绝。化刀坞炼器工艺在七派之中
最为突出，各品阶刀器皆出于此。阁中弟子以刀法传承为基，淬炼刀意，坞内刀场
之上刀风呼啸，刀意弥漫。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/huadao/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/xiaoer" : 1,
                __DIR__"npc/hantianya" : 1,
        ]));

        setup();
}
