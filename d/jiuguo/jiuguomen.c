// d/jiuguo/jiuguomen.c
// 九国盟 - 边境关卡（占位）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "九国盟边境关卡");
        set("long", @LONG
这里是九国盟的边境关卡。九国盟由天南南部九个国家组成，以抵御慕兰族法
士入侵为己任，虞国阗天城是联盟的交易中心。关卡城墙上旌旗猎猎，军士
林立，往南便是与慕兰草原之间的万里黄土野地。
（此区域为后续扩充预留：九国盟/慕兰草原将在后续票中建设，当前仅立关卡。）
LONG );
        set("exits", ([
                "south" : "/d/mulan/mulancaoyuan",
        ]));

        setup();
}
