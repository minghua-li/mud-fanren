// d/yueguo/lingshou/fac/chongfang.c
// 灵兽山虫房（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "灵兽山虫房");
        set("long", @LONG
虫房内暖湿之气蒸腾，一排排玉匣中培育着各色灵虫：金线蛊、噬灵虫、
冰蚕……虫鸣嘤嘤，隐隐透出凶戾之气。灵兽山弟子以役虫术操控灵虫，
孵化、进阶皆在此处，虫房是灵兽山御虫一脉的根基。
LONG );
        set("sect_facility", "lingshou_chongfang");
        set("exits", ([
                "east" : "/d/yueguo/lingshou/dadian",
        ]));

        setup();
}
