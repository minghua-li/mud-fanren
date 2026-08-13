// d/yueguo/tainan/gunao.c
// 太南谷 - 谷口
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "太南谷谷口");
        set("long", @LONG
穿过白雾，眼前豁然开朗——这便是太南谷的谷口。谷中灵气氤氲，草木葱翠，
与谷外的凡俗世界判若两处。谷口立着一块石碑，上刻"太南谷"三字，笔法
飘逸。常有散修在此出入，三三两两结伴而行。往南便是谷中的坊市，谷中
修士交易聚会的所在。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/tainan/poshan",
                "south" : "/d/yueguo/tainan/fangshi",
        ]));

        setup();
}
