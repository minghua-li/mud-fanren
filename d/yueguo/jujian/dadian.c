// d/yueguo/jujian/dadian.c
// 巨剑门 - 大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "巨剑门大殿");
        set("long", @LONG
大殿古朴厚重，殿中供着一柄数丈长的石剑。巨剑门重剑碾压、体剑双修，
万家先祖当年以散修之身入门，终使万家在越国立足，门中祖训：不准歧视散修。
血禁试炼之中，巨剑门高人负责以石剑测试禁制强弱。
LONG );
        set("exits", ([
                "down" : "/d/yueguo/jujian/shanmen",
                "north" : "/d/yueguo/jujian/chuangong",
                "west" : "/d/yueguo/jujian/fac/jianzhong",
                "east" : "/d/yueguo/jujian/fac/yanwu",
        ]));
        set("objects", ([
                __DIR__"npc/wanjia" : 1,
        ]));

        setup();
}
