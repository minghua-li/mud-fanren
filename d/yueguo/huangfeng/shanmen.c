// d/yueguo/huangfeng/shanmen.c
// 黄枫谷 - 山门
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "黄枫谷山门");
        set("long", @LONG
太岳山脉绵延数千里，原始山林之中，黄枫谷山门若隐若现。护山大阵在谷口
运转，泛着淡淡的灵光，有结丹师叔祖坐镇。谷内药香随风飘散，隐约可见成片
灵田与药园。黄枫谷以丹药符箓起家，是韩立当年出身之地，弟子万余人。
LONG );
        set("exits", ([
                "southeast" : "/d/yueguo/transmit",
                "north" : "/d/yueguo/huangfeng/dadian",
        ]));
        set("objects", ([
                __DIR__"npc/dizi" : 1,
        ]));

        setup();
}
