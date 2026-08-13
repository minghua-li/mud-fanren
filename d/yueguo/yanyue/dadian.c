// d/yueguo/yanyue/dadian.c
// 掩月宗 - 月华大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "掩月宗月华大殿");
        set("long", @LONG
大殿宽阔宏伟，穹顶绘着明月图案，月光透过天窗洒落殿中，映得满殿清辉。
这里是掩月宗议事之所，七派齐聚之时最为醒目。殿内陈设素雅，隐有清幽香气，
门中提倡双修之术，所招弟子最起码一半是女性，容貌不上等者绝不收录。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/yanyue/shanmen",
                "north" : "/d/yueguo/yanyue/chuangong",
        ]));
        set("objects", ([
                __DIR__"npc/nishang" : 1,
                __DIR__"npc/nannanwan" : 1,
        ]));

        setup();
}
