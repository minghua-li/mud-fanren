// d/yueguo/jingzhou/zhoucheng.c
// 镜州 - 镜州城（州城）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "镜州城");
        set("long", @LONG
镜州城是镜州的首府，城墙不算高大，市面却还兴旺。城中百姓多经营田地
山货，往来商旅不少。七玄门昔日在此呼风唤雨，如今已被官府挤出了州城，
退居彩霞山，城中只剩些打探消息的江湖耳目。向西的土路通向青牛镇，再往
北便是彩霞山七玄门地界。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/qingniu/jishi",
        ]));
        set("objects", ([
                __DIR__"npc/guanbing" : 1,
        ]));

        setup();
}
