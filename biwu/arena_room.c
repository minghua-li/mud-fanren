// arena_room.c
// 演武场竞技房间 - PVP战斗专用
// 玩家在此进行公平对决，战斗结束后自动返回原位置

#include <ansi.h>
#include <pvp.h>

inherit ROOM;

void create()
{
    set("short", HIC "演武场" NOR);
    set("long", @LONG
这是一个宽阔的演武场，四周围着高墙，地面铺着平整的
青石板。空气中弥漫着肃杀之气，显然这里经常进行激烈的对决。
LONG
    );
    set("no_die", 1);
    set("no_death_penalty", 1);
    set("no_drop", 1);
    set("no_get", 1);
    set("no_fight", 0);     // 允许战斗
    set("no_magic", 0);     // 允许法术
    set("no_steal", 1);
    set("no_sleep", 1);
    set("no_remote", 1);

    // 无出口，战斗结束后通过特殊方式离开
    set("exits", ([
    ]));

    setup();
}

void init()
{
    add_action("do_leave", "leave");
    add_action("do_leave", "exit");
    add_action("do_leave", "go");
    add_action("do_check", "hp");       // 允许查看状态
    add_action("do_yield", "yield");
    add_action("do_yield", "认输");
}

// 禁止从演武场直接离开
int do_leave(string arg)
{
    object me = this_player();

    if (me->query_temp(PVP_TEMP_IN_ARENA))
    {
        write(HIR "战斗尚未结束，你不能离开演武场！\n" NOR);
        return 1;
    }
    return 0;
}

// 认输
int do_yield(string arg)
{
    object me = this_player();
    object *inv;
    object opponent;
    int i;

    if (!me->is_fighting())
    {
        write("你现在没有在战斗中。\n");
        return 1;
    }

    inv = me->query_enemy();
    if (sizeof(inv) < 1)
    {
        write("你现在没有对手。\n");
        return 1;
    }

    // 找第一个对手
    opponent = inv[0];

    message_vision(HIY "\n$N高声叫道：我认输了！\n" NOR, me);

    // 让对方停止战斗
    me->remove_enemy(opponent);
    opponent->remove_enemy(me);
    me->stop_busy();
    opponent->stop_busy();

    // 通知PVP_D处理胜负
    PVP_D->end_arena_fight(opponent, me);

    return 1;
}

// 检查战斗是否结束（由COMBAT_D或heart_beat调用）
void check_arena_fight()
{
    object *inv;
    object a, b;
    int i;

    inv = all_inventory(this_object());
    a = 0;
    b = 0;

    for (i = 0; i < sizeof(inv); i++)
    {
        if (living(inv[i]) && userp(inv[i]))
        {
            if (a == 0)
                a = inv[i];
            else if (b == 0)
                b = inv[i];
        }
    }

    // 如果有人在战斗中被杀或quit，只有一个玩家留下
    if (a && !b)
    {
        // 剩下的是赢家
        if (a->query_temp(PVP_TEMP_IN_ARENA))
            PVP_D->end_arena_fight(a, 0);
        return;
    }

    if (!a && !b)
        return;  // 没人了

    // 检查是否有玩家已经死亡
    if (a && !living(a) && b && living(b))
    {
        // b赢了
        PVP_D->end_arena_fight(b, a);
        return;
    }
    else if (b && !living(b) && a && living(a))
    {
        // a赢了
        PVP_D->end_arena_fight(a, b);
        return;
    }
    else if (a && !living(a) && b && !living(b))
    {
        // 都死了 - 平局
        PVP_D->end_arena_fight(0, a);
        return;
    }
}

// 玩家退出时自动处理
void player_leave(object player)
{
    if (!objectp(player))
        return;

    if (player->query_temp(PVP_TEMP_IN_ARENA))
    {
        player->delete_temp(PVP_TEMP_IN_ARENA);
        PVP_D->player_disconnect(player);
    }
}
