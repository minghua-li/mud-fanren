// d/yueguo/qingniu/zhenkou.c
// 青牛镇 - 镇口（出生地，镜州）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "青牛镇镇口");
        set("long", @LONG
这里是镜州青牛镇的镇口，一座年久失修的石碑立在道旁，上书"青牛镇"三字。
小镇是七玄门控制下的十几个小城镇之一，虽不繁华，却也安宁。往北是彩霞山
方向，七玄门总舵便在那山中；往南进镇，便是镇中心了。镇口地面刻着一座
传送阵纹，隐隐泛着灵光——这是越国各地传送网络的一处节点。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/qixuanmen/caixiashan",
                "south" : "/d/yueguo/qingniu/zhenzhong",
        ]));
        set("objects", ([
                __DIR__"npc/xiaofan" : 1,
        ]));

        setup();
}
