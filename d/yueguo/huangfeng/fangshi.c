// d/yueguo/huangfeng/fangshi.c
// 黄枫谷 - 坊市（太岳山脉东北边缘）
// Created for ticket #67

inherit ROOM;

void create()
{
        set("short", "黄枫谷坊市");
        set("long", @LONG
这里是黄枫谷的坊市，位于太岳山脉东北边缘，向北百余里便是元武国地界。
坊市不大，却因地处要道，往来修士不少。摊位上摆着灵草、矿石、符箓等物，
黄枫谷弟子与散修在此各取所需。谷中弟子若想买卖各色材料，多会前往岚州
太南谷的大坊市——那里货物更全，交易也公道。
LONG );
        set("exits", ([
                "west" : "/d/yueguo/huangfeng/shanmen",
        ]));

        setup();
}
