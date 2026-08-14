// d/yueguo/jujian/fac/yanwu.c
// 巨剑门演武场（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "巨剑门演武场");
        set("long", @LONG
粗犷的演武场以巨石铺地，场中放着数柄沉重的银色巨剑，供弟子试剑比试。
巨剑门以重剑碾压著称，一劈可破上品法器护罩，弟子在此苦练剑力，地面
随处可见重剑砸出的裂痕。
LONG );
        set("sect_facility", "jujian_yanwu");
        set("exits", ([
                "west" : "/d/yueguo/jujian/dadian",
        ]));

        setup();
}
