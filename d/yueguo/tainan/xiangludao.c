// d/yueguo/tainan/xiangludao.c
// 太南谷 - 乡鲁大道（通嘉元城）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "乡鲁大道");
        set("long", @LONG
乡鲁大道是岚州的主干道，路况颇好，两旁水田连片，沟渠纵横。岚州是越国
水网最密的州郡，风调雨顺，为产粮大区。沿着大道东行，便可抵达岚州第一大
城嘉元城；折向西，则回到太南谷。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/tainan/fangshi",
                "east" : "/d/yueguo/jiayuan/chengmen",
        ]));

        setup();
}
