// d/mulan/mulancaoyuan.c
// 慕兰大草原 - 草原边缘（占位）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "慕兰大草原");
        set("long", @LONG
眼前是一望无际的慕兰大草原，草原绵延万里，风沙与牧草交织。这里修炼资
源极其贫乏，却是慕兰族法士的天下。草原尽头传说通往大晋帝国——那才是
真正意义上的修仙大世界。
（此区域为后续扩充预留：慕兰草原将在后续票中建设，当前仅立入口占位。）
LONG );
        set("exits", ([
                "north" : "/d/jiuguo/jiuguomen",
        ]));

        setup();
}
