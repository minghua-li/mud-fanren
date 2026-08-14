// d/yueguo/huadao/dadian.c
// 化刀坞 - 议事大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "化刀坞议事大殿");
        set("long", @LONG
大殿以青石砌成，四壁悬挂历代名刀，刀气纵横。化刀坞以刀法立派，坞中弟子
以刀意速攻见长，刀法凶悍凌厉。血禁试炼之中，化刀坞弟子与清虚门弟子厮杀
结下恩怨，与巨剑门的刀剑之争也由来已久。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/huadao/shanmen",
                "north" : "/d/yueguo/huadao/chuangong",
                "west" : "/d/yueguo/huadao/fac/lianqi",
        ]));
        set("objects", ([
                __DIR__"npc/zhishilao" : 1,
        ]));

        setup();
}
