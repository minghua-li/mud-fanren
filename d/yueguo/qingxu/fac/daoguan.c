// d/yueguo/qingxu/fac/daoguan.c
// 清虚门道观（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "清虚门道观");
        set("long", @LONG
道观依山而建，青瓦白墙，古朴清幽。观内藏书万卷，皆为道门经卷收藏，
道藏、剑典、符箓之书琳琅满目。观中辟有论道之所，蒲团列坐，修士常于此
清净论道，参悟道法，辩经之声隐约可闻。
LONG );
        set("sect_facility", "qingxu_daoguan");
        set("exits", ([
                "west" : "/d/yueguo/qingxu/dadian",
        ]));

        setup();
}
