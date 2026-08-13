// d/yueguo/lingshou/fac/shoulan.c
// 灵兽山兽栏（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "灵兽山兽栏");
        set("long", @LONG
依山而建的兽栏围着一片片林地，各种灵兽栖息其中：青纹虎、赤焰狼、铁背
犀……嘶吼低鸣之声不绝于耳。弟子在此驯养灵兽，训练其作战，兽栏深处
更有罕见的通灵妖兽，由门中长老亲自看管。
LONG );
        set("sect_facility", "lingshou_shoulan");
        set("exits", ([
                "west" : "/d/yueguo/lingshou/dadian",
        ]));

        setup();
}
