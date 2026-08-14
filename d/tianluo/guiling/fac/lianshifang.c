// d/tianluo/guiling/fac/lianshifang.c
// 鬼灵门炼尸房（门派设施）
// Created for ticket #60

inherit ROOM;

void create()
{
        set("short", "鬼灵门炼尸房");
        set("long", @LONG
阴气森森的炼尸房内，一口口石棺排列整齐，棺中躺着正在炼制的尸傀。
墙壁上刻满鬼道符纹，阴火在阵纹间明灭跳动，惨绿色的光芒映得满室幽暗。
鬼灵门以炼尸术闻名，这里是鬼道核心之地，寻常弟子靠近便觉寒气透骨。
LONG );
        set("sect_facility", "guiling_lianshifang");
        set("exits", ([
                "east" : "/d/tianluo/guiling/dadian",
        ]));

        setup();
}
