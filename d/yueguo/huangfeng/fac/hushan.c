// d/yueguo/huangfeng/fac/hushan.c
// 黄枫谷护山大阵（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "黄枫谷护山大阵");
        set("long", @LONG
谷口处灵光流转，一道道禁制符纹在半空交织成巨大的光幕，这便是黄枫谷的
护山大阵。阵眼处盘坐着结丹期的师叔祖，常年坐镇主持，防备外敌入侵。
大阵每七日便需灵石充能，谷中弟子定期轮值加固。
LONG );
        set("sect_facility", "huangfeng_hushan");
        set("exits", ([
                "east" : "/d/yueguo/huangfeng/shanmen",
        ]));

        setup();
}
