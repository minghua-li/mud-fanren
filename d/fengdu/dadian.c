// d/fengdu/dadian.c
// 风都国 - 正道盟议事大殿
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "正道盟议事大殿");
        set("long", @LONG
正道盟的议事大殿，穹顶高阔，正中一张长案，两侧列着各盟派长老的座席。
殿中悬挂一幅天南舆图，越国、风都国、天罗国、九国盟的位置标注分明。正
魔大战之后，正道盟正是以此殿为中枢，统合天南正派势力。
LONG );
        set("exits", ([
                "south" : "/d/fengdu/zhengyuan",
        ]));

        setup();
}
