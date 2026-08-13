// d/yueguo/qingxu/fac/yanwu.c
// 清虚门演武场（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "清虚门演武场");
        set("long", @LONG
道观前的演武场开阔平整，道剑符双修弟子常在此比试。剑符齐飞，道法纵横，
场内禁制柔和，点到即止。演武场边立着刻满道经的石碑，供弟子比试之余
参悟。
LONG );
        set("sect_facility", "qingxu_yanwu");
        set("exits", ([
                "east" : "/d/yueguo/qingxu/dadian",
        ]));

        setup();
}
