// d/yueguo/qingniu/chunxiang.c
// 青牛镇 - 春香酒楼
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "春香酒楼");
        set("long", @LONG
春香酒楼是青牛镇上最大的酒楼，掌柜的是韩三叔。酒楼的木楼有两层，楼下
大堂摆着七八张桌子，常有行商走卒在此歇脚饮酒。柜台后的酒坛码得整整齐
齐，墙上挂着价目木牌。据说韩三叔与镇上各色人等都有交情，消息也灵通。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/qingniu/zhenzhong",
        ]));
        set("objects", ([
                __DIR__"npc/hansan" : 1,
        ]));

        setup();
}
