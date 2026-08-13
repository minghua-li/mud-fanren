// d/yueguo/huangfeng/dadian.c
// 黄枫谷 - 议事大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "黄枫谷议事大殿");
        set("long", @LONG
这是一座三层大门、数十米高的石殿，矗立谷中，气势巍峨。殿内宽敞，两侧列着
粗大的石柱，掌门钟灵道在此执掌谷中日常事务。谷中炼气期弟子逾九成，筑基期
数百人，结丹期寥寥数人，元婴期唯有云游在外的令狐老祖一人。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/huangfeng/shanmen",
                "north" : "/d/yueguo/huangfeng/chuangong",
                "west" : "/d/yueguo/huangfeng/yuexudian",
        ]));
        set("objects", ([
                __DIR__"npc/zhonglingdao" : 1,
                __DIR__"npc/linghulaozu" : 1,
        ]));

        setup();
}
