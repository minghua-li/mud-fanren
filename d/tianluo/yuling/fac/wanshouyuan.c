// d/tianluo/yuling/fac/wanshouyuan.c
// 御灵宗万兽园（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "御灵宗万兽园");
        set("long", @LONG
绵延百里的万兽园中，灵兽灵虫数以万计，虎啸猿啼、虫鸣鸟唱交织成一片
生机盎然的世界。御灵宗以御兽役虫立宗，兽苑中豢养着无数珍奇异兽，
弟子在此驯养灵兽、培育灵虫，与灵兽山同源而更胜一筹。
LONG );
        set("sect_facility", "yuling_wanshouyuan");
        set("exits", ([
                "east" : "/d/tianluo/yuling/dadian",
        ]));

        setup();
}
