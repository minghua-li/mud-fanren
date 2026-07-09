// trade_d.c
// 交易系统守护进程 - 面对面交易、交易限制、交易日志
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第5.2章

#include <ansi.h>
#include <reputation_ext.h>

inherit F_DBASE;
inherit F_SAVE;

// 交易日志
nosave mapping trade_log = ([]);

// 拍卖行
nosave mapping auction_items = ([]);
nosave int auction_id_counter;

// 摆摊记录
nosave mapping stall_items = ([]);

void create()
{
    seteuid(getuid());
    restore();
    set("channel_id", HIG "交易系统" NOR);

    if (!auction_id_counter) auction_id_counter = 1;
}

string query_save_file()
{
    return "/data/trade_d";
}

// ======== 面对面交易 ========

// 发送交易请求
int send_trade_request(object from, object to)
{
    if (!from || !to) return 0;

    // 检查黑名单
    if (to->is_blacklisted(from->query("id")))
    {
        tell_object(from, "对方已将你加入黑名单，无法交易。\n");
        return 0;
    }

    // 检查交易上限
    int daily = from->query(TRADE_DAILY_PATH + "/" + (time() / 86400));
    int limit = get_daily_trade_limit(from);
    if (daily >= limit)
    {
        tell_object(from, "你的每日交易额度已达上限。\n");
        return 0;
    }

    tell_object(to, HIG + from->query("name") + " 想与你进行交易。\n" NOR);
    tell_object(to, "请使用 trade accept " + from->query("id") + " 接受，或 trade reject 拒绝。\n");
    tell_object(from, "已向 " + to->query("name") + " 发送交易请求。\n");

    from->set_temp("pending_trade/to", to->query("id"));
    from->set_temp("pending_trade/time", time());
    to->set_temp("pending_trade/from", from->query("id"));
    to->set_temp("pending_trade/time", time());

    return 1;
}

// 检查交易物品是否符合规则
int validate_trade_item(object player, object item)
{
    if (!item) return 0;

    // 检查境界限制 - 不可交易超过自身境界1阶以上的物品
    int item_level = item->query("item_level");
    if (item_level)
    {
        int player_level = player->query("realm_level");
        if (item_level > player_level + 1)
        {
            tell_object(player, "你的境界不足以交易此物品。\n");
            return 0;
        }
    }

    // 检查绑定状态
    if (item->query("bind"))
    {
        tell_object(player, "绑定物品不可交易。\n");
        return 0;
    }

    return 1;
}

// 执行交易
int execute_trade(object from, object to, mixed *from_items, int from_money,
                  mixed *to_items, int to_money)
{
    if (!from || !to) return 0;

    // 检查交易额度
    int today_key = time() / 86400;
    int from_daily = from->query(TRADE_DAILY_PATH + "/" + today_key);
    int to_daily = to->query(TRADE_DAILY_PATH + "/" + today_key);
    int from_limit = get_daily_trade_limit(from);
    int to_limit = get_daily_trade_limit(to);

    if (from_money > 0 && from_daily + from_money > from_limit)
    {
        tell_object(from, "你的每日交易额度不足(剩余" + (from_limit - from_daily) + "灵石)。\n");
        return 0;
    }
    if (to_money > 0 && to_daily + to_money > to_limit)
    {
        tell_object(to, "你的每日交易额度不足(剩余" + (to_limit - to_daily) + "灵石)。\n");
        return 0;
    }

    // 执行物品转移
    // from → to
    for (int i = 0; i < sizeof(from_items); i++)
    {
        object item = from_items[i];
        if (!item) continue;

        if (!validate_trade_item(from, item)) continue;

        item->move(to);
    }

    // to → from
    for (int i = 0; i < sizeof(to_items); i++)
    {
        object item = to_items[i];
        if (!item) continue;

        if (!validate_trade_item(to, item)) continue;

        item->move(from);
    }

    // 灵石转移
    // 使用灵石系统API（简化版）
    if (from_money > 0)
    {
        // from扣灵石
        MONEY_D->player_pay(from, from_money);
        MONEY_D->player_receive(to, from_money);
    }
    if (to_money > 0)
    {
        MONEY_D->player_pay(to, to_money);
        MONEY_D->player_receive(from, to_money);
    }

    // 更新交易额度
    if (from_money > 0)
        from->add(TRADE_DAILY_PATH + "/" + today_key, from_money);
    if (to_money > 0)
        to->add(TRADE_DAILY_PATH + "/" + today_key, to_money);

    // 记录交易日志
    log_trade(from, to, from_items, from_money, to_items, to_money);

    // 清理临时数据
    from->delete_temp("pending_trade");
    to->delete_temp("pending_trade");

    return 1;
}

// 记录交易日志
void log_trade(object from, object to, mixed *from_items, int from_money,
               mixed *to_items, int to_money)
{
    int now = time();
    string log_entry;

    log_entry = sprintf("[%s] %s(%s) ↔ %s(%s) | 灵石:%d→%d | 物品:%d件→%d件\n",
                        ctime(now),
                        from->query("name"), from->query("id"),
                        to->query("name"), to->query("id"),
                        from_money, to_money,
                        sizeof(from_items), sizeof(to_items));

    string date_key = ctime(now)[0..10]; // 取日期部分
    if (!trade_log[date_key])
        trade_log[date_key] = ({});
    trade_log[date_key] += ({ log_entry });

    // 清理过期日志
    clean_expired_logs();
}

// 清理过期日志
void clean_expired_logs()
{
    int now = time();
    string *dates = keys(trade_log);

    foreach (string date in dates)
    {
        // 简单清理: 删除超过7天的日期键
        // 实际应解析日期，此处简化
        if (sizeof(trade_log[date]) > 10000)
            trade_log[date] = trade_log[date][5000..];
    }
}

// ======== 交易限制 ========

// 获取每日交易上限
int get_daily_trade_limit(object player)
{
    int exp = player->query("combat_exp");
    if (exp >= 50000000) return TRADE_DAILY_LIMIT_YUANYING;
    if (exp >= 10000000) return TRADE_DAILY_LIMIT_JIEDAN;
    if (exp >= 1000000) return TRADE_DAILY_LIMIT_ZHUIJI;
    return TRADE_DAILY_LIMIT_QIYIN;
}

// 获取交易税率
float get_trade_tax(int trade_type)
{
    switch (trade_type)
    {
    case TRADE_TYPE_FACE:  return TRADE_TAX_FACE;
    case TRADE_TYPE_MAIL:  return TRADE_TAX_MAIL;
    case TRADE_TYPE_AUCTION: return TRADE_TAX_AUCTION;
    case TRADE_TYPE_STALL: return TRADE_TAX_STALL;
    case TRADE_TYPE_BLACK: return TRADE_TAX_BLACK;
    default: return 0.05;
    }
}

// ======== 拍卖行 ========

// 上架拍卖品
int auction_list(object seller, object item, int starting_price, int duration)
{
    if (!seller || !item) return 0;

    int id = auction_id_counter++;
    auction_items[id] = ([
        "id": id,
        "seller_id": seller->query("id"),
        "seller_name": seller->query("name"),
        "item": item,
        "item_name": item->query("name"),
        "starting_price": starting_price,
        "current_price": starting_price,
        "bidder": 0,
        "end_time": time() + duration,
        "status": "active"
    ]);

    save();
    return id;
}

// 竞拍
int auction_bid(int auction_id, object bidder, int amount)
{
    if (!auction_items[auction_id])
        return -1; // 不存在
    if (auction_items[auction_id]["status"] != "active")
        return -2; // 已结束

    if (amount <= auction_items[auction_id]["current_price"])
        return -3; // 出价过低

    // 退还上一竞拍者
    if (auction_items[auction_id]["bidder"])
    {
        object prev = find_player(auction_items[auction_id]["bidder"]);
        if (prev)
            MONEY_D->player_receive(prev, auction_items[auction_id]["current_price"]);
    }

    // 扣当前竞拍者灵石
    if (!MONEY_D->player_pay(bidder, amount))
        return -4; // 灵石不足

    auction_items[auction_id]["bidder"] = bidder->query("id");
    auction_items[auction_id]["bidder_name"] = bidder->query("name");
    auction_items[auction_id]["current_price"] = amount;

    save();
    return 1;
}

// 结算拍卖
void settle_auction(int auction_id)
{
    if (!auction_items[auction_id]) return;
    if (auction_items[auction_id]["status"] != "active") return;

    int now = time();
    if (now < auction_items[auction_id]["end_time"]) return;

    mixed *item = auction_items[auction_id];
    string seller_id = item["seller_id"];
    string bidder_id = item["bidder"];
    int price = item["current_price"];

    if (bidder_id)
    {
        // 有竞拍者→成交
        object seller = find_player(seller_id);
        object bidder = find_player(bidder_id);

        if (seller)
        {
            int tax = to_int(price * TRADE_TAX_AUCTION);
            MONEY_D->player_receive(seller, price - tax);
            tell_object(seller, HIG "你在拍卖行上架的 " + item["item_name"] +
                        " 已以 " + price + " 灵石的价格成交(税后" + (price - tax) + ")。\n" NOR);
        }

        if (bidder && item["item"])
        {
            item["item"]->move(bidder);
            tell_object(bidder, HIG "你在拍卖行竞拍的 " + item["item_name"] +
                        " 已成交，花费 " + price + " 灵石。\n" NOR);
        }
    }
    else
    {
        // 流拍→退还卖家
        object seller = find_player(seller_id);
        if (seller && item["item"])
        {
            item["item"]->move(seller);
            tell_object(seller, "你在拍卖行上架的 " + item["item_name"] + " 流拍，物品已退还。\n");
        }
    }

    auction_items[auction_id]["status"] = "ended";
    save();
}

// ======== 摆摊系统 ========

// 设置摊位
int set_stall(object player, int slot, object item, int price)
{
    if (!player || !item) return 0;
    if (slot < 1 || slot > 5) return 0;

    string stall_path = STALL_PATH + "/" + player->query("id");
    mapping stalls = query(stall_path);
    if (!mapp(stalls)) stalls = ([]);

    stalls[slot] = ([
        "item": item,
        "item_name": item->query("name"),
        "price": price,
        "time": time()
    ]);

    set(stall_path, stalls);
    save();
    return 1;
}

// 购买摊位物品
int buy_stall(string seller_id, int slot, object buyer)
{
    string stall_path = STALL_PATH + "/" + seller_id;
    mapping stalls = query(stall_path);
    if (!mapp(stalls) || !stalls[slot]) return 0;

    mapping item_info = stalls[slot];
    int price = item_info["price"];

    // 扣买家灵石
    if (!MONEY_D->player_pay(buyer, price))
        return -1; // 灵石不足

    // 给卖家灵石
    object seller = find_player(seller_id);
    if (seller)
        MONEY_D->player_receive(seller, price);

    // 物品交给买家
    if (item_info["item"])
        item_info["item"]->move(buyer);

    // 删除摊位
    map_delete(stalls, slot);
    set(stall_path, stalls);
    save();

    return 1;
}

// 交易日志查询(管理员)
string query_trade_log(string date)
{
    if (!trade_log[date]) return "该日期无交易记录。\n";

    string output = "====== " + date + " 交易记录 ======\n";
    for (int i = 0; i < sizeof(trade_log[date]) && i < 50; i++)
        output += trade_log[date][i];

    return output;
}

// 清理所有过期数据(定时调用)
void cleanup()
{
    clean_expired_logs();

    // 结算过期拍卖
    foreach (int id, mapping item in auction_items)
    {
        if (item["status"] == "active" && time() >= item["end_time"])
            settle_auction(id);
    }
}
