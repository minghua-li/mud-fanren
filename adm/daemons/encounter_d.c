// encounter_d.c
// 奇遇/隐藏任务守护进程 — 随机事件触发、稀有奖励、奇遇追踪
//
// 职责：
//   1. 奇遇事件模板注册（随机事件/隐藏任务/特殊NPC）
//   2. 概率触发检测（普通/优秀/稀有/传说）
//   3. 奇遇进度追踪与持久化
//   4. 稀有奖励发放
//   5. 冷却与频率控制
//
// 关联设计文档：02-扩充内容/02-任务链与奖励曲线.md 第五、七章

#include <ansi.h>
#include <encounter.h>
#include <quest_chain.h>

inherit F_DBASE;

// ─── 全局数据 ───

// 奇遇事件模板注册表
nosave mapping encounter_templates = ([]);

// 按区域索引的奇遇
nosave mapping area_encounters = ([]);

// ═══════════════════════════════════════════
//  方法声明
// ═══════════════════════════════════════════

void create();
int clean_up();

// 模板管理
int register_encounter(mapping template);
int unregister_encounter(string enc_id);
mapping query_encounter(string enc_id);
mapping *query_encounters_by_area(string area);

// 触发检测
int try_trigger_encounter(object player, string area);
int check_encounter_conditions(object player, mapping template);
int roll_encounter_probability(int rarity);

// 玩家操作
int activate_encounter(object player, string enc_id);
int complete_encounter(object player, string enc_id);
int expire_encounter(object player, string enc_id);

// 查询
mapping get_player_active_encounters(object player);
int has_active_encounter(object player, string enc_id);
int is_encounter_completed(object player, string enc_id);
mapping *get_encounter_history(object player);
int get_encounter_cooldown(object player, int rarity);

// 内部工具
int get_player_realm_index(object player);
int check_cooldown(object player, string enc_id);
void set_cooldown(object player, string enc_id, int cooldown_time);

// ═══════════════════════════════════════════
//  初始化
// ═══════════════════════════════════════════

void create()
{
    seteuid(ROOT_UID);
    set("channel_id", "奇遇系统");
    
    encounter_templates = ([]);
    area_encounters = ([]);
    
    // 注册预定义的奇遇模板
    register_default_encounters();
    
    CHANNEL_D->do_channel(this_object(), "sys", "奇遇系统已经启动。");
}

int clean_up()
{
    return 1;
}

// ═══════════════════════════════════════════
//  预定义奇遇模板
// ═══════════════════════════════════════════

void register_default_encounters()
{
    // ── 掌天瓶觉醒（传说奇遇）──
    register_encounter(([
        "id":           "enc_vial_awakening",
        "name":         "掌天瓶觉醒",
        "type":         ENC_HIDDEN_QUEST,
        "rarity":       ENC_RARE_LEGENDARY,
        "area":         "七玄门",
        "conditions":   ({
            ([ "type": ENC_COND_REALM, "value": "炼气" ]),
            ([ "type": ENC_COND_ITEM,  "value": "/obj/vial" ]),
        }),
        "description":  "你怀中的掌天瓶突然发出微弱的荧光，似乎与七玄门深处的某处产生了共鸣。",
        "rewards":      ([
            "exp":      5000,
            "coin":     500,
            "title":    "瓶中人",
            "special":  "掌天瓶觉醒",
            "items":    ({ "/clone/pill/huang_long_dan" }),
        ]),
        "time_limit":   0,  // 无时间限制
    ]));

    // ── 万年灵乳发现（稀有奇遇）──
    register_encounter(([
        "id":           "enc_milk_discovery",
        "name":         "万年灵乳发现",
        "type":         ENC_RANDOM_EVENT,
        "rarity":       ENC_RARE_RARE,
        "area":         "血色禁地",
        "conditions":   ({
            ([ "type": ENC_COND_REALM, "value": "筑基" ]),
        ]),
        "description":  "你在血色禁地深处发现一处隐蔽的洞穴，洞壁上有乳白色的液体缓缓渗出，散发着浓郁的药香——这是传说中的万年灵乳！",
        "rewards":      ([
            "exp":      10000,
            "coin":     1000,
            "items":    ({ "/clone/pill/wannian_lingru" }),
        ]),
        "time_limit":   3600,  // 1小时内需采集
    ]));

    // ── 密境商人（普通奇遇）──
    register_encounter(([
        "id":           "enc_mystic_merchant",
        "name":         "神秘商人",
        "type":         ENC_SPECIAL_NPC,
        "rarity":       ENC_RARE_COMMON,
        "area":         "天星城",
        "conditions":   ({
            ([ "type": ENC_COND_REALM, "value": "结丹" ]),
        ]),
        "description":  "天星城的坊市中，你瞥见一位身披斗篷的神秘商人，他出售的货物似乎非同寻常。",
        "rewards":      ([
            "exp":      500,
            "coin":     200,
            "items":    ({ "/clone/pill/bu_tian_dan" }),
        ]),
        "time_limit":   1800,  // 30分钟内
    ]));

    // ── 逆天改命（隐藏成就联动）──
    register_encounter(([
        "id":           "enc_defy_fate",
        "name":         "逆天改命",
        "type":         ENC_HIDDEN_QUEST,
        "rarity":       ENC_RARE_RARE,
        "area":         "任意",
        "conditions":   ({
            ([ "type": ENC_COND_REALM, "value": "结丹" ]),
            ([ "type": ENC_COND_ITEM,  "value": "special_pill" ]),
        ]),
        "description":  "以伪灵根之姿走到结丹之路的尽头，天道似乎为你降下了一丝特殊机缘。",
        "rewards":      ([
            "exp":      50000,
            "coin":     5000,
            "title":    "逆天者",
            "special":  "突破概率+5%",
        ]),
        "time_limit":   0,
    ]));

    // ── 海底遗迹（优秀奇遇）──
    register_encounter(([
        "id":           "enc_sea_ruins",
        "name":         "海底遗迹",
        "type":         ENC_RANDOM_EVENT,
        "rarity":       ENC_RARE_UNCOMMON,
        "area":         "乱星海",
        "conditions":   ({
            ([ "type": ENC_COND_REALM, "value": "元婴" ]),
        ]),
        "description":  "你在乱星海的海底发现了一座古老的遗迹，似乎隐藏着上古修士的传承。",
        "rewards":      ([
            "exp":      30000,
            "coin":     5000,
            "items":    ({ "/clone/pill/tong_tian_ling_bao" }),
        ]),
        "time_limit":   7200,  // 2小时内
    ]));
}

// ═══════════════════════════════════════════
//  模板管理
// ═══════════════════════════════════════════

int register_encounter(mapping template)
{
    string enc_id, area;
    
    if (!mapp(template))
        return 0;
    
    enc_id = template["id"];
    if (!stringp(enc_id) || enc_id == "")
        return 0;
    
    if (!template["name"] || !template["type"])
        return 0;
    
    encounter_templates[enc_id] = template;
    
    // 按区域建立索引
    area = template["area"];
    if (stringp(area) && area != "" && area != "任意")
    {
        if (!area_encounters[area])
            area_encounters[area] = ({});
        if (member_array(enc_id, area_encounters[area]) == -1)
            area_encounters[area] += ({ enc_id });
    }
    else
    {
        // "任意"区域的事件也加入所有可用的区域索引
        // 使用"*"通配符表示全区域
        if (!area_encounters["*"])
            area_encounters["*"] = ({});
        if (member_array(enc_id, area_encounters["*"]) == -1)
            area_encounters["*"] += ({ enc_id });
    }
    
    return 1;
}

int unregister_encounter(string enc_id)
{
    if (!encounter_templates[enc_id])
        return 0;
    
    // 从区域索引移除
    // 简化：不删除区域索引（remove掉的不多，影响不大）
    map_delete(encounter_templates, enc_id);
    return 1;
}

mapping query_encounter(string enc_id)
{
    return encounter_templates[enc_id];
}

mapping *query_encounters_by_area(string area)
{
    mapping *result = ({});
    string *ids;
    int i;
    
    // 检查精确区域
    ids = area_encounters[area];
    if (!arrayp(ids))
        ids = ({});
    
    // 追加全区域事件
    string *global_ids = area_encounters["*"];
    if (arrayp(global_ids))
        ids += global_ids;
    
    for (i = 0; i < sizeof(ids); i++)
    {
        mapping tmpl = encounter_templates[ids[i]];
        if (mapp(tmpl))
            result += ({ tmpl });
    }
    
    return result;
}

// ═══════════════════════════════════════════
//  触发检测
// ═══════════════════════════════════════════

// 尝试在指定区域触发奇遇（由区域进入/战斗结束时调用）
int try_trigger_encounter(object player, string area)
{
    mapping *candidates;
    int i;
    
    if (!objectp(player) || !userp(player))
        return 0;
    
    // 获取该区域可触发的事件
    candidates = query_encounters_by_area(area);
    if (sizeof(candidates) == 0)
        return 0;
    
    // 按稀有度从高到低检查（高稀有度优先判定，但概率低）
    // 先随机打乱
    candidates = shuffle_array(candidates);
    
    for (i = 0; i < sizeof(candidates); i++)
    {
        mapping tmpl = candidates[i];
        int rarity = tmpl["rarity"];
        
        // 检查冷却
        if (check_cooldown(player, tmpl["id"]))
            continue;
        
        // 检查是否已完成
        if (is_encounter_completed(player, tmpl["id"]))
            continue;
        
        // 掷概率
        if (!roll_encounter_probability(rarity))
            continue;
        
        // 检查条件
        if (!check_encounter_conditions(player, tmpl))
            continue;
        
        // 触发！
        return activate_encounter(player, tmpl["id"]);
    }
    
    return 0;
}

// 检查奇遇条件是否满足
int check_encounter_conditions(object player, mapping template)
{
    mixed *conditions;
    int i;
    
    conditions = template["conditions"];
    if (!arrayp(conditions) || sizeof(conditions) == 0)
        return 1;  // 无条件，总是可触发
    
    for (i = 0; i < sizeof(conditions); i++)
    {
        mapping cond = conditions[i];
        int cond_type = cond["type"];
        string cond_value = cond["value"];
        
        switch (cond_type)
        {
        case ENC_COND_REALM:
        {
            // 境界检查
            string player_realm = player->query("realm");
            if (stringp(player_realm) && stringp(cond_value))
            {
                if (strsrch(player_realm, cond_value) == -1)
                    return 0;
            }
            break;
        }
        case ENC_COND_ITEM:
        {
            // 物品检查
            if (stringp(cond_value))
            {
                if (!player->query_temp("encounter/item_check_done"))
                {
                    object *inv = all_inventory(player);
                    int j, found;
                    for (j = 0; j < sizeof(inv); j++)
                    {
                        if (base_name(inv[j]) == cond_value ||
                            inv[j]->query("id") == cond_value)
                        {
                            found = 1;
                            break;
                        }
                    }
                    if (!found)
                        return 0;
                }
            }
            break;
        }
        case ENC_COND_QUEST:
        {
            // 任务完成检查
            mapping completed = player->query(QUEST_CHAIN_COMPLETED);
            if (mapp(completed))
            {
                if (!completed[cond_value])
                {
                    mapping side_completed = player->query(SIDE_QUEST_COMPLETED);
                    if (!mapp(side_completed) || !side_completed[cond_value])
                        return 0;
                }
            }
            break;
        }
        case ENC_COND_COMBAT:
        {
            // 战斗表现
            // 由调用方在特定战斗后标记
            break;
        }
        default:
            break;
        }
    }
    
    return 1;
}

// 掷稀有度概率
int roll_encounter_probability(int rarity)
{
    int roll = random(10000);  // 万分制
    int threshold;
    
    switch (rarity)
    {
    case ENC_RARE_LEGENDARY:
        threshold = to_int(ENC_BASE_PROB_LEGENDARY * 100);
        break;
    case ENC_RARE_RARE:
        threshold = ENC_BASE_PROB_RARE * 100;
        break;
    case ENC_RARE_UNCOMMON:
        threshold = ENC_BASE_PROB_UNCOMMON * 100;
        break;
    case ENC_RARE_COMMON:
    default:
        threshold = ENC_BASE_PROB_COMMON * 100;
        break;
    }
    
    return (roll < threshold);
}

// 数组随机打乱
mapping *shuffle_array(mapping *arr)
{
    int i, j;
    mapping tmp;
    
    for (i = sizeof(arr) - 1; i > 0; i--)
    {
        j = random(i + 1);
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
    return arr;
}

// ═══════════════════════════════════════════
//  玩家操作
// ═══════════════════════════════════════════

// 激活奇遇
int activate_encounter(object player, string enc_id)
{
    mapping template;
    mapping active;
    int cooldown;
    
    template = encounter_templates[enc_id];
    if (!mapp(template))
        return 0;
    
    // 初始化玩家活跃奇遇
    if (!mapp(player->query(ENCOUNTER_ACTIVE)))
        player->set(ENCOUNTER_ACTIVE, ([]));
    
    active = player->query(ENCOUNTER_ACTIVE);
    active[enc_id] = ([
        "status":     ENC_STATUS_ACTIVE,
        "start_time": time(),
        "rarity":     template["rarity"],
    ]);
    player->set(ENCOUNTER_ACTIVE, active);
    
    // 设置冷却
    cooldown = get_encounter_cooldown(player, template["rarity"]);
    set_cooldown(player, enc_id, cooldown);
    
    // 奇遇触发提示
    tell_object(player, sprintf(
        HIM "\n╔══════════════════════════════╗\n" NOR
        HIM "║           奇 遇 ！           ║\n" NOR
        HIM "╚══════════════════════════════╝\n" NOR
        HIW "【%s】\n" NOR
        "%s\n",
        template["name"],
        template["description"]));
    
    if (template["time_limit"] > 0)
    {
        tell_object(player, sprintf(
            HIY "限时：%d 分钟内完成！\n" NOR,
            template["time_limit"] / 60));
    }
    
    tell_object(player, "输入 " HIG "questlog encounter" NOR " 查看详情。\n");
    
    return 1;
}

// 完成奇遇
int complete_encounter(object player, string enc_id)
{
    mapping template;
    mapping active;
    mapping completed;
    mapping rewards;
    int i;
    
    template = encounter_templates[enc_id];
    if (!mapp(template))
        return 0;
    
    active = player->query(ENCOUNTER_ACTIVE);
    if (!mapp(active) || !active[enc_id])
        return 0;
    
    // 从活跃列表移除
    map_delete(active, enc_id);
    player->set(ENCOUNTER_ACTIVE, active);
    
    // 加入已完成
    if (!mapp(player->query(ENCOUNTER_COMPLETED)))
        player->set(ENCOUNTER_COMPLETED, ([]));
    completed = player->query(ENCOUNTER_COMPLETED);
    completed[enc_id] = time();
    player->set(ENCOUNTER_COMPLETED, completed);
    
    // 记录历史
    mapping *history = player->query(ENCOUNTER_HISTORY);
    if (!arrayp(history))
        history = ({});
    history += ({ ([
        "id":      enc_id,
        "name":    template["name"],
        "time":    time(),
        "rarity":  template["rarity"],
        "status":  "completed",
    ]) });
    // 限制历史记录数量
    if (sizeof(history) > ENC_HISTORY_MAX)
        history = history[sizeof(history) - ENC_HISTORY_MAX..<1];
    player->set(ENCOUNTER_HISTORY, history);
    
    // 发放奖励
    rewards = template["rewards"];
    if (mapp(rewards))
    {
        int exp_val = rewards["exp"];
        int coin_val = rewards["coin"];
        
        if (exp_val > 0)
            player->add("combat_exp", exp_val);
        
        if (coin_val > 0)
        {
            object coin = new("/clone/money/coin");
            if (coin)
            {
                coin->set_amount(coin_val);
                coin->move(player);
            }
        }
        
        // 物品奖励
        if (arrayp(rewards["items"]))
        {
            for (i = 0; i < sizeof(rewards["items"]); i++)
            {
                string item_path = rewards["items"][i];
                if (stringp(item_path) && item_path != "")
                {
                    object item = new(item_path);
                    if (item)
                    {
                        item->move(player);
                        tell_object(player, sprintf("获得物品：%s。\n", item->query("name")));
                    }
                }
            }
        }
        
        // 称号奖励
        if (rewards["title"])
        {
            player->set("title", rewards["title"]);
            tell_object(player, sprintf("获得称号：%s。\n", rewards["title"]));
        }
        
        tell_object(player, sprintf(
            HIM "奇遇【%s】完成！获得经验：%d，灵石：%d\n" NOR,
            template["name"], exp_val, coin_val));
    }
    
    return 1;
}

// 奇遇过期
int expire_encounter(object player, string enc_id)
{
    mapping active;
    
    active = player->query(ENCOUNTER_ACTIVE);
    if (!mapp(active) || !active[enc_id])
        return 0;
    
    active[enc_id]["status"] = ENC_STATUS_EXPIRED;
    player->set(ENCOUNTER_ACTIVE, active);
    
    tell_object(player, sprintf("奇遇【%s】已超时消失。\n",
        encounter_templates[enc_id]["name"]));
    
    map_delete(active, enc_id);
    player->set(ENCOUNTER_ACTIVE, active);
    
    return 1;
}

// ═══════════════════════════════════════════
//  查询
// ═══════════════════════════════════════════

mapping get_player_active_encounters(object player)
{
    if (!objectp(player))
        return ([]);
    return player->query(ENCOUNTER_ACTIVE);
}

int has_active_encounter(object player, string enc_id)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    active = player->query(ENCOUNTER_ACTIVE);
    if (!mapp(active))
        return 0;
    
    return mapp(active[enc_id]) && active[enc_id]["status"] == ENC_STATUS_ACTIVE;
}

int is_encounter_completed(object player, string enc_id)
{
    mapping completed;
    
    if (!objectp(player))
        return 0;
    
    completed = player->query(ENCOUNTER_COMPLETED);
    if (!mapp(completed))
        return 0;
    
    return (completed[enc_id] > 0);
}

mapping *get_encounter_history(object player)
{
    if (!objectp(player))
        return ({});
    return player->query(ENCOUNTER_HISTORY);
}

int get_encounter_cooldown(object player, int rarity)
{
    switch (rarity)
    {
    case ENC_RARE_LEGENDARY:
        return ENC_COOLDOWN_LEGEND;
    case ENC_RARE_RARE:
        return ENC_COOLDOWN_RARE;
    default:
        return ENC_COOLDOWN_BASE;
    }
}

// ═══════════════════════════════════════════
//  内部工具
// ═══════════════════════════════════════════

int get_player_realm_index(object player)
{
    string realm;
    string *realm_names = ({ "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘" });
    int i;
    
    if (!objectp(player))
        return 0;
    
    realm = player->query("realm");
    if (!stringp(realm) || realm == "")
        return 0;
    
    for (i = 0; i < sizeof(realm_names); i++)
    {
        if (strsrch(realm, realm_names[i]) != -1)
            return i;
    }
    
    return 0;
}

int check_cooldown(object player, string enc_id)
{
    mapping cooldowns;
    int last_time;
    
    cooldowns = player->query_temp("encounter/cooldowns");
    if (!mapp(cooldowns))
        return 0;
    
    last_time = cooldowns[enc_id];
    if (!last_time)
        return 0;
    
    return (time() < last_time);
}

void set_cooldown(object player, string enc_id, int cooldown_time)
{
    mapping cooldowns;
    
    cooldowns = player->query_temp("encounter/cooldowns");
    if (!mapp(cooldowns))
        cooldowns = ([]);
    
    cooldowns[enc_id] = time() + cooldown_time;
    player->set_temp("encounter/cooldowns", cooldowns);
}
