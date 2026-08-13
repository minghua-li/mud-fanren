// d/yueguo/huangfeng/chuangong.c
// 黄枫谷 - 传功阁
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "黄枫谷传功阁");
        set("long", @LONG
传功阁分内外两层，外阁授长春功等基础功法，内阁藏青元剑诀残本等秘术。
阁中弟子往来，习练之声不绝。青元剑诀残本共九层，前六层筑基可练，后三层
需结丹期方可修炼，每三层便是一门神通，剑影分化，凌厉无匹。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/huangfeng/dadian",
                "north" : "/d/yueguo/huangfeng/fac/danfang",
                "west" : "/d/yueguo/huangfeng/fac/qingjianchang",
        ]));
        set("objects", ([
                __DIR__"npc/lihuayuan" : 1,
        ]));

        setup();
}
