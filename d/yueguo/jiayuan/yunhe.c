// d/yueguo/jiayuan/yunhe.c
// 嘉元城 - 运河码头
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "运河码头");
        set("long", @LONG
这里是乡鲁大运河的码头，货船客船往来不断。运河穿嘉元城而过，是越国南
北水运的命脉，也正因此，嘉元城才成了岚州商贸第一城。码头上号子声此起
彼伏，船工们扛着麻袋上下跳板。水面波光粼粼，远处的帆影连成一线。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/jiayuan/dajie",
        ]));

        setup();
}
