// d/xuejin/heixin.c
// 血色禁地 - 核心地下洞窟（第 3 层）
// Created for ticket #108

inherit ROOM;

void create()
{
        set("short", "地下洞窟");
        set("long", @LONG
遗迹广场的地面在你脚下裂开，石阶向下盘旋，尽头是黑暗中的地下洞窟。
洞窟岩壁上渗着暗红色的血光，灵气在这里扭曲成肉眼可见的旋涡，法力恢复
的速度明显慢了下来。更深处传来低沉的呼吸声，每一次起伏都让洞顶的碎石
簌簌落下——那是 3 级妖兽的地盘。
LONG );
        set("exits", ([
                "north" : "/d/xuejin/chonguo",
                "south" : "/d/xuejin/zhongceng",
        ]));
        set("sr_layer", 3);
        // 本层妖兽（#108 c2，1G §3.2.1 怪物表·核心外围）：真实 NPC 实体，探索时由守护进程生成
        set("sr_mobs", ([
                "/d/xuejin/npc/dushi": 2,
        ]));

        setup();
}
