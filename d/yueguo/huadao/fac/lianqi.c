// d/yueguo/huadao/fac/lianqi.c
// 化刀坞炼器工坊（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "化刀坞炼器工坊");
        set("long", @LONG
工坊内炉火通明，锤声不绝。化刀坞炼器工艺为越国七派之最，刀器、法器
皆出于此。铸台上摆着半成品的刀胚，赤红的铁水在槽中流淌，工坊深处
更有历代炼器大师留下的器方与心得。
LONG );
        set("sect_facility", "huadao_lianqi");
        set("exits", ([
                "east" : "/d/yueguo/huadao/dadian",
        ]));

        setup();
}
