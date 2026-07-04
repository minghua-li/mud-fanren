// moneyd.c  钱的功能 + 灵石产出总控
// by Xiang@XKX (95/12/22)
// 灵石经济扩展 by BCubed 团队 (#19)

// 境界产出系数（基准）
#define REALM_COEFF_QIGE     1.0    // 炼气
#define REALM_COEFF_ZHUJI    3.0    // 筑基
#define REALM_COEFF_JIEDAN   10.0   // 结丹
#define REALM_COEFF_YUANYING 30.0   // 元婴
#define REALM_COEFF_HUASHEN  80.0   // 化神+

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
        // 同时通知通胀监控
        if (find_object(INFLATION_D))
                INFLATION_D->check_economy_health();
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

