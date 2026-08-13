// root_refine_d.c
// 灵根洗练与突破系统守护进程
// 对应设计文档：02-扩充内容/02-灵根养成与突破.md
//
// 功能：
//   1. 灵根经验成长与等级提升（修炼/战斗获得经验）
//   2. 灵根洗练（消耗资源重新洗练属性/品质）
//   3. 灵根品质提升（伪→假→真→变→天）
//   4. 境界突破系统（概率计算、事件触发、失败惩罚）
//   5. 灵根 debuff 机制（震荡、不稳、损伤、封闭）
//   6. 突破任务链管理

#include <ansi.h>
#include <spirit_root.h>

inherit F_DBASE;

// 函数前向声明
void create();

// 公开接口
// ----- 基础查询 -----
int query_spirit_root_quality(object ob);
int query_spirit_root_strength(object ob);
int query_spirit_root_purity(object ob);
string *query_spirit_root_elements(object ob);
string query_spirit_root_main_element(object ob);
int query_spirit_root_exp(object ob);
int query_spirit_root_level(object ob);
int query_spirit_root_break_count(object ob);
int query_spirit_root_refine_count(object ob);
int query_spirit_root_fail_streak(object ob);
mapping query_spirit_root_debuff(object ob);
string query_spirit_root_quality_name(object ob);
string query_spirit_root_quality_stars(object ob);

// ----- 成长系统 -----
int add_spirit_root_exp(object ob, int exp);
int gain_exp_from_cultivation(object ob, int base_exp);
int gain_exp_from_combat(object ob, int combat_exp);
int check_spirit_root_level_up(object ob);
int level_up_spirit_root(object ob);

// ----- 洗练系统 -----
int do_refine(object ob, int method);
int refine_cost_check(object ob, int method);
int refine_cost_pay(object ob, int method);
mapping refine_calculate_result(object ob, int method);
int refine_apply_result(object ob, mapping result);
int refine_trigger_special_event(object ob, int method);

// ----- 品质提升 -----
int check_quality_upgrade(object ob);
int do_quality_upgrade(object ob, string item_type);
int quality_upgrade_cost_check(object ob, string item_type);
int quality_upgrade_cost_pay(object ob, string item_type);
int query_can_upgrade_quality(object ob);
int validate_quality_upgrade_path(object ob);

// ----- 突破系统 -----
int query_breakthrough_probability(object ob, int method);
int do_breakthrough(object ob, int method);
int breakthrough_event_trigger(object ob, int method);
int breakthrough_success(object ob, int method);
int breakthrough_failure(object ob, int method);
mapping query_breakthrough_details(object ob, int method);
int query_pseudo_core_limit_reached(object ob);

// ----- Debuff 系统 -----
int apply_root_debuff(object ob, int debuff_type, int duration);
int remove_root_debuff(object ob, int debuff_type);
int clear_all_root_debuffs(object ob);
mapping apply_debuff_effects(object ob);
int query_debuff_effect(object ob, string effect_type);

// ----- 任务链 -----
string *query_breakthrough_task_chain(object ob, int realm);
int check_task_chain_completion(object ob, string chain_id);

// ----- 属性计算（供外部调用） -----
float query_cultivation_speed_factor(object ob);
float query_mana_limit_factor(object ob);
float query_mana_regen_factor(object ob);
float query_magic_damage_bonus(object ob, string element);
float query_element_resistance(object ob, string element);
int query_skill_level_limit(object ob, string skill_name);

// ----- 工具函数 -----
int is_variant_element(string element);
int is_normal_element(string element);
string *get_matching_elements(object ob);
int calculate_refine_cooldown_remaining(object ob, int method);
int random_between(int min, int max);
string *random_elements(int count);

//=============================================================================
// 创建与初始化
//=============================================================================

void create()
{
    seteuid(getuid());
    set("name", "灵根洗练守护进程");
    set("id", "root_refine_d");
}

//=============================================================================
// 基础查询接口
//=============================================================================

int query_spirit_root_quality(object ob)
{
    if (!ob) return SPIRIT_ROOT_NONE;
    return ob->query(ROOT_PROP_QUALITY);
}

int query_spirit_root_strength(object ob)
{
    if (!ob) return 0;
    int strength = ob->query(ROOT_PROP_STRENGTH);
    if (!strength) strength = 0;
    return strength;
}

int query_spirit_root_purity(object ob)
{
    if (!ob) return 0;
    int purity = ob->query(ROOT_PROP_PURITY);
    if (!purity) purity = 50;  // 默认精纯度 50%
    return purity;
}

string *query_spirit_root_elements(object ob)
{
    if (!ob) return ({});
    string *elements = ob->query(ROOT_PROP_ELEMENTS);
    if (!elements) return ({});
    return elements;
}

string query_spirit_root_main_element(object ob)
{
    if (!ob) return 0;
    return ob->query(ROOT_PROP_MAIN_ELEMENT);
}

int query_spirit_root_exp(object ob)
{
    if (!ob) return 0;
    return ob->query(ROOT_PROP_EXP);
}

int query_spirit_root_level(object ob)
{
    if (!ob) return 0;
    int level = ob->query(ROOT_PROP_LEVEL);
    if (!level) level = 1;
    return level;
}

int query_spirit_root_break_count(object ob)
{
    if (!ob) return 0;
    int c = ob->query(ROOT_PROP_BREAK_COUNT);
    if (!c) return 0;
    return c;
}

int query_spirit_root_refine_count(object ob)
{
    if (!ob) return 0;
    int c = ob->query(ROOT_PROP_REFINE_COUNT);
    if (!c) return 0;
    return c;
}

int query_spirit_root_fail_streak(object ob)
{
    if (!ob) return 0;
    int f = ob->query(ROOT_PROP_FAIL_STREAK);
    if (!f) return 0;
    return f;
}

mapping query_spirit_root_debuff(object ob)
{
    if (!ob) return 0;
    return ob->query(ROOT_PROP_DEBUFF);
}

string query_spirit_root_quality_name(object ob)
{
    int quality = query_spirit_root_quality(ob);
    switch (quality)
    {
    case SPIRIT_ROOT_NONE:     return "无灵根";
    case SPIRIT_ROOT_PSEUDO:   return "伪灵根";
    case SPIRIT_ROOT_FAKE:     return "假灵根";
    case SPIRIT_ROOT_TRUE:     return "真灵根";
    case SPIRIT_ROOT_VARIANT:  return "变异灵根";
    case SPIRIT_ROOT_HEAVENLY: return "天灵根";
    default: return "未知";
    }
}

string query_spirit_root_quality_stars(object ob)
{
    int quality = query_spirit_root_quality(ob);
    switch (quality)
    {
    case SPIRIT_ROOT_NONE:     return "☆☆☆☆☆☆";
    case SPIRIT_ROOT_PSEUDO:   return "★☆☆☆☆☆";
    case SPIRIT_ROOT_FAKE:     return "★★☆☆☆☆";
    case SPIRIT_ROOT_TRUE:     return "★★★☆☆☆";
    case SPIRIT_ROOT_VARIANT:  return "★★★★☆☆";
    case SPIRIT_ROOT_HEAVENLY: return "★★★★★★";
    default: return "☆☆☆☆☆☆";
    }
}

//=============================================================================
// 灵根经验成长系统
//=============================================================================

// 灵根升级所需经验公式：level^2 * 100
int exp_for_level(int level)
{
    return level * level * 100;
}

// 增加灵根经验，自动检测是否升级
int add_spirit_root_exp(object ob, int exp)
{
    if (!ob || exp <= 0) return 0;
    if (!userp(ob)) return 0;

    int quality = query_spirit_root_quality(ob);
    if (quality <= SPIRIT_ROOT_NONE) return 0;

    // 品质影响经验获取效率（低品质灵根成长慢，但升级需求也低）
    float quality_factor;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   quality_factor = 0.6;  break;
    case SPIRIT_ROOT_FAKE:     quality_factor = 0.8;  break;
    case SPIRIT_ROOT_TRUE:     quality_factor = 1.0;  break;
    case SPIRIT_ROOT_VARIANT:  quality_factor = 1.1;  break;
    case SPIRIT_ROOT_HEAVENLY: quality_factor = 1.2;  break;
    default: quality_factor = 1.0; break;
    }

    int actual_exp = to_int(exp * quality_factor);
    int current_exp = ob->query(ROOT_PROP_EXP);
    ob->set(ROOT_PROP_EXP, current_exp + actual_exp);

    // 检查是否升级
    if (check_spirit_root_level_up(ob))
    {
        level_up_spirit_root(ob);
        tell_object(ob, HIG "你的灵根有所感悟，灵根等级提升了！\n" NOR);
    }

    return actual_exp;
}

// 修炼获得的灵根经验
int gain_exp_from_cultivation(object ob, int base_exp)
{
    if (!ob || base_exp <= 0) return 0;

    // 灵根经验 = 基础经验 * 灵根速度系数
    float speed_factor = query_cultivation_speed_factor(ob);
    int gained = to_int(base_exp * speed_factor);

    // 精纯度修正：高精纯度额外加成
    int purity = query_spirit_root_purity(ob);
    float purity_bonus = 1.0 + (purity - 50) * 0.002;
    if (purity_bonus < 0.7) purity_bonus = 0.7;
    gained = to_int(gained * purity_bonus);

    if (gained < 1) gained = 1;
    return add_spirit_root_exp(ob, gained);
}

// 战斗获得的灵根经验
int gain_exp_from_combat(object ob, int combat_exp)
{
    if (!ob || combat_exp <= 0) return 0;

    // 战斗获得的灵根经验按比例折算（约为战斗修为的20%）
    int gained = combat_exp / 5;
    if (gained < 1) gained = 1;

    // 元素匹配加成：所修功法属性与灵根属性匹配时额外加成
    // （此处简化为直接调用 add）
    return add_spirit_root_exp(ob, gained);
}

// 检查灵根是否可升级
int check_spirit_root_level_up(object ob)
{
    if (!ob) return 0;

    int current_level = query_spirit_root_level(ob);
    int current_exp = ob->query(ROOT_PROP_EXP);
    int needed = exp_for_level(current_level + 1);

    // 等级上限受灵根品质制约
    int quality = query_spirit_root_quality(ob);
    int max_level;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   max_level = 30;  break;
    case SPIRIT_ROOT_FAKE:     max_level = 50;  break;
    case SPIRIT_ROOT_TRUE:     max_level = 80;  break;
    case SPIRIT_ROOT_VARIANT:  max_level = 95;  break;
    case SPIRIT_ROOT_HEAVENLY: max_level = 100; break;
    default: max_level = 1; break;
    }

    if (current_level >= max_level) return 0;
    return (current_exp >= needed);
}

// 提升灵根等级
int level_up_spirit_root(object ob)
{
    if (!ob) return 0;

    int current_level = query_spirit_root_level(ob);
    int new_level = current_level + 1;

    ob->set(ROOT_PROP_LEVEL, new_level);

    // 每次升级时灵根强度随之增长
    int current_strength = query_spirit_root_strength(ob);
    int strength_gain = 1 + new_level / 10;  // 每10级额外多+1
    int new_strength = current_strength + strength_gain;

    int quality = query_spirit_root_quality(ob);
    int max_strength;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   max_strength = 80;  break;
    case SPIRIT_ROOT_FAKE:     max_strength = 85;  break;
    case SPIRIT_ROOT_TRUE:     max_strength = 95;  break;
    case SPIRIT_ROOT_VARIANT:  max_strength = 100; break;
    case SPIRIT_ROOT_HEAVENLY: max_strength = 100; break;
    default: max_strength = 0; break;
    }
    if (new_strength > max_strength) new_strength = max_strength;
    ob->set(ROOT_PROP_STRENGTH, new_strength);

    // 消耗对应经验
    int cost = exp_for_level(new_level);
    int current_exp = ob->query(ROOT_PROP_EXP);
    ob->set(ROOT_PROP_EXP, current_exp - cost);

    return 1;
}

//=============================================================================
// 洗练系统
//=============================================================================

// 消耗检查
int refine_cost_check(object ob, int method)
{
    if (!ob) return 0;

    switch (method)
    {
    case REFINE_METHOD_NORMAL:
        // 灵石 × 5000
        if (ob->query("balance") < REFINE_COST_BASE_NORMAL)
            return 0;
        return 1;

    case REFINE_METHOD_DEEPER:
        // 灵石 × 10000 + 门派贡献 × 2000
        if (ob->query("balance") < REFINE_COST_BASE_DEEPER)
            return 0;
        if (ob->query("contributions") < REFINE_CONTRIBUTION_DEEPER)
            return 0;
        return 1;

    case REFINE_METHOD_PILL:
        // 洗髓丹 × 3（假设以物品形式存在于背包）
        if (!ob->query_temp("refine_pill_check"))
            return 0;
        return 1;

    case REFINE_METHOD_HERB:
        // 天材地宝（净灵莲等），由调用方检查
        return 1;

    default:
        return 0;
    }
}

// 消耗支付
int refine_cost_pay(object ob, int method)
{
    if (!ob) return 0;
    if (!refine_cost_check(ob, method)) return 0;

    switch (method)
    {
    case REFINE_METHOD_NORMAL:
        ob->add("balance", -REFINE_COST_BASE_NORMAL);
        return 1;

    case REFINE_METHOD_DEEPER:
        ob->add("balance", -REFINE_COST_BASE_DEEPER);
        ob->add("contributions", -REFINE_CONTRIBUTION_DEEPER);
        return 1;

    case REFINE_METHOD_PILL:
        // 调用方扣除物品
        return 1;

    case REFINE_METHOD_HERB:
        // 调用方扣除物品
        return 1;

    default:
        return 0;
    }
}

// 计算洗练结果
mapping refine_calculate_result(object ob, int method)
{
    mapping result = ([]);

    int quality = query_spirit_root_quality(ob);
    if (quality <= SPIRIT_ROOT_NONE) return 0;

    int current_strength = query_spirit_root_strength(ob);
    int max_strength;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   max_strength = 80;  break;
    case SPIRIT_ROOT_FAKE:     max_strength = 85;  break;
    case SPIRIT_ROOT_TRUE:     max_strength = 95;  break;
    case SPIRIT_ROOT_VARIANT:  max_strength = 100; break;
    case SPIRIT_ROOT_HEAVENLY: max_strength = 100; break;
    default: max_strength = 0; break;
    }

    if (current_strength >= max_strength)
    {
        result["result"] = REFINE_RESULT_NOTHING;
        result["strength_gain"] = 0;
        result["purity_gain"] = 0;
        result["msg"] = "你的灵根已臻至当前品质的极致，再难寸进。";
        return result;
    }

    // 基础洗练效果范围
    int min_gain, max_gain, purity_gain;
    int result_type;
    int roll = random(100);

    switch (method)
    {
    case REFINE_METHOD_NORMAL:
        // 强度 +5~15，精纯度 +1%~3%，品质加成
        if (quality == SPIRIT_ROOT_TRUE)      { min_gain = 7; max_gain = 17; }
        else if (quality == SPIRIT_ROOT_HEAVENLY) { min_gain = 10; max_gain = 20; }
        else { min_gain = 5; max_gain = 15; }
        purity_gain = 1 + random(3);
        break;

    case REFINE_METHOD_DEEPER:
        // 强度 +10~20，精纯度 +2%~5%
        min_gain = 10; max_gain = 20;
        purity_gain = 2 + random(4);
        break;

    case REFINE_METHOD_PILL:
        // 强度 +2（单颗），精纯度 +1%
        min_gain = 2; max_gain = 2;
        purity_gain = 1;
        break;

    case REFINE_METHOD_HERB:
        // 天材地宝：大幅提升
        min_gain = 15; max_gain = 30;
        purity_gain = 5 + random(10);
        break;

    default:
        min_gain = 1; max_gain = 5;
        purity_gain = 0;
        break;
    }

    // 结果类型判定
    if (roll < 15)
        result_type = REFINE_RESULT_MINOR;
    else if (roll < 50)
        result_type = REFINE_RESULT_MODERATE;
    else if (roll < 80)
        result_type = REFINE_RESULT_MAJOR;
    else
        result_type = REFINE_RESULT_NOTHING;

    // 丹药协同：浸泡前服用洗髓丹效果 ×1.5
    if (method == REFINE_METHOD_NORMAL && ob->query_temp("refine_pill_synergy"))
    {
        min_gain = to_int(min_gain * 1.5);
        max_gain = to_int(max_gain * 1.5);
        purity_gain = to_int(purity_gain * 1.5);
    }

    int strength_gain = random_between(min_gain, max_gain);
    if (result_type == REFINE_RESULT_MINOR) strength_gain = strength_gain / 2;
    if (strength_gain < 1) strength_gain = 1;

    // 不超过上限
    if (current_strength + strength_gain > max_strength)
        strength_gain = max_strength - current_strength;

    result["result"] = result_type;
    result["strength_gain"] = strength_gain;
    result["purity_gain"] = purity_gain;

    // 10% 概率触发特殊事件
    if (random(100) < 10)
    {
        mapping event = refine_trigger_special_event(ob, method);
        if (event) result["event"] = event;
    }

    return result;
}

// 应用洗练结果
int refine_apply_result(object ob, mapping result)
{
    if (!ob || !mapp(result)) return 0;

    int strength_gain = result["strength_gain"];
    int purity_gain = result["purity_gain"];

    if (strength_gain > 0)
        ob->add(ROOT_PROP_STRENGTH, strength_gain);

    if (purity_gain > 0)
    {
        int current_purity = ob->query(ROOT_PROP_PURITY);
        int new_purity = current_purity + purity_gain;
        if (new_purity > 100) new_purity = 100;
        ob->set(ROOT_PROP_PURITY, new_purity);
    }

    // 记录洗练次数
    ob->add(ROOT_PROP_REFINE_COUNT, 1);

    // 设置冷却时间
    ob->set(ROOT_PROP_LAST_REFINE, time());

    // 特殊事件处理
    if (result["event"])
    {
        mapping ev = result["event"];
        string event_type = ev["event_type"];
        switch (event_type)
        {
        case "tide":
            // 灵气潮汐：当次效果翻倍（已体现在 result 中）
            break;
        case "resonance":
            // 灵脉共鸣：额外 +10 强度
            ob->add(ROOT_PROP_STRENGTH, 10);
            break;
        case "spirit":
            // 洗灵池之灵：临时提升品质观察（不实际修改，标记临时buff）
            ob->set_temp("spirit_root/temp_quality_boost", 1);
            ob->set_temp("spirit_root/temp_boost_expire", time() + 1800); // 30分钟
            break;
        case "undercurrent":
            // 池底暗流：效果减半（已体现在 result 中）
            break;
        case "disturbance":
            // 他人干扰：没有实际效果增益，但这里记录事件
            break;
        }
    }

    return 1;
}

// 特殊事件触发
mapping refine_trigger_special_event(object ob, int method)
{
    mapping event = ([]);
    int roll = random(100);

    // 概率分布根据设计文档
    // 灵气潮汐 - 随机
    if (roll < 10)
    {
        event["event_type"] = "tide";
        event["msg"] = HIC "忽然，洗灵池中灵气翻涌，形成一股灵气潮汐！你的灵根尽情吸收着浓郁的灵气。\n" NOR;
    }
    // 池底暗流
    else if (roll < 20)
    {
        event["event_type"] = "undercurrent";
        event["msg"] = HIB "池底突然涌出一股暗流，打断了你的洗练进程！\n" NOR;
        event["penalty"] = "half_effect";
    }
    // 灵脉共鸣（需要身处高阶灵脉区域，检查临时标记）
    else if (roll < 25 && ob->query_temp("spirit_root/high_grade_vein"))
    {
        event["event_type"] = "resonance";
        event["msg"] = HIM "你感受到脚下的灵脉与洗灵池产生共鸣，灵力疯狂涌入你的体内！\n" NOR;
    }
    // 他人干扰（有一定随机性）
    else if (roll < 30)
    {
        event["event_type"] = "disturbance";
        event["msg"] = HIY "洗灵池外传来喧哗声，有人试图闯入！你的洗练受到干扰。\n" NOR;
    }
    // 洗灵池之灵（极稀有 1%）
    else if (roll < 31)
    {
        event["event_type"] = "spirit";
        event["msg"] = HIW "一道灵光自池底升起，洗灵池之灵显化！你的灵根得到了一次升华的契机！\n" NOR;
    }
    else
    {
        return 0;  // 无事件
    }

    return event;
}

// 冷却时间检查
int calculate_refine_cooldown_remaining(object ob, int method)
{
    if (!ob) return 0;

    int last_refine = ob->query(ROOT_PROP_LAST_REFINE);
    if (!last_refine) return 0;

    int cooldown_days;
    switch (method)
    {
    case REFINE_METHOD_NORMAL: cooldown_days = REFINE_COOLDOWN_NORMAL; break;
    case REFINE_METHOD_DEEPER: cooldown_days = REFINE_COOLDOWN_DEEPER; break;
    default: cooldown_days = 0; break;
    }

    // 游戏时间换算（假设 1 现实天 = 5 游戏天）
    // 简化处理：用现实时间秒数 × 系数模拟游戏时间
    // 实际应由游戏时间系统提供
    int game_time_elapsed = (time() - last_refine) * 5;  // 假设计算
    int cd_seconds = cooldown_days * 86400 / 5;  // 转换为现实秒数

    if (game_time_elapsed >= cd_seconds) return 0;
    return cd_seconds - (time() - last_refine);
}

//=============================================================================
// 品质提升系统
//=============================================================================

// 检查是否能提升品质
int query_can_upgrade_quality(object ob)
{
    if (!ob) return 0;
    int quality = query_spirit_root_quality(ob);
    // 最高只能提升到真灵根（后天上限）
    if (quality >= SPIRIT_ROOT_ACQUIRED_MAX) return 0;
    if (quality <= SPIRIT_ROOT_NONE) return 0;
    return 1;
}

// 验证提升路径是否合法
int validate_quality_upgrade_path(object ob)
{
    if (!ob) return 0;
    int quality = query_spirit_root_quality(ob);

    // 品质提升路径：
    // 伪灵根 → 假灵根 → 真灵根
    // 真灵根 → 变异灵根（极低概率，<5%）或 天灵根
    if (quality < SPIRIT_ROOT_PSEUDO || quality > SPIRIT_ROOT_HEAVENLY)
        return 0;
    if (quality >= SPIRIT_ROOT_VARIANT) return 0;  // 变异及以上不可后天提升
    return 1;
}

// 品质提升消耗检查
int quality_upgrade_cost_check(object ob, string item_type)
{
    if (!ob) return 0;

    if (item_type == "净灵莲")
    {
        // 净灵莲消耗（终生1次，上限T2）
        if (ob->query("spirit_root/used_jinglinglian"))
            return 0;
        // 净灵莲物品检查——由外部调用方确保
        return 1;
    }
    else if (item_type == "补天丹")
    {
        // 补天丹消耗（终生1次）
        if (ob->query("spirit_root/used_butiandan"))
            return 0;
        // 补天丹物品检查——由外部调用方确保
        return 1;
    }

    return 0;
}

// 品质提升消耗支付
int quality_upgrade_cost_pay(object ob, string item_type)
{
    if (!ob) return 0;
    if (!quality_upgrade_cost_check(ob, item_type)) return 0;

    if (item_type == "净灵莲")
    {
        ob->set("spirit_root/used_jinglinglian", 1);
        return 1;
    }
    else if (item_type == "补天丹")
    {
        ob->set("spirit_root/used_butiandan", 1);
        return 1;
    }

    return 0;
}

// 执行品质提升
int do_quality_upgrade(object ob, string item_type)
{
    if (!ob) return 0;
    if (!validate_quality_upgrade_path(ob)) return 0;
    if (!quality_upgrade_cost_check(ob, item_type)) return 0;

    int current_quality = query_spirit_root_quality(ob);
    int target_quality;

    if (item_type == "净灵莲")
    {
        // 净灵莲：提升1档，上限真灵根
        target_quality = current_quality + 1;
        if (target_quality > SPIRIT_ROOT_TRUE)
            target_quality = SPIRIT_ROOT_TRUE;
    }
    else if (item_type == "补天丹")
    {
        // 补天丹：提升1档
        // 真灵根 → 变异/天灵根（极低概率 <5%）
        target_quality = current_quality + 1;

        // 如果当前是真灵根，有极低概率突破到变异或天灵根
        if (current_quality == SPIRIT_ROOT_TRUE)
        {
            if (random(100) < 5)
            {
                // 5% 概率突破上限
                if (random(2) == 0)
                    target_quality = SPIRIT_ROOT_VARIANT;
                else
                    target_quality = SPIRIT_ROOT_HEAVENLY;
            }
            else
            {
                // 95% 保持真灵根，但提升强度和精纯度作为补偿
                target_quality = SPIRIT_ROOT_TRUE;
            }
        }
    }
    else
    {
        return 0;
    }

    // 支付消耗
    if (!quality_upgrade_cost_pay(ob, item_type)) return 0;

    // 如果品质实际提升了
    if (target_quality > current_quality)
    {
        ob->set(ROOT_PROP_QUALITY, target_quality);
        ob->set(ROOT_PROP_QUALITY_SOURCE, "acquired");

        // 品质提升带来的补偿增益
        ob->add(ROOT_PROP_STRENGTH, 10);  // 强度+10
        int purity = ob->query(ROOT_PROP_PURITY);
        purity += 10;
        if (purity > 100) purity = 100;
        ob->set(ROOT_PROP_PURITY, purity);

        // 变异灵根特殊处理：随机分配变异属性
        if (target_quality == SPIRIT_ROOT_VARIANT)
        {
            string *variants = SPIRIT_ELEMENT_VARIANTS;
            string chosen = variants[random(sizeof(variants))];
            ob->set(ROOT_PROP_MAIN_ELEMENT, chosen);
            ob->set(ROOT_PROP_ELEMENTS, ({ chosen }));
        }
        // 天灵根：随机选择一种主属性
        else if (target_quality == SPIRIT_ROOT_HEAVENLY)
        {
            string *normal = SPIRIT_ELEMENT_ALL;
            string chosen = normal[random(sizeof(normal))];
            ob->set(ROOT_PROP_MAIN_ELEMENT, chosen);
            ob->set(ROOT_PROP_ELEMENTS, ({ chosen }));
        }
    }
    else
    {
        // 品质未提升（真灵根使用补天丹未突破），补偿：
        ob->add(ROOT_PROP_STRENGTH, 20);
        int purity = ob->query(ROOT_PROP_PURITY);
        purity += 15;
        if (purity > 100) purity = 100;
        ob->set(ROOT_PROP_PURITY, purity);
        tell_object(ob, "补天丹药力融入你的灵根，虽未突破品质上限，但灵根变得更加精纯了。\n");
    }

    return 1;
}

// 检查可用的品质提升途径（给玩家显示）
int check_quality_upgrade(object ob)
{
    if (!ob) return 0;
    int quality = query_spirit_root_quality(ob);

    if (quality >= SPIRIT_ROOT_TRUE)
    {
        tell_object(ob, "你的灵根品质已臻至真灵根，是后天养成的极限了。\n");
        return 0;
    }

    // 检查是否使用过净灵莲
    if (!ob->query("spirit_root/used_jinglinglian") && quality < SPIRIT_ROOT_FAKE)
    {
        tell_object(ob, "若能得到一株净灵莲，或许能让你的灵根品质更上一层楼。\n");
    }
    else if (quality == SPIRIT_ROOT_FAKE && !ob->query("spirit_root/used_butiandan"))
    {
        tell_object(ob, "若是传说中的补天丹，或许能助你的灵根再做突破。\n");
    }

    return 1;
}

//=============================================================================
// 突破系统
//=============================================================================

// 计算当前灵根强度系数（灵根强度/100，平滑增益）
float query_strength_coefficient(object ob)
{
    int strength = query_spirit_root_strength(ob);
    return strength / 100.0;
}

// 查询小境界突破概率
// 小境界：功法层数突破
int query_breakthrough_probability(object ob, int method)
{
    if (!ob) return 0;

    int quality = query_spirit_root_quality(ob);
    // 天灵根小境界无瓶颈（自动突破）
    if (quality == SPIRIT_ROOT_HEAVENLY && method == BREAK_METHOD_NATURAL)
        return 100;

    // 基础成功率由方法决定
    int base_rate;
    switch (method)
    {
    case BREAK_METHOD_NATURAL:      base_rate = 35; break;
    case BREAK_METHOD_PILL_AID:     base_rate = 50; break;
    case BREAK_METHOD_SPIRIT_STONE: base_rate = 45; break;
    case BREAK_METHOD_TREASURE:     base_rate = 70; break;
    case BREAK_METHOD_SECRET_REALM: base_rate = 60; break;
    default: base_rate = 35; break;
    }

    // 灵根强度系数（0~1.0）
    float strength_coeff = query_strength_coefficient(ob);
    int prob = to_int(base_rate * strength_coeff);

    // 丹药修正（调用方传递的临时加成）
    int pill_bonus = ob->query_temp("breakthrough/pill_bonus");
    prob += pill_bonus;

    // 功法修正（匹配属性功法加成）
    int skill_bonus = ob->query_temp("breakthrough/skill_bonus");
    prob += skill_bonus;

    // 连续失败保底
    int fail_streak = query_spirit_root_fail_streak(ob);
    if (fail_streak >= BREAK_STREAK_FAIL_THRESHOLD)
    {
        int extra = (fail_streak - BREAK_STREAK_FAIL_THRESHOLD + 1) * BREAK_STREAK_FAIL_BONUS;
        if (extra > BREAK_STREAK_FAIL_BONUS * 3)
            extra = BREAK_STREAK_FAIL_BONUS * 3;  // 上限
        prob += extra;
    }

    // debuff 修正（灵根不稳等）
    mapping debuff = query_spirit_root_debuff(ob);
    if (mapp(debuff) && debuff["unstable"])
        prob -= 10;

    // 上限/下限保护
    if (prob < 5) prob = 5;
    if (prob > 95) prob = 95;

    return prob;
}

// 大境界突破概率
int query_major_breakthrough_probability(object ob, int realm)
{
    if (!ob) return 0;

    int quality = query_spirit_root_quality(ob);

    // 天灵根特殊规则
    if (quality == SPIRIT_ROOT_HEAVENLY)
    {
        // 天灵根自动结丹（无瓶颈）
        if (realm == REALM_CORE_FORMATION)
            return 100;
    }

    // 境界基础概率
    int base_realm_rate;
    switch (realm)
    {
    case REALM_QI_REFINERY:      base_realm_rate = 90;  break;  // 凡人→炼气（几乎必成）
    case REALM_FOUNDATION:       base_realm_rate = 10;  break;  // 炼气→筑基
    case REALM_CORE_FORMATION:   base_realm_rate = 5;   break;  // 筑基→结丹
    case REALM_NASCENT_SOUL:     base_realm_rate = 3;   break;  // 结丹→元婴
    case REALM_SPIRIT_TRANSFORM: base_realm_rate = 1;   break;  // 元婴→化神
    default: base_realm_rate = 5; break;
    }

    // 灵根品质系数
    float quality_factor;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   quality_factor = 0.3;  break;
    case SPIRIT_ROOT_FAKE:     quality_factor = 0.7;  break;
    case SPIRIT_ROOT_TRUE:     quality_factor = 1.0;  break;
    case SPIRIT_ROOT_VARIANT:  quality_factor = 1.2;  break;
    case SPIRIT_ROOT_HEAVENLY: quality_factor = 5.0;  break;
    default: quality_factor = 0.0; break;
    }

    // 伪灵根/假灵根的大境界额外惩罚
    int realm_penalty = 0;
    if (quality == SPIRIT_ROOT_PSEUDO)
        realm_penalty = -15 * (realm - REALM_FOUNDATION);
    else if (quality == SPIRIT_ROOT_FAKE)
        realm_penalty = -5 * (realm - REALM_FOUNDATION);

    if (realm_penalty < -50) realm_penalty = -50;

    int prob = to_int(base_realm_rate * quality_factor) + realm_penalty;

    // 辅助修正（丹药+功法+环境+护法）
    int aux_bonus = ob->query_temp("breakthrough/aux_bonus");
    prob += aux_bonus;

    // 丹药辅助修正（#73 接线：突破辅助丹如筑基丹/结金丹服用后写入
    // breakthrough/pill_bonus，与 #61 预留的读取端对接）
    int pill_bonus = ob->query_temp("breakthrough/pill_bonus");
    prob += pill_bonus;

    // 伪灵根结丹上限
    if (quality == SPIRIT_ROOT_PSEUDO && realm == REALM_CORE_FORMATION)
    {
        if (prob > PSEUDO_MAX_BREAK_RATE)
            prob = PSEUDO_MAX_BREAK_RATE;
    }

    if (prob < 1) prob = 1;
    if (prob > 99) prob = 99;

    return prob;
}

// 突破详情查询（给玩家展示用）
mapping query_breakthrough_details(object ob, int method)
{
    mapping details = ([]);

    details["method"] = method;
    details["base_prob"] = query_breakthrough_probability(ob, method);
    details["strength"] = query_spirit_root_strength(ob);
    details["purity"] = query_spirit_root_purity(ob);
    details["quality"] = query_spirit_root_quality(ob);
    details["fail_streak"] = query_spirit_root_fail_streak(ob);

    // 连续失败保底信息
    if (details["fail_streak"] >= BREAK_STREAK_FAIL_THRESHOLD)
    {
        int bonus = (details["fail_streak"] - BREAK_STREAK_FAIL_THRESHOLD + 1) * BREAK_STREAK_FAIL_BONUS;
        if (bonus > BREAK_STREAK_FAIL_BONUS * 3)
            bonus = BREAK_STREAK_FAIL_BONUS * 3;
        details["streak_bonus"] = bonus;
    }

    // debuff 检查
    mapping debuff = query_spirit_root_debuff(ob);
    if (mapp(debuff))
    {
        if (debuff["unstable"])
            details["unstable_penalty"] = -10;
    }

    return details;
}

// 执行突破
int do_breakthrough(object ob, int method)
{
    if (!ob) return 0;

    // 获取突破概率
    int prob = query_breakthrough_probability(ob, method);

    // 15% 概率触发随机事件分支
    int event_triggered = 0;
    if (random(100) < 15)
        event_triggered = breakthrough_event_trigger(ob, method);

    if (event_triggered)
    {
        // 事件中可能修改概率或直接决定结果
        // 在天降机缘事件中直接成功
        if (ob->query_temp("breakthrough/auto_success"))
        {
            ob->delete_temp("breakthrough/auto_success");
            return breakthrough_success(ob, method);
        }
        // 心魔入侵失败则必然失败
        if (ob->query_temp("breakthrough/auto_fail"))
        {
            ob->delete_temp("breakthrough/auto_fail");
            return breakthrough_failure(ob, method);
        }
        // 其他事件可能修改概率，重新计算
        prob = query_breakthrough_probability(ob, method);
    }

    // 随机判定
    int roll = random(100);
    if (roll < prob)
        return breakthrough_success(ob, method);
    else
        return breakthrough_failure(ob, method);
}

// 突破事件触发
int breakthrough_event_trigger(object ob, int method)
{
    int roll = random(100);

    // 心魔入侵 5%
    if (roll < BREAK_EVENT_HEART_DEVIL)
    {
        tell_object(ob, HIR "心魔骤起！你的内心深处的恐惧和欲望化为实体，试图吞噬你的意志！\n" NOR);
        // 追加心魔战斗判定（简化处理：50%胜率）
        if (random(100) < 50)
        {
            tell_object(ob, HIG "你凭借坚定的意志击退了心魔！突破概率大幅提升！\n" NOR);
            ob->add_temp("breakthrough/pill_bonus", 30);
        }
        else
        {
            tell_object(ob, HIB "你未能压制心魔，本次突破已然失败……\n" NOR);
            ob->set_temp("breakthrough/auto_fail", 1);
        }
        return 1;
    }

    // 天降机缘 3%
    if (roll < BREAK_EVENT_HEAVENLY_BOON + BREAK_EVENT_HEART_DEVIL)
    {
        tell_object(ob, HIY "一道金光从天而降！你感到一股浩瀚的力量涌入体内——天降机缘！\n" NOR);
        ob->set_temp("breakthrough/auto_success", 1);
        ob->set_temp("breakthrough/heavenly_boon", 1);  // 额外灵根强度+10
        return 1;
    }

    // 灵力暴走 4%
    if (roll < BREAK_EVENT_MANA_RAMPAGE + BREAK_EVENT_HEAVENLY_BOON + BREAK_EVENT_HEART_DEVIL)
    {
        tell_object(ob, HIM "体内的灵力突然暴走！狂暴的灵力在经脉中横冲直撞……但似乎并无大碍。\n" NOR);
        // 获得临时buff（30分钟内消耗减半）
        ob->set_temp("breakthrough/mana_rampage", 1);
        ob->set_temp("breakthrough/mana_rampage_expire", time() + 1800);
        return 1;
    }

    // 瓶颈感应 2%
    if (roll < BREAK_EVENT_BOTTLE_NECK + BREAK_EVENT_MANA_RAMPAGE + BREAK_EVENT_HEAVENLY_BOON + BREAK_EVENT_HEART_DEVIL)
    {
        int current_prob = query_breakthrough_probability(ob, method);
        tell_object(ob, HIC "你心中忽生感应——冥冥中觉知此次突破成功率极低（约" + current_prob + "%），建议准备更充分些。\n" NOR);
        // 不改变概率，仅提示
        return 1;
    }

    // 功法共鸣 1%
    if (roll < BREAK_EVENT_SKILL_RESONANCE + BREAK_EVENT_BOTTLE_NECK + BREAK_EVENT_MANA_RAMPAGE + BREAK_EVENT_HEAVENLY_BOON + BREAK_EVENT_HEART_DEVIL)
    {
        tell_object(ob, HIM "你修炼的功法与灵根产生共鸣！功法运转速度骤然加快！\n" NOR);
        ob->add_temp("breakthrough/skill_bonus", 20);
        return 1;
    }

    return 0;
}

// 突破成功
int breakthrough_success(object ob, int method)
{
    if (!ob) return 0;

    // 必得：灵根强度+10
    ob->add(ROOT_PROP_STRENGTH, 10);

    // 精纯度+3%（100%）
    int purity = ob->query(ROOT_PROP_PURITY);
    purity += 3;
    if (purity > 100) purity = 100;
    ob->set(ROOT_PROP_PURITY, purity);

    // 额外奖励
    if (random(100) < 30)
        ob->add(ROOT_PROP_STRENGTH, 5);  // 额外强度+5（30%）

    if (random(100) < 20)
    {
        // 精纯度额外+5%（20%）
        int purity2 = ob->query(ROOT_PROP_PURITY);
        purity2 += 5;
        if (purity2 > 100) purity2 = 100;
        ob->set(ROOT_PROP_PURITY, purity2);
    }

    // 领悟被动「灵根共鸣」5%
    if (random(100) < 5)
    {
        ob->set("spirit_root/resonance", 1);
        tell_object(ob, HIW "你领悟了「灵根共鸣」！修炼对应属性功法时速度+10%！\n" NOR);
    }

    // 重置失败连续计数
    ob->set(ROOT_PROP_FAIL_STREAK, 0);

    // 记录突破次数
    ob->add(ROOT_PROP_BREAK_COUNT, 1);

    // 天降机缘额外奖励
    if (ob->query_temp("breakthrough/heavenly_boon"))
    {
        ob->add(ROOT_PROP_STRENGTH, 10);
        ob->delete_temp("breakthrough/heavenly_boon");
    }

    tell_object(ob, HIG "\n═══════════════║\n");
    tell_object(ob, "  灵根突破成功！\n");
    tell_object(ob, "  灵根强度 +10\n");
    tell_object(ob, "  灵根精纯度 +3%\n");
    tell_object(ob, "║═══════════════\n" NOR);

    return 1;
}

// 突破失败
int breakthrough_failure(object ob, int method)
{
    if (!ob) return 0;

    // 灵根强度惩罚（因突破方式而异）
    int penalty;
    switch (method)
    {
    case BREAK_METHOD_NATURAL:      penalty = 5;  break;
    case BREAK_METHOD_PILL_AID:     penalty = 3;  break;
    case BREAK_METHOD_SPIRIT_STONE: penalty = 0;  break;
    case BREAK_METHOD_TREASURE:     penalty = 10; break;
    case BREAK_METHOD_SECRET_REALM: penalty = 0;  break;
    default: penalty = 5; break;
    }

    if (penalty > 0)
    {
        int current_strength = query_spirit_root_strength(ob);
        // 保护：灵根强度不会降至品质要求以下
        int quality = query_spirit_root_quality(ob);
        int min_strength;
        switch (quality)
        {
        case SPIRIT_ROOT_PSEUDO:   min_strength = 30; break;
        case SPIRIT_ROOT_FAKE:     min_strength = 50; break;
        case SPIRIT_ROOT_TRUE:     min_strength = 70; break;
        case SPIRIT_ROOT_VARIANT:  min_strength = 90; break;
        case SPIRIT_ROOT_HEAVENLY: min_strength = 100; break;
        default: min_strength = 0; break;
        }
        int actual_penalty = penalty;
        if (current_strength - actual_penalty < min_strength)
            actual_penalty = current_strength - min_strength;
        if (actual_penalty > 0)
            ob->add(ROOT_PROP_STRENGTH, -actual_penalty);
    }

    // 增加失败计数
    ob->add(ROOT_PROP_FAIL_STREAK, 1);

    // 概率触发附加效果
    // 10% 灵根震荡（24h内修炼效率-30%）
    if (random(100) < 10)
    {
        apply_root_debuff(ob, ROOT_DEBUFF_SHOCK, ROOT_DEBUFF_DURATION_SHORT);
    }

    // 5% 灵根不稳（下次突破基础概率-10%）
    if (random(100) < 5)
    {
        apply_root_debuff(ob, ROOT_DEBUFF_UNSTABLE, ROOT_DEBUFF_DURATION_LONG);
    }

    // 提示失败
    tell_object(ob, HIR "\n═══════════════║\n");
    tell_object(ob, "  灵根突破失败！\n");
    if (penalty > 0)
        tell_object(ob, "  灵根强度 -" + penalty + "\n");
    tell_object(ob, "║═══════════════\n" NOR);

    // 连续失败保底的提示
    int fail_streak = query_spirit_root_fail_streak(ob);
    if (fail_streak >= BREAK_STREAK_FAIL_THRESHOLD)
    {
        tell_object(ob, HIC "连续失败多次，下次突破概率将获得额外加成！\n" NOR);
    }

    return 1;
}

// 伪灵根结丹上限查询
int query_pseudo_core_limit_reached(object ob)
{
    if (!ob) return 0;
    int quality = query_spirit_root_quality(ob);
    if (quality != SPIRIT_ROOT_PSEUDO) return 0;

    // 伪灵根特殊方式可以突破上限
    // 三转重元功二转已完成?
    if (ob->query("spirit_root/pseudo_core_boost") >= 3)
        return 1;  // 已达最大叠加

    return 0;
}

//=============================================================================
// Debuff 系统
//=============================================================================

// 应用灵根负面效果
int apply_root_debuff(object ob, int debuff_type, int duration)
{
    if (!ob) return 0;

    mapping debuff = ob->query(ROOT_PROP_DEBUFF);
    if (!mapp(debuff)) debuff = ([]);

    switch (debuff_type)
    {
    case ROOT_DEBUFF_SHOCK:
        debuff["shock"] = 1;
        debuff["shock_expire"] = time() + duration;
        break;

    case ROOT_DEBUFF_UNSTABLE:
        debuff["unstable"] = 1;
        debuff["unstable_expire"] = time() + duration;
        break;

    case ROOT_DEBUFF_DAMAGE:
        debuff["damage"] = 1;
        debuff["damage_expire"] = time() + duration;
        debuff["damage_amount"] = 5;  // 默认损伤值
        break;

    case ROOT_DEBUFF_SEAL:
        debuff["seal"] = 1;
        debuff["seal_expire"] = time() + duration;
        break;

    case ROOT_DEBUFF_SIDE_EFFECT:
        debuff["side_effect"] = 1;
        debuff["side_effect_expire"] = time() + duration;
        break;

    default:
        return 0;
    }

    ob->set(ROOT_PROP_DEBUFF, debuff);
    return 1;
}

// 移除指定类型的 debuff
int remove_root_debuff(object ob, int debuff_type)
{
    if (!ob) return 0;

    mapping debuff = ob->query(ROOT_PROP_DEBUFF);
    if (!mapp(debuff)) return 0;

    switch (debuff_type)
    {
    case ROOT_DEBUFF_SHOCK:
        map_delete(debuff, "shock");
        map_delete(debuff, "shock_expire");
        break;
    case ROOT_DEBUFF_UNSTABLE:
        map_delete(debuff, "unstable");
        map_delete(debuff, "unstable_expire");
        break;
    case ROOT_DEBUFF_DAMAGE:
        map_delete(debuff, "damage");
        map_delete(debuff, "damage_expire");
        map_delete(debuff, "damage_amount");
        break;
    case ROOT_DEBUFF_SEAL:
        map_delete(debuff, "seal");
        map_delete(debuff, "seal_expire");
        break;
    case ROOT_DEBUFF_SIDE_EFFECT:
        map_delete(debuff, "side_effect");
        map_delete(debuff, "side_effect_expire");
        break;
    default:
        return 0;
    }

    if (!sizeof(debuff))
        ob->delete(ROOT_PROP_DEBUFF);
    else
        ob->set(ROOT_PROP_DEBUFF, debuff);

    return 1;
}

// 清除所有 debuff
int clear_all_root_debuffs(object ob)
{
    if (!ob) return 0;
    ob->delete(ROOT_PROP_DEBUFF);
    return 1;
}

// 应用 debuff 效果（返回当前影响映射）
mapping apply_debuff_effects(object ob)
{
    mapping effects = ([]);
    if (!ob) return effects;

    mapping debuff = ob->query(ROOT_PROP_DEBUFF);
    if (!mapp(debuff)) return effects;

    int now = time();

    // 灵根震荡 -> 修炼效率-30%
    if (debuff["shock"] && debuff["shock_expire"] > now)
        effects["cultivation_speed"] = -30;

    // 灵根不稳 -> 下次突破概率-10%（在突破时处理，这里仅标记）
    if (debuff["unstable"] && debuff["unstable_expire"] > now)
        effects["breakthrough_penalty"] = -10;

    // 灵根损伤 -> 灵根强度持续下降
    if (debuff["damage"] && debuff["damage_expire"] > now)
    {
        effects["strength_damage"] = debuff["damage_amount"];
        // 实际损伤由定时器触发
    }

    // 灵根封闭 -> 所有灵根效果归零
    if (debuff["seal"] && debuff["seal_expire"] > now)
        effects["all_sealed"] = 1;

    // 变异副作用 -> 特定条件下触发
    if (debuff["side_effect"] && debuff["side_effect_expire"] > now)
        effects["side_effect_active"] = 1;

    // 过期清理
    if (debuff["shock"] && debuff["shock_expire"] <= now)
    {
        map_delete(debuff, "shock");
        map_delete(debuff, "shock_expire");
    }
    if (debuff["unstable"] && debuff["unstable_expire"] <= now)
    {
        map_delete(debuff, "unstable");
        map_delete(debuff, "unstable_expire");
    }
    if (debuff["damage"] && debuff["damage_expire"] <= now)
    {
        map_delete(debuff, "damage");
        map_delete(debuff, "damage_expire");
        map_delete(debuff, "damage_amount");
    }
    if (debuff["seal"] && debuff["seal_expire"] <= now)
    {
        map_delete(debuff, "seal");
        map_delete(debuff, "seal_expire");
    }
    if (debuff["side_effect"] && debuff["side_effect_expire"] <= now)
    {
        map_delete(debuff, "side_effect");
        map_delete(debuff, "side_effect_expire");
    }

    if (!sizeof(debuff))
        ob->delete(ROOT_PROP_DEBUFF);
    else
        ob->set(ROOT_PROP_DEBUFF, debuff);

    return effects;
}

// 查询 debuff 对某一具体属性的影响
int query_debuff_effect(object ob, string effect_type)
{
    mapping effects = apply_debuff_effects(ob);
    if (!mapp(effects)) return 0;

    // 灵根封闭时所有效果归零
    if (effects["all_sealed"]) return -999;

    return effects[effect_type];
}

//=============================================================================
// 突破任务链系统
//=============================================================================

// 查询突破任务链
string *query_breakthrough_task_chain(object ob, int realm)
{
    if (!ob) return ({});

    switch (realm)
    {
    case REALM_QI_REFINERY:
        // 凡人 → 炼气期（入门任务）
        return ({
            "拜入修仙门派",
            "感应天地灵气",
            "完成入门修炼",
            "打通第一条经脉",
        });

    case REALM_FOUNDATION:
        // 炼气 → 筑基
        return ({
            "收集筑基丹材料（主药×3 + 辅药×5）",
            "修炼至炼气大圆满",
            "完成门派筑基试炼",
            "寻找一处灵气充沛之地准备突破",
        });

    case REALM_CORE_FORMATION:
        // 筑基 → 结丹
        return ({
            "收集雪灵水×1 或 天火液×1",
            "修炼至筑基大圆满",
            "灵根强度达到当前品质上限",
            "（伪灵根）完成三转重元功第一转",
            "前往灵脉交汇处准备突破",
        });

    case REALM_NASCENT_SOUL:
        // 结丹 → 元婴
        return ({
            "收集凝婴丹材料",
            "修炼至结丹大圆满",
            "准备心魔对抗秘法",
            "寻找一处隐秘洞府闭关",
            "完成门派元婴试炼",
        });

    default:
        return ({ "尚未定义的突破任务链" });
    }
}

// 检查任务链完成度
int check_task_chain_completion(object ob, string chain_id)
{
    if (!ob) return 0;

    // 具体检查逻辑由任务系统实现
    // 这里提供接口框架
    mapping progress = ob->query("quest/breakthrough/" + chain_id);
    if (!mapp(progress)) return 0;

    int completed = progress["completed"];
    int total = progress["total"];

    if (total <= 0) return 0;
    return (completed >= total);
}

//=============================================================================
// 属性计算（供外部调用）
//=============================================================================

// 查询修炼速度系数
float query_cultivation_speed_factor(object ob)
{
    if (!ob) return 0.0;

    int quality = query_spirit_root_quality(ob);
    if (quality <= SPIRIT_ROOT_NONE) return 0.0;

    // 检查灵根封闭
    mapping effects = apply_debuff_effects(ob);
    if (effects["all_sealed"]) return 0.0;

    // 基础速度系数
    float base_speed;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   base_speed = 0.3;  break;
    case SPIRIT_ROOT_FAKE:     base_speed = 0.6;  break;
    case SPIRIT_ROOT_TRUE:     base_speed = 1.0;  break;
    case SPIRIT_ROOT_VARIANT:  base_speed = 2.3;  break;
    case SPIRIT_ROOT_HEAVENLY: base_speed = 2.5;  break;
    default: base_speed = 0.0; break;
    }

    // 精纯度修正
    int purity = query_spirit_root_purity(ob);
    float purity_factor = 1.0 - (1.0 - purity / 100.0) * 0.3;
    if (purity_factor < 0.7) purity_factor = 0.7;

    // 灵根共鸣加成（被动）
    if (ob->query("spirit_root/resonance"))
        base_speed *= 1.1;

    // 灵根震荡惩罚
    if (effects["cultivation_speed"])
        purity_factor *= 0.7;

    return base_speed * purity_factor;
}

// 灵力上限修正
float query_mana_limit_factor(object ob)
{
    if (!ob) return 0.0;

    int quality = query_spirit_root_quality(ob);
    if (quality <= SPIRIT_ROOT_NONE) return 0.0;

    mapping effects = apply_debuff_effects(ob);
    if (effects["all_sealed"]) return 0.0;

    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   return 0.8;
    case SPIRIT_ROOT_FAKE:     return 0.9;
    case SPIRIT_ROOT_TRUE:     return 1.0;
    case SPIRIT_ROOT_VARIANT:  return 1.2;
    case SPIRIT_ROOT_HEAVENLY: return 1.3;
    default: return 0.0;
    }
}

// 灵力恢复速度
float query_mana_regen_factor(object ob)
{
    if (!ob) return 0.0;

    int quality = query_spirit_root_quality(ob);
    if (quality <= SPIRIT_ROOT_NONE) return 0.0;

    mapping effects = apply_debuff_effects(ob);
    if (effects["all_sealed"]) return 0.0;

    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   return 0.6;
    case SPIRIT_ROOT_FAKE:     return 0.8;
    case SPIRIT_ROOT_TRUE:     return 1.0;
    case SPIRIT_ROOT_VARIANT:  return 1.3;
    case SPIRIT_ROOT_HEAVENLY: return 1.5;
    default: return 0.0;
    }
}

// 法术伤害加成
float query_magic_damage_bonus(object ob, string element)
{
    if (!ob) return 0.0;
    if (!element) return 0.0;

    int quality = query_spirit_root_quality(ob);
    if (quality <= SPIRIT_ROOT_NONE) return 0.0;

    mapping effects = apply_debuff_effects(ob);
    if (effects["all_sealed"]) return 0.0;

    string main_elem = query_spirit_root_main_element(ob);
    string *elements = query_spirit_root_elements(ob);
    int is_main = (element == main_elem);
    int is_sub = member_array(element, elements) != -1;

    switch (quality)
    {
    case SPIRIT_ROOT_HEAVENLY:
        if (is_main) return 0.50;
        return -0.20;  // 单一属性天灵根，非主属性降低

    case SPIRIT_ROOT_VARIANT:
        if (is_main)
        {
            // 变异属性大幅加成
            if (member_array(element, SPIRIT_ELEMENT_VARIANTS) != -1)
                return 0.80;
            return 0.30;
        }
        return -0.10;

    case SPIRIT_ROOT_TRUE:
        if (is_main) return 0.20;
        if (is_sub) return 0.15;
        return -0.15;

    case SPIRIT_ROOT_FAKE:
        if (is_main || is_sub) return 0.10;
        return -0.05;

    case SPIRIT_ROOT_PSEUDO:
        if (is_main || is_sub) return 0.05;
        return 0.0;

    default:
        return 0.0;
    }
}

// 五行抗性
float query_element_resistance(object ob, string element)
{
    if (!ob) return 0.0;
    if (!element) return 0.0;

    string *elements = query_spirit_root_elements(ob);
    if (member_array(element, elements) == -1) return 0.0;

    // 拥有该属性灵根，提供20%抗性
    return 0.20;
}

// 功法层数上限
int query_skill_level_limit(object ob, string skill_name)
{
    if (!ob) return 0;

    int quality = query_spirit_root_quality(ob);
    int strength = query_spirit_root_strength(ob);

    if (quality <= SPIRIT_ROOT_NONE) return 0;

    // 品质基准强度
    int base_strength;
    switch (quality)
    {
    case SPIRIT_ROOT_PSEUDO:   base_strength = 30; break;
    case SPIRIT_ROOT_FAKE:     base_strength = 50; break;
    case SPIRIT_ROOT_TRUE:     base_strength = 70; break;
    case SPIRIT_ROOT_VARIANT: base_strength = 90; break;
    case SPIRIT_ROOT_HEAVENLY: base_strength = 100; break;
    default: base_strength = 0; break;
    }

    // 功法层数上限由强度/基准强度决定
    float ratio = strength * 1.0 / base_strength;
    if (ratio > 1.0) ratio = 1.0;

    // 假设功法总层数为100（由调用方确定具体功法的总层数）
    // 返回比例系数，调用方乘以功法总层数即可
    return to_int(ratio * 100);
}

//=============================================================================
// 境界(realm)属性接线：统一境界存储约定
//   realm      = 中文境界字符串（"炼气1层"~"炼气13层"（层数为 ASCII 数字，sect_d 依赖）/
//                 "筑基初期"~"大乘后期"）
//   realm_sub  = 子阶段（"初期"/"中期"/"后期"/"巅峰"/"大圆满"，供 achievement 等系统）
//   realm_index= 大境界索引（0=凡人 1=炼气 2=筑基 3=结丹 4=元婴 5=化神 6=炼虚 7=合体 8=大乘）
//   xiuwei     = 修为值（打坐/灵石灌注获得，突破消耗）
// 读取端（sect_d/quest_chain_d/activity_d/achievement_d/economy_bridge_d）均以
// query("realm") 为真值来源，本约定与各自解析逻辑兼容。
//=============================================================================

// 大境界名称（下标即 realm_index，0=凡人）
nosave string *realm_stage_names = ({ "凡人", "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘" });
nosave string *realm_sub_stage_names = ({ "初期", "中期", "后期", "巅峰", "大圆满" });

// 炼气期层数上限
#define REALM_QI_MAX_LAYER       13
// 炼气期 13 层 → 筑基的修为门槛（含各子境界门槛）
// 设计依据：quest_chain.h REALM_BASE_*（炼气=50 筑基=200 …）× 放大系数 200
#define XIUWEI_QI_TO_ZHU         10000
#define XIUWEI_ZHU_SUB           5000    // 筑基初→中→后 每档
#define XIUWEI_ZHU_TO_JIE        30000   // 筑基→结丹
#define XIUWEI_JIE_SUB           20000   // 结丹初→中→后 每档
#define XIUWEI_JIE_TO_YING       100000  // 结丹→元婴
#define XIUWEI_YING_SUB          50000   // 元婴初→中→后 每档
#define XIUWEI_YING_TO_HUA       500000  // 元婴→化神

// 打坐修炼：每次心跳获得的基础修为（按大境界）
#define XIUWEI_BASE_QI           10
#define XIUWEI_BASE_ZHU          30
#define XIUWEI_BASE_JIE          100
#define XIUWEI_BASE_YING         400
#define XIUWEI_BASE_HUA          1500

// 突破失败冷却（现实秒）：对齐 realm-breakthrough-failure-penalty 设计约定
//   下境界大境界突破失败 15 游戏天 ≈ 现实 1 天；上境界 45 游戏天 ≈ 现实 3 天
#define BREAK_CD_QI_TO_ZHU       86400
#define BREAK_CD_ZHU_TO_JIE      86400
#define BREAK_CD_JIE_TO_YING     259200
#define BREAK_CD_YING_TO_HUA     259200

// 大境界连续失败保底（≥3 次后下次概率 +15%，直到成功为止）
#define MAJOR_BREAK_STREAK_THRESHOLD  3
#define MAJOR_BREAK_STREAK_BONUS      15

// 大境界突破失败修为回退比例（千分比，保留部分进度不重置）
#define BREAK_FAIL_XIUWEI_KEEP    500   // 失败保留 50% 当前突破门槛

// ----- 查询接口 -----

// 玩家当前大境界索引（0=凡人）
int query_player_realm_index(object ob)
{
    int idx;

    if (!objectp(ob)) return 0;
    idx = ob->query("realm_index");
    if (intp(idx) && idx > 0) return idx;
    // 兜底：从 realm 字符串解析
    return query_realm_index_from_string(ob->query("realm"));
}

// 从境界字符串解析大境界索引（与 sect_d.parse_realm 口径一致）
int query_realm_index_from_string(string realm)
{
    int i;

    if (!stringp(realm) || realm == "") return 0;
    for (i = 1; i < sizeof(realm_stage_names); i++)
        if (strsrch(realm, realm_stage_names[i]) != -1)
            return i;
    return 0;
}

// 玩家炼气层数（非炼气期返回 0）
int query_player_realm_layer(object ob)
{
    string realm;
    int idx, i, len, start, end;

    if (!objectp(ob)) return 0;
    idx = query_player_realm_index(ob);
    if (idx != 1) return 0;

    realm = ob->query("realm");
    if (!stringp(realm)) return 0;
    len = strlen(realm);
    start = -1;
    for (i = 0; i < len; i++)
    {
        int ch = realm[i];
        if (ch >= 48 && ch <= 57)
        {
            start = i;
            break;
        }
    }
    if (start == -1) return 1;
    end = start;
    while (end < len)
    {
        int ch = realm[end];
        if (ch < 48 || ch > 57) break;
        end++;
    }
    return to_int(realm[start..end-1]);
}

// 玩家境界显示名（无则"凡人"）
string query_player_realm(object ob)
{
    string realm;

    if (!objectp(ob)) return "凡人";
    realm = ob->query("realm");
    if (stringp(realm) && realm != "") return realm;
    return "凡人";
}

// 玩家子阶段名（供 achievement_d 等系统）
string query_player_realm_sub(object ob)
{
    string sub;

    if (!objectp(ob)) return "初期";
    sub = ob->query("realm_sub");
    if (stringp(sub) && sub != "") return sub;
    return "初期";
}

// 境界索引 + 子阶段 → 中文境界字符串
// index=1（炼气）时 sub 为层数（1~13）；index>=2 时 sub 为子阶段序号（0初/1中/2后）
string realm_name(int index, int sub)
{
    if (index <= 0) return "凡人";
    if (index >= sizeof(realm_stage_names)) index = sizeof(realm_stage_names) - 1;

    if (index == 1)
    {
        if (sub < 1) sub = 1;
        if (sub > REALM_QI_MAX_LAYER) sub = REALM_QI_MAX_LAYER;
        // 层数用 ASCII 数字（sect_d.extract_layer 只解析 0-9，中文数字无法提取）
        return "炼气" + sub + "层";
    }
    else
    {
        if (sub < 0) sub = 0;
        if (sub > 2) sub = 2;
        return realm_stage_names[index] + realm_sub_stage_names[sub];
    }
}

// 炼气层数 → 子阶段名
string qi_layer_sub_name(int layer)
{
    if (layer <= 3) return "初期";
    if (layer <= 6) return "中期";
    if (layer <= 9) return "后期";
    if (layer <= 12) return "巅峰";
    return "大圆满";
}

// 子阶段序号 → 名（0初/1中/2后）
string sub_stage_name(int stage)
{
    if (stage < 0) stage = 0;
    if (stage > 2) stage = 2;
    return realm_sub_stage_names[stage];
}

// 写入玩家境界（统一入口）
void set_player_realm(object ob, int index, int sub)
{
    if (!objectp(ob)) return;

    ob->set("realm", realm_name(index, sub));
    ob->set("realm_index", index);
    if (index == 1)
        ob->set("realm_sub", qi_layer_sub_name(sub));
    else
        ob->set("realm_sub", sub_stage_name(sub));
}

// ----- 修为存取 -----

// 玩家当前修为
int query_xiuwei(object ob)
{
    int x;

    if (!objectp(ob)) return 0;
    x = ob->query("xiuwei");
    if (!intp(x)) return 0;
    return x;
}

// 增加修为（0 以下归零保护）
int add_xiuwei(object ob, int n)
{
    int cur;

    if (!objectp(ob) || n == 0) return query_xiuwei(ob);
    cur = query_xiuwei(ob) + n;
    if (cur < 0) cur = 0;
    ob->set("xiuwei", cur);
    return cur;
}

// 消耗修为（够则扣并返回 1，不够返回 0）
int spend_xiuwei(object ob, int n)
{
    if (!objectp(ob) || n <= 0) return 0;
    if (query_xiuwei(ob) < n) return 0;
    add_xiuwei(ob, -n);
    return 1;
}

// 炼气第 N 层 → N+1 层所需修为（1→2 需 100，2→3 需 200 … 12→13 需 1200）
int query_layer_xiuwei_need(int layer)
{
    if (layer < 1) layer = 1;
    return layer * 100;
}

// 当前炼气层 → 下一层的修为需求；已大圆满返回 0
int query_next_layer_need(object ob)
{
    int layer;

    if (!objectp(ob)) return 0;
    if (query_player_realm_index(ob) != 1) return 0;
    layer = query_player_realm_layer(ob);
    if (layer >= REALM_QI_MAX_LAYER) return 0;
    return query_layer_xiuwei_need(layer);
}

// 当前子境界 → 下一子境界的修为需求（筑基/结丹/元婴）；非此类返回 0
int query_next_sub_need(object ob)
{
    int idx;

    if (!objectp(ob)) return 0;
    idx = query_player_realm_index(ob);
    switch (idx)
    {
    case 2: return XIUWEI_ZHU_SUB;   // 筑基
    case 3: return XIUWEI_JIE_SUB;   // 结丹
    case 4: return XIUWEI_YING_SUB;  // 元婴
    default: return 0;
    }
}

// 当前境界突破到下一大境界所需修为（非大圆满前不可大突破）
int query_major_break_need(object ob)
{
    int idx, layer;

    if (!objectp(ob)) return 0;
    idx = query_player_realm_index(ob);
    switch (idx)
    {
    case 1:
        layer = query_player_realm_layer(ob);
        if (layer >= REALM_QI_MAX_LAYER) return XIUWEI_QI_TO_ZHU;
        return 0;
    case 2:
        if (query_player_sub_stage(ob) >= 2) return XIUWEI_ZHU_TO_JIE;
        return 0;
    case 3:
        if (query_player_sub_stage(ob) >= 2) return XIUWEI_JIE_TO_YING;
        return 0;
    case 4:
        if (query_player_sub_stage(ob) >= 2) return XIUWEI_YING_TO_HUA;
        return 0;
    default:
        return 0;
    }
}

// 玩家子阶段序号（0初/1中/2后；炼气按层数折算）
int query_player_sub_stage(object ob)
{
    string sub;

    if (!objectp(ob)) return 0;
    if (query_player_realm_index(ob) == 1)
    {
        int layer = query_player_realm_layer(ob);
        if (layer <= 3) return 0;
        if (layer <= 6) return 1;
        return 2;
    }
    sub = ob->query("realm_sub");
    if (!stringp(sub)) return 0;
    if (strsrch(sub, "中期") != -1) return 1;
    if (strsrch(sub, "后期") != -1) return 2;
    return 0;
}

// ----- 修炼（时间换修为） -----

// 当前境界的打坐基础修为（每次心跳）
int query_cultivation_base(object ob)
{
    int idx;

    if (!objectp(ob)) return 0;
    idx = query_player_realm_index(ob);
    switch (idx)
    {
    case 0: return 0;                    // 凡人无法修炼
    case 1: return XIUWEI_BASE_QI;
    case 2: return XIUWEI_BASE_ZHU;
    case 3: return XIUWEI_BASE_JIE;
    case 4: return XIUWEI_BASE_YING;
    default: return XIUWEI_BASE_HUA;     // 化神及以上
    }
}

// 单次心跳修炼所得修为 = 境界基准 × 灵根速度系数（含精纯度修正）
int query_heartbeat_cultivation_gain(object ob)
{
    int base;
    float factor;

    if (!objectp(ob)) return 0;
    base = query_cultivation_base(ob);
    if (base <= 0) return 0;
    factor = query_cultivation_speed_factor(ob);
    if (factor <= 0.0) return 0;
    return to_int(to_float(base) * factor);
}

// 检查并执行炼气期层数自动提升（修为足够即升层，新人保护无概率判定）
// 返回 1=已升层，0=未升层
int check_qi_layer_up(object ob)
{
	int layer, need, new_layer;

	if (!objectp(ob)) return 0;
	if (query_player_realm_index(ob) != 1) return 0;

	layer = query_player_realm_layer(ob);
	if (layer >= REALM_QI_MAX_LAYER) return 0;
	need = query_layer_xiuwei_need(layer);
	if (query_xiuwei(ob) < need) return 0;

	new_layer = layer + 1;
	spend_xiuwei(ob, need);
	set_player_realm(ob, 1, new_layer);
	if (new_layer >= REALM_QI_MAX_LAYER)
		tell_object(ob, HIG "你的修为已达炼气" + REALM_QI_MAX_LAYER +
		            "层大圆满！可以尝试突破筑基（tupo）。\n" NOR);
	else
		tell_object(ob, HIG "水到渠成，你的境界提升至炼气" + new_layer + "层！\n" NOR);
	return 1;
}

// 执行一次心跳修炼，返回实际获得修为（同时喂养灵根经验）
int do_heartbeat_cultivation(object ob)
{
	int gain;

	if (!objectp(ob) || !userp(ob)) return 0;
	gain = query_heartbeat_cultivation_gain(ob);
	if (gain <= 0)
	{
		tell_object(ob, "你尚未测出灵根，无法引气入体。请先前往门派「测灵殿」检测灵根（root 查看）。\n");
		return 0;
	}

	add_xiuwei(ob, gain);
	// 灵根经验同步成长（修炼获得灵根经验）
	gain_exp_from_cultivation(ob, gain / 2);
	// 炼气期层数自动提升
	check_qi_layer_up(ob);

	return gain;
}

// ----- 大境界突破 -----

// 大境界突破失败冷却（秒），按当前境界
int query_break_cd(object ob)
{
    int idx;

    if (!objectp(ob)) return 0;
    idx = query_player_realm_index(ob);
    switch (idx)
    {
    case 1: return BREAK_CD_QI_TO_ZHU;   // 炼气→筑基
    case 2: return BREAK_CD_ZHU_TO_JIE;  // 筑基→结丹
    case 3: return BREAK_CD_JIE_TO_YING; // 结丹→元婴
    default: return BREAK_CD_YING_TO_HUA; // 元婴及以上
    }
}

// 剩余突破冷却（秒），0 表示可突破
int query_break_cooldown_remaining(object ob)
{
    int last, remain;

    if (!objectp(ob)) return 0;
    last = ob->query(ROOT_PROP_LAST_BREAK);
    if (!intp(last) || last <= 0) return 0;
    remain = last + query_break_cd(ob) - time();
    if (remain < 0) return 0;
    return remain;
}

// 大境界突破成功：写入真实境界
int major_breakthrough_success(object ob, int method)
{
    int idx, new_idx, purity;
    string msg;

    if (!objectp(ob)) return 0;
    idx = query_player_realm_index(ob);

    // 炼气→筑基：新境界筑基初期；其余大境界进入下一境界初期
    new_idx = idx + 1;
    set_player_realm(ob, new_idx, 0);
    ob->set(ROOT_PROP_LAST_BREAK, 0);

    // 突破奖励：灵根强度 +10、精纯度 +3%（与灵根突破一致）
    ob->add(ROOT_PROP_STRENGTH, 10);
    purity = ob->query(ROOT_PROP_PURITY);
    purity += 3;
    if (purity > 100) purity = 100;
    ob->set(ROOT_PROP_PURITY, purity);
    ob->set(ROOT_PROP_FAIL_STREAK, 0);

    msg = sprintf(HIG "\n═══════════════════\n"
                  "  恭喜！你突破至%s！\n"
                  "  灵根强度 +10，精纯度 +3%%\n"
                  "═══════════════════\n" NOR, query_player_realm(ob));
    tell_object(ob, msg);
    // 让同房间的人看到突破异象
    if (environment(ob))
        message_vision(HIM "$N身上灵光暴涨，气息陡然大增——竟然突破了境界！\n" NOR, ob);

    log_file("realm", sprintf("%s %s breakthrough -> %s\n",
              ctime(time()), ob->query("id"), query_player_realm(ob)));
    return 1;
}

// 大境界突破失败：修为回退 + 冷却 + 连续失败保底
int major_breakthrough_failure(object ob, int method)
{
    int need, keep, penalty, streak;

    if (!objectp(ob)) return 0;
    need = query_major_break_need(ob);
    // 修为回退：保留当前突破门槛的 50%（保留部分进度，不重置）
    keep = to_int(to_float(need) * BREAK_FAIL_XIUWEI_KEEP / 1000.0);
    if (keep > 0)
    {
        penalty = need - keep;
        add_xiuwei(ob, -penalty);
    }

    // 设置失败冷却
    ob->set(ROOT_PROP_LAST_BREAK, time());

    // 连续失败计数（大境界失败才累计，成功清零）
    ob->add(ROOT_PROP_FAIL_STREAK, 1);
    streak = ob->query(ROOT_PROP_FAIL_STREAK);

    // 道痕裂伤（简化版：连续失败 ≥2 触发灵根震荡 debuff，修炼效率下降）
    if (streak >= 2 && !ob->query(ROOT_PROP_DEBUFF))
        apply_root_debuff(ob, ROOT_DEBUFF_SHOCK, ROOT_DEBUFF_DURATION_SHORT);

    tell_object(ob, HIR "\n═══════════════════\n"
                "  突破失败！\n");
    if (penalty > 0)
        tell_object(ob, sprintf("  修为受损 -%d\n", penalty));
    tell_object(ob, sprintf("  经脉受创，%d 秒内无法再次尝试大境界突破\n", query_break_cd(ob)));
    tell_object(ob, "═══════════════════\n" NOR);

    if (streak >= MAJOR_BREAK_STREAK_THRESHOLD)
        tell_object(ob, HIC "连续失败多次，下次突破将获得额外加成！\n" NOR);

    return 1;
}

// 执行大境界突破（返回：1=成功 2=失败 0=条件不满足）
// method 复用灵根突破方式常量（BREAK_METHOD_*）
int do_major_breakthrough(object ob, int method)
{
    int need, prob, streak, idx, target;
    int roll, cd;

    if (!objectp(ob)) return 0;

    idx = query_player_realm_index(ob);
    if (idx <= 0 || idx >= sizeof(realm_stage_names) - 1)
    {
        tell_object(ob, "你已修炼至当前界面的最高境界，再无可突破之境。\n");
        return 0;
    }

    need = query_major_break_need(ob);
    if (need <= 0)
    {
        tell_object(ob, "你尚未修炼至当前境界的圆满，无法尝试大境界突破。\n");
        return 0;
    }

    if (query_xiuwei(ob) < need)
    {
        tell_object(ob, sprintf("修为不足：突破需修为 %d（当前 %d）。请继续打坐修炼（dazuo）。\n",
                    need, query_xiuwei(ob)));
        return 0;
    }

    cd = query_break_cooldown_remaining(ob);
    if (cd > 0)
    {
        tell_object(ob, sprintf("经脉尚未恢复，还需 %d 秒才能再次尝试突破。\n", cd));
        return 0;
    }

    // 天灵根特例：结丹及以下自动成功（query_major_breakthrough_probability 已内置）
    target = idx + 1;
    prob = query_major_breakthrough_probability(ob, target);

    tell_object(ob, sprintf(HIC "你盘膝而坐，引动体内灵力冲击%s瓶颈……成功率约 %d%%\n" NOR,
                realm_stage_names[target], prob));

    // 连续失败保底
    streak = ob->query(ROOT_PROP_FAIL_STREAK);
    if (streak >= MAJOR_BREAK_STREAK_THRESHOLD)
    {
        prob += MAJOR_BREAK_STREAK_BONUS;
        if (prob > 99) prob = 99;
        tell_object(ob, HIC "连续失败的积累让你对瓶颈有了更深感悟，突破概率提升！\n" NOR);
    }

    roll = random(100);
    if (roll < prob)
    {
        spend_xiuwei(ob, need);
        return major_breakthrough_success(ob, method) ? 1 : 0;
    }
    else
    {
        return major_breakthrough_failure(ob, method) ? 2 : 0;
    }
}

// 当前突破任务链提示（接 root_refine_d 任务链设计）
string query_current_break_task_chain(object ob)
{
    int idx, target, i;
    string *chain;
    string ret;

    if (!objectp(ob)) return "";
    idx = query_player_realm_index(ob);
    target = idx + 1;
    if (target >= sizeof(realm_stage_names)) return "";
    chain = query_breakthrough_task_chain(ob, target);
    if (!sizeof(chain)) return "";
    ret = HIC "突破任务链（" + realm_stage_names[idx] + "→" +
          realm_stage_names[target] + "）：\n" NOR;
    for (i = 0; i < sizeof(chain); i++)
        ret += sprintf("  %d. %s\n", i + 1, chain[i]);
    return ret;
}

//=============================================================================
// 工具函数
//=============================================================================

int is_variant_element(string element)
{
    string *variants = SPIRIT_ELEMENT_VARIANTS;
    return (member_array(element, variants) != -1);
}

int is_normal_element(string element)
{
    string *normal = SPIRIT_ELEMENT_ALL;
    return (member_array(element, normal) != -1);
}

int random_between(int min, int max)
{
    if (min >= max) return min;
    return min + random(max - min + 1);
}

string *random_elements(int count)
{
    string *all = SPIRIT_ELEMENT_ALL;
    string *result = ({});

    if (count <= 0) return result;
    if (count >= sizeof(all)) return all;

    // 随机选取 count 个不重复的属性
    string *pool = copy(all);
    for (int i = 0; i < count; i++)
    {
        int idx = random(sizeof(pool));
        result += ({ pool[idx] });
        pool -= ({ pool[idx] });
    }

    return result;
}
