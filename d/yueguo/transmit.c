// d/yueguo/transmit.c
// 越国七派传送阵（枢纽）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "越国七派传送阵");
        set("long", @LONG
这是一座由七派合力维护的传送阵枢纽，地面以青石铺就，阵纹纵横交错，
隐隐泛着灵光。阵台四周立着七根玉柱，分别刻着掩月、黄枫、灵兽、清虚、
化刀、天阙、巨剑七派印记。越国七派弟子可在此借传送阵往来各地，
阵外有山道分别通向七派山门。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/yanyue/shanmen",
                "northwest" : "/d/yueguo/huangfeng/shanmen",
                "north" : "/d/yueguo/lingshou/shanmen",
                "northeast" : "/d/yueguo/qingxu/shanmen",
                "east" : "/d/yueguo/huadao/shanmen",
                "southeast" : "/d/yueguo/tianque/shanmen",
                "south" : "/d/yueguo/jujian/shanmen",
        ]));

        setup();
}
