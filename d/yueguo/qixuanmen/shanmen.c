// d/yueguo/qixuanmen/shanmen.c
// 七玄门 - 山门
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "七玄门山门");
        set("long", @LONG
这里是七玄门的山门。七玄门由七绝上人于二百年前创立，曾雄霸镜州数十载，
威震越国，如今虽已衰落，仍是镜州江湖第一大势力，门下弟子三四千人。山门
由青石砌成，两侧各有一名佩刀弟子把守。从山底到峰顶只有这一条路，沿途
设了十三处哨卡。东边通神手谷，西边是炼骨崖，向北则直上落日峰。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/qixuanmen/caixiashan",
                "north" : "/d/yueguo/qixuanmen/luorifeng",
                "east" : "/d/yueguo/qixuanmen/shenshougu",
                "west" : "/d/yueguo/qixuanmen/liangucai",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
