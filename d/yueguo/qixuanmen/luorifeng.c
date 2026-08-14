// d/yueguo/qixuanmen/luorifeng.c
// 七玄门 - 落日峰（主峰）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "落日峰");
        set("long", @LONG
落日峰是彩霞山的主峰，也是七玄门总舵所在。峰顶建有一座青石大殿，门前
悬着一块巨匾，上书"七玄门"三个大字，笔力雄健。这里是七玄门门主与长老
会议事之地，外门四堂（飞鸟堂、聚宝堂、四海堂、外刃堂）与内门四堂（百锻
堂、七绝堂、供奉堂、血刃堂）的堂主也常在此出入。殿内隐约传出呼喝练武
之声，山风猎猎，松涛阵阵。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/qixuanmen/shanmen",
        ]));
        set("objects", ([
                __DIR__"npc/zhanglao" : 1,
        ]));

        setup();
}
