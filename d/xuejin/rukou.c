// d/xuejin/rukou.c
// 血色禁地 - 入口
// Created for ticket #106

inherit ROOM;

void create()
{
        set("short", "血色禁地入口");
        set("long", @LONG
越国建州境内，一片被风属性古禁封闭的独立空间。禁制之外血雾如潮，
五色风刃在雾中穿梭，隐隐有低沉的兽吼自深处传来。越国七派每五年
联手破禁一次，各派弟子在此集结，破禁之后方可踏入禁地。传闻深处
有上古凶物盘踞，却也产筑基丹主药，每年都有人死在里面，也每年
都有人带着凝液草活着回来。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/transmit",
        ]));

        setup();
}
