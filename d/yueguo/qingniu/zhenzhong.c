// d/yueguo/qingniu/zhenzhong.c
// 青牛镇 - 镇中心
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "青牛镇中心");
        set("long", @LONG
青牛镇的中心是一片石板铺就的小广场，几棵老槐树遮出一片阴凉。镇子不大，
却因地处镜州通往彩霞山的要道上，往来行人不断。东首挂着酒旗的正是春香
酒楼，南边是镇上集市。镇上百姓大多知道，彩霞山上的七玄门每隔几年便来
镇上招收弟子，不少少年由此踏上江湖路。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/qingniu/zhenkou",
                "south" : "/d/yueguo/qingniu/jishi",
                "east" : "/d/yueguo/qingniu/chunxiang",
        ]));

        setup();
}
