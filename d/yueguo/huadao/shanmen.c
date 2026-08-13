// d/yueguo/huadao/shanmen.c
// 化刀坞 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "化刀坞山门");
        set("long", @LONG
化刀坞依水而建，坞中刀光凛冽，打铁之声不绝于耳。山门两侧插着各式刀器，
刀修汇聚之地，煞气逼人。化刀坞以刀法立派，弟子刀法凶悍，血禁试炼中
威名赫赫，炼器工艺在七派中亦属上乘。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/transmit",
                "north" : "/d/yueguo/huadao/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
