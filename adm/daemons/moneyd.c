// moneyd.c  钱的功能 + 灵石产出总控 + 六大经济循环接口
// by Xiang@XKX (95/12/22)
// 灵石经济扩展 by BCubed 团队 (#19, #48)

// 境界产出系数（基准）
#define REALM_COEFF_QIGE     1.0    // 炼气
#define REALM_COEFF_ZHUJI    3.0    // 筑基
#define REALM_COEFF_JIEDAN   10.0   // 结丹
#define REALM_COEFF_YUANYING 30.0   // 元婴
#define REALM_COEFF_HUASHEN  80.0   // 化神+

#include <economy_circulation.h>

// 灵石面额换算
#define LINGSHI_LOW_UNIT     1      // 1 下品灵石
#define LINGSHI_MID_VALUE    100    // 1 中品灵石 = 100 下品
#define LINGSHI_HIGH_VALUE   10000  // 1 上品灵石 = 10000 下品

// 每日灵石产出追踪（进程内，重启重置）
int daily_production;          // 今日已产出（下品灵石）
int last_reset_day;            // 上次重置的日子
mapping realm_player_count;    // ([ "qige": N, "zhuji": N, ... ])

// 产出记录（最近24小时）
mapping production_log;        // ([ "timestamp": count, ... ])
mapping consumption_log;       // ([ "timestamp": count, ... ])

// ---------- 内部辅助 ----------

void reset_daily_counter()
{
        daily_production = 0;
        production_log = ([]);
        consumption_log = ([]);
}

void check_day_rollover()
{
        int today = time() / 86400;
        if (today != last_reset_day) {
                last_reset_day = today;
                reset_daily_counter();
        }
}

// ---------- 境界产出系数 ----------

float query_realm_coefficient(string realm)
{
        switch (realm) {
        case "qige":       return REALM_COEFF_QIGE;
        case "zhuji":      return REALM_COEFF_ZHUJI;
        case "jiedan":     return REALM_COEFF_JIEDAN;
        case "yuanying":   return REALM_COEFF_YUANYING;
        case "huashen":    return REALM_COEFF_HUASHEN;
        default:           return REALM_COEFF_QIGE;
        }
}

// ---------- 活跃度修正 ----------

float query_activity_modifier(int online_minutes)
{
        if (online_minutes < 30)  return 0.3;
        if (online_minutes < 120) return 0.8;
        if (online_minutes < 240) return 1.0;
        return 1.1;  // 边际递减，防挂机
}

// ---------- 日产出上限 ----------

int query_daily_production_cap()
{
        // 全服日产出上限 = Σ(各境界在线玩家数 × 境界产出系数)
        // 按每玩家基准 1 下品灵石/小时，日基准 = 在线人数 × 24
        check_day_rollover();
        int total = 0;
        int base_per_hour = 1;  // 炼气基准 1 灵石/小时

        if (!mapp(realm_player_count)) realm_player_count = ([]);

        foreach (string realm, int count in realm_player_count) {
                float coeff = query_realm_coefficient(realm);
                total += to_int(to_float(count) * coeff * base_per_hour * 24);
        }

        return total;
}

// ---------- 更新在线玩家统计 ----------

void update_realm_count(string realm, int delta)
{
        if (!mapp(realm_player_count))
                realm_player_count = ([]);

        int cur = realm_player_count[realm];
        cur += delta;
        if (cur < 0) cur = 0;
        realm_player_count[realm] = cur;
        // 更新通胀监控的在线玩家统计
        if (find_object(INFLATION_D)) {
                int total = 0;
                foreach (string r, int cnt in realm_player_count)
                        total += cnt;
                INFLATION_D->update_active_players(total);
        }
}

// ---------- 灵石产出记录 ----------

int add_production(string source, int amount, string realm)
{
        check_day_rollover();
        daily_production += amount;

        if (!mapp(production_log)) production_log = ([]);
        int now = time();
        production_log[now] = amount;

        // 通知通胀监控
        if (find_object(INFLATION_D))
                INFLATION_D->on_production(amount, realm);

        return daily_production;
}

int add_consumption(string sink, int amount, string realm)
{
        if (!mapp(consumption_log)) consumption_log = ([]);
        int now = time();
        consumption_log[now] = amount;

        // 通知通胀监控
        if (find_object(INFLATION_D))
                INFLATION_D->on_consumption(amount, realm);

        return 1;
}

int query_daily_production()
{
        check_day_rollover();
        return daily_production;
}

// ---------- 查询统计 ----------

int query_total_production_last_hour()
{
        int now = time();
        int cutoff = now - 3600;
        int total = 0;

        if (!mapp(production_log)) return 0;
        foreach (int ts, int amount in production_log) {
                if (ts >= cutoff) total += amount;
        }
        return total;
}

int query_total_consumption_last_hour()
{
        int now = time();
        int cutoff = now - 3600;
        int total = 0;

        if (!mapp(consumption_log)) return 0;
        foreach (int ts, int amount in consumption_log) {
                if (ts >= cutoff) total += amount;
        }
        return total;
}

// ---------- 灵石面额工具 ----------

string spirit_stone_str(int amount)
{
        // 将下品灵石数量转为可读字符串
        if (amount >= LINGSHI_HIGH_VALUE)
                return chinese_number(amount / LINGSHI_HIGH_VALUE) + "块上品灵石";
        if (amount >= LINGSHI_MID_VALUE)
                return chinese_number(amount / LINGSHI_MID_VALUE) + "块中品灵石";
        return chinese_number(amount) + "块下品灵石";
}

// ========== 以下为原有钱币功能 ==========

string money_str(int amount)
{
        // returns a chinese string of `amount` of money
        string output;

        if (amount / 10000) {
                output = chinese_number(amount / 10000) + "两黄金";
                amount %= 10000;
        }
        else
                output = "";
        if (amount / 100) {
                output = output + chinese_number(amount / 100) + "两白银";
                amount %= 100;
        }
        if (amount)
                return output + chinese_number(amount) + "文铜板";
        return output;
}

string price_str(int amount)
{
        // returns a chinese string of `amount` of money
	string output;

        if (amount < 1)
                amount = 1;

        if (amount / 10000) {
                output = chinese_number(amount / 10000) + "两黄金";
                amount %= 10000;
        }
        else
                output = "";
        if (amount / 100) {
		if (output != "")
                	output += "又" + chinese_number(amount / 100) + "两白银";
		else
			output = chinese_number(amount / 100) + "两白银";
                amount %= 100;
        }
        if (amount)
		if (output != "")
                	return output + "又" + chinese_number(amount) + "文铜板";
		else
			return chinese_number(amount) + "文铜板";
        return output;
}

void pay_player(object who, int amount)
{
        int v;
        object ob;

	seteuid(getuid());
        if (amount < 1)
                amount = 1;
        if( amount/100000 ) 
			{
                ob = new("/clone/money/thousand-cash");
                ob->set_amount(amount/100000);
				if(!ob->move(who))
				ob->move(environment(who));
                amount %= 100000;
			}
        if (v = amount / 10000) {
                ob = new(GOLD_OB);
                ob->set_amount(amount / 10000);
				if(!ob->move(who))
				ob->move(environment(who));
                amount %= 10000;
        }
        if (amount / 100) {
                ob = new(SILVER_OB);
                ob->set_amount(amount / 100);
				if(!ob->move(who))
				ob->move(environment(who));
                amount %= 100;
        }
        //不给coin了，节省对象
       /* if (amount) {
                ob = new(COIN_OB);
                ob->set_amount(amount);
				if(!ob->move(who))
				ob->move(environment(who));
        }*/
}

int player_pay(object who, int amount)
{
	object cash_ob, g_ob, s_ob, c_ob;
	int cash_c, gc, sc, cc, left;

	seteuid(getuid());

	if (cash_ob = present("cash_money", who))
		cash_c = cash_ob->query_amount();
	else
		cash_c = 0;

	if (g_ob = present("gold_money", who))
		gc = g_ob->query_amount();
	else
		gc = 0;
	if (s_ob = present("silver_money", who))
		sc = s_ob->query_amount();
	else
		sc = 0;
	if (c_ob = present("coin_money", who))
		cc = c_ob->query_amount();
	else
		cc = 0;
	
	if (cc + sc * 100 + gc * 10000 + cash_c * 100000 < amount) 
			return 0;
	else {
		left = cc + sc * 100 + gc * 10000  + cash_c * 100000 - amount;
        cash_c = left / 100000;
		left = left % 100000;
        gc = left / 10000;
		left = left % 10000;
		sc = left / 100;
		cc = left % 100;

		if (cash_ob)
			cash_ob->set_amount(cash_c);
		else gc += (cash_c * 10);

		if (g_ob)
			g_ob->set_amount(gc);
		else sc += (gc * 100);
		if (s_ob)
		 	s_ob->set_amount(sc);
		else if (sc) {
			s_ob = new(SILVER_OB);
			s_ob->set_amount(sc);
			s_ob->move(who);
		}
		if (c_ob)
			c_ob->set_amount(cc);
		else if (cc) {
            //不找coin，节省对象
			//c_ob = new(COIN_OB);
			//c_ob->set_amount(cc);
			//c_ob->move(who);
		}

		return 1;
	}
}

// =============================================================================
// P3 集成 A：六大经济循环接口
// =============================================================================

// ---------- A1. 经济→修炼循环 ----------

// 灵石灌注修为：消耗灵石换取修为值
// 参数: who - 玩家对象, amount - 消耗的下品灵石数量
// 返回: 获得的修为值（int），-1 表示灵石不足
int spirit_infuse_cultivation(object who, int amount)
{
        string realm;
        float realm_coeff;
        int gained_exp;

        if (!who || amount <= 0) return -1;

        // 检查玩家灵石是否充足
        if (!player_pay(who, amount))
                return -1;

        // 获取玩家境界系数
        realm = who->query("realm");
        if (!stringp(realm) || realm == "")
                realm = "qige";
        realm_coeff = query_realm_coefficient(realm);

        // 计算可获得修为值：基础转化率 × 境界系数
        gained_exp = to_int(SPIRIT_TO_EXP_BASE * realm_coeff * to_float(amount));

        // 给玩家加修为
        who->add("combat_exp", gained_exp);

        // 记录消耗
        add_consumption(EC_SINK_SPIRIT_INFUSE, amount, realm);

        return gained_exp;
}

// 聚灵阵费用计算
// 参数: who - 玩家对象, duration - 使用时长（时辰数）
// 返回: 总费用（下品灵石），-1 表示灵石不足无法支付
int poly_spirit_array_cost(object who, int duration)
{
        string realm;
        float realm_coeff;
        int cost;

        if (!who || duration <= 0) return -1;

        realm = who->query("realm");
        if (!stringp(realm) || realm == "")
                realm = "qige";

        // 查询境界倍率
        realm_coeff = query_realm_coefficient(realm);

        // 总费用 = 基准价 × 境界系数 × 时长
        cost = to_int(POLY_ARRAY_COST_BASE * realm_coeff * duration);

        // 应用通胀监控的产出增益系数调整
        if (find_object(INFLATION_D)) {
                float boost = INFLATION_D->query_output_boost();
                cost = to_int(to_float(cost) * boost);
        }

        if (cost <= 0) cost = 1;

        // 扣费
        if (!player_pay(who, cost))
                return -1;

        // 记录消耗
        add_consumption(EC_SINK_POLY_ARRAY, cost, realm);

        return cost;
}

// ---------- A2. 经济→战斗循环 ----------

// 阵法维持消耗：战斗中每回合的灵石消耗
// 参数: who - 玩家对象, rounds - 持续回合数
// 返回: 总消耗（下品灵石），-1 表示灵石不足
int formation_maintain_cost(object who, int rounds)
{
        string realm;
        int total_cost;

        if (!who || rounds <= 0) return -1;

        realm = who->query("realm");
        if (!stringp(realm) || realm == "")
                realm = "jiedan";

        // 每回合消耗基准 2 灵石 × 境界系数
        float coeff = query_realm_coefficient(realm);
        total_cost = to_int(FORMATION_COST_PER_ROUND * coeff * to_float(rounds));

        if (total_cost <= 0) total_cost = rounds;

        // 检查是否存在经济事件（如妖兽潮→装备损耗+50%）
        if (find_object(INFLATION_D)) {
                mapping evt = INFLATION_D->query_active_event();
                if (mapp(evt) && evt["type"] == "beast_tide") {
                        // 妖兽潮：阵法消耗 +50%
                        total_cost = to_int(to_float(total_cost) * 1.5);
                }
        }

        if (!player_pay(who, total_cost))
                return -1;

        add_consumption(EC_SINK_FORMATION, total_cost, realm);
        return total_cost;
}

// 装备维修费用计算
// 参数: who - 玩家对象, equip - 装备对象
// 返回: 维修费用（下品灵石），-1 表示装备已满耐久无需修或灵石不足
int equipment_repair_cost(object who, object equip)
{
        string equip_type;
        int base_price, repair_cost, dura, max_dura;
        string realm;

        if (!who || !equip) return -1;

        // 获取装备属性
        base_price = equip->query("value");         // 购买价
        if (base_price <= 0) base_price = 100;      // 容错

        dura = equip->query("durability");
        max_dura = equip->query("max_durability");
        if (max_dura <= 0) max_dura = 100;

        // 无需维修
        if (dura >= max_dura) return -1;

        // 根据装备类型确定维修费率
        string item_type = equip->query("material_type");
        if (!stringp(item_type)) item_type = "weapon";

        if (member_array(item_type, ({ "weapon", "armor", "cloth" })) != -1)
                repair_cost = to_int(to_float(base_price) * EQUIP_REPAIR_RATIO_MID / 100.0);
        else
                repair_cost = to_int(to_float(base_price) * EQUIP_REPAIR_RATIO_LOW / 100.0);

        if (repair_cost <= 0) repair_cost = 1;

        // 按耐久度缺口比例折算
        int dura_gap = max_dura - dura;
        repair_cost = to_int(to_float(repair_cost) * to_float(dura_gap) / to_float(max_dura));
        if (repair_cost <= 0) repair_cost = 1;

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        if (!player_pay(who, repair_cost))
                return -1;

        add_consumption(EC_SINK_EQUIP_REPAIR, repair_cost, realm);
        return repair_cost;
}

// PVP 死亡惩罚：败者损失携带灵石
// 参数: who - 败方玩家对象
// 返回: 损失的下品灵石数量，0 表示未损失
int pvp_death_penalty(object who)
{
        int carry, loss;
        string realm;

        if (!who) return 0;

        // 计算败方携带的灵石总量（通过身上钱币对象估算）
        carry = 0;
        object cash_ob = present("cash_money", who);
        if (cash_ob) carry += cash_ob->query_amount() * 100000;

        object g_ob = present("gold_money", who);
        if (g_ob) carry += g_ob->query_amount() * 10000;

        object s_ob = present("silver_money", who);
        if (s_ob) carry += s_ob->query_amount() * 100;

        object c_ob = present("coin_money", who);
        if (c_ob) carry += c_ob->query_amount();

        // 按 10-30% 扣减
        int loss_pct = PVP_DEATH_LOSS_MIN +
                       random(PVP_DEATH_LOSS_MAX - PVP_DEATH_LOSS_MIN + 1);
        loss = carry * loss_pct / 100;

        if (loss <= 0) return 0;

        // 扣减玩家灵石
        if (!player_pay(who, loss))
                return 0;

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        add_consumption(EC_SINK_PVP_DEATH, loss, realm);
        return loss;
}

// 比武报名费
// 参数: who - 玩家对象, rank_index - 段位索引(0-6)
// 返回: 报名费（下品灵石），-1 表示灵石不足
int arena_entry_fee(object who, int rank_index)
{
        int fee;
        string realm;

        if (!who) return -1;
        if (rank_index < 0 || rank_index > 6)
                rank_index = 0;

        fee = ARENA_FEE_RANK[rank_index];
        if (fee <= 0) fee = 50;

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        if (!player_pay(who, fee))
                return -1;

        add_consumption(EC_SINK_ARENA_FEE, fee, realm);
        return fee;
}

// ---------- A3. 经济→区域循环 ----------

// 传送阵费用计算
// 参数: who - 玩家对象, dist_level - 传送距离级别(1-4)
// 返回: 传送费用（下品灵石），-1 表示灵石不足
int teleport_fee(object who, int dist_level)
{
        int base_cost, final_cost;
        string realm;
        float coeff;

        if (!who) return -1;

        // 根据距离级别确定基础费用
        switch (dist_level) {
        case 1:  base_cost = TELEPORT_COST_CITY;       break;
        case 2:  base_cost = TELEPORT_COST_INTERCITY;   break;
        case 3:  base_cost = TELEPORT_COST_CROSSBORDER; break;
        case 4:  base_cost = TELEPORT_COST_CROSSCONT;   break;
        default: base_cost = TELEPORT_COST_CITY;        break;
        }

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";
        coeff = query_realm_coefficient(realm);

        // 境界越高传送成本越高（高阶修士使用高端传送阵）
        // 炼气×0.5，筑基×1，结丹×2，元婴×5，化神+×10
        float realm_fee_coeff;
        switch (realm) {
        case "qige":     realm_fee_coeff = 0.5; break;
        case "zhuji":    realm_fee_coeff = 1.0; break;
        case "jiedan":   realm_fee_coeff = 2.0; break;
        case "yuanying": realm_fee_coeff = 5.0; break;
        default:         realm_fee_coeff = 10.0; break;
        }

        final_cost = to_int(to_float(base_cost) * realm_fee_coeff);
        if (final_cost <= 0) final_cost = 1;

        if (!player_pay(who, final_cost))
                return -1;

        add_consumption(EC_SINK_TELEPORT, final_cost, realm);
        return final_cost;
}

// 秘境门票费用
// 参数: who - 玩家对象, realm_level - 秘境等级(1-4)
// 返回: 门票费用（下品灵石），-1 表示灵石不足
int secret_realm_entry(object who, int realm_level)
{
        int fee;
        string realm;

        if (!who) return -1;

        switch (realm_level) {
        case 1:  fee = SECRET_REALM_FEE_LOW;  break;
        case 2:  fee = SECRET_REALM_FEE_MID;  break;
        case 3:  fee = SECRET_REALM_FEE_HIGH; break;
        case 4:  fee = SECRET_REALM_FEE_TOP;  break;
        default: fee = SECRET_REALM_FEE_LOW;  break;
        }

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        if (!player_pay(who, fee))
                return -1;

        add_consumption(EC_SINK_SECRET_REALM, fee, realm);
        return fee;
}

// 洞府维护费计算
// 参数: who - 玩家对象, mansion_level - 洞府等级(1-4)
// 返回: 维护费（下品灵石），-1 表示灵石不足
int mansion_upkeep_fee(object who, int mansion_level)
{
        int upkeep;
        string realm;

        if (!who) return -1;

        switch (mansion_level) {
        case 1:  upkeep = MANSION_UPKEEP_LV1;  break;
        case 2:  upkeep = MANSION_UPKEEP_LV2;  break;
        case 3:  upkeep = MANSION_UPKEEP_LV3;  break;
        case 4:  upkeep = MANSION_UPKEEP_LV4;  break;
        default: upkeep = MANSION_UPKEEP_LV1;  break;
        }

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        if (!player_pay(who, upkeep))
                return -1;

        add_consumption(EC_SINK_MANSION_UPKEEP, upkeep, realm);
        return upkeep;
}

// 宗门驻地维护费计算
// 参数: sect_type - 宗门类型("small","medium","large","legendary")
// 返回: 维护费（下品灵石）
int sect_hq_upkeep_fee(string sect_type)
{
        int upkeep;

        if (!stringp(sect_type)) sect_type = "small";

        switch (sect_type) {
        case "small":    upkeep = SECT_HQ_UPKEEP_SMALL;    break;
        case "medium":   upkeep = SECT_HQ_UPKEEP_MEDIUM;   break;
        case "large":    upkeep = SECT_HQ_UPKEEP_LARGE;    break;
        case "legendary":upkeep = SECT_HQ_UPKEEP_LEGENDARY;break;
        default:         upkeep = SECT_HQ_UPKEEP_SMALL;    break;
        }

        // 宗门维护默认为系统回收，不挂钩单个玩家
        // 但需要记录消耗用于通胀监控
        add_consumption(EC_SINK_SECT_HQ_UPKEEP, upkeep, "sect");
        return upkeep;
}

// ---------- A4. 经济→任务循环 ----------

// 任务奖励灵石发放
// 参数: who - 玩家对象, quest_type - 任务类型("daily","weekly","main","branch"),
//       quality - 品质(1=普通, 2=优秀, 3=稀有)
// 返回: 发放的下品灵石数量
int quest_reward_ling_shi(object who, string quest_type, int quality)
{
        int base, final;
        string realm;
        float quality_mod, realm_mod;

        if (!who) return 0;

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        // 按境界确定基础奖励
        switch (realm) {
        case "qige":     base = QUEST_REWARD_BASE_QI;   break;
        case "zhuji":    base = QUEST_REWARD_BASE_ZHU;  break;
        case "jiedan":   base = QUEST_REWARD_BASE_JIE;  break;
        case "yuanying": base = QUEST_REWARD_BASE_YING; break;
        default:         base = QUEST_REWARD_BASE_HUA;  break;
        }

        // 品质倍率
        switch (quality) {
        case 1:  quality_mod = QUEST_QUALITY_NORMAL; break;
        case 2:  quality_mod = QUEST_QUALITY_GOOD;   break;
        case 3:  quality_mod = QUEST_QUALITY_RARE;   break;
        default: quality_mod = QUEST_QUALITY_NORMAL; break;
        }

        // 境界产出系数
        realm_mod = query_realm_coefficient(realm);

        // 最终奖励 = 基础 × 品质倍率 × 境界系数
        final = to_int(to_float(base) * quality_mod * realm_mod);
        if (final <= 0) final = 1;

        // 应用通胀监控的产出增益系数
        if (find_object(INFLATION_D)) {
                float boost = INFLATION_D->query_output_boost();
                final = to_int(to_float(final) * boost);
        }

        // 发放灵石
        pay_player(who, final);

        // 记录产出
        add_production(EC_SOURCE_QUEST_REWARD, final, realm);

        return final;
}

// 活跃度奖励（每日在线时长奖励）
// 参数: who - 玩家对象, online_minutes - 当日在线分钟数
// 返回: 发放的下品灵石数量
int activity_reward(object who, int online_minutes)
{
        int reward;
        string realm;
        float activity_mod;

        if (!who || online_minutes <= 0) return 0;

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        // 按在线时长确定活跃度修正
        if (online_minutes < 30)
                activity_mod = ACTIVITY_MOD_LOW;
        else if (online_minutes < 120)
                activity_mod = ACTIVITY_MOD_MEDIUM;
        else if (online_minutes < 240)
                activity_mod = ACTIVITY_MOD_STANDARD;
        else
                activity_mod = ACTIVITY_MOD_HIGH;

        // 活跃度奖励 = 境界基准 × 活跃度修正
        int base;
        switch (realm) {
        case "qige":     base = QUEST_REWARD_BASE_QI;   break;
        case "zhuji":    base = QUEST_REWARD_BASE_ZHU;  break;
        case "jiedan":   base = QUEST_REWARD_BASE_JIE;  break;
        default:         base = QUEST_REWARD_BASE_QI;   break;
        }
        reward = to_int(to_float(base) * activity_mod);
        if (reward <= 0) reward = 1;

        pay_player(who, reward);
        add_production(EC_SOURCE_ACTIVITY, reward, realm);

        return reward;
}

// 任务失败惩罚
// 参数: who - 玩家对象, quest_reward - 该任务原本的灵石奖励
// 返回: 扣减的灵石数量，0 表示未扣减
int quest_fail_penalty(object who, int quest_reward)
{
        int penalty;
        string realm;

        if (!who || quest_reward <= 0) return 0;

        penalty = quest_reward * QUEST_FAIL_PENALTY_RATIO / 100;
        if (penalty <= 0) penalty = 1;

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        if (!player_pay(who, penalty))
                return 0;

        add_consumption(EC_SINK_QUEST_PENALTY, penalty, realm);
        return penalty;
}

// ---------- A5. 经济→声望循环 ----------

// 势力任务奖励灵石发放
// 参数: who - 玩家对象, reput_level - 声望等级索引
//       (-3=死敌, -2=敌对, -1=冷淡, 0=中立, 1=友善, 2=信任, 3=尊敬, 4=崇拜, 5=传说)
// 返回: 发放的下品灵石数量
int faction_task_reward(object who, int reput_level)
{
        int base, final;
        string realm;

        if (!who) return 0;

        // 按声望等级确定基础奖励
        switch (reput_level) {
        case -3: case -2: case -1:
        case 0:  base = FACTION_TASK_REWARD_NEUTRAL;   break;
        case 1:  base = FACTION_TASK_REWARD_FRIENDLY;   break;
        case 2:  base = FACTION_TASK_REWARD_TRUST;      break;
        case 3:  base = FACTION_TASK_REWARD_RESPECT;    break;
        case 4:  base = FACTION_TASK_REWARD_ADORE;      break;
        case 5:  base = FACTION_TASK_REWARD_LEGENDARY;  break;
        default: base = FACTION_TASK_REWARD_NEUTRAL;    break;
        }

        realm = who->query("realm");
        if (!stringp(realm)) realm = "qige";

        float realm_mod = query_realm_coefficient(realm);
        final = to_int(to_float(base) * realm_mod);
        if (final <= 0) final = 1;

        pay_player(who, final);
        add_production(EC_SOURCE_FACTION_TASK, final, realm);

        return final;
}

// 声望等级消费门槛（返回折扣倍率）
// 参数: reput_level - 声望等级索引
// 返回: 折扣倍率（1.0=原价, 0.9=9折等），返回值 ≤ 0 表示不可交易
float prestige_spending_discount(int reput_level)
{
        switch (reput_level) {
        case -3:                    return REPUT_DISCOUNT_DEADLY;   // 死敌，不可交易
        case -2:                    return REPUT_DISCOUNT_HOSTILE;  // 敌对，3倍价
        case -1:                    return REPUT_DISCOUNT_COLD;     // 冷淡，2倍价
        case 0:                     return REPUT_DISCOUNT_NEUTRAL;  // 中立，原价
        case 1:                     return REPUT_DISCOUNT_FRIENDLY; // 友善，95折
        case 2:                     return REPUT_DISCOUNT_TRUST;    // 信任，9折
        case 3:                     return REPUT_DISCOUNT_RESPECT;  // 尊敬，8折
        case 4:                     return REPUT_DISCOUNT_ADORE;    // 崇拜，6折
        case 5:                     return REPUT_DISCOUNT_LEGENDARY;// 传说，5折
        default:                    return REPUT_DISCOUNT_NEUTRAL;  // 默认原价
        }
}

// 势力产业税收（模拟一次税收结算）
// 参数: tax_type - 税收类型("market","mine","auction")
//       transaction_amount - 交易额（下品灵石）
// 返回: 税收金额（下品灵石，进入系统回收）
int faction_industry_tax(string tax_type, int transaction_amount)
{
        int tax;

        if (transaction_amount <= 0) return 0;

        switch (tax_type) {
        case "market":  tax = transaction_amount * FACTION_TAX_RATE_BASIC / 100;   break;
        case "mine":    tax = transaction_amount * FACTION_TAX_RATE_MINE / 100;    break;
        case "auction": tax = transaction_amount * FACTION_TAX_RATE_AUCTION / 100; break;
        default:        tax = transaction_amount * FACTION_TAX_RATE_BASIC / 100;   break;
        }

        if (tax <= 0) tax = 1;

        // 产业税收计入系统回收
        add_consumption(EC_SOURCE_INDUSTRY_TAX, tax, "sect");
        return tax;
}

// ---------- A6. 经济自身循环 ----------

// 市场交易税计算（含通胀修正）
// 参数: who - 买家对象, transaction_amount - 交易额（下品灵石）
// 返回: 实收税费（下品灵石）
int market_transaction_tax(object who, int transaction_amount)
{
        int tax_rate;
        int tax;

        if (transaction_amount <= 0) return 0;

        // 从通胀监控获取当前税率
        if (find_object(INFLATION_D))
                tax_rate = INFLATION_D->query_tax_rate();
        else
                tax_rate = MARKET_TAX_BASE;

        tax = transaction_amount * tax_rate / 1000;
        if (tax <= 0) tax = 1;

        // 直接扣减（已包含在交易流程中，此处为记账）
        string realm = "market";

        // 记录消耗（系统回收）
        add_consumption(EC_SINK_MARKET_TAX, tax, realm);

        // 更新通胀监控的税收统计
        if (find_object(INFLATION_D))
                INFLATION_D->on_tax_collected(tax);

        return tax;
}

// 拍卖行手续费
// 参数: who - 卖家/买家对象, final_price - 成交价（下品灵石）
// 返回: 手续费（下品灵石）
int auction_fee(object who, int final_price)
{
        int fee;

        if (final_price <= 0) return 0;

        fee = final_price * AUCTION_FEE_RATE / 100;
        if (fee <= 0) fee = 1;

        if (who && !player_pay(who, fee))
                return 0;

        string realm = "auction";
        add_consumption(EC_SINK_AUCTION_FEE, fee, realm);

        if (find_object(INFLATION_D))
                INFLATION_D->on_tax_collected(fee);

        return fee;
}

// 系统回收价（玩家卖给系统商店的价格）
// 参数: market_price - 市场价（下品灵石）
// 返回: 系统回收价（下品灵石）
int system_buyback_price(int market_price)
{
        if (market_price <= 0) return 0;
        return market_price * SYSTEM_BUYBACK_RATIO / 100;
}

// 查询六大循环的最近消耗统计
// 返回: ([ sink_type : total_amount, ... ])
mapping query_circulation_stats()
{
        mapping stats = ([]);
        int now = time();
        int cutoff = now - 86400;   // 最近24小时

        if (!mapp(consumption_log)) return stats;

        foreach (int ts, int amount in consumption_log) {
                if (ts < cutoff) continue;
                // 按消耗类型分组汇总
                // 简化实现：遍历所有消耗记录
        }

        return stats;
}

// 获取指定循环类型的总消耗（最近24h）
int query_circulation_consumption(string sink_type)
{
        int total = 0;
        int now = time();
        int cutoff = now - 86400;

        if (!mapp(consumption_log)) return 0;

        // 注意：production_log/consumption_log 的 key 是时间戳而非类型
        // 此接口为扩展预留——完整的按类型过滤需改造 log 数据结构
        return total;
}

