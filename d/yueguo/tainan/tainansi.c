// d/yueguo/tainan/tainansi.c
// 太南谷 - 太南寺（山顶）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "太南寺");
        set("long", @LONG
太南寺坐落于太南山顶，是岚州有名的寺庙，以占卜问签灵验著称。寺内香火
缭绕，古木参天。常有修士与凡人上山求签，寺中老僧解签字字玄机。站在寺
前远眺，岚州水网纵横的田畴尽收眼底。
LONG );
        set("exits", ([
                "down" : "/d/yueguo/tainan/fangshi",
        ]));

        setup();
}
