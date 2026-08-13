// d/yueguo/huangfeng/fac/baiyaoyuan.c
// 黄枫谷百药园（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "黄枫谷百药园");
        set("long", @LONG
太岳山脉深处的百药园，成片灵田依山势层层铺开，药香扑鼻。灵田里栽种着
灵草、黄龙草等灵药，更有数百年药龄的紫丹参，是黄枫谷炼丹原料的根基。
谷中炼气期弟子常被派来此打理药园，当年韩立初入谷时也曾在此劳作。
LONG );
        set("sect_facility", "huangfeng_baiyaoyuan");
        set("exits", ([
                "west" : "/d/yueguo/huangfeng/dadian",
        ]));

        setup();
}
