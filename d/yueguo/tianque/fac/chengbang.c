// d/yueguo/tianque/fac/chengbang.c
// 天阙堡城堡工事（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "天阙堡城堡工事");
        set("long", @LONG
巍峨的城墙绵延数里，箭楼林立，垛口间架着沉重的连弩。天阙堡以筑堡建州
闻名，城堡工事与堡外护山大阵互为犄角，阵法核心就在城楼之上，阵纹密布，
灵石嵌于阵眼，闪烁着幽蓝的光芒。
LONG );
        set("sect_facility", "tianque_chengbang");
        set("exits", ([
                "east" : "/d/yueguo/tianque/dadian",
        ]));

        setup();
}
