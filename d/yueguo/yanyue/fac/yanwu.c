// d/yueguo/yanyue/fac/yanwu.c
// 掩月宗演武场（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "掩月宗演武场");
        set("long", @LONG
宽阔平整的演武场以青石铺就，两侧立着测试法力的玉柱。掩月宗演武场底蕴为
越国七派之最，法修双修弟子皆在此切磋精进，月华之夜常有女弟子凌空对招，
剑光如月。场内设有禁制护罩，防止比试伤及旁人。
LONG );
        set("sect_facility", "yanyue_yanwu");
        set("exits", ([
                "east" : "/d/yueguo/yanyue/dadian",
        ]));

        setup();
}
