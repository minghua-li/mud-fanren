// d/yueguo/qixuanmen/caixiashan.c
// 七玄门 - 彩霞山脚
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "彩霞山脚");
        set("long", @LONG
彩霞山原名落凤山，相传五色彩凤曾落于此山化成，故此得名。山势不高，
方圆却有十几里，是镜州境内第二大山。山脚有块青石，刻着"彩霞山"三字。
一条山道蜿蜒而上，通向北面的七玄门山门；东边有条山间驿道，可通岚州
方向。山下青牛镇方向炊烟袅袅，一派田园景象。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/qingniu/zhenkou",
                "north" : "/d/yueguo/qixuanmen/shanmen",
                "east" : "/d/yueguo/qixuanmen/yidao",
        ]));

        setup();
}
