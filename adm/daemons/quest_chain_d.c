// quest_chain_d.c
// 任务链守护进程 — 任务链框架与奖励曲线系统
//
// 职责：
//   1. 任务链框架：串行/分支/并行/条件/限时 5种链式触发与进度追踪
//   2. 奖励曲线系统：经验/灵石/声望奖励随境界、任务等级、链长度动态计算
//
// 关联设计文档：02-扩充内容/02-任务链与奖励曲线.md
// 接 quests/1G-任务副本奇遇

#include <ansi.h>
#include <quest_chain.h>

inherit F_DBASE;

// ─── 全局数据 ───

// 任务模板注册表：quest_id → mapping 完整模板
nosave mapping quest_templates = ([]);

// 任务链注册表：chain_id → mapping 链定义
nosave mapping chain_registry = ([]);

// 玩家在线状态缓存：player_id → mapping 活跃任务状态
// 持久化通过玩家自身的 quest_chain 属性存储
nosave mapping player_cache = ([]);

// 日常任务池缓存：realm_index → ({ quest_id, ... })
nosave mapping daily_pool_cache = ([]);

// 周常任务池缓存
nosave mapping weekly_pool_cache = ([]);

// ─── 方法声明 ───

// === 初始化 ===
void create();
int clean_up();

// === 任务模板注册与管理 ===
int register_quest(mapping template);
int unregister_quest(string quest_id);
mapping query_quest(string quest_id);
mapping *query_quests_by_type(int quest_type);
mapping *query_quests_by_realm(int realm_index);
string *list_all_quests();

// === 任务链管理 ===
int register_chain(string chain_id, int chain_type, string *quest_ids, mapping chain_data);
mapping query_chain(string chain_id);
string *get_chain_quests(string chain_id);
string get_next_chain_quest(string chain_id, string current_quest_id, mapping player_state);
string *get_branch_options(string chain_id, string branch_quest_id, mapping player_state);
int is_chain_completed(string chain_id, mapping player_state);
int get_chain_progress(string chain_id, mapping player_state);
int get_chain_total(string chain_id);

// === 玩家任务操作 ===
int assign_quest(object player, string quest_id);
int complete_quest(object player, string quest_id);
int abandon_quest(object player, string quest_id);
int check_quest_progress(object player, string quest_id);
mapping get_player_quests(object player);
mapping get_player_active_quest(object player, string quest_id);
int has_active_quest(object player);
int get_active_quest_count(object player);

// === 日常/周常刷新 ===
mapping get_daily_pool(int realm_index);
mapping get_weekly_pool(int realm_index);
int refresh_daily();
int refresh_weekly();
int check_daily_limit(object player);
int check_weekly_limit(object player);
int check_daily_same_type_limit(object player, string group);
int get_daily_max(int realm_index);
int get_daily_rare_chance(int realm_index);

// === 奖励曲线计算 ===
int calc_exp_reward(object player, mapping quest_template);
int calc_coin_reward(object player, mapping quest_template);
int calc_reputation_reward(object player, mapping quest_template, string faction);
float calc_realm_reward_scale(object player, mapping quest_template);
float calc_daily_bonus(int consecutive_days);
float calc_chain_length_bonus(string chain_id, mapping player_state);
float calc_quality_coefficient(int quality);
int calc_realm_base(int realm_index);
int calc_drop_probability(object player, mapping quest_template, string item_type);

// === 内部工具 ===
int get_player_realm_index(object player);
int is_quest_available(string quest_id, object player);
int check_prerequisites(mapping prereqs, object player);
void save_player_quest_state(object player);
void load_player_quest_state(object player);

// ═══════════════════════════════════════════
//  初始化
// ═══════════════════════════════════════════

void create()
{
    seteuid(ROOT_UID);
    set("channel_id", "任务链系统");
    
    quest_templates = ([]);
    chain_registry = ([]);
    daily_pool_cache = ([]);
    weekly_pool_cache = ([]);
    
    CHANNEL_D->do_channel(this_object(), "sys", "任务链系统已经启动。");
}

int clean_up()
{
    return 1;
}

// ═══════════════════════════════════════════
//  任务模板注册与管理
// ═══════════════════════════════════════════

// 注册一个任务模板
// template 格式：
//   ([
//     "id":           "quest_xxx",
//     "name":         "任务名",
//     "type":         QUEST_TYPE_*,
//     "chain_type":   CHAIN_*,           // 可选
//     "chain_id":     "chain_xxx",       // 所属链id
//     "realm_range":  ({ min_idx, max_idx }),  // 境界范围
//     "prerequisites":([ ... ]),         // 前置条件
//     "objectives":   ({
//       ([ "type": OBJ_*, "target": "...", "amount": N ]),
//     }),
//     "rewards":      ([ ... ]),         // 奖励定义
//     "quality":      QUEST_QUALITY_*,   // 品质（日常用）
//     "group":        QUEST_GROUP_*,     // 分组
//     "refresh":      REFRESH_*,         // 刷新类型
//     "daily_limit":  N,                 // 每日次数
//     "time_limit":   N,                 // 限时(秒)
//     "description":  "任务描述",
//   ])
int register_quest(mapping template)
{
    string quest_id;
    
    if (!mapp(template))
        return 0;
    
    quest_id = template["id"];
    if (!stringp(quest_id) || quest_id == "")
        return 0;
    
    // 验证必要字段
    if (!template["name"] || !template["type"])
        return 0;
    
    quest_templates[quest_id] = template;
    return 1;
}

// 注销任务模板
int unregister_quest(string quest_id)
{
    if (!quest_templates[quest_id])
        return 0;
    
    map_delete(quest_templates, quest_id);
    return 1;
}

// 查询任务模板
mapping query_quest(string quest_id)
{
    return quest_templates[quest_id];
}

// 按类型查询任务模板
mapping *query_quests_by_type(int quest_type)
{
    mapping *result = ({});
    string *ids;
    int i;
    
    ids = keys(quest_templates);
    for (i = 0; i < sizeof(ids); i++)
    {
        if (quest_templates[ids[i]]["type"] == quest_type)
            result += ({ quest_templates[ids[i]] });
    }
    return result;
}

// 按境界查询任务模板
mapping *query_quests_by_realm(int realm_index)
{
    mapping *result = ({});
    string *ids;
    int i;
    mapping t;
    int *range;
    
    ids = keys(quest_templates);
    for (i = 0; i < sizeof(ids); i++)
    {
        t = quest_templates[ids[i]];
        range = t["realm_range"];
        if (arrayp(range) && sizeof(range) >= 2)
        {
            if (realm_index >= range[0] && realm_index <= range[1])
                result += ({ t });
        }
    }
    return result;
}

// 列出所有任务ID
string *list_all_quests()
{
    return keys(quest_templates);
}

// ═══════════════════════════════════════════
//  任务链管理
// ═══════════════════════════════════════════

// 注册任务链
// chain_data 包含链的特定属性，如：
//   serial:    ([ "ordered": 1 ])
//   branch:    ([ "branches": ([ "quest_a": ({ "option1", "option2" }) ]) ])
//   parallel:  ([ "allow_concurrent": 1 ])
//   conditional: ([ "conditions": ([ "quest_id": ([ ... ]) ]) ])
//   timed:     ([ "time_limit": 86400 ])
int register_chain(string chain_id, int chain_type, string *quest_ids, mapping chain_data)
{
    if (!stringp(chain_id) || chain_id == "")
        return 0;
    if (!arrayp(quest_ids) || sizeof(quest_ids) < 1)
        return 0;
    if (!mapp(chain_data))
        chain_data = ([]);
    
    chain_registry[chain_id] = ([
        "id":         chain_id,
        "type":       chain_type,
        "quests":     quest_ids,
        "data":       chain_data,
        "total":      sizeof(quest_ids),
    ]);
    
    return 1;
}

// 查询链
mapping query_chain(string chain_id)
{
    return chain_registry[chain_id];
}

// 获取链内所有任务ID
string *get_chain_quests(string chain_id)
{
    mapping chain;
    chain = chain_registry[chain_id];
    if (!mapp(chain))
        return ({});
    return chain["quests"];
}

// 获取串行链的下一个任务
// current_quest_id 为当前任务ID，null 表示获取第一个
string get_next_chain_quest(string chain_id, string current_quest_id, mapping player_state)
{
    mapping chain;
    string *quests;
    int idx;
    
    chain = chain_registry[chain_id];
    if (!mapp(chain))
        return 0;
    
    quests = chain["quests"];
    if (chain["type"] != CHAIN_SERIAL && chain["type"] != CHAIN_TIMED)
    {
        // 非串行链需要特殊处理
        if (chain["type"] == CHAIN_CONDITIONAL)
        {
            // 条件链：检查所有未完成且条件满足的任务
            int i;
            for (i = 0; i < sizeof(quests); i++)
            {
                if (!player_state[quests[i]] || player_state[quests[i]] != QUEST_STATUS_COMPLETED)
                {
                    mapping prereqs;
                    mapping conds = chain["data"]["conditions"];
                    if (mapp(conds) && conds[quests[i]])
                    {
                        prereqs = conds[quests[i]];
                        // 简化检查：如果玩家有prereq条件中指定的属性则通过
                        // 具体检查逻辑由 check_prerequisites 实现
                        if (check_prerequisites(prereqs, 0)) // 0 = 占位，实际应有player
                            return quests[i];
                    }
                    else
                    {
                        return quests[i];
                    }
                }
            }
            return 0;
        }
        return 0;
    }
    
    // 串行链或限时链
    if (!stringp(current_quest_id))
    {
        // 没有当前任务，返回第一个
        return quests[0];
    }
    
    idx = member_array(current_quest_id, quests);
    if (idx == -1)
        return 0;
    
    if (idx + 1 < sizeof(quests))
        return quests[idx + 1];
    
    return 0; // 链已结束
}

// 获取分支链当前节点的可选分支
string *get_branch_options(string chain_id, string branch_quest_id, mapping player_state)
{
    mapping chain;
    mapping branches;
    mixed options;
    
    chain = chain_registry[chain_id];
    if (!mapp(chain))
        return ({});
    
    if (chain["type"] != CHAIN_BRANCH)
        return ({});
    
    branches = chain["data"]["branches"];
    if (!mapp(branches))
        return ({});
    
    options = branches[branch_quest_id];
    if (arrayp(options))
        return options;
    
    return ({});
}

// 判断链是否完成
int is_chain_completed(string chain_id, mapping player_state)
{
    mapping chain;
    string *quests;
    int i;
    
    chain = chain_registry[chain_id];
    if (!mapp(chain))
        return 0;
    
    quests = chain["quests"];
    for (i = 0; i < sizeof(quests); i++)
    {
        if (!player_state[quests[i]] || player_state[quests[i]] != QUEST_STATUS_COMPLETED)
            return 0;
    }
    return 1;
}

// 获取链完成进度（已完成数量）
int get_chain_progress(string chain_id, mapping player_state)
{
    mapping chain;
    string *quests;
    int i, count;
    
    chain = chain_registry[chain_id];
    if (!mapp(chain))
        return 0;
    
    quests = chain["quests"];
    count = 0;
    for (i = 0; i < sizeof(quests); i++)
    {
        if (player_state[quests[i]] && player_state[quests[i]] == QUEST_STATUS_COMPLETED)
            count++;
    }
    return count;
}

// 获取链总任务数
int get_chain_total(string chain_id)
{
    mapping chain;
    chain = chain_registry[chain_id];
    if (!mapp(chain))
        return 0;
    return chain["total"];
}

// ═══════════════════════════════════════════
//  玩家任务操作
// ═══════════════════════════════════════════

// 派发任务给玩家
int assign_quest(object player, string quest_id)
{
    mapping template;
    mapping active;
    string *active_ids;
    int count;
    
    if (!objectp(player) || !userp(player))
        return 0;
    
    if (!stringp(quest_id))
        return 0;
    
    template = quest_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    // 检查任务是否可用
    if (!is_quest_available(quest_id, player))
        return 0;
    
    // 检查同时持有上限
    if (get_active_quest_count(player) >= DAILY_CONCURRENT_MAX)
        return 0;
    
    // 加载玩家任务状态
    load_player_quest_state(player);
    
    // 检查是否已有该任务
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (mapp(active) && active[quest_id])
        return 0;
    
    // 检查日常/周常限额
    if (template["type"] == QUEST_TYPE_DAILY)
    {
        if (!check_daily_limit(player))
            return 0;
        if (!check_daily_same_type_limit(player, template["group"]))
            return 0;
    }
    if (template["type"] == QUEST_TYPE_WEEKLY)
    {
        if (!check_weekly_limit(player))
            return 0;
    }
    
    // 添加到活跃任务
    if (!mapp(active))
        active = ([]);
    
    active[quest_id] = ([
        "status":      QUEST_STATUS_ACTIVE,
        "start_time":  time(),
        "progress":    ([]),
        "chain_id":    template["chain_id"],
    ]);
    
    player->set(QUEST_CHAIN_ACTIVE, active);
    save_player_quest_state(player);
    
    return 1;
}

// 完成任务
int complete_quest(object player, string quest_id)
{
    mapping active;
    mapping completed;
    mapping template;
    mapping quest_state;
    string chain_id;
    string next_quest;
    
    if (!objectp(player) || !userp(player))
        return 0;
    
    if (!stringp(quest_id))
        return 0;
    
    template = quest_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    load_player_quest_state(player);
    
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active) || !active[quest_id])
        return 0;
    
    quest_state = active[quest_id];
    chain_id = quest_state["chain_id"];
    
    // 从活跃任务移除
    map_delete(active, quest_id);
    player->set(QUEST_CHAIN_ACTIVE, active);
    
    // 加入已完成列表
    completed = player->query(QUEST_CHAIN_COMPLETED);
    if (!mapp(completed))
        completed = ([]);
    completed[quest_id] = time();
    player->set(QUEST_CHAIN_COMPLETED, completed);
    
    // 更新进度映射
    player->set(QUEST_CHAIN_PROGRESS + "/" + quest_id, QUEST_STATUS_COMPLETED);
    
    // 处理链式触发
    if (stringp(chain_id) && chain_registry[chain_id])
    {
        int chain_type = chain_registry[chain_id]["type"];
        
        if (chain_type == CHAIN_SERIAL || chain_type == CHAIN_TIMED)
        {
            // 串行链：自动接续下一个
            next_quest = get_next_chain_quest(chain_id, quest_id, player->query(QUEST_CHAIN_PROGRESS));
            if (stringp(next_quest) && next_quest != "")
            {
                // 自动派发下一个任务
                assign_quest(player, next_quest);
            }
        }
        else if (chain_type == CHAIN_PARALLEL)
        {
            // 并行链：检查所有并行任务是否完成
            if (is_chain_completed(chain_id, player->query(QUEST_CHAIN_PROGRESS)))
            {
                // 链完成，可以触发链完结奖励或事件
                // 奖励由调用方处理
            }
        }
    }
    
    // 日常统计
    if (template["type"] == QUEST_TYPE_DAILY)
    {
        player->add(QUEST_CHAIN_DAILY_COUNT, 1);
        // 更新连续完成天数
        int streak = player->query(QUEST_CHAIN_DAILY_STREAK);
        if (!streak)
            streak = 0;
        player->set(QUEST_CHAIN_DAILY_STREAK, streak + 1);
    }
    if (template["type"] == QUEST_TYPE_WEEKLY)
    {
        player->add(QUEST_CHAIN_WEEKLY_COUNT, 1);
    }
    
    save_player_quest_state(player);
    return 1;
}

// 放弃任务
int abandon_quest(object player, string quest_id)
{
    mapping active;
    mapping template;
    int abandon_count;
    
    if (!objectp(player) || !userp(player))
        return 0;
    
    if (!stringp(quest_id))
        return 0;
    
    template = quest_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    // 只有日常任务可放弃（其他类型不可放弃）
    if (template["type"] != QUEST_TYPE_DAILY)
        return 0;
    
    load_player_quest_state(player);
    
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active) || !active[quest_id])
        return 0;
    
    // 检查放弃次数限制
    abandon_count = player->query_temp("quest_chain/abandon_today");
    if (abandon_count >= DAILY_ABANDON_MAX)
        return 0;
    
    // 检查冷却时间
    int last_abandon = player->query_temp("quest_chain/last_abandon_time");
    if (last_abandon && time() - last_abandon < DAILY_ABANDON_CD)
        return 0;
    
    // 从活跃任务移除
    map_delete(active, quest_id);
    player->set(QUEST_CHAIN_ACTIVE, active);
    
    // 记录放弃
    player->set_temp("quest_chain/abandon_today", abandon_count + 1);
    player->set_temp("quest_chain/last_abandon_time", time());
    
    // 放弃日常会中断连续
    if (template["type"] == QUEST_TYPE_DAILY)
    {
        player->set(QUEST_CHAIN_DAILY_STREAK, 0);
    }
    
    save_player_quest_state(player);
    return 1;
}

// 检查任务进度
int check_quest_progress(object player, string quest_id)
{
    mapping active;
    mapping template;
    mapping objectives;
    mapping progress;
    int i, done;
    
    if (!objectp(player))
        return 0;
    
    load_player_quest_state(player);
    
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active))
        return 0;
    
    progress = active[quest_id]["progress"];
    template = quest_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    objectives = template["objectives"];
    if (!arrayp(objectives))
        return 1; // 无目标，默认完成
    
    // 检查每个目标是否达到数量要求
    done = 1;
    for (i = 0; i < sizeof(objectives); i++)
    {
        mapping obj = objectives[i];
        string obj_key = "obj_" + i;
        int current = progress[obj_key];
        int required = obj["amount"];
        
        if (current < required)
        {
            done = 0;
            break;
        }
    }
    
    return done;
}

// 获取玩家所有活跃任务
mapping get_player_quests(object player)
{
    if (!objectp(player))
        return ([]);
    
    load_player_quest_state(player);
    return player->query(QUEST_CHAIN_ACTIVE);
}

// 获取玩家指定活跃任务详情
mapping get_player_active_quest(object player, string quest_id)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    load_player_quest_state(player);
    
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active))
        return 0;
    
    return active[quest_id];
}

// 玩家是否有活跃任务
int has_active_quest(object player)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active))
        return 0;
    
    return sizeof(active) > 0;
}

// 获取活跃任务数量
int get_active_quest_count(object player)
{
    mapping active;
    
    if (!objectp(player))
        return 0;
    
    active = player->query(QUEST_CHAIN_ACTIVE);
    if (!mapp(active))
        return 0;
    
    return sizeof(active);
}

// ═══════════════════════════════════════════
//  日常/周常刷新
// ═══════════════════════════════════════════

// 获取指定境界的日常任务池
mapping get_daily_pool(int realm_index)
{
    mapping pool;
    string cache_key;
    int i;
    string *ids;
    mapping t;
    int *range;
    
    cache_key = "daily_" + realm_index;
    if (daily_pool_cache[cache_key])
        return daily_pool_cache[cache_key];
    
    pool = ([]);
    ids = keys(quest_templates);
    
    for (i = 0; i < sizeof(ids); i++)
    {
        t = quest_templates[ids[i]];
        if (t["type"] != QUEST_TYPE_DAILY)
            continue;
        
        range = t["realm_range"];
        if (arrayp(range) && sizeof(range) >= 2)
        {
            if (realm_index >= range[0] && realm_index <= range[1])
                pool[ids[i]] = t;
        }
    }
    
    daily_pool_cache[cache_key] = pool;
    return pool;
}

// 获取指定境界的周常任务池
mapping get_weekly_pool(int realm_index)
{
    mapping pool;
    string cache_key;
    int i;
    string *ids;
    mapping t;
    int *range;
    
    cache_key = "weekly_" + realm_index;
    if (weekly_pool_cache[cache_key])
        return weekly_pool_cache[cache_key];
    
    pool = ([]);
    ids = keys(quest_templates);
    
    for (i = 0; i < sizeof(ids); i++)
    {
        t = quest_templates[ids[i]];
        if (t["type"] != QUEST_TYPE_WEEKLY)
            continue;
        
        range = t["realm_range"];
        if (arrayp(range) && sizeof(range) >= 2)
        {
            if (realm_index >= range[0] && realm_index <= range[1])
                pool[ids[i]] = t;
        }
    }
    
    weekly_pool_cache[cache_key] = pool;
    return pool;
}

// 每日刷新（由外部定时器调用）
int refresh_daily()
{
    daily_pool_cache = ([]);
    
    // 遍历所有在线玩家，重置日常计数和连续天数
    object *users = users();
    int i;
    
    for (i = 0; i < sizeof(users); i++)
    {
        if (objectp(users[i]))
        {
            users[i]->set(QUEST_CHAIN_DAILY_COUNT, 0);
            users[i]->delete_temp("quest_chain/abandon_today");
            users[i]->delete_temp("quest_chain/last_abandon_time");
            
            // 重置日常相关活跃任务（将过期日常标记为过期）
            mapping active = users[i]->query(QUEST_CHAIN_ACTIVE);
            if (mapp(active))
            {
                string *a_ids = keys(active);
                int j;
                for (j = 0; j < sizeof(a_ids); j++)
                {
                    mapping t = quest_templates[a_ids[j]];
                    if (mapp(t) && t["type"] == QUEST_TYPE_DAILY)
                    {
                        map_delete(active, a_ids[j]);
                    }
                }
                users[i]->set(QUEST_CHAIN_ACTIVE, active);
            }
            
            users[i]->set(QUEST_CHAIN_DAILY_RESET, time());
            save_player_quest_state(users[i]);
        }
    }
    
    return 1;
}

// 每周刷新
int refresh_weekly()
{
    weekly_pool_cache = ([]);
    
    object *users = users();
    int i;
    
    for (i = 0; i < sizeof(users); i++)
    {
        if (objectp(users[i]))
        {
            users[i]->set(QUEST_CHAIN_WEEKLY_COUNT, 0);
            
            // 重置周常相关活跃任务
            mapping active = users[i]->query(QUEST_CHAIN_ACTIVE);
            if (mapp(active))
            {
                string *a_ids = keys(active);
                int j;
                for (j = 0; j < sizeof(a_ids); j++)
                {
                    mapping t = quest_templates[a_ids[j]];
                    if (mapp(t) && t["type"] == QUEST_TYPE_WEEKLY)
                    {
                        map_delete(active, a_ids[j]);
                    }
                }
                users[i]->set(QUEST_CHAIN_ACTIVE, active);
            }
            
            users[i]->set(QUEST_CHAIN_WEEKLY_RESET, time());
            save_player_quest_state(users[i]);
        }
    }
    
    return 1;
}

// 检查玩家当日日常限额
int check_daily_limit(object player)
{
    int daily_count;
    int max;
    int realm_idx;
    
    daily_count = player->query(QUEST_CHAIN_DAILY_COUNT);
    if (!daily_count) daily_count = 0;
    
    realm_idx = get_player_realm_index(player);
    max = get_daily_max(realm_idx);
    
    return daily_count < max;
}

// 检查玩家当周周常限额
int check_weekly_limit(object player)
{
    int weekly_count;
    
    weekly_count = player->query(QUEST_CHAIN_WEEKLY_COUNT);
    if (!weekly_count) weekly_count = 0;
    
    return weekly_count < 5; // 周常上限固定5个
}

// 检查同类日常限额
int check_daily_same_type_limit(object player, string group)
{
    // 简化为按组统计——实际实现需要追踪每组完成了多少
    // 这里让 feature/quest.c 或具体任务逻辑负责调用者检查
    return 1;
}

// 获取境界对应的每日可接任务数
int get_daily_max(int realm_index)
{
    switch (realm_index)
    {
    case 0: return DAILY_MAX_QI;      // 炼气
    case 1: return DAILY_MAX_ZHU;     // 筑基
    case 2: return DAILY_MAX_JIE;     // 结丹
    case 3: return DAILY_MAX_YING;    // 元婴
    default: return DAILY_MAX_HUA;    // 化神+
    }
}

// 获取境界对应的稀有日常概率(%)
int get_daily_rare_chance(int realm_index)
{
    switch (realm_index)
    {
    case 0: return DAILY_RARE_CHANCE_QI;
    case 1: return DAILY_RARE_CHANCE_ZHU;
    case 2: return DAILY_RARE_CHANCE_JIE;
    case 3: return DAILY_RARE_CHANCE_YING;
    default: return DAILY_RARE_CHANCE_HUA;
    }
}

// ═══════════════════════════════════════════
//  奖励曲线计算
// ═══════════════════════════════════════════

// 计算经验奖励
// 公式：基准值 × 境界系数 × 难度系数 × (1 + 连续天数 × 0.05) × 品质系数 × 链长加成
int calc_exp_reward(object player, mapping quest_template)
{
    float base;
    float realm_scale;
    float difficulty;
    float streak_bonus;
    float quality_coeff;
    float chain_bonus;
    float total;
    mapping rewards;
    int chain_len;
    string chain_id;
    
    if (!mapp(quest_template))
        return 0;
    
    rewards = quest_template["rewards"];
    if (!mapp(rewards))
        return 0;
    
    // 基础经验（从模板获取或日基准）
    base = rewards["exp"];
    if (!base) base = 0;
    
    // 境界缩放
    realm_scale = calc_realm_reward_scale(player, quest_template);
    
    // 难度系数（基于任务等级/类型）
    difficulty = 1.0;
    switch (quest_template["type"])
    {
    case QUEST_TYPE_MAIN:
        difficulty = 2.5;  // 主线是高奖励
        break;
    case QUEST_TYPE_SIDE:
        difficulty = 1.5;
        break;
    case QUEST_TYPE_DAILY:
        difficulty = 1.0;
        break;
    case QUEST_TYPE_WEEKLY:
        difficulty = 2.0;
        break;
    case QUEST_TYPE_ACHIEVEMENT:
        difficulty = 3.0;  // 成就是最高倍率
        break;
    case QUEST_TYPE_ENCOUNTER:
        difficulty = 2.0;
        break;
    case QUEST_TYPE_FACTION:
        difficulty = 1.2;
        break;
    }
    
    // 连续天数加成
    streak_bonus = calc_daily_bonus(player->query(QUEST_CHAIN_DAILY_STREAK));
    
    // 品质系数
    quality_coeff = calc_quality_coefficient(quest_template["quality"]);
    
    // 链长加成
    chain_id = quest_template["chain_id"];
    if (stringp(chain_id) && chain_registry[chain_id])
    {
        mapping player_progress = player->query(QUEST_CHAIN_PROGRESS);
        chain_bonus = calc_chain_length_bonus(chain_id, player_progress);
    }
    else
    {
        chain_bonus = 1.0;
    }
    
    total = base * realm_scale * difficulty * streak_bonus * quality_coeff * chain_bonus;
    
    // 确保最低值
    if (total < 1) total = 1;
    
    return to_int(total);
}

// 计算灵石奖励
// 公式：经验奖励 × (20%~30%)，最低 10 灵石
int calc_coin_reward(object player, mapping quest_template)
{
    int exp_reward;
    int coin;
    float ratio;
    mapping rewards;
    
    if (!mapp(quest_template))
        return 0;
    
    rewards = quest_template["rewards"];
    if (!mapp(rewards))
        return 0;
    
    // 如果有自定义灵石奖励
    coin = rewards["coin"];
    if (coin && coin > 0)
        return coin;
    
    // 默认：按经验奖励的比例计算
    exp_reward = calc_exp_reward(player, quest_template);
    ratio = COIN_RATIO_MIN + random(COIN_RATIO_MAX - COIN_RATIO_MIN + 1);
    
    coin = to_int(exp_reward * ratio / 100);
    
    if (coin < COIN_FLOOR)
        coin = COIN_FLOOR;
    
    return coin;
}

// 计算声望奖励
int calc_reputation_reward(object player, mapping quest_template, string faction)
{
    mapping rewards;
    mixed rep;
    int i;
    
    if (!mapp(quest_template))
        return 0;
    
    rewards = quest_template["rewards"];
    if (!mapp(rewards))
        return 0;
    
    rep = rewards["reputation"];
    if (!arrayp(rep))
        return 0;
    
    for (i = 0; i < sizeof(rep); i++)
    {
        if (rep[i]["faction"] == faction)
            return rep[i]["value"];
    }
    
    return 0;
}

// 计算境界缩放系数
// 同境界：1.0；低境界做高难：0.4；高境界做低日常：衰减
float calc_realm_reward_scale(object player, mapping quest_template)
{
    int player_realm;
    int quest_realm_min, quest_realm_max;
    int *range;
    float scale;
    int diff;
    
    player_realm = get_player_realm_index(player);
    if (player_realm < 0) player_realm = 0;
    
    range = quest_template["realm_range"];
    if (!arrayp(range) || sizeof(range) < 2)
        return 1.0;
    
    quest_realm_min = range[0];
    quest_realm_max = range[1];
    
    // 任务建议境界范围的中值
    int quest_mid = (quest_realm_min + quest_realm_max) / 2;
    
    if (player_realm == quest_mid)
    {
        scale = REWARD_SAME_REALM;
    }
    else if (player_realm < quest_mid)
    {
        // 玩家境界低于任务建议——同类任务通常不可接，但以防万一
        // 低境界做高难度任务本身不应发生
        scale = REWARD_LOWER_REALM;
    }
    else
    {
        // 高境界做低日常：衰减
        diff = player_realm - quest_mid;
        scale = 1.0;
        while (diff > 0)
        {
            scale *= REWARD_HIGHER_REALM_DECAY;
            diff--;
        }
        if (scale < REWARD_HIGHER_REALM_MIN)
            scale = REWARD_HIGHER_REALM_MIN;
    }
    
    return scale;
}

// 计算连续完成加成
float calc_daily_bonus(int consecutive_days)
{
    float bonus;
    
    if (consecutive_days <= 0)
        return 1.0;
    
    if (consecutive_days >= DAILY_STREAK_MAX_DAYS)
        return DAILY_STREAK_VIP_BONUS;
    
    // 每连续一天加成5%
    bonus = 1.0 + consecutive_days * DAILY_STREAK_BONUS;
    return bonus;
}

// 计算链长加成
// 链越长，奖励越高：每完成一个前置任务 +5%
float calc_chain_length_bonus(string chain_id, mapping player_state)
{
    int completed;
    int total;
    float bonus;
    
    if (!stringp(chain_id) || !chain_registry[chain_id])
        return 1.0;
    
    total = get_chain_total(chain_id);
    completed = get_chain_progress(chain_id, player_state);
    
    if (total <= 1)
        return 1.0;
    
    // 已完成部分占链总长度的比例带来加成
    bonus = 1.0 + (completed * 0.05);
    if (bonus > 2.0) bonus = 2.0; // 最高2倍
    
    return bonus;
}

// 计算品质系数
float calc_quality_coefficient(int quality)
{
    switch (quality)
    {
    case QUEST_QUALITY_GOOD:
        return QUALITY_COEFF_GOOD;
    case QUEST_QUALITY_RARE:
        return QUALITY_COEFF_RARE;
    default:
        return QUALITY_COEFF_NORMAL;
    }
}

// 获取境界基准值
int calc_real_base(int realm_index)
{
    switch (realm_index)
    {
    case 0: return REALM_BASE_QI;
    case 1: return REALM_BASE_ZHU;
    case 2: return REALM_BASE_JIE;
    case 3: return REALM_BASE_YING;
    case 4: return REALM_BASE_HUA;
    case 5: return REALM_BASE_LIANXU;
    case 6: return REALM_BASE_HETI;
    case 7: return REALM_BASE_DACHENG;
    default: return REALM_BASE_DACHENG;
    }
}

// 计算掉落概率
// 根据任务模板中定义的掉落概率 + 境界缩放
int calc_drop_probability(object player, mapping quest_template, string item_type)
{
    mapping rewards;
    mixed items;
    int base_prob;
    int realm_idx;
    int adjusted_prob;
    int i;
    
    if (!mapp(quest_template))
        return 0;
    
    rewards = quest_template["rewards"];
    if (!mapp(rewards))
        return 0;
    
    items = rewards["items"];
    if (!arrayp(items))
        return 0;
    
    // 查找目标物品的掉落概率
    base_prob = 0;
    for (i = 0; i < sizeof(items); i++)
    {
        mixed item = items[i];
        if (stringp(item))
        {
            if (item == item_type)
                base_prob = 100; // 未指定概率时默认100%
        }
        else if (mapp(item))
        {
            if (item["id"] == item_type)
            {
                base_prob = item["prob"];
                if (!base_prob) base_prob = 100;
            }
        }
    }
    
    if (base_prob <= 0)
        return 0;
    
    // 境界缩放：高境界获得更好掉落率
    realm_idx = get_player_realm_index(player);
    
    // 境界每高一级，掉落概率 +5%（相对值）
    // 境界每低一级，掉落概率 -10%（相对值）
    int quest_mid;
    int *range = quest_template["realm_range"];
    if (arrayp(range) && sizeof(range) >= 2)
    {
        quest_mid = (range[0] + range[1]) / 2;
        int diff = realm_idx - quest_mid;
        if (diff > 0)
            adjusted_prob = base_prob + diff * 5;
        else if (diff < 0)
            adjusted_prob = base_prob + diff * 10; // diff为负数，所以是减
        else
            adjusted_prob = base_prob;
    }
    else
    {
        adjusted_prob = base_prob;
    }
    
    // 确保在合理范围
    if (adjusted_prob < 5) adjusted_prob = 5;
    if (adjusted_prob > 100) adjusted_prob = 100;
    
    return adjusted_prob;
}

// ═══════════════════════════════════════════
//  内部工具
// ═══════════════════════════════════════════

// 获取玩家境界索引（0=炼气, 1=筑基, ...）
// 从玩家的"realm"属性读取，如果没有则返回0
int get_player_realm_index(object player)
{
    string realm;
    string *realm_names;
    int i;
    
    if (!objectp(player))
        return 0;
    
    realm = player->query("realm");
    if (!stringp(realm) || realm == "")
        return 0;
    
    realm_names = REALM_NAMES;
    for (i = 0; i < sizeof(realm_names); i++)
    {
        if (strsrch(realm, realm_names[i]) != -1)
            return i;
    }
    
    return 0; // 默认为炼气
}

// 检查任务是否对玩家可用
int is_quest_available(string quest_id, object player)
{
    mapping template;
    mapping completed;
    mapping progress;
    int status;
    
    template = quest_templates[quest_id];
    if (!mapp(template))
        return 0;
    
    // 检查是否已经完成（一次性任务）
    if (template["refresh"] == REFRESH_ONCE)
    {
        completed = player->query(QUEST_CHAIN_COMPLETED);
        if (mapp(completed) && completed[quest_id])
            return 0;
    }
    
    // 检查进度状态
    progress = player->query(QUEST_CHAIN_PROGRESS);
    if (mapp(progress))
    {
        status = progress[quest_id];
        if (status == QUEST_STATUS_COMPLETED || status == QUEST_STATUS_FAILED)
        {
            if (template["refresh"] == REFRESH_ONCE)
                return 0;
        }
        if (status == QUEST_STATUS_ACTIVE)
            return 0; // 已在活跃中
    }
    
    // 检查前置条件
    if (!check_prerequisites(template["prerequisites"], player))
        return 0;
    
    // 检查境界范围
    int *range = template["realm_range"];
    if (arrayp(range) && sizeof(range) >= 2)
    {
        int p_realm = get_player_realm_index(player);
        if (p_realm < range[0] || p_realm > range[1])
            return 0;
    }
    
    return 1;
}

// 检查前置条件（简化的通用检查）
int check_prerequisites(mapping prereqs, object player)
{
    if (!mapp(prereqs) || sizeof(prereqs) == 0)
        return 1;
    
    if (!objectp(player))
        return 0;
    
    // min_level: ([ "realm": "炼气", "level": "3层" ])
    if (prereqs["min_level"])
    {
        // 简化：直接读取玩家的境界/等级属性
        string player_realm = player->query("realm");
        if (stringp(player_realm))
        {
            // 境界比较由具体游戏逻辑实现，这里简化为球员存在
        }
    }
    
    // 检查前置任务
    if (prereqs["quests"])
    {
        string *req_quests = prereqs["quests"];
        mapping completed = player->query(QUEST_CHAIN_COMPLETED);
        int i;
        for (i = 0; i < sizeof(req_quests); i++)
        {
            if (!mapp(completed) || !completed[req_quests[i]])
                return 0;
        }
    }
    
    // min_exp
    if (prereqs["min_exp"])
    {
        int min_exp = prereqs["min_exp"];
        if (player->query("combat_exp") < min_exp)
            return 0;
    }
    
    return 1;
}

// 保存玩家任务状态（持久化通过玩家的 F_SAVE 机制）
void save_player_quest_state(object player)
{
    if (!objectp(player))
        return;
    
    // 玩家 F_DBASE 会自动处理属性的持久化
    // 这里可以触发显式保存
    player->save();
}

// 加载玩家任务状态
void load_player_quest_state(object player)
{
    if (!objectp(player))
        return;
    
    // 玩家登录时已经加载了全部数据
    // 确保必要的映射已初始化
    if (!mapp(player->query(QUEST_CHAIN_PROGRESS)))
        player->set(QUEST_CHAIN_PROGRESS, ([]));
    if (!mapp(player->query(QUEST_CHAIN_COMPLETED)))
        player->set(QUEST_CHAIN_COMPLETED, ([]));
    if (!mapp(player->query(QUEST_CHAIN_ACTIVE)))
        player->set(QUEST_CHAIN_ACTIVE, ([]));
}
