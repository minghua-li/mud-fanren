// d/yueguo/yanyue/chuangong.c
// 掩月宗 - 传功阁
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "掩月宗传功阁");
        set("long", @LONG
传功阁内设有灵根测试台，这是越国最为严谨的灵根测试体系所在。阁中玉架上
陈列着历代功法玉简，双修之术、玄月吸阴功等秘籍皆有收藏。掩月宗数百年前
自合欢宗分裂自立，双修功法体系与合欢宗同源，门中弟子以此为根基精进法力。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/yanyue/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/qionglao" : 1,
        ]));

        setup();
}
