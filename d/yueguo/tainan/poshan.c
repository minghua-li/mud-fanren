// d/yueguo/tainan/poshan.c
// 太南谷 - 太南山北坡（入口）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "太南山北坡");
        set("long", @LONG
这里是太南山的北面山坡。太南山高三千余米，是岚州第四高山。山坡上草木
繁茂，一条若隐若现的小径通向山南。前方一片山坡常年被浓浓的白雾笼罩，
雾气翻涌不休，隐约可见雾气深处有山谷轮廓——那便是太南谷。据说太南谷
是修仙之人聚集之地，凡人进不得那雾，强行闯入便会迷失方向。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/qixuanmen/yidao",
                "south" : "/d/yueguo/tainan/gunao",
        ]));

        setup();
}
