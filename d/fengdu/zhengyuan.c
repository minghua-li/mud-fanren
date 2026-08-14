// d/fengdu/zhengyuan.c
// 风都国 - 正道盟总坛
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "正道盟总坛");
        set("long", @LONG
这里是风都国正道盟的总坛。风都国是天南东部大国，正魔大战后，天南修仙
界以风都国为中心结成正道盟，与盘踞天罗国的魔道六宗对峙。总坛殿宇森严，
门前弟子执剑而立。坛中议事大殿常有各派长老往来，商讨抵御魔道的大计。
（此区域为后续扩充预留，当前仅建成总坛门面。）
LONG );
        set("exits", ([
                "north" : "/d/fengdu/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
