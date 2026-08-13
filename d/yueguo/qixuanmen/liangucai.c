// d/yueguo/qixuanmen/liangucai.c
// 七玄门 - 炼骨崖（入门测试地）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "炼骨崖");
        set("long", @LONG
炼骨崖是七玄门的入门测试之地。从一片茂密的竹林穿过，攀上陡峭的岩壁，
便来到这处山崖。崖边立着一块石碑，刻着入门测试的规矩：凡欲入七玄门者，
须自竹林起，攀岩壁上崖，方算过了第一关。崖风凛冽，往下望去，山林如海，
胆小之人光是在崖边一站便已腿软。
LONG );
        set("exits", ([
                "east" : "/d/yueguo/qixuanmen/shanmen",
        ]));

        setup();
}
