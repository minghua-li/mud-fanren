// side_quest_d.c
// 支线任务守护进程 — 模板注册、触发检测、进度追踪、奖励发放
//
// 职责：
//   1. 支线任务模板注册与管理（5种支线类型）
//   2. 触发条件检测（NPC对话/区域进入/物品持有/声望/任务链）
//   3. 玩家支线进度追踪
//   4. 境界缩放奖励计算
//   5. 分支链支持
//
// 关联设计文档：02-扩充内容/02-任务链与奖励曲线.md 第三章
// 接 quest_chain_d 任务链框架

#include <ansi.h>
#include <side_quest.h>
#include <quest_chain.h>

inherit F_DBASE;

// ─── 全局数据 ───

// 支线模板注册表：quest_id → mapping 完整模板
nosave mapping side_templates = ([]);

// 支线触发索引：trigger_type → ({ quest_id, ... })
nosave mapping trigger_index = ([]);

// ─── 方法声明 ───

void create();
int clean_up();

// 模板管理
int register_side_quest(mapping template);
int unregister_side_quest(string quest_id);
mapping query_side_quest(string quest_id);
mapping *query_side_quests_by_type(int side_type);
mapping *query_side_quests_by_area(string area);
string *list_all_side_quests();

// 触发检测
mapping *detect_triggers(object player, int trigger_type, mixed trigger_data);
int check_trigger_conditions(mapping trigger, object player);
int try_assign_side_quest(object player, string quest_id);

// 玩家操作
int assign_side_quest(object player, string quest_id);
int complete_side_quest(object player, string quest_id);
int abandon_side_quest(object player, string quest_id);
int update_side_quest_progress(object player, string quest_id, string obj_key, int amount);

// 奖励计算
int calc_side_exp_reward(object player, mapping template);
int calc_side_coin_reward(object player, mapping template);
mapping calc_side_realm_rewards(int realm_index);

// 查询
mapping get_player_side_quests(object player);
int has_active_side_quest(object player, string quest_id);
int is_side_quest_completed(object player, string quest_id);
int get_side_quest_count(object player);
int get_player_realm_index(object player);

// ═══════════════════════════════════════════
//  初始化
// ═══════════════════════════════════════════

void create()
{
    seteuid(ROOT_UID);
    set("channel_id", "支线任务系统");
    
    side_templates = ([]);
    trigger_index = ([]);
    
    // 注册预定义的支线模板
    register_default_templates();
    
    CHANNEL_D->do_channel(this_object(), "sys", "支线任务系统已经启动。");
}

int clean_up()
{
    return 1;
}

// ═══════════════════════════════════════════
//  预定义支线模板
// ═══════════════════════════════════════════

void register_default_templates()
{
    // ── 角色支线: 墨府恩怨 ──
    register_side_quest(([
        "id":           "side_mansion_enmity",
        "name":         "墨府恩怨",
        "type":         SIDE_CHARACTER,
        "trigger_type": TRIGGER_NPC_DIALOG,
        "trigger_data": ([
            "npc":        "墨府管家",
            "area":       "嘉元城",
        ]),
        "realm_range":  ({ 0, 3 }),          // 凡人~炼气后期
        "prerequisites":([
            "min_realm": ({ "炼气", "7层" }),
            "max_realm": ({ "筑基", "后期" }),
            "reputation": ([ "嘉元城": 100 ]),
        ]),
        "objectives":   ({
            ([ "type": OBJ_TALK, "target": "墨府管家", "amount": 1 ]),
            ([ "type": OBJ_TALK, "target": "墨夫人", "amount": 1 ]),
            ([ "type": OBJ_KILL, "target": "墨府叛徒", "amount": 1 ]),
        }),
        "rewards":      ([
            "exp":    SIDE_EXP_QI * 2,
            "coin":   SIDE_COIN_QI * 2,
            "reputation": ({ ([ "faction": "嘉元城", "value": 30 ]) }),
        ]),
        "description":  "墨府内部暗流涌动，墨管家似乎在谋划什么。你需要深入调查此事。",
        "chain_info":   ([
            "pre_quest":   0,
            "next_quests": ({ "side_mansion_girl_1", "side_mansion_girl_2" }),
            "branch_point": 1,
        ]),
    ]));

    // ── 场景支线: 七玄门密室 ──
    register_side_quest(([
        "id":           "side_qixuan_secret",
        "name":         "七玄门密室",
        "type":         SIDE_SCENE,
        "trigger_type": TRIGGER_AREA_ENTER,
        "trigger_data": ([
            "area":       "七玄门",
        ]),
        "realm_range":  ({ 0, 1 }),
        "prerequisites":([
            "min_level": 1,
        ]),
        "objectives":   ({
            ([ "type": OBJ_EXPLORE, "target": "七玄门密室", "amount": 1 ]),
            ([ "type": OBJ_COLLECT, "target": "密室秘籍", "amount": 1 ]),
        }),
        "rewards":      ([
            "exp":    SIDE_EXP_QI,
            "coin":   SIDE_COIN_QI,
            "items":  ({ "/clone/book/qixuan_manual" }),
        ]),
        "description":  "传闻七玄门内有一处隐秘的密室，藏有历代门主留下的功法秘籍。",
    ]));

    // ── 收集支线: 灵药大全 ──
    register_side_quest(([
        "id":           "side_herb_collection",
        "name":         "灵药大全",
        "type":         SIDE_COLLECTION,
        "trigger_type": TRIGGER_ITEM_HOLD,
        "trigger_data": ([
            "item":       "/clone/herb/ling_zhi",
        ]),
        "realm_range":  ({ 0, 16 }),
        "prerequisites":([
            "min_level": 1,
        ]),
        "objectives":   ({
            ([ "type": OBJ_COLLECT, "target": "十年灵药", "amount": 10 ]),
            ([ "type": OBJ_COLLECT, "target": "百年灵药", "amount": 5 ]),
            ([ "type": OBJ_COLLECT, "target": "千年灵药", "amount": 1 ]),
        }),
        "rewards":      ([
            "exp":    SIDE_EXP_QI * 3,
            "coin":   SIDE_COIN_QI * 5,
            "title":  "百草通",
        ]),
        "description":  "一位游方道士委托你收集各种灵药，要求品种齐全。",
    ]));

    // ── 门派支线: 黄枫谷巡查 ──
    register_side_quest(([
        "id":           "side_huangfeng_patrol",
        "name":         "黄枫谷巡查",
        "type":         SIDE_FACTION,
        "trigger_type": TRIGGER_REPUTATION,
        "trigger_data": ([
            "faction":    "黄枫谷",
            "min_rep":    200,
        ]),
        "realm_range":  ({ 1, 6 }),
        "prerequisites":([
            "min_level": 7,
        ]),
        "objectives":   ({
            ([ "type": OBJ_KILL, "target": "入侵妖兽", "amount": 5 ]),
            ([ "type": OBJ_REACH, "target": "黄枫谷山门", "amount": 1 ]),
        }),
        "rewards":      ([
            "exp":    SIDE_EXP_ZHU,
            "coin":   SIDE_COIN_ZHU,
            "reputation": ({ ([ "faction": "黄枫谷", "value": 50 ]) }),
        ]),
        "description":  "黄枫谷最近频繁遭到妖兽侵扰，需要弟子巡查山门。",
    ]));

    // ── 场景支线: 血色禁地前奏 ──
    register_side_quest(([
        "id":           "side_blood_red_prelude",
        "name":         "血色禁地前奏",
        "type":         SIDE_SCENE,
        "trigger_type": TRIGGER_NPC_DIALOG,
        "trigger_data": ([
            "npc":        "李化元",
            "area":       "黄枫谷",
        ]),
        "realm_range":  ({ 1, 4 }),
        "prerequisites":([
            "min_level": 7,
            "quests":    ({ "node_1_1" }),
        ]),
        "objectives":   ({
            ([ "type": OBJ_COLLECT, "target": "禁地情报", "amount": 3 ]),
            ([ "type": OBJ_TALK, "target": "禁地守卫", "amount": 1 ]),
        }),
        "rewards":      ([
            "exp":    SIDE_EXP_ZHU * 2,
            "coin":   SIDE_COIN_ZHU * 3,
            "items":  ({ "/clone/pill/huang_long_dan" }),
        ]),
        "description":  "血色禁地即将开启，李化元师叔需要你收集有关禁地的情报。",
    ]));
}

// ═══════════════════════════════════════════
//  模板管理
// ═══════════════════════════════════════════

int register_side_quest(mapping template)
{
    string quest_id;
    int trigger_type;
    
    if (!mapp(template))
        return 0;
    
    quest_id = template["id"];
    if (!stringp(quest_id) || quest_id == "")
        return 0;
    
    if (!template["name"] || !template["type"])
        return 0;
    
    side_templates[quest_id] = template;
    
    // 建立触发索引
    trigger_type = template["trigger_type"];
    if (trigger_type)
    {
        if (!trigger_index[trigger_type])
            trigger_index[trigger_type] = ({});
        if (member_array(quest_id, trigger_index[trigger_type]) == -1)
            trigger_index[trigger_type] += ({ quest_id });
    }
    
    return 1;
}

int unregister_side_quest(string quest_id)
{
    mapping template;
    int trigger_type;
    
    template = side_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    // 从触发索引移除
    trigger_type = template["trigger_type"];
    if (trigger_type && trigger_index[trigger_type])
    {
        string *list = trigger_index[trigger_type];
        int idx = member_array(quest_id, list);
        if (idx != -1)
            trigger_index[trigger_type] = list[0..idx-1] + list[idx+1..<1];
    }
    
    map_delete(side_templates, quest_id);
    return 1;
}

mapping query_side_quest(string quest_id)
{
    return side_templates[quest_id];
}

mapping *query_side_quests_by_type(int side_type)
{
    mapping *result = ({});
    string *ids;
    int i;
    
    ids = keys(side_templates);
    for (i = 0; i < sizeof(ids); i++)
    {
        if (side_templates[ids[i]]["type"] == side_type)
            result += ({ side_templates[ids[i]] });
    }
    return result;
}

mapping *query_side_quests_by_area(string area)
{
    mapping *result = ({});
    string *ids;
    int i;
    mapping trigger_data;
    
    ids = keys(side_templates);
    for (i = 0; i < sizeof(ids); i++)
    {
        trigger_data = side_templates[ids[i]]["trigger_data"];
        if (mapp(trigger_data) && trigger_data["area"] == area)
            result += ({ side_templates[ids[i]] });
    }
    return result;
}

string *list_all_side_quests()
{
    return keys(side_templates);
}

// ═══════════════════════════════════════════
//  触发检测
// ═══════════════════════════════════════════

// 检测某类触发条件下是否有可派发的支线任务
// 返回可接取的支线模板列表
mapping *detect_triggers(object player, int trigger_type, mixed trigger_data)
{
    mapping *results = ({});
    string *candidates;
    int i;
    
    if (!objectp(player))
        return ({});
    
    candidates = trigger_index[trigger_type];
    if (!arrayp(candidates))
        return ({});
    
    for (i = 0; i < sizeof(candidates); i++)
    {
        mapping tmpl = side_templates[candidates[i]];
        if (!mapp(tmpl))
            continue;
        
        // 检查触发数据是否匹配
        mapping td = tmpl["trigger_data"];
        if (mapp(td) && mapp(trigger_data))
        {
            int match = 1;
            string *td_keys = keys(trigger_data);
            int j;
            for (j = 0; j < sizeof(td_keys); j++)
            {
                if (td[td_keys[j]] != trigger_data[td_keys[j]])
                {
                    match = 0;
                    break;
                }
            }
            if (!match)
                continue;
        }
        
        // 检查玩家状态（是否可接取）
        if (!try_assign_side_quest(player, candidates[i]))
            continue;
        
        results += ({ tmpl });
    }
    
    return results;
}

// 触发点：玩家与NPC对话时调用
// 返回可触发的支线任务ID列表
string *on_npc_talk(object player, string npc_name, string area)
{
    mapping *triggered;
    string *result = ({});
    int i;
    
    triggered = detect_triggers(player, TRIGGER_NPC_DIALOG, ([
        "npc":  npc_name,
        "area": area,
    ]));
    
    for (i = 0; i < sizeof(triggered); i++)
    {
        if (assign_side_quest(player, triggered[i]["id"]))
            result += ({ triggered[i]["id"] });
    }
    
    return result;
}

// 触发点：玩家进入区域时调用
string *on_area_enter(object player, string area_name)
{
    mapping *triggered;
    string *result = ({});
    int i;
    
    triggered = detect_triggers(player, TRIGGER_AREA_ENTER, ([
        "area": area_name,
    ]));
    
    for (i = 0; i < sizeof(triggered); i++)
    {
        if (assign_side_quest(player, triggered[i]["id"]))
            result += ({ triggered[i]["id"] });
    }
    
    return result;
}

// 触发点：玩家获得物品时调用
string *on_item_obtain(object player, string item_id)
{
    mapping *triggered;
    string *result = ({});
    int i;
    
    triggered = detect_triggers(player, TRIGGER_ITEM_HOLD, ([
        "item": item_id,
    ]));
    
    for (i = 0; i < sizeof(triggered); i++)
    {
        if (assign_side_quest(player, triggered[i]["id"]))
            result += ({ triggered[i]["id"] });
    }
    
    return result;
}

// 检查玩家是否可接取某个支线任务
int try_assign_side_quest(object player, string quest_id)
{
    mapping template;
    mapping prereqs;
    
    template = side_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    // 检查是否已完成（一次性）
    if (is_side_quest_completed(player, quest_id))
        return 0;
    
    // 检查是否已在活跃中
    if (has_active_side_quest(player, quest_id))
        return 0;
    
    // 检查活跃数量上限
    if (get_side_quest_count(player) >= SIDE_CONCURRENT_MAX)
        return 0;
    
    // 检查前置条件
    prereqs = template["prerequisites"];
    if (mapp(prereqs))
    {
        // 检查链前置
        mapping chain_info = template["chain_info"];
        if (mapp(chain_info) && chain_info["pre_quest"])
        {
            string pre = chain_info["pre_quest"];
            if (!is_side_quest_completed(player, pre))
                return 0;
        }
        
        // 检查境界范围
        int *range = template["realm_range"];
        if (arrayp(range) && sizeof(range) >= 2)
        {
            int p_realm = get_player_realm_index(player);
            if (p_realm < range[0] || p_realm > range[1])
                return 0;
        }
    }
    
    return 1;
}

// ═══════════════════════════════════════════
//  玩家操作
// ═══════════════════════════════════════════

int assign_side_quest(object player, string quest_id)
{
    mapping template;
    mapping active;
    mapping quest_state;
    
    if (!objectp(player) || !userp(player))
        return 0;
    
    template = side_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    if (!try_assign_side_quest(player, quest_id))
        return 0;
    
    // 初始化玩家数据
    if (!mapp(player->query(SIDE_QUEST_ACTIVE)))
        player->set(SIDE_QUEST_ACTIVE, ([]));
    
    active = player->query(SIDE_QUEST_ACTIVE);
    
    // 初始化任务状态
    quest_state = ([
        "status":      SIDE_STATUS_ACTIVE,
        "start_time":  time(),
        "progress":    ([]),
    ]);
    
    // 初始化每个目标的进度
    if (arrayp(template["objectives"]))
    {
        int i;
        for (i = 0; i < sizeof(template["objectives"]); i++)
        {
            quest_state["progress"]["obj_" + i] = 0;
        }
    }
    
    active[quest_id] = quest_state;
    player->set(SIDE_QUEST_ACTIVE, active);
    
    tell_object(player, sprintf(
        HIC "\n╔══════════════════════════════╗\n" NOR
        HIC "║       新 支 线 任 务         ║\n" NOR
        HIC "╚══════════════════════════════╝\n" NOR
        HIW "【%s】\n" NOR
        "%s\n\n"
        "输入 " HIG "questlog" NOR " 查看任务详情。\n",
        template["name"],
        template["description"]));
    
    return 1;
}

int complete_side_quest(object player, string quest_id)
{
    mapping template;
    mapping active;
    mapping completed;
    mapping quest_state;
    mapping rewards;
    string *next_quests;
    int i;
    
    if (!objectp(player))
        return 0;
    
    template = side_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    active = player->query(SIDE_QUEST_ACTIVE);
    if (!mapp(active) || !active[quest_id])
        return 0;
    
    quest_state = active[quest_id];
    quest_state["status"] = SIDE_STATUS_COMPLETED;
    quest_state["end_time"] = time();
    
    // 从活跃列表移除
    map_delete(active, quest_id);
    player->set(SIDE_QUEST_ACTIVE, active);
    
    // 加入已完成列表
    if (!mapp(player->query(SIDE_QUEST_COMPLETED)))
        player->set(SIDE_QUEST_COMPLETED, ([]));
    completed = player->query(SIDE_QUEST_COMPLETED);
    completed[quest_id] = time();
    player->set(SIDE_QUEST_COMPLETED, completed);
    
    // 发放奖励
    rewards = template["rewards"];
    if (mapp(rewards))
    {
        int exp_reward = calc_side_exp_reward(player, template);
        int coin_reward = calc_side_coin_reward(player, template);
        
        if (exp_reward > 0)
            player->add("combat_exp", exp_reward);
        
        if (coin_reward > 0)
        {
            object coin = new("/clone/money/coin");
            if (coin)
            {
                coin->set_amount(coin_reward);
                coin->move(player);
            }
        }
        
        // 声望奖励
        if (arrayp(rewards["reputation"]))
        {
            for (i = 0; i < sizeof(rewards["reputation"]); i++)
            {
                mapping rep_entry = rewards["reputation"][i];
                // 通过声望系统发放（简化：直接记录）
                string faction = rep_entry["faction"];
                int value = rep_entry["value"];
                player->add("reputation/" + faction, value);
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
        }
        
        tell_object(player, sprintf(
            HIG "\n支线任务【%s】完成！\n" NOR
            "获得经验：%d，灵石：%d\n",
            template["name"], exp_reward, coin_reward));
    }
    
    // 分支链处理：解锁后续支线
    mapping chain_info = template["chain_info"];
    if (mapp(chain_info) && chain_info["branch_point"])
    {
        next_quests = chain_info["next_quests"];
        if (arrayp(next_quests))
        {
            tell_object(player, sprintf(
                HIY "支线出现新的分支：%d 个后续任务已解锁。\n" NOR,
                sizeof(next_quests)));
        }
    }
    
    return 1;
}

int abandon_side_quest(object player, string quest_id)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    active = player->query(SIDE_QUEST_ACTIVE);
    if (!mapp(active) || !active[quest_id])
        return 0;
    
    map_delete(active, quest_id);
    player->set(SIDE_QUEST_ACTIVE, active);
    
    // 标记为失败
    mapping completed = player->query(SIDE_QUEST_COMPLETED);
    if (!mapp(completed))
        completed = ([]);
    completed[quest_id] = -1;  // -1 表示放弃/失败
    player->set(SIDE_QUEST_COMPLETED, completed);
    
    tell_object(player, sprintf("你放弃了支线任务。\n"));
    return 1;
}

// 更新支线任务进度
int update_side_quest_progress(object player, string quest_id, string obj_key, int amount)
{
    mapping active;
    mapping quest_state;
    mapping progress;
    mapping template;
    mapping *objectives;
    int i, all_done;
    
    if (!objectp(player))
        return 0;
    
    // 检查主线用 complete 完成的支线
    active = player->query(SIDE_QUEST_ACTIVE);
    if (!mapp(active) || !active[quest_id])
        return 0;
    
    quest_state = active[quest_id];
    progress = quest_state["progress"];
    
    if (!mapp(progress))
        progress = ([]);
    
    // 更新进度
    int current = progress[obj_key];
    if (!current) current = 0;
    progress[obj_key] = current + amount;
    quest_state["progress"] = progress;
    active[quest_id] = quest_state;
    player->set(SIDE_QUEST_ACTIVE, active);
    
    // 检查是否全部完成
    template = side_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    objectives = template["objectives"];
    if (!arrayp(objectives))
        return 0;
    
    all_done = 1;
    for (i = 0; i < sizeof(objectives); i++)
    {
        string key = "obj_" + i;
        int current_val = progress[key];
        int required = objectives[i]["amount"];
        
        if (current_val < required)
        {
            all_done = 0;
            break;
        }
    }
    
    if (all_done)
    {
        tell_object(player, sprintf(
            HIG "支线任务【%s】的所有目标已完成！\n" NOR
            "返回任务发布人处提交任务。\n",
            template["name"]));
        
        // 标记为可提交状态
        quest_state["status"] = SIDE_STATUS_COMPLETED;
        active[quest_id] = quest_state;
        player->set(SIDE_QUEST_ACTIVE, active);
        return 2;  // 可提交
    }
    
    return 1;  // 进行中
}

// ═══════════════════════════════════════════
//  奖励计算
// ═══════════════════════════════════════════

int calc_side_exp_reward(object player, mapping template)
{
    int *range;
    int player_realm;
    int quest_mid;
    int diff;
    float scale;
    int base_exp;
    
    range = template["realm_range"];
    if (!arrayp(range) || sizeof(range) < 2)
        return 100;
    
    player_realm = get_player_realm_index(player);
    quest_mid = (range[0] + range[1]) / 2;
    base_exp = template["rewards"]["exp"];
    if (!base_exp) base_exp = 100;
    
    // 境界缩放
    if (player_realm <= quest_mid)
    {
        scale = 1.0;
    }
    else
    {
        diff = player_realm - quest_mid;
        // 每超过1级衰减20%
        scale = 1.0;
        while (diff > 0)
        {
            scale *= 0.8;
            diff--;
        }
        if (scale < SIDE_REWARD_DECAY_MIN)
            scale = SIDE_REWARD_DECAY_MIN;
    }
    
    return to_int(base_exp * scale);
}

int calc_side_coin_reward(object player, mapping template)
{
    int exp_reward;
    mapping rewards;
    int coin;
    
    rewards = template["rewards"];
    if (!mapp(rewards))
        return 0;
    
    coin = rewards["coin"];
    if (!coin) coin = 0;
    
    // 按经验比例计算灵石
    exp_reward = calc_side_exp_reward(player, template);
    if (coin <= 0)
        coin = to_int(exp_reward * 0.2);
    
    return coin;
}

// 根据境界索引获取对应档位的奖励基准
mapping calc_side_realm_rewards(int realm_index)
{
    switch (realm_index)
    {
    case 0:  // 炼气
    case 1:
    case 2:
    case 3:
        return ([ "exp": SIDE_EXP_QI, "coin": SIDE_COIN_QI, "special": SIDE_SPECIAL_QI ]);
    case 4:  // 筑基
    case 5:
    case 6:
        return ([ "exp": SIDE_EXP_ZHU, "coin": SIDE_COIN_ZHU, "special": SIDE_SPECIAL_ZHU ]);
    case 7:  // 结丹
    case 8:
    case 9:
        return ([ "exp": SIDE_EXP_JIE, "coin": SIDE_COIN_JIE, "special": SIDE_SPECIAL_JIE ]);
    case 10: // 元婴
    case 11:
    case 12:
        return ([ "exp": SIDE_EXP_YING, "coin": SIDE_COIN_YING, "special": SIDE_SPECIAL_YING ]);
    default: // 化神+
        return ([ "exp": SIDE_EXP_HUA, "coin": SIDE_COIN_HUA, "special": SIDE_SPECIAL_HUA ]);
    }
}

// ═══════════════════════════════════════════
//  查询
// ═══════════════════════════════════════════

mapping get_player_side_quests(object player)
{
    if (!objectp(player))
        return ([]);
    return player->query(SIDE_QUEST_ACTIVE);
}

int has_active_side_quest(object player, string quest_id)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    active = player->query(SIDE_QUEST_ACTIVE);
    if (!mapp(active))
        return 0;
    
    return mapp(active[quest_id]);
}

int is_side_quest_completed(object player, string quest_id)
{
    mapping completed;
    
    if (!objectp(player))
        return 0;
    
    completed = player->query(SIDE_QUEST_COMPLETED);
    if (!mapp(completed))
        return 0;
    
    return (completed[quest_id] > 0);
}

int get_side_quest_count(object player)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    active = player->query(SIDE_QUEST_ACTIVE);
    if (!mapp(active))
        return 0;
    
    return sizeof(active);
}

// 获取玩家境界索引（与 quest_chain_d 保持兼容）
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
