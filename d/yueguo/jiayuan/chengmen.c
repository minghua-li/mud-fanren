// d/yueguo/jiayuan/chengmen.c
// 嘉元城 - 城门
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "嘉元城南门");
        set("long", @LONG
嘉元城是岚州第一大城，也是越国南部的商贸要地。城门高逾三丈，城墙青砖
斑驳，门洞内人来人往。乡鲁大道自西而来，穿城而过，正是贯穿越国南北的
水陆要道。城门处有官兵把守，查验往来行商。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/jiayuan/dajie",
                "west" : "/d/yueguo/tainan/xiangludao",
        ]));
        set("objects", ([
                __DIR__"npc/chengwei" : 1,
        ]));

        setup();
}
