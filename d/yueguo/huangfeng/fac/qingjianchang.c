// d/yueguo/huangfeng/fac/qingjianchang.c
// 黄枫谷青元剑场（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "黄枫谷青元剑场");
        set("long", @LONG
宽阔的剑场以青石铺就，地面刻着深浅不一的剑痕，皆是历代剑修比试留下的
印记。青元剑诀弟子常在此较技，剑气纵横，剑影分化。剑场四周设有看台，
门派试炼之时，全谷弟子都会聚集于此观战。
LONG );
        set("sect_facility", "huangfeng_qingjianchang");
        set("exits", ([
                "east" : "/d/yueguo/huangfeng/chuangong",
        ]));

        setup();
}
