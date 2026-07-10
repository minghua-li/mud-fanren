// economy_bridge_d.c  经济循环桥接守护进程 - A1~A3 集成接口
// by BCubed 团队 (#48)
//
// 本守护进程提供经济系统（灵石）与各子系统的桥接接口，
// 负责灵石扣减/记录与跨系统联动的计算逻辑。
//
// 职责范围（对应 ARCH-P3 集成实施计划）：
//   A1 经济→修炼循环：灵石灌注修炼、聚灵阵消耗
//   A2 经济→战斗循环：阵法维持消耗、装备维修、PVP死亡惩罚、比武报名费
//   A3 经济→区域循环：传送阵费用、秘境门票、洞府维护、宗门驻地维护
//
// 依赖：
//   MONEY_D     - 灵石扣减/产出记录/面额换算
//   ECONOMY_D   - 动态定价/区域物价
//   INFLATION_D - 通胀监控/税率查询
//
// 设计原则：
//   1. 只做计算+扣减+记录，不做业务逻辑（不决定阵法是否开启、不判定战斗胜负）
//   2. 扣减记录统一经 MONEY_D->add_consumption() 上报通胀监控
//   3. 所有费用数值对齐设计文档的参考值，不引入硬编码兜底

inherit F_DBASE;

#include <ansi.h>
#include <teleport.h>
#include <region_economy.h>
#include <secret_realm.h>
#include <spirit_root.h>

// ============================================================================
// 常量区
// ============================================================================

// ---------- 灵石面额 ----------
#define LINGSHI_LOW_UNIT     1      // 1 下品灵石
#define LINGSHI_MID_VALUE    100    // 1 中品灵石 = 100 下品
#define LINGSHI_HIGH_VALUE   10000  // 1 上品灵石 = 10000 下品

// ---------- A1：经济→修炼 ----------
// 灵石灌注修炼：灵石→修为值转化比率（按境界）
// 设计参考：02-经济与资源.md §1.2 + 02-灵根养成与突破.md §3.2/§5.2
#define CULTIVATE_RATE_QI        5    // 炼气：每灵石 5 修为值
#define CULTIVATE_RATE_ZHU       3    // 筑基：每灵石 3 修为值
#define CULTIVATE_RATE_JIE       1    // 结丹：每灵石 1 修为值
#define CULTIVATE_RATE_YING      0.5  // 元婴：每灵石 0.5 修为值
#define CULTIVATE_RATE_HUA       0.2  // 化神：每灵石 0.2 修为值

// 聚灵阵费用（下品灵石/时辰，按境界）
// 设计参考：02-经济与资源.md §1.2 "1-100/时辰"
#define GATHER_COST_QI           1    // 炼气：1 灵石/时辰
#define GATHER_COST_ZHU          5    // 筑基：5 灵石/时辰
#define GATHER_COST_JIE          25   // 结丹：25 灵石/时辰
#define GATHER_COST_YING         100  // 元婴：100 灵石/时辰
#define GATHER_COST_HUA          500  // 化神：500 灵石/时辰

// 灵石灌注修炼的单次上限（防刷）
#define MAX_CULTIVATE_PER_TIME   10000 // 单次最多灌注灵石数

// ---------- A2：经济→战斗 ----------
// 阵法维持消耗（下品灵石/回合）
// 设计参考：02-战斗机制与平衡.md §1.3 "灵石×1/回合"~"灵石×3/回合"
#define ARRAY_COST_GATHERING     1    // 聚灵阵：1 灵石/回合
#define ARRAY_COST_NI_TIAN       3    // 颠倒五行阵：3 灵石/回合
#define ARRAY_COST_BEI_DOU       2    // 天罡北斗阵：2 灵石/回合

// 装备维修费率（按品级，维修价=购买价×费率）
// 设计参考：02-经济与资源.md §5.2
#define REPAIR_RATE_FAQI         0.20 // 法器：购买价的 20%
#define REPAIR_RATE_FA_BAO       0.30 // 法宝：购买价的 30%
#define REPAIR_RATE_GU_BAO       0.50 // 古宝：购买价的 50%
#define REPAIR_RATE_TONG_TIAN    0.10 // 通天灵宝：购买价的 10%（每次微量）

// PVP 死亡惩罚（携带灵石的比例范围）
// 设计参考：02-经济与资源.md §1.2 "携带灵石的 10-30%"
#define PVP_PENALTY_MIN          0.10 // 最低惩罚比例 10%
#define PVP_PENALTY_MAX          0.30 // 最高惩罚比例 30%

// 比武报名费（按境界档位）
// 设计参考：02-经济与资源.md §3.3 "50-2000"
#define ARENA_FEE_QI             50    // 炼气期：50 灵石
#define ARENA_FEE_ZHU            200   // 筑基期：200 灵石
#define ARENA_FEE_JIE            500   // 结丹期：500 灵石
#define ARENA_FEE_YING           2000  // 元婴期：2000 灵石
#define ARENA_FEE_HUA            5000  // 化神期+：5000 灵石

// ---------- A3：经济→区域 ----------
// 秘境门票（按秘境类型和难度）
// 设计参考：02-区域游戏玩法.md §2.3 + 02-经济与资源.md §1.2 "50-20000"
#define TICKET_STORY_BASE        50    // 剧情秘境：50 灵石
#define TICKET_TIMED_BASE        500   // 限时秘境：500 灵石
#define TICKET_CHALLENGE_BASE    200   // 挑战秘境：200 灵石
#define TICKET_FORTUNE_BASE      100   // 机缘秘境：100 灵石

#define TICKET_DIFF_EASY         1.0   // 简单难度
#define TICKET_DIFF_NORMAL       1.5   // 普通难度
#define TICKET_DIFF_HARD         2.5   // 困难难度
#define TICKET_DIFF_HELL         4.0   // 地狱难度

// 洞府维护费（按洞府等级，周期 7 天）
// 设计参考：02-区域游戏玩法.md §3.4.3
#define CAVE_MAINT_LV1           0     // Lv.1 草庐：免费
#define CAVE_MAINT_LV2           100   // Lv.2 普通洞府：100 灵石/7天
#define CAVE_MAINT_LV3           500   // Lv.3 灵脉洞府：500 灵石/7天
#define CAVE_MAINT_LV4           2000  // Lv.4 顶级洞府：2000 灵石/7天
#define CAVE_MAINT_LV5           10000 // Lv.5 圣地级：10000 灵石/7天

// 宗门驻地维护费（按驻地等级，周期 7 天）
// 设计参考：02-区域游戏玩法.md §4.4
#define SECT_MAINT_SIMPLE        1000  // 山门驻地：1000 灵石/周
#define SECT_MAINT_STANDARD      5000  // 标准驻地：5000 灵石/周
#define SECT_MAINT_CORE          20000 // 核心驻地：20000 灵石/周
#define SECT_MAINT_HOLY          100000 // 圣地：100000 灵石/周


// ============================================================================
// 初始化
// ============================================================================

void create()
{
        seteuid(getuid());
        set("name", "经济循环桥接系统");
        set("id", "economy_bridge_d");
}

// ---------- 境界名称映射 ----------

// 中文境界名 → 短代码映射（用于 moneyd.c 接口）
// 来源：quest_chain.h REALM_NAMES
string chinese_to_realm_code(string chinese)
{
        if (!stringp(chinese)) return "qige";

        if (strsrch(chinese, "炼气") != -1) return "qige";
        if (strsrch(chinese, "筑基") != -1) return "zhuji";
        if (strsrch(chinese, "结丹") != -1) return "jiedan";
        if (strsrch(chinese, "金丹") != -1) return "jiedan";
        if (strsrch(chinese, "元婴") != -1) return "yuanying";
        if (strsrch(chinese, "化神") != -1) return "huashen";
        if (strsrch(chinese, "炼虚") != -1) return "lianxu";
        if (strsrch(chinese, "合体") != -1) return "heti";
        if (strsrch(chinese, "大乘") != -1) return "dacheng";
        if (strsrch(chinese, "渡劫") != -1) return "dacheng";

        return "qige";
}

// 从玩家对象获取境界短代码
string get_player_realm_code(object player)
{
        string realm_str;

        if (!objectp(player)) return "qige";

        realm_str = player->query("realm");
        if (!stringp(realm_str) || realm_str == "")
                realm_str = player->query("realm_str");

        if (!stringp(realm_str) || realm_str == "")
                return "qige";

        // 如果已经是短代码格式就直接返回
        if (strlen(realm_str) <= 8)
                return realm_str;

        return chinese_to_realm_code(realm_str);
}


// ============================================================================
// A1：经济→修炼循环
// ============================================================================

// 获取灵石转化为修为值的比率
// 参数：realm - 境界字符串（"qige"/"zhuji"/"jiedan"/"yuanying"/"huashen"/"lianxu"）
// 返回：每灵石可兑换的修为值（float）
float query_cultivation_rate(string realm)
{
        switch (realm) {
        case "qige":      return CULTIVATE_RATE_QI;
        case "zhuji":     return CULTIVATE_RATE_ZHU;
        case "jiedan":    return CULTIVATE_RATE_JIE;
        case "yuanying":  return CULTIVATE_RATE_YING;
        case "huashen":   return CULTIVATE_RATE_HUA;
        case "lianxu":    return CULTIVATE_RATE_HUA;  // 炼虚同化神
        case "heti":      return CULTIVATE_RATE_HUA;  // 合体同化神
        case "dacheng":   return CULTIVATE_RATE_HUA;  // 大乘同化神
        default:          return CULTIVATE_RATE_QI;   // 默认炼气
        }
}

// 计算灵石灌注修炼可获得的理论修为值
// 参数：amount - 打算灌注的灵石数
//       realm  - 玩家境界
// 返回：可获得的理论修为值（int）
int calculate_cultivation_gain(int amount, string realm)
{
        float rate = query_cultivation_rate(realm);
        return to_int(to_float(amount) * rate);
}

// 执行灵石灌注修炼（扣减灵石，返回实际获得修为值）
// 参数：player - 玩家对象
//       amount - 灌注的灵石数量
// 返回：实际获得的修为值（0 表示灵石不足或参数非法）
// 调用方负责将返回值加到玩家修为上
int perform_spirit_stone_cultivation(object player, int amount)
{
        string realm;

        if (!objectp(player)) return 0;
        if (amount <= 0) return 0;

        // 单次灌注上限
        if (amount > MAX_CULTIVATE_PER_TIME)
                amount = MAX_CULTIVATE_PER_TIME;

        // 获取玩家境界
        realm = get_player_realm_code(player);

        // 扣除灵石
        if (!MONEY_D->player_pay(player, amount))
                return 0;

        // 计算应得修为值
        int gain = calculate_cultivation_gain(amount, realm);

        // 记录消耗到通胀监控
        MONEY_D->add_consumption("cultivation", amount, realm);

        return gain;
}

// 获取聚灵阵使用费用
// 参数：hours  - 使用时长（时辰数，1时辰=游戏内2小时）
//       realm  - 玩家境界
// 返回：总费用（下品灵石）
int query_gathering_array_fee(int hours, string realm)
{
        int rate;

        switch (realm) {
        case "qige":      rate = GATHER_COST_QI;    break;
        case "zhuji":     rate = GATHER_COST_ZHU;   break;
        case "jiedan":    rate = GATHER_COST_JIE;   break;
        case "yuanying":  rate = GATHER_COST_YING;  break;
        case "huashen":   rate = GATHER_COST_HUA;   break;
        case "lianxu":    rate = GATHER_COST_HUA;   break;
        case "heti":      rate = GATHER_COST_HUA;   break;
        case "dacheng":   rate = GATHER_COST_HUA;   break;
        default:          rate = GATHER_COST_QI;    break;
        }

        // 经济事件影响：灵气潮汐时修炼消耗+25%
        if (find_object(INFLATION_D)) {
                mapping active_event = INFLATION_D->query_active_event();
                if (mapp(active_event) && active_event["type"] == "spirit_tide")
                        rate = to_int(to_float(rate) * 1.25);
        }

        return rate * hours;
}

// 收取聚灵阵费用（直接扣减，用于聚灵阵启动时预扣）
// 参数：player - 玩家对象
//       hours  - 使用时长
//       realm  - 玩家境界
// 返回：1=成功扣减，0=灵石不足
int charge_gathering_array(object player, int hours, string realm)
{
        int fee = query_gathering_array_fee(hours, realm);

        if (fee <= 0) return 1;
        if (!MONEY_D->player_pay(player, fee))
                return 0;

        MONEY_D->add_consumption("gathering_array", fee, realm);
        return 1;
}


// ============================================================================
// A2：经济→战斗循环
// ============================================================================

// 获取阵法维持消耗 / 回合
// 参数：array_type - 阵法类型标识
// 返回：每回合灵石消耗
int query_array_maintenance_cost(string array_type)
{
        int cost;

        switch (array_type) {
        case "gathering":      // 聚灵阵
                cost = ARRAY_COST_GATHERING;
                break;
        case "reverse_five":   // 颠倒五行阵
                cost = ARRAY_COST_NI_TIAN;
                break;
        case "big_dipper":     // 天罡北斗阵
                cost = ARRAY_COST_BEI_DOU;
                break;
        default:
                cost = ARRAY_COST_GATHERING;
                break;
        }

        // 经济事件影响：妖兽潮时装备/阵法损耗+50%
        if (find_object(INFLATION_D)) {
                mapping active_event = INFLATION_D->query_active_event();
                if (mapp(active_event) && active_event["type"] == "beast_tide")
                        cost = to_int(to_float(cost) * 1.5);
        }

        return cost;
}

// 收取一回合阵法维持费
// 参数：player    - 玩家对象
//       array_type - 阵法类型
//       realm     - 玩家境界（用于记录）
// 返回：1=扣减成功，0=灵石不足（应导致阵法失效）
int charge_array_maintenance(object player, string array_type, string realm)
{
        int cost = query_array_maintenance_cost(array_type);

        if (cost <= 0) return 1;
        if (!MONEY_D->player_pay(player, cost))
                return 0;

        MONEY_D->add_consumption("array_maintenance", cost, realm);
        return 1;
}

// 计算连续多回合阵法维持总消耗
// 参数：array_type - 阵法类型
//       rounds     - 回合数
// 返回：总消耗（下品灵石）
int calculate_array_total_cost(string array_type, int rounds)
{
        return query_array_maintenance_cost(array_type) * rounds;
}

// 获取装备维修费用（按装备品级）
// 参数：purchase_price - 装备购买价或估价
//       quality         - 装备品级（"faqi"/"fabao"/"gubao"/"tongtian"）
// 返回：维修费用（下品灵石）
int query_equipment_repair_cost(int purchase_price, string quality)
{
        float rate;

        switch (quality) {
        case "faqi":           // 法器
                rate = REPAIR_RATE_FAQI;
                break;
        case "fabao":          // 法宝
                rate = REPAIR_RATE_FA_BAO;
                break;
        case "gubao":          // 古宝
                rate = REPAIR_RATE_GU_BAO;
                break;
        case "tongtian":       // 通天灵宝
                rate = REPAIR_RATE_TONG_TIAN;
                break;
        default:
                rate = REPAIR_RATE_FAQI;
                break;
        }

        // 经济事件：妖兽潮维修费+50%
        if (find_object(INFLATION_D)) {
                mapping active_event = INFLATION_D->query_active_event();
                if (mapp(active_event) && active_event["type"] == "beast_tide")
                        rate = rate * 1.5;
        }

        return to_int(to_float(purchase_price) * rate);
}

// 收取装备维修费
// 参数：player         - 玩家对象
//       purchase_price  - 装备购买价
//       quality         - 装备品级
//       realm           - 玩家境界
// 返回：1=成功，0=灵石不足
int charge_equipment_repair(object player, int purchase_price, string quality, string realm)
{
        int fee = query_equipment_repair_cost(purchase_price, quality);

        if (fee <= 0) return 1;
        if (!MONEY_D->player_pay(player, fee))
                return 0;

        MONEY_D->add_consumption("equipment_repair", fee, realm);
        return 1;
}

// 查询玩家身上携带的下品灵石总量
// 内部辅助函数，供 PVP 死亡惩罚计算用
int query_carried_spirit_stones(object player)
{
        object cash, gold, silver;
        int total = 0;

        if (!objectp(player)) return 0;

        cash = present("cash_money", player);
        gold = present("gold_money", player);
        silver = present("silver_money", player);

        if (cash) total += cash->query_amount() * 100000;
        if (gold) total += gold->query_amount() * 10000;
        if (silver) total += silver->query_amount() * 100;

        return total;
}

// 计算 PVP 死亡惩罚（败者损失灵石的 10-30%）
// 参数：player - 败者玩家对象
// 返回：应损失的下品灵石数量
int calculate_pvp_death_penalty(object player)
{
        int carried;
        float penalty_ratio;

        if (!objectp(player)) return 0;

        carried = query_carried_spirit_stones(player);
        if (carried <= 0) return 0;

        // 随机选取惩罚比例 10-30%
        penalty_ratio = to_float(10 + random(21)) / 100.0;

        return to_int(to_float(carried) * penalty_ratio);
}

// 执行 PVP 死亡惩罚（扣减败者灵石并记录）
// 参数：loser   - 败者玩家对象
//       winner - 胜者玩家对象（可选，为 0 时灵石归系统回收）
// 返回：实际扣减的灵石数
int apply_pvp_death_penalty(object loser, object winner)
{
        string realm;
        int penalty;

        if (!objectp(loser)) return 0;

        penalty = calculate_pvp_death_penalty(loser);
        if (penalty <= 0) return 0;

        // 获取败者境界
        realm = get_player_realm_code(loser);

        // 扣减败者灵石
        if (!MONEY_D->player_pay(loser, penalty))
                return 0;

        // 胜利者获取 80%，系统回收 20%（作为税收）
        if (objectp(winner)) {
                int to_winner = to_int(to_float(penalty) * 0.8);
                int to_system = penalty - to_winner;
                MONEY_D->pay_player(winner, to_winner);
                MONEY_D->add_consumption("pvp_tax", to_system, realm);
        } else {
                // 无胜者（如被怪物击杀或自杀），全部回收
                MONEY_D->add_consumption("pvp_death", penalty, realm);
        }

        MONEY_D->add_consumption("pvp_penalty", penalty, realm);
        return penalty;
}

// 获取比武报名费
// 参数：realm - 玩家境界字符串
// 返回：报名费（下品灵石）
int query_arena_entry_fee(string realm)
{
        switch (realm) {
        case "qige":      return ARENA_FEE_QI;
        case "zhuji":     return ARENA_FEE_ZHU;
        case "jiedan":    return ARENA_FEE_JIE;
        case "yuanying":  return ARENA_FEE_YING;
        case "huashen":   return ARENA_FEE_HUA;
        case "lianxu":    return ARENA_FEE_HUA;
        case "heti":      return ARENA_FEE_HUA;
        case "dacheng":   return ARENA_FEE_HUA;
        default:          return ARENA_FEE_QI;
        }
}

// 收取比武报名费
// 参数：player - 玩家对象
// 返回：1=成功，0=灵石不足
int charge_arena_entry(object player)
{
        string realm;

        if (!objectp(player)) return 0;

        realm = get_player_realm_code(player);

        int fee = query_arena_entry_fee(realm);
        if (fee <= 0) return 1;

        if (!MONEY_D->player_pay(player, fee))
                return 0;

        // 比武报名费 50% 进入奖金池，50% 系统回收
        MONEY_D->add_consumption("arena_fee", fee, realm);
        return 1;
}


// ============================================================================
// A3：经济→区域循环
// ============================================================================

// 获取传送费用
// 参数：level      - 传送等级（TP_LV*）
//       dist_coeff  - 距离系数（TP_DIST_*）
//       realm_level - 玩家境界索引（TP_REALM_*）
// 返回：传送费用（下品灵石）
int query_teleport_fee(int level, int dist_coeff, int realm_level)
{
        int base_fee;
        int realm_coeff_pct;  // 千分比

        // 基础费
        switch (level) {
        case TP_LV0_CITY:
                base_fee = TP_BASE_CITY;
                break;
        case TP_LV1_REGION:
                base_fee = TP_BASE_INTERCITY;
                break;
        case TP_LV2_CONTINENT:
                base_fee = TP_BASE_CROSSBORDER;
                break;
        case TP_LV3_CROSS:
                base_fee = TP_BASE_CROSSCONT;
                break;
        case TP_LV4_REALM:
                base_fee = TP_BASE_CROSSCONT * 10;  // 跨界更贵
                break;
        default:
                base_fee = TP_BASE_INTERCITY;
                break;
        }

        // 境界系数
        realm_coeff_pct = TP_REALM_COEFF[realm_level];
        if (realm_coeff_pct <= 0) realm_coeff_pct = 100;

        // 公式：费用 = 基础费 × 距离系数 × (境界系数/100)
        int fee = to_int(to_float(base_fee) * to_float(dist_coeff)
                        * to_float(realm_coeff_pct) / 100.0);

        if (fee < 1) fee = 1;
        return fee;
}

// 收取传送费
// 参数：player      - 玩家对象
//       level        - 传送等级
//       dist_coeff   - 距离系数
//       realm_level  - 玩家境界索引
//       region_from  - 出发区域（用于记录）
//       region_to    - 目标区域（用于记录）
// 返回：1=成功，0=灵石不足
int charge_teleport(object player, int level, int dist_coeff,
                    int realm_level, string region_from, string region_to)
{
        int fee = query_teleport_fee(level, dist_coeff, realm_level);

        if (fee <= 0) return 1;
        if (!MONEY_D->player_pay(player, fee))
                return 0;

        MONEY_D->add_consumption("teleport", fee, "transfer");
        return 1;
}

// 获取秘境门票价格
// 参数：type     - 秘境类型（SR_TYPE_*）
//       difficulty - 难度等级（SR_DIFFICULTY_*）
// 返回：门票价格（下品灵石）
int query_secret_realm_ticket(int type, int difficulty)
{
        int base;

        switch (type) {
        case SR_TYPE_STORY:
                base = TICKET_STORY_BASE;
                break;
        case SR_TYPE_TIMED:
                base = TICKET_TIMED_BASE;
                break;
        case SR_TYPE_CHALLENGE:
                base = TICKET_CHALLENGE_BASE;
                break;
        case SR_TYPE_FORTUNE:
                base = TICKET_FORTUNE_BASE;
                break;
        default:
                base = TICKET_STORY_BASE;
                break;
        }

        // 难度系数
        float diff_mod;
        switch (difficulty) {
        case SR_DIFFICULTY_EASY:   diff_mod = TICKET_DIFF_EASY;   break;
        case SR_DIFFICULTY_NORMAL: diff_mod = TICKET_DIFF_NORMAL; break;
        case SR_DIFFICULTY_HARD:   diff_mod = TICKET_DIFF_HARD;   break;
        case SR_DIFFICULTY_HELL:   diff_mod = TICKET_DIFF_HELL;   break;
        default:                   diff_mod = TICKET_DIFF_NORMAL; break;
        }

        return to_int(to_float(base) * diff_mod);
}

// 收取秘境门票
// 参数：player     - 玩家对象
//       type        - 秘境类型
//       difficulty  - 难度等级
//       realm      - 玩家境界（记录用）
// 返回：1=成功，0=灵石不足
int charge_secret_realm_ticket(object player, int type, int difficulty, string realm)
{
        int fee = query_secret_realm_ticket(type, difficulty);

        if (fee <= 0) return 1;
        if (!MONEY_D->player_pay(player, fee))
                return 0;

        MONEY_D->add_consumption("secret_realm_ticket", fee, realm);
        return 1;
}

// 获取洞府维护费
// 参数：cave_level - 洞府等级（1-5）
//       period     - 周期："weekly"（7天）/ "monthly"（30天）
// 返回：维护费用（下品灵石）
int query_cave_maintenance_fee(int cave_level, string period)
{
        int base;

        switch (cave_level) {
        case 1:  base = CAVE_MAINT_LV1; break;
        case 2:  base = CAVE_MAINT_LV2; break;
        case 3:  base = CAVE_MAINT_LV3; break;
        case 4:  base = CAVE_MAINT_LV4; break;
        case 5:  base = CAVE_MAINT_LV5; break;
        default: base = CAVE_MAINT_LV2; break;
        }

        if (period == "monthly")
                base = to_int(to_float(base) * 4.3);  // 约 4.3 周/月

        return base;
}

// 收取洞府维护费
// 参数：player     - 玩家对象
//       cave_level  - 洞府等级
//       period     - 周期
//       realm      - 玩家境界
// 返回：1=成功，0=灵石不足
int charge_cave_maintenance(object player, int cave_level, string period, string realm)
{
        int fee = query_cave_maintenance_fee(cave_level, period);

        if (fee <= 0) return 1;
        if (!MONEY_D->player_pay(player, fee))
                return 0;

        MONEY_D->add_consumption("cave_maintenance", fee, realm);
        return 1;
}

// 获取宗门驻地维护费
// 参数：sect_level - 驻地等级（1-4）
//       period     - 周期："weekly" / "monthly"
// 返回：维护费用（下品灵石，此为全宗总费用，由宗主或仓库支付）
int query_sect_maintenance_fee(int sect_level, string period)
{
        int base;

        switch (sect_level) {
        case 1:  base = SECT_MAINT_SIMPLE;   break;
        case 2:  base = SECT_MAINT_STANDARD; break;
        case 3:  base = SECT_MAINT_CORE;     break;
        case 4:  base = SECT_MAINT_HOLY;     break;
        default: base = SECT_MAINT_SIMPLE;   break;
        }

        if (period == "monthly")
                base = base * 4;  // 约 4 周/月

        // 经济事件：灵气潮汐时消耗+25%
        if (find_object(INFLATION_D)) {
                mapping active_event = INFLATION_D->query_active_event();
                if (mapp(active_event) && active_event["type"] == "spirit_tide")
                        base = to_int(to_float(base) * 1.25);
        }

        return base;
}

// ---------- 通用扣款与记录 ----------

// 通用灵石扣减与消耗记录（供外部系统直接调用）
// 参数：player  - 玩家对象
//       sink_type - 消耗类型标识（如 "teleport", "repair", "cultivation"）
//       amount    - 扣减数量
//       realm     - 玩家境界
// 返回：1=成功，0=灵石不足
int deduct_spirit_stones(object player, string sink_type, int amount, string realm)
{
        if (!objectp(player)) return 0;
        if (amount <= 0) return 1;
        if (!stringp(sink_type)) return 0;
        if (!stringp(realm)) realm = "qige";

        if (!MONEY_D->player_pay(player, amount))
                return 0;

        MONEY_D->add_consumption(sink_type, amount, realm);
        return 1;
}

// 获取全服经济事件对某类消耗的修正系数
// 用于外部系统在显示费用时加注说明
// 返回：修正系数，1.0=无修正
float query_event_cost_modifier()
{
        if (!find_object(INFLATION_D))
                return 1.0;

        mapping active_event = INFLATION_D->query_active_event();
        if (!mapp(active_event) || sizeof(active_event) == 0)
                return 1.0;

        string event_type = active_event["type"];

        // 妖兽潮：装备/阵法损耗+50% → 对应的费用系数
        if (event_type == "beast_tide")
                return 1.5;

        // 灵气潮汐：修炼消耗+25% → 聚灵阵/洞府等费用系数
        if (event_type == "spirit_tide")
                return 1.25;

        return 1.0;
}

// 获取经济事件描述文本（用于系统广播/提示）
string query_event_description()
{
        if (!find_object(INFLATION_D))
                return "";

        mapping active_event = INFLATION_D->query_active_event();
        if (!mapp(active_event) || sizeof(active_event) == 0)
                return "";

        string event_type = active_event["type"];
        int remaining = active_event["remaining"];
        string remain_str;

        if (remaining >= 86400)
                remain_str = sprintf("%d 天", remaining / 86400);
        else if (remaining >= 3600)
                remain_str = sprintf("%d 小时", remaining / 3600);
        else
                remain_str = sprintf("%d 分钟", remaining / 60);

        switch (event_type) {
        case "beast_tide":
                return HIR "【妖兽潮】" NOR "装备损耗+50%，阵法消耗+50%，持续中（剩余" + remain_str + "）";
        case "spirit_tide":
                return HIY "【灵气潮汐】" NOR "修炼消耗+25%，持续中（剩余" + remain_str + "）";
        default:
                return "";
        }
}
