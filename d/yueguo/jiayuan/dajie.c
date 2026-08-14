// d/yueguo/jiayuan/dajie.c
// 嘉元城 - 大街（JIA_YUAN 传送节点）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "嘉元城大街");
        set("long", @LONG
嘉元城的大街，路面铺着青石，两旁店铺林立，绸缎庄、药铺、粮行、茶馆一
应俱全。街上行人如织，有行商、有船工、有江湖客。向北是城中巨富墨府的
宅邸，向东可到运河码头。街心地面刻着一座传送阵纹，灵光流转，是越国传
送网络的一处节点。惊蛟会的势力遍及全城，市井间颇有些暗流涌动。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/jiayuan/chengmen",
                "north" : "/d/yueguo/jiayuan/mofu",
                "east" : "/d/yueguo/jiayuan/yunhe",
        ]));

        setup();
}
