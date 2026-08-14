// d/yueguo/yanyue/fac/tianyuezhou.c
// 掩月宗天月神舟船坞（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "掩月宗天月神舟船坞");
        set("long", @LONG
巨大的船坞依山而建，停泊着一艘通体银白的巨型飞行法器——天月神舟。舟身
遍刻月华阵纹，翼展数十丈，是掩月宗镇宗之宝，全宗集体出行的中枢。船坞
两侧立有灵石充能的阵柱，常年有弟子轮值看护。
LONG );
        set("sect_facility", "yanyue_tianyuezhou");
        set("exits", ([
                "west" : "/d/yueguo/yanyue/dadian",
        ]));

        setup();
}
