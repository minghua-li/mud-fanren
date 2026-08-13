// d/yueguo/yanyue/shanmen.c
// 掩月宗 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "掩月宗山门");
        set("long", @LONG
掩月峰高耸入云，山门气派远胜越国其余各派。青石牌坊上刻着『掩月宗』三个
大字，两侧玉柱之上月华流转，似有月光常驻不散。石阶宽阔平整，偶见女弟子
出入，容貌秀丽，衣袂飘飘。作为越国七派之首，掩月宗底蕴之深，从山门便可见
一斑。
LONG );
        set("exits", ([
                "east" : "/d/yueguo/transmit",
                "north" : "/d/yueguo/yanyue/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
