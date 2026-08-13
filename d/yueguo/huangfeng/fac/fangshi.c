// d/yueguo/huangfeng/fac/fangshi.c
// 黄枫谷坊市（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "黄枫谷坊市");
        set("long", @LONG
坊市位于太岳山脉东北边缘，与元武国交界之处，是越国七派中少有的常设
修仙者交易之所。摊位林立，灵草、矿石、丹药、符箓应有尽有，各派弟子
与散修往来不绝，讨价还价之声此起彼伏。
LONG );
        set("sect_facility", "huangfeng_fangshi");
        set("exits", ([
                "southwest" : "/d/yueguo/huangfeng/dadian",
        ]));

        setup();
}
