// d/yueguo/qingxu/dadian.c
// 清虚门 - 三清大殿
// Created for ticket #58

inherit ROOM;

void create()
{
        set("short", "清虚门三清大殿");
        set("long", @LONG
三清大殿庄严肃穆，殿中供奉三清道祖，香火缭绕。清虚门与黄枫谷同进退，
血禁试炼中两派同行。门中博学之士无游子通晓鬼灵门来历，常为同道解惑。
殿侧悬着一方白色虹桥状的飞行法器雪虹绫，载人飞行时如虹桥横空。
LONG );
        set("exits", ([
                "south" : "/d/yueguo/qingxu/shanmen",
                "north" : "/d/yueguo/qingxu/chuangong",
                "east" : "/d/yueguo/qingxu/fac/daoguan",
                "west" : "/d/yueguo/qingxu/fac/yanwu",
        ]));
        set("objects", ([
                __DIR__"npc/wuyouzi" : 1,
        ]));

        setup();
}
