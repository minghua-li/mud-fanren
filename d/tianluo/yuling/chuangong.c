// d/tianluo/yuling/chuangong.c
// 御灵宗 - 传功阁（兽苑）
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "御灵宗传功阁（兽苑）");
        set("long", @LONG
传功阁后是广袤的兽苑，灵兽栖息其间，蛊房之中万蛊蠕动，兽斗场上兽宠嘶吼
搏杀。御灵宗弟子在此修习御兽术、役虫术与万蛊诀，养蛊役虫、虫兽双修，
正魔大战之时，兽潮推进之势令越国七派胆寒。
LONG );
        set("exits", ([
                "south" : "/d/tianluo/yuling/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/zhanglao" : 1,
        ]));

        setup();
}
