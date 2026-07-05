// faction.c
// 阵营交互与声望查看命令
// 提供: 查看声望、浏览商店、购买物品、查看阵营关系、种族外交

#include <ansi.h>
#include <reputation.h>
#include <globals.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

// 主入口
int main(object me, string arg)
{
    string cmd, subarg;

    if (!arg)
    {
        // 默认显示声望总览
        return show_summary(me);
    }

    if (sscanf(arg, "%s %s", cmd, subarg) != 2)
    {
        cmd = arg;
        subarg = "";
    }

    switch (cmd)
    {
    case "list":
        return list_factions(me, subarg);
    case "info":
        return show_faction_info(me, subarg);
    case "shop":
        return show_shop(me, subarg);
    case "buy":
        return buy_item(me, subarg);
    case "race":
        return show_race(me, subarg);
    case "rank":
        return show_reputation_rank(me, subarg);
    case "relation":
        return set_relation(me, subarg);
    case "help":
        return show_help(me);
    default:
        // 尝试作为势力名/ID查看声望详情
        return show_faction_detail(me, arg);
    }
}

// 显示声望总览
int show_summary(object me)
{
    string output;
    string *factions = REPUTATION_D->get_all_factions();

    output = HIC "≡  ≡  ≡  ≡  ≡  ≡  【 声望总览 】 ≡  ≡  ≡  ≡  ≡  ≡\n" NOR;
    output += "你可以使用 "HIY"faction list"NOR" 查看所有势力列表\n";
    output += "使用 "HIY"faction <势力名/ID>"NOR" 查看具体声望详情\n";
    output += "使用 "HIY"faction shop <势力ID>"NOR" 浏览商店\n";
    output += "使用 "HIY"faction race"NOR" 查看灵界种族关系\n\n";

    output += HIC "◈ 势力声望概览\n" NOR;
    output += sprintf("%-20s %-16s %-10s %s\n", "势力", "声望等级", "数值", "关系");
    output += "─────────────────────────────────────────────\n";

    // 根据声望等级排序显示
    mapping ranked = ([]);
    foreach (string fid in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(fid);
        if (!info) continue;
        int rep = REPUTATION_D->query_reputation_value(me, fid);
        int level = REPUTATION_D->query_reputation_level(me, fid);
        string rel = REPUTATION_D->query_faction_relation(me, fid);
        string rel_name = (rel == "ally") ? HIG "盟友" NOR :
                          (rel == "hostile") ? HIR "敌对" NOR : "中立";

        string level_str = REPUTATION_D->get_reputation_level_name(level);

        // 按门派type分组显示
        string type_prefix;
        int t = info["type"];
        if (t == FACTION_TYPE_RIGHTEOUS) type_prefix = HIC + "{" + info["name"] + "}" + NOR;
        else if (t == FACTION_TYPE_EVIL) type_prefix = HIR + "{" + info["name"] + "}" + NOR;
        else if (t == FACTION_TYPE_NEUTRAL) type_prefix = HIY + "{" + info["name"] + "}" + NOR;
        else type_prefix = HIM + "{" + info["name"] + "}" + NOR;

        output += sprintf("%-22s %-18s %-+8d %s\n", type_prefix, level_str, rep, rel_name);
    }

    // 全局声望
    int righteous_rep = me->query(REP_PATH_GLOBAL + "/righteous");
    int evil_rep = me->query(REP_PATH_GLOBAL + "/evil");
    output += "\n" HIC "◈ 全局声望\n" NOR;
    output += sprintf("  正道声望: %-+10d  魔道声望: %-+10d\n", righteous_rep, evil_rep);

    output += "\n" HIC "◈ 种族声望（使用 "HIY"faction race"NOR + HIC " 查看详情）\n" NOR;

    me->start_more(output);
    return 1;
}

// 列出所有势力
int list_factions(object me, string arg)
{
    string output;
    string *factions = REPUTATION_D->get_all_factions();

    output = HIC "≡  ≡  ≡  ≡  【 势力列表 】≡  ≡  ≡  ≡\n\n" NOR;

    output += HIC "◇ 正道势力\n" NOR;
    foreach (string fid in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(fid);
        if (info && info["type"] == FACTION_TYPE_RIGHTEOUS)
            output += sprintf("  %-20s %s\n", info["name"], info["desc"]);
    }

    output += "\n" HIR "◇ 魔道势力\n" NOR;
    foreach (string fid in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(fid);
        if (info && info["type"] == FACTION_TYPE_EVIL)
            output += sprintf("  %-20s %s\n", info["name"], info["desc"]);
    }

    output += "\n" HIY "◇ 中立势力\n" NOR;
    foreach (string fid in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(fid);
        if (info && info["type"] == FACTION_TYPE_NEUTRAL)
            output += sprintf("  %-20s %s\n", info["name"], info["desc"]);
    }

    output += "\n" HIM "◇ 组织\n" NOR;
    foreach (string fid in factions)
    {
        mapping info = REPUTATION_D->get_faction_info(fid);
        if (info && info["type"] == FACTION_TYPE_ORGANIZATION)
            output += sprintf("  %-20s %s\n", info["name"], info["desc"]);
    }

    output += "\n" HIW "使用 faction <势力名> 查看详情" NOR "\n";

    me->start_more(output);
    return 1;
}

// 显示具体势力详情（含商店入口）
int show_faction_detail(object me, string arg)
{
    if (!arg) return notify_fail("你想查看哪个势力的声望？\n");

    // 尝试按名称或ID查找
    string *factions = REPUTATION_D->get_all_factions();
    string target_id = 0;
    mapping info = 0;

    foreach (string fid in factions)
    {
        mapping fi = REPUTATION_D->get_faction_info(fid);
        if (fi)
        {
            if (fid == arg || fi["name"] == arg)
            {
                target_id = fid;
                info = fi;
                break;
            }
        }
    }

    if (!info) return notify_fail("找不到这个势力。使用 "HIY"faction list"NOR" 查看所有势力。\n");

    string output;
    int rep = REPUTATION_D->query_reputation_value(me, target_id);
    int level = REPUTATION_D->query_reputation_level(me, target_id);
    string rel = REPUTATION_D->query_faction_relation(me, target_id);

    // 势力名称带颜色
    string type_color;
    switch (info["type"])
    {
    case FACTION_TYPE_RIGHTEOUS:  type_color = HIC; break;
    case FACTION_TYPE_EVIL:       type_color = HIR; break;
    case FACTION_TYPE_NEUTRAL:    type_color = HIY; break;
    case FACTION_TYPE_ORGANIZATION: type_color = HIM; break;
    default: type_color = NOR;
    }

    output = type_color + "≡  ≡  ≡  ≡  【 " + info["name"] + "】≡  ≡  ≡  ≡\n\n" NOR;

    output += sprintf(HIW "  势力类型" NOR ": %s\n", 
                      info["type"] == FACTION_TYPE_RIGHTEOUS ? "正道" :
                      info["type"] == FACTION_TYPE_EVIL ? "魔道" :
                      info["type"] == FACTION_TYPE_NEUTRAL ? "中立" : "组织");
    output += sprintf(HIW "  势力简介" NOR ": %s\n\n", info["desc"]);

    // 声望信息
    output += sprintf(HIW "  当前声望" NOR ": %-+8d\n", rep);
    output += sprintf(HIW "  声望等级" NOR ": %s\n", REPUTATION_D->get_reputation_level_name(level));
    output += sprintf(HIW "  阵营关系" NOR ": %s\n",
                      rel == "ally" ? HIG "盟友" NOR :
                      rel == "hostile" ? HIR "敌对" NOR : "中立");

    // 可用的交互选项
    mixed *actions = REPUTATION_D->get_available_actions(me, target_id);
    if (sizeof(actions) > 0)
    {
        output += sprintf(HIW "  可用交互" NOR ": ");
        string *action_names = ({});
        for (int i = 0; i < sizeof(actions); i++)
            action_names += ({ REPUTATION_D->get_action_name(actions[i]) });
        output += implode(action_names, "、") + "\n";
    }
    else
    {
        output += HIR "  可用交互: 无（声望过低）\n" NOR;
    }

    // 折扣信息
    if (level >= REP_LEVEL_NEUTRAL)
    {
        float discount = REPUTATION_D->query_discount(target_id);
        output += sprintf(HIW "  商店折扣" NOR ": %.0f%%\n", (1 - discount) * 100);
    }
    else if (level <= REP_LEVEL_HOSTILE)
    {
        output += HIR "  商店状态: 已封锁（敌对关系）\n" NOR;
    }
    else if (level == REP_LEVEL_COLD)
    {
        output += HIR "  商店状态: 限制交易（冷淡，价格翻倍）\n" NOR;
    }

    // 商店入口
    output += sprintf("\n" HIY "  使用 "HIG"faction shop %s"HIY" 浏览该势力的商店\n" NOR, target_id);
    output += sprintf(HIY "  使用 "HIG"faction relation %s [ally|hostile|neutral]"HIY" 设置阵营关系\n" NOR, target_id);

    me->start_more(output);
    return 1;
}

// 显示势力信息（简版）
int show_faction_info(object me, string arg)
{
    return show_faction_detail(me, arg);
}

// 商店浏览
int show_shop(object me, string arg)
{
    if (!arg) return notify_fail("用法: faction shop <势力ID>\n");

    string *factions = REPUTATION_D->get_all_factions();
    string target_id = 0;
    mapping info = 0;

    foreach (string fid in factions)
    {
        mapping fi = REPUTATION_D->get_faction_info(fid);
        if (fi && (fid == arg || fi["name"] == arg))
        {
            target_id = fid;
            info = fi;
            break;
        }
    }

    if (!info) return notify_fail("找不到这个势力。\n");

    int level = REPUTATION_D->query_reputation_level(me, target_id);
    if (level <= REP_LEVEL_DEADLY)
        return notify_fail(HIR + info["name"] + "与你处于死敌状态，无法交易！\n" NOR);

    mixed *items = SHOP_D->query_available_items(me, target_id);
    if (sizeof(items) == 0)
        return notify_fail("你在" + info["name"] + "的声望不足以购买任何物品。\n");

    string output;
    output = HIY "≡  ≡  ≡  ≡  【 " + info["name"] + "商店 】≡  ≡  ≡  ≡\n\n" NOR;

    // 各层级分开显示
    int current_tier = -1;

    for (int i = 0; i < sizeof(items); i++)
    {
        mixed *item = items[i];
        int tier = item[7];

        if (tier != current_tier)
        {
            string tier_name;
            switch (tier)
            {
            case SHOP_TIER_BASIC:       tier_name = HIG "◆ " SHOP_TIER_NAME_BASIC; break;
            case SHOP_TIER_INTERMEDIATE:tier_name = HIB "◆ " SHOP_TIER_NAME_INTERMEDIATE; break;
            case SHOP_TIER_ADVANCED:    tier_name = HIM "◆ " SHOP_TIER_NAME_ADVANCED; break;
            case SHOP_TIER_CORE:        tier_name = HIY "◆ " SHOP_TIER_NAME_CORE; break;
            case SHOP_TIER_SECRET:      tier_name = HIR "◆ " SHOP_TIER_NAME_SECRET; break;
            default:                    tier_name = "◆ 未知层级";
            }
            output += "\n" + tier_name + NOR "\n";
            output += "─────────────────────────────────────────────\n";
            current_tier = tier;
        }

        string item_id = item[0];
        string name = item[1];
        string type = item[2];
        int price = item[4];
        int rep_cost = item[6];
        string desc = item[9];
        int stock = SHOP_D->query_stock(item_id);

        // 计算折扣价
        float discount = REPUTATION_D->query_discount(target_id);
        int final_price = to_int(price * discount);

        string type_str;
        switch (type)
        {
        case "weapon": type_str = HIR "武器" NOR; break;
        case "armor":  type_str = HIC "防具" NOR; break;
        case "pill":   type_str = HIG "丹药" NOR; break;
        case "skill":  type_str = HIM "功法" NOR; break;
        default:       type_str = HIW "物品" NOR;
        }

        string stock_str;
        if (stock == -1) stock_str = "∞";
        else if (stock <= 0) stock_str = HIR "售罄" NOR;
        else stock_str = sprintf("%d", stock);

        output += sprintf("  " HIG "%s" NOR " [%s] %s\n", item_id, type_str, name);
        output += sprintf("    价格: " HIY "%d灵石" NOR, final_price);
        if (rep_cost > 0)
            output += sprintf(" + 消耗声望: " HIC "%d" NOR, rep_cost);
        else
            output += sprintf(" + 消耗声望: " HIC "0" NOR);
        output += sprintf("  库存: %s\n", stock_str);
        output += sprintf("    说明: %s\n", desc);
        output += sprintf("    购买: " HIG "faction buy %s %s [数量]" NOR "\n", target_id, item_id);
    }

    output += "\n" HIW "使用 faction buy <势力ID> <物品ID> [数量] 购买物品\n" NOR;
    output += sprintf(HIW "当前折扣: %.0f%%\n" NOR, (1 - discount) * 100);

    me->start_more(output);
    return 1;
}

// 购买物品
int buy_item(object me, string arg)
{
    string faction, item_id, quantity_str;
    int quantity = 1;

    if (!arg) return notify_fail("用法: faction buy <势力ID> <物品ID> [数量]\n");

    if (sscanf(arg, "%s %s %d", faction, item_id, quantity) == 3)
        ;
    else if (sscanf(arg, "%s %s", faction, item_id) == 2)
        ;
    else
        return notify_fail("用法: faction buy <势力ID> <物品ID> [数量]\n");

    // 查找势力
    string *factions = REPUTATION_D->get_all_factions();
    string target_id = 0;
    foreach (string fid in factions)
    {
        mapping fi = REPUTATION_D->get_faction_info(fid);
        if (fi && (fid == faction || fi["name"] == faction))
        {
            target_id = fid;
            break;
        }
    }
    if (!target_id) return notify_fail("找不到这个势力。\n");

    // 执行购买
    int result = SHOP_D->buy_item(me, item_id, quantity);

    switch (result)
    {
    case 1:
    {
        mixed *item = SHOP_D->get_item_by_id(item_id);
        if (!item) return notify_fail("购买成功！\n");

        string item_name = item[1];
        mapping fi = REPUTATION_D->get_faction_info(target_id);
        string faction_name = fi ? fi["name"] : target_id;

        // 实际物品给予（需在对应NPC处调用）
        tell_object(me, HIG "你成功从" + faction_name + "购买了" + item_name + NOR "\n");
        // 实际应由商店NPC调用此接口，传入物品生成函数
        write(HIG "购买成功！请在对应商店NPC处领取物品。\n" NOR);
        return 1;
    }
    case -1:
        return notify_fail("你的声望等级不足以购买此物品。\n");
    case -2:
        return notify_fail("你的声望值不足以支付此次购买消耗。\n");
    case -3:
        return notify_fail("你的灵石不足以支付此次购买。\n");
    case -4:
        return notify_fail("该物品库存不足。\n");
    case -5:
        return notify_fail("该势力与你处于敌对状态，无法交易。\n");
    default:
        return notify_fail("购买失败（未知错误）。\n");
    }
}

// 种族外交查看
int show_race(object me, string arg)
{
    string output;
    string *races = REPUTATION_D->get_all_races();

    output = HIC "≡  ≡  ≡  ≡  【 灵界种族外交 】≡  ≡  ≡  ≡\n\n" NOR;

    // 如有参数查看特定种族
    if (arg)
    {
        foreach (string rid in races)
        {
            mapping ri = REPUTATION_D->get_race_info(rid);
            if (ri && (rid == arg || ri["name"] == arg))
            {
                int rep = me->query(REP_PATH_RACE + "/" + rid);
                int level = REPUTATION_D->query_race_relation_level(me, rid);
                int initial = REPUTATION_D->query_race_initial(rid);

                output += sprintf(HIW "  【%s】\n" NOR, ri["name"]);
                output += sprintf("  种族描述: %s\n", ri["desc"]);
                output += sprintf("  初始关系: %s\n", REPUTATION_D->get_race_relation_name(initial));
                output += sprintf("  当前声望: %+d\n", rep);
                output += sprintf("  关系等级: %s\n", REPUTATION_D->get_reputation_level_name(level));

                // 根据关系显示交互提示
                if (level <= REP_LEVEL_DEADLY)
                    output += HIR "  ⚠ 死敌状态: 见面即厮杀\n" NOR;
                else if (level <= REP_LEVEL_HOSTILE)
                    output += HIR "  ⚠ 敌对: 进入领地将被攻击\n" NOR;
                else if (level >= REP_LEVEL_RESPECT)
                    output += HIG "  ✓ 高声望: 可解锁特殊种族任务\n" NOR;

                me->start_more(output);
                return 1;
            }
        }
        return notify_fail("找不到该种族。\n");
    }

    // 列表模式
    output += sprintf("%-14s %-12s %-10s %s\n", "种族", "初始关系", "当前声望", "等级");
    output += "─────────────────────────────────────────────\n";

    foreach (string rid in races)
    {
        mapping ri = REPUTATION_D->get_race_info(rid);
        if (!ri) continue;

        int rep = me->query(REP_PATH_RACE + "/" + rid);
        int level = REPUTATION_D->query_race_relation_level(me, rid);

        string rep_str;
        if (rep > 0) rep_str = HIG + sprintf("+%d", rep) + NOR;
        else if (rep < 0) rep_str = HIR + sprintf("%d", rep) + NOR;
        else rep_str = sprintf("%d", rep);

        string init_str;
        int init = ri["initial"];
        switch (init)
        {
        case RACE_RELATION_ALLY:  init_str = HIG "友善" NOR; break;
        case RACE_RELATION_NEUTRAL: init_str = "中立"; break;
        case RACE_RELATION_HOSTILE: init_str = HIR "不友好" NOR; break;
        case RACE_RELATION_DEADLY:  init_str = HIR "敌对" NOR; break;
        default: init_str = "中立";
        }

        output += sprintf("%-14s %-12s %-10s %s\n",
                          ri["name"], init_str, rep_str,
                          REPUTATION_D->get_reputation_level_name(level));
    }

    output += "\n" HIW "使用 faction race <种族名> 查看详情" NOR "\n";

    me->start_more(output);
    return 1;
}

// 查看声望排行
int show_reputation_rank(object me, string arg)
{
    // 这是一个简易的声望等级展示
    string output;

    output = HIY "≡  ≡  ≡  ≡  【 声望等级指南 】≡  ≡  ≡  ≡\n\n" NOR;

    output += sprintf("%-12s %-10s %-8s %s\n", "等级", "名称", "数值范围", "效果");
    output += "─────────────────────────────────────────────\n";
    output += sprintf("%-12s %-10s %-8s %s\n", "5", REPUTATION_D->get_reputation_level_name(5),
                      ">80000", "隐藏内容，亲传弟子资格");
    output += sprintf("%-12s %-10s %-8s %s\n", "4", REPUTATION_D->get_reputation_level_name(4),
                      "25001~80000", "6折优惠，独有传承");
    output += sprintf("%-12s %-10s %-8s %s\n", "3", REPUTATION_D->get_reputation_level_name(3),
                      "8001~25000", "8折优惠，核心功法");
    output += sprintf("%-12s %-10s %-8s %s\n", "2", REPUTATION_D->get_reputation_level_name(2),
                      "2001~8000", "9折优惠，精英任务");
    output += sprintf("%-12s %-10s %-8s %s\n", "1", REPUTATION_D->get_reputation_level_name(1),
                      "101~2000", "95折优惠，普通任务");
    output += sprintf("%-12s %-10s %-8s %s\n", "0", REPUTATION_D->get_reputation_level_name(0),
                      "-100~100", "正常交易");
    output += sprintf("%-12s %-10s %-8s %s\n", "-1", REPUTATION_D->get_reputation_level_name(-1),
                      "-1999~-101", "价格翻倍");
    output += sprintf("%-12s %-10s %-8s %s\n", "-2", REPUTATION_D->get_reputation_level_name(-2),
                      "-9999~-2000", "被攻击，商店封锁");
    output += sprintf("%-12s %-10s %-8s %s\n", "-3", REPUTATION_D->get_reputation_level_name(-3),
                      "≤-10000", "见面即厮杀");

    output += "\n" HIW "每日声望获取上限（依境界提升）：\n" NOR;
    output += "  炼气: 500/日\n";
    output += "  筑基: 2000/日\n";
    output += "  结丹: 6000/日\n";
    output += "  元婴: 20000/日\n";
    output += "  化神: 50000/日\n";
    output += "  炼虚+: 100000/日\n";

    me->start_more(output);
    return 1;
}

// 设置阵营关系
int set_relation(object me, string arg)
{
    string faction, relation;

    if (!arg || sscanf(arg, "%s %s", faction, relation) != 2)
        return notify_fail("用法: faction relation <势力名> ally|hostile|neutral\n");

    string *factions = REPUTATION_D->get_all_factions();
    string target_id = 0;
    foreach (string fid in factions)
    {
        mapping fi = REPUTATION_D->get_faction_info(fid);
        if (fi && (fid == faction || fi["name"] == faction))
        {
            target_id = fid;
            break;
        }
    }
    if (!target_id) return notify_fail("找不到这个势力。\n");

    if (member_array(relation, ({ "ally", "hostile", "neutral" })) == -1)
        return notify_fail("关系只能是 ally(盟友), hostile(敌对), neutral(中立) 之一。\n");

    int level = REPUTATION_D->query_reputation_level(me, target_id);
    if (relation == "ally" && level < REP_LEVEL_TRUST)
        return notify_fail("你的声望等级不够与" + faction + "结盟（需要信任及以上）。\n");

    REPUTATION_D->set_faction_relation(me, target_id, relation);

    mapping fi = REPUTATION_D->get_faction_info(target_id);
    string relation_name = (relation == "ally") ? HIG "盟友" NOR :
                           (relation == "hostile") ? HIR "敌对" NOR :
                           HIW "中立" NOR;

    tell_object(me, sprintf("你将与%s的关系设为：%s。\n",
                fi["name"], relation_name));
    return 1;
}

// 帮助
int show_help(object me)
{
    write(@HELP
╔═══════════════════════════════════════════════╗
║  faction  - 阵营交互与声望系统                ║
╠═══════════════════════════════════════════════╣
║  faction                    声望总览          ║
║  faction list               列出所有势力      ║
║  faction <势力名>           查看势力声望详情  ║
║  faction shop <势力ID>      浏览势力商店      ║
║  faction buy <ID> <物品> [数量] 购买物品      ║
║  faction race               种族外交概览      ║
║  faction race <种族名>      查看种族详情      ║
║  faction rank               声望等级指南      ║
║  faction relation <名> <关系> 设置阵营关系    ║
║                               ally/hostile/neutral
║  faction help               显示本帮助        ║
╚═══════════════════════════════════════════════╝
HELP
    );
    return 1;
}

int help(object me)
{
    return show_help(me);
}
