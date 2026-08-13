// d/yueguo/qingniu/jishi.c
// 青牛镇 - 集市
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "青牛镇集市");
        set("long", @LONG
这里是青牛镇的集市，逢五逢十便开集，平日也有零散摊贩。卖粮的、卖布的、
卖山货的、卖草药的，吆喝声此起彼伏。集市东头有条土路通向镜州城，西边
的山野则常有野狼帮的探子出没，镇上人提起便皱眉。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/qingniu/zhenzhong",
                "east" : "/d/yueguo/jingzhou/zhoucheng",
        ]));

        setup();
}
