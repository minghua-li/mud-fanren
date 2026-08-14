// d/yueguo/huangfeng/fac/danfang.c
// 岳麓殿丹房（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "岳麓殿丹房");
        set("long", @LONG
岳麓殿内的炼丹核心所在，数座青铜丹炉列于殿中，炉下地火长燃，青烟袅袅。
丹房内药香浓郁，全谷的丹方配方皆藏于殿中藏室。谷中大部分丹药配方已然
失传，仅存配方皆由此出，筑基丹主药出自血色禁地，是谷内最核心的资源。
LONG );
        set("sect_facility", "huangfeng_danfang");
        set("exits", ([
                "south" : "/d/yueguo/huangfeng/chuangong",
        ]));

        setup();
}
