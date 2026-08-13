// d/yueguo/tainan/fangshi.c
// 太南谷 - 坊市（修仙坊市，材料买卖入口）
// Created for ticket #67
//
// 坊市买卖入口：玩家可用 buy <货物id> [数量] / sell <货物id> / list 与坊市交易材料。
// 经济接口（P5-4 预留）：
//   - 定价：ECONOMY_D->register_goods / query_current_price（单位：下品灵石）
//   - 结算：MONEY_D->player_pay / pay_player（单位：文，1 灵石 = 100 文）
//   - 记账：ECONOMY_D->record_purchase / record_sale（联动通胀与产出监控）
// 本票只做坊市房间与买卖入口，不实现炼制（FORGE_D / 炼丹系统不在本票范围）。

#include <ansi.h>

inherit ROOM;

// 坊市货物表：id -> ([ "name": 货物名, "file": 材料物件路径, "type": 经济商品类型, "base": 基准价（下品灵石） ])
// 商品类型对齐 include/region_economy.h REGION_SPECIAL_PRODUCTS（ore_basic/herb_basic/hide_basic）
nosave mapping goods = ([
        "tiejing" : ([ "name" : "铁精", "file" : __DIR__"obj/tiejing", "type" : "ore_basic", "base" : 2 ]),
        "lingcao" : ([ "name" : "灵草", "file" : __DIR__"obj/lingcao", "type" : "herb_basic", "base" : 1 ]),
        "shoupi" : ([ "name" : "兽皮", "file" : __DIR__"obj/shoupi", "type" : "hide_basic", "base" : 1 ]),
        "huanglongcao" : ([ "name" : "黄龙草", "file" : __DIR__"obj/huanglongcao", "type" : "herb_basic", "base" : 5 ]),
]);

void create()
{
        set("short", "太南谷坊市");
        set("long", @LONG
这里是太南谷的坊市，也是太南小会的常设交易之地。谷中空地搭着几十个
简易摊位，散修、小家族修士在此交换法器、丹药与各色材料。每逢太南小会
开市，此处更是人头攒动。坊市管事守着正中一座柜台，柜台上摆着各样材料
样品，可向管事询价（list）、购买（buy）或出售（sell）材料。东边是通往
嘉元城的乡鲁大道，北面谷口方向可望见太南寺所在的山峰。
LONG );
        set("exits", ([
                "north" : "/d/yueguo/tainan/gunao",
                "up" : "/d/yueguo/tainan/tainansi",
                "east" : "/d/yueguo/tainan/xiangludao",
        ]));
        set("objects", ([
                __DIR__"npc/guanli" : 1,
                __DIR__"npc/qingyan" : 1,
        ]));

        // P5-4 经济接口预留：注册坊市材料商品（幂等；重复注册仅刷新基准价）
        if (find_object(ECONOMY_D)) {
                ECONOMY_D->register_goods("ore_basic", 2, 50);
                ECONOMY_D->register_goods("herb_basic", 1, 100);
                ECONOMY_D->register_goods("hide_basic", 1, 80);
        }

        setup();
}

void init()
{
        add_action("do_list", "list");
        add_action("do_buy", "buy");
        add_action("do_sell", "sell");
}

// 查询某货物的当前单价（下品灵石）；ECONOMY_D 不可用或未注册时回退基准价
int query_price(string id)
{
        mapping g;

        g = goods[id];
        if (!mapp(g)) return 0;

        if (find_object(ECONOMY_D)) {
                int p = ECONOMY_D->query_current_price(g["type"]);
                if (p > 0) return p;
        }
        return g["base"];
}

int do_list()
{
        string *ids;
        string msg;
        int i;

        ids = keys(goods);
        msg = HIC "太南谷坊市今日货物（单位：下品灵石）：\n" NOR;
        for (i = 0; i < sizeof(ids); i++) {
                msg += sprintf("  %-12s %s：%d 灵石\n",
                        ids[i], goods[ids[i]]["name"], query_price(ids[i]));
        }
        msg += "可用 buy <货物id> [数量] 购买，sell <货物id> 出售。\n";
        write(msg);
        return 1;
}

int do_buy(string arg)
{
        object me, ob;
        mapping g;
        string id;
        int num, price, total, i;

        me = this_player();
        if (me->is_busy() || me->is_fighting())
                return notify_fail("你正忙着呢。\n");

        if (!stringp(arg) || arg == "")
                return notify_fail("你要买什么？可用 list 查看坊市货物。\n");

        if (sscanf(arg, "%s %d", id, num) != 2) {
                id = arg;
                num = 1;
        }
        if (num < 1) num = 1;
        if (num > 99) num = 99;

        g = goods[id];
        if (!mapp(g))
                return notify_fail("坊市上没有这种货物，可用 list 查看。\n");

        price = query_price(id) * 100;   // 灵石 → 文
        total = price * num;

        if (!MONEY_D->player_pay(me, total))
                return notify_fail("你的钱不够支付" + chinese_number(num) +
                        "份" + g["name"] + "（需" + MONEY_D->money_str(total) + "）。\n");

        for (i = 0; i < num; i++) {
                ob = new(g["file"]);
                ob->move(me);
        }
        message_vision("$N向坊市管事买下" + chinese_number(num) + "份" + g["name"] + "。\n", me);

        // P5-4 记账：购买量（灵石）+ 通胀监控
        if (find_object(ECONOMY_D) && find_object(ECONOMY_BRIDGE_D))
                ECONOMY_D->record_purchase(g["type"], query_price(id) * num,
                        ECONOMY_BRIDGE_D->get_player_realm_code(me));

        return 1;
}

int do_sell(string arg)
{
        object me, ob;
        mapping g;
        int price, type_price;

        me = this_player();
        if (me->is_busy() || me->is_fighting())
                return notify_fail("你正忙着呢。\n");

        if (!stringp(arg) || arg == "")
                return notify_fail("你要卖什么材料？\n");

        ob = present(arg, me);
        if (!objectp(ob))
                return notify_fail("你身上没有这种东西。\n");
        if (!ob->query("is_material"))
                return notify_fail("坊市只收炼器炼丹用的材料。\n");

        g = goods[ob->query("material_id")];
        if (!mapp(g))
                return notify_fail("坊市不收这种材料。\n");

        // 收购价 = 当前价 × 80%（对齐区域特产规则：非产地在远方的最低折扣 0.80）
        type_price = query_price(ob->query("material_id"));
        price = type_price * 80 / 100 * 100;   // 灵石 → 文（×0.8）

        MONEY_D->pay_player(me, price);
        message_vision("$N把$n卖给坊市，换得" + MONEY_D->money_str(price) + "。\n", me, ob);

        // P5-4 记账：上架量（灵石）+ 产出监控
        if (find_object(ECONOMY_D) && find_object(ECONOMY_BRIDGE_D))
                ECONOMY_D->record_sale(g["type"], type_price,
                        ECONOMY_BRIDGE_D->get_player_realm_code(me));

        destruct(ob);
        return 1;
}
