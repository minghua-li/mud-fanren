// d/yueguo/jiayuan/mofu.c
// 嘉元城 - 墨府
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "墨府大门");
        set("long", @LONG
墨府是嘉元城数一数二的宅邸，青砖高墙，朱漆大门，门前蹲着两座石狮。墨
家是岚州有名的世家，府中人口众多，佣仆成群。大门平日紧闭，偶有访客
递帖，才有门房开门通传。嘉元城中有传言，墨府与城中水路上的生意颇有关
联，个中恩怨外人难知。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/jiayuan/dajie",
        ]));
        set("objects", ([
                __DIR__"npc/mofuguanjia" : 1,
        ]));

        setup();
}
