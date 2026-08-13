// d/yueguo/qixuanmen/yidao.c
// 七玄门 - 山间驿道（镜州通往岚州）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "山间驿道");
        set("long", @LONG
这是一条穿行于山林间的驿道，蜿蜒向南，连接镜州与岚州。道旁古木参天，
枝叶遮蔽天光，偶尔有商旅结伴而行。往西折返可回彩霞山，往东走便出了
镜州地界，渐渐接近岚州。据过往行商说，岚州最南边的太南山下有一座太南
谷，是修仙之人聚集之地，常年被浓雾笼罩，凡人不得其门而入。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/qixuanmen/caixiashan",
                "east" : "/d/yueguo/tainan/poshan",
        ]));

        setup();
}
