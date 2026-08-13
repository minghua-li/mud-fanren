// d/yueguo/jujian/fac/jianzhong.c
// 巨剑门剑冢（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "巨剑门剑冢");
        set("long", @LONG
幽深的谷地中插满了历代巨剑，或直插土中，或斜倚山壁，剑身锈迹斑驳却
仍透着凛冽剑意。剑冢是巨剑门的禁地，唯有门中弟子方可入内参悟。历代
前辈的剑意留存于此，是巨剑门剑修传承的根基。
LONG );
        set("sect_facility", "jujian_jianzhong");
        set("exits", ([
                "east" : "/d/yueguo/jujian/dadian",
        ]));

        setup();
}
