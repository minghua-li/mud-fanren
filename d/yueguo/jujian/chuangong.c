// d/yueguo/jujian/chuangong.c
// 巨剑门 - 传功阁（巨剑场）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "巨剑门传功阁（巨剑场）");
        set("long", @LONG
传功阁前是开阔的巨剑场，弟子挥动巨剑，剑风呼啸，尘土飞扬。银色巨剑一劈
之下，上品法器护罩不堪一击。剑冢之中历代巨剑陈列，剑意森然，门中弟子
在此淬炼体魄，习练重剑剑法。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/jujian/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/gaoren" : 1,
                __DIR__"npc/yinjujian" : 1,
        ]));

        setup();
}
