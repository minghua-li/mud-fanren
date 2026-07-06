// secret_realm_d.c
// 秘境副本系统守护进程 —— 秘境生成/进入/探索/奖励/重置全周期管理
// Created for ticket #31

#include <ansi.h>
#include <secret_realm.h>

inherit F_DBASE;

// -------- 全局状态 -------------------------------------------------

// 所有已注册的秘境定义
nosave mapping realms = ([]);

// 玩家/队伍激活的秘境实例
// instance_id -> ([
//     "realm_id"    : 关联的秘境定义 id,
//     "owner"       : 创建者 id,
//     "members"     : ({ "id1", "id2", ... }),
//     "created"     : time(),
//     "expires"     : time(),
//     "status"      : SR_STATUS_*,
//     "layer"       : 当前层数,
//     "max_layers"  : 总层数,
//     "mobs"        : ({ 已生成怪物快照 }),
//     "score"       : 累计积分,
// ])
nosave mapping instances = ([]);

// 玩家冷却记录
// player_id -> ([ realm_id -> last_enter_time ])
nosave mapping player_cooldowns = ([]);

// 限时秘境全局调度
// realm_id -> next_open_time
nosave mapping timed_schedule = ([]);

// 秘境积分排行榜
// player_id -> total_points
nosave mapping realm_points = ([]);

// 序列号生成
nosave int instance_serial = 0;

// -------- 前向声明 ------------------------------------------------
int is_in_cooldown(object me, string realm_id);
int set_cooldown(object me, string realm_id);
string generate_instance_id();
int cleanup_expired_instances();
int timed_realm_check();

// ======== 创建与心跳 ==============================================

void create()
{
    seteuid(getuid());
    set("channel_id", "秘境精灵");
    set("name", "秘境系统");
    
    // 注册内置秘境定义
    load_default_realms();
    
    CHANNEL_D->do_channel(this_object(), "sys", "秘境副本系统已启动。");
    set_heart_beat(60);  // 每分钟检查一次
}

void heart_beat()
{
    // 清理过期实例
    cleanup_expired_instances();
    
    // 限时秘境调度检查
    timed_realm_check();
}

// ======== 秘境定义注册 ============================================

// 注册一个新秘境
int register_realm(mapping realm_def)
{
    string id;
    
    if (!mapp(realm_def) || !stringp(realm_def[SR_FIELD_ID]))
        return 0;
    
    id = realm_def[SR_FIELD_ID];
    realms[id] = realm_def;
    realms[id][SR_FIELD_STATUS] = SR_STATUS_CLOSED;
    
    // 将秘境入口注册到地图守护进程
    if (stringp(realm_def[SR_FIELD_ENTRY]))
        MAP_D->set_secret_realm_entry(id, realm_def[SR_FIELD_ENTRY]);
    
    return 1;
}

// 查询秘境定义
mapping query_realm(string id)
{
    if (!stringp(id) || undefinedp(realms[id]))
        return ([]);
    
    return realms[id];
}

// 按类型查询秘境列表
string *query_realms_by_type(int type)
{
    string *result = ({}), *ids;
    int i;
    
    ids = keys(realms);
    for (i = 0; i < sizeof(ids); i++)
    {
        if (realms[ids[i]][SR_FIELD_TYPE] == type)
            result += ({ ids[i] });
    }
    
    return result;
}

// 获取所有已注册秘境 ID
string *query_all_realms()
{
    return keys(realms);
}

// 变更秘境状态
int set_realm_status(string id, int status)
{
    if (undefinedp(realms[id]))
        return 0;
    
    realms[id][SR_FIELD_STATUS] = status;
    return 1;
}

// 查询秘境状态
int query_realm_status(string id)
{
    if (undefinedp(realms[id]))
        return -1;
    
    return realms[id][SR_FIELD_STATUS];
}

// ======== 进入条件校验 ============================================

// 检查玩家能否进入指定秘境
// 返回 0=可进入, 正数=错误码
int check_entry_condition(object me, string realm_id)
{
    mapping realm;
    int min_lv, max_lv, cur_lv;
    string item_req;
    int item_count;
    
    if (!objectp(me) || !userp(me))
        return 1;
    
    if (undefinedp(realms[realm_id]))
        return 2;  // 秘境不存在
    
    realm = realms[realm_id];
    
    // 检查秘境状态
    if (realm[SR_FIELD_STATUS] != SR_STATUS_OPEN &&
        realm[SR_FIELD_STATUS] != SR_STATUS_ACTIVE)
        return 3;  // 未开放
    
    // 检查境界要求
    min_lv = realm[SR_FIELD_MIN_LEVEL];
    max_lv = realm[SR_FIELD_MAX_LEVEL];
    cur_lv = me->query("level");
    
    if (cur_lv < min_lv)
        return 10;  // 境界不足
    
    if (max_lv > 0 && cur_lv > max_lv)
        return 11;  // 超出境界上限
    
    // 检查物品要求
    item_req = realm[SR_FIELD_ITEM_REQ];
    if (stringp(item_req) && item_req != "")
    {
        string *parts;
        parts = explode(item_req, ":");
        if (sizeof(parts) >= 2)
        {
            object item_ob = present(parts[0], me);
            item_count = item_ob ? item_ob->query_amount() : 0;
            if (item_count < to_int(parts[1]))
                return 20;  // 缺少物品
        }
    }
    
    // 检查冷却
    if (is_in_cooldown(me, realm_id))
        return 30;  // 冷却中
    
    return 0;  // 可进入
}

// 获取拒绝进入的文本信息
string entry_fail_reason(int errcode)
{
    switch (errcode)
    {
    case 1:  return "只有玩家才能进入秘境。\n";
    case 2:  return "该秘境不存在。\n";
    case 3:  return "该秘境目前未开放。\n";
    case 10: return "你的境界不足以进入此秘境。\n";
    case 11: return "你的境界超出此秘境的进入限制。\n";
    case 20: return "你缺少进入秘境所需的物品。\n";
    case 30: return "你刚刚离开秘境，还需要休息一段时间才能再次进入。\n";
    default: return "无法进入秘境。\n";
    }
}

// ======== 冷却管理 ================================================

int is_in_cooldown(object me, string realm_id)
{
    mapping cd_record;
    string pid;
    int last_enter, cd_time;
    
    pid = me->query("id");
    if (!stringp(pid))
        return 0;
    
    if (undefinedp(player_cooldowns[pid]))
        return 0;
    
    cd_record = player_cooldowns[pid];
    if (undefinedp(cd_record[realm_id]))
        return 0;
    
    last_enter = cd_record[realm_id];
    if (undefinedp(realms[realm_id]))
        return 0;
    
    cd_time = realms[realm_id][SR_FIELD_CD];
    if (cd_time <= 0)
        return 0;
    
    if (time() - last_enter >= cd_time)
    {
        // 冷却已过，清理记录
        map_delete(cd_record, realm_id);
        return 0;
    }
    
    return 1;
}

int set_cooldown(object me, string realm_id)
{
    string pid;
    
    pid = me->query("id");
    if (!stringp(pid))
        return 0;
    
    if (undefinedp(player_cooldowns[pid]))
        player_cooldowns[pid] = ([]);
    
    player_cooldowns[pid][realm_id] = time();
    return 1;
}

// 查询玩家在指定秘境的剩余冷却时间（秒）
int query_remaining_cooldown(object me, string realm_id)
{
    mapping cd_record;
    string pid;
    int last_enter, cd_time, elapsed;
    
    pid = me->query("id");
    if (!stringp(pid) || undefinedp(player_cooldowns[pid]))
        return 0;
    
    cd_record = player_cooldowns[pid];
    if (undefinedp(cd_record[realm_id]))
        return 0;
    
    last_enter = cd_record[realm_id];
    if (undefinedp(realms[realm_id]))
        return 0;
    
    cd_time = realms[realm_id][SR_FIELD_CD];
    if (cd_time <= 0)
        return 0;
    
    elapsed = time() - last_enter;
    if (elapsed >= cd_time)
        return 0;
    
    return cd_time - elapsed;
}

// ======== 秘境实例管理 ============================================

// 生成唯一实例 ID
string generate_instance_id()
{
    instance_serial++;
    return sprintf("SR-INST-%d-%d", time(), instance_serial);
}

// 创建秘境实例（玩家或队伍进入时调用）
// 返回实例 ID，失败返回 0
string create_instance(object me, string realm_id)
{
    mapping realm, inst;
    string inst_id;
    string *members = ({});
    object *team;
    int i;
    
    if (undefinedp(realms[realm_id]))
        return 0;
    
    realm = realms[realm_id];
    inst_id = generate_instance_id();
    
    // 检查组队情况
    if (realm[SR_FIELD_TEAM_REQ] > 1)
    {
        if (me->is_team_leader())
        {
            team = me->query_team();
            for (i = 0; i < sizeof(team); i++)
            {
                if (objectp(team[i]) && userp(team[i]))
                    members += ({ team[i]->query("id") });
            }
        }
    }
    
    if (sizeof(members) == 0)
        members = ({ me->query("id") });
    
    // 构建实例
    inst = ([
        "realm_id"     : realm_id,
        "owner"        : me->query("id"),
        "members"      : members,
        "created"      : time(),
        "expires"      : time() + realm[SR_FIELD_DURATION],
        "status"       : SR_STATUS_ACTIVE,
        "layer"        : 1,
        "max_layers"   : realm[SR_FIELD_LAYERS],
        "mobs"         : ({}),
        "score"        : 0,
        "rewards"      : ({}),
    ]);
    
    instances[inst_id] = inst;
    
    // 为所有成员设置冷却
    for (i = 0; i < sizeof(members); i++)
    {
        object player;
        player = find_player(members[i]);
        if (objectp(player))
            set_cooldown(player, realm_id);
    }
    
    return inst_id;
}

// 查询实例信息
mapping query_instance(string inst_id)
{
    if (!stringp(inst_id) || undefinedp(instances[inst_id]))
        return ([]);
    
    return instances[inst_id];
}

// 更新实例数据
int update_instance(string inst_id, mapping data)
{
    if (undefinedp(instances[inst_id]) || !mapp(data))
        return 0;
    
    instances[inst_id] += data;
    return 1;
}

// 销毁实例
int destroy_instance(string inst_id)
{
    if (undefinedp(instances[inst_id]))
        return 0;
    
    map_delete(instances, inst_id);
    return 1;
}

// 清理过期的实例
int cleanup_expired_instances()
{
    string *ids;
    int now, count;
    int i;
    
    now = time();
    ids = keys(instances);
    count = 0;
    
    for (i = 0; i < sizeof(ids); i++)
    {
        if (instances[ids[i]]["expires"] <= now ||
            instances[ids[i]]["status"] == SR_STATUS_CLOSED)
        {
            // 执行结算（如果尚未结算）
            if (instances[ids[i]]["status"] == SR_STATUS_ACTIVE)
                settle_instance(ids[i]);
            
            map_delete(instances, ids[i]);
            count++;
        }
    }
    
    return count;
}

// ======== 秘境生成逻辑 ============================================

// 生成秘境层内的怪物配置
// 返回怪物数组：({ ({ "monster_id", "monster_name", level, hp }), ... })
mixed generate_layer_mobs(string realm_id, int layer)
{
    mapping realm;
    int mob_count, base_level, i;
    mixed *mobs = ({});
    
    if (undefinedp(realms[realm_id]))
        return ({});
    
    realm = realms[realm_id];
    
    // 随层数递增的怪物数量和强度
    mob_count = 2 + layer + random(3);
    base_level = realm[SR_FIELD_DIFFICULTY] * 10 + layer * 2;
    
    for (i = 0; i < mob_count; i++)
    {
        mixed *mob = ({
            sprintf("sr_mob_%s_l%d_%d", realm_id, layer, i),
            sprintf("秘境守卫-%s第%d层", realm[SR_FIELD_NAME], layer),
            base_level + random(5),
            100 + realm[SR_FIELD_DIFFICULTY] * 50 + layer * 30,
        });
        mobs += ({ mob });
    }
    
    // 每 3 层生成 BOSS
    if (layer % 3 == 0)
    {
        mixed *boss = ({
            sprintf("sr_boss_%s_l%d", realm_id, layer),
            sprintf("秘境统领-%s第%d层", realm[SR_FIELD_NAME], layer),
            base_level + 10 + random(5),
            500 + realm[SR_FIELD_DIFFICULTY] * 200 + layer * 100,
        });
        mobs += ({ boss });
    }
    
    return mobs;
}

// 为玩家/队伍生成秘境地图（当前层布局说明文本）
string generate_layer_description(string realm_id, int layer)
{
    mapping realm;
    string desc, layer_name;
    
    if (undefinedp(realms[realm_id]))
        return "未知秘境。\n";
    
    realm = realms[realm_id];
    
    switch (realm[SR_FIELD_TYPE])
    {
    case SR_TYPE_STORY:
        layer_name = "剧情回廊";
        break;
    case SR_TYPE_TIMED:
        layer_name = "时限试炼场";
        break;
    case SR_TYPE_CHALLENGE:
        layer_name = "挑战之厅";
        break;
    case SR_TYPE_FORTUNE:
        layer_name = "机缘幻境";
        break;
    default:
        layer_name = "未知楼层";
    }
    
    desc = sprintf(
        HIW "╔══════════════════════════════════════╗\n" NOR
        HIW "║  " HIC "【%s】第 %d 层 - %s" HIW "          ║\n" NOR
        HIW "╚══════════════════════════════════════╝\n" NOR
        "\n"
        "你踏入了一片未知的秘境空间。\n"
        "四周弥漫着浓郁的灵气，隐约可以感受到危险的气息。\n"
        "前方有守护者拦路，击败它们才能继续深入。\n"
        "\n"
        "可用指令：\n"
        "  explore      — 探索当前层\n"
        "  layer_info  — 查看当前层信息\n"
        "  exit_realm  — 离开秘境\n",
        realm[SR_FIELD_NAME], layer, layer_name
    );
    
    return desc;
}

// ======== 探索逻辑 ================================================

// 执行探索动作
// 返回探索结果文本
string do_explore(object me, string inst_id)
{
    mapping inst;
    mixed *mobs;
    int mob_count, killed, reward_exp, i;
    string result;
    
    if (undefinedp(instances[inst_id]))
        return "你的秘境实例已不存在。\n";
    
    inst = instances[inst_id];
    
    if (inst["status"] != SR_STATUS_ACTIVE)
        return "该秘境当前无法探索。\n";
    
    // 检查玩家是否在实例成员中
    if (member_array(me->query("id"), inst["members"]) == -1)
        return "你并非这个秘境实例的成员。\n";
    
    // 生成当前层怪物
    mobs = generate_layer_mobs(inst["realm_id"], inst["layer"]);
    mob_count = sizeof(mobs);
    
    // 模拟战斗结果：计算击杀数量
    killed = mob_count / 2 + random(mob_count / 2 + 1);
    if (killed > mob_count)
        killed = mob_count;
    
    // 计算探索积分
    inst["score"] += killed * 10;
    
    // 生成探索结果文本
    result = sprintf(
        HIY "╔══════════════════════════════╗\n" NOR
        HIY "║  " HIC "秘境探索 - 第 %d 层" HIY "          ║\n" NOR
        HIY "╚══════════════════════════════╝\n" NOR
        "\n"
        "你在此层遭遇了 %d 个守护者。\n"
        "经过一番激战，你击败了其中 %d 个。\n"
        "\n",
        inst["layer"], mob_count, killed
    );
    
    // BOSS 战
    if (inst["layer"] % 3 == 0)
    {
        result += HIR "在层底你遭遇了秘境统领！经过苦战，你成功击败了它！\n" NOR;
        inst["score"] += 50;
    }
    
    // 通过奖励
    reward_exp = 100 + inst["layer"] * 50 + random(100);
    result += sprintf(
        "\n"
        "当前层探索完成！获得经验：%d\n"
        "累计积分：%d\n"
        "\n"
        "你可以继续深入下一层。\n",
        reward_exp, inst["score"]
    );
    
    // 记录奖励
    inst["rewards"] += ({ ([
        "layer" : inst["layer"],
        "exp"   : reward_exp,
        "score" : killed * 10 + (inst["layer"] % 3 == 0 ? 50 : 0),
    ]) });
    
    inst["layer"]++;
    
    // 检查是否超过最大层数
    if (inst["layer"] > inst["max_layers"])
    {
        result += "\n" HIW "你已到达秘境最深处！秘境即将关闭并进行结算。\n" NOR;
        inst["expires"] = time();  // 触发结算
    }
    
    return result;
}

// 查看当前层信息
string query_layer_info(string inst_id)
{
    mapping inst;
    
    if (undefinedp(instances[inst_id]))
        return "你的秘境实例已不存在。\n";
    
    inst = instances[inst_id];
    
    if (inst["status"] != SR_STATUS_ACTIVE)
        return "该秘境当前未处于探索状态。\n";
    
    return sprintf(
        "╔══════════════════════════════╗\n"
        "║  秘境当前状态                    ║\n"
        "╚══════════════════════════════╝\n"
        "\n"
        "当前层数   ：第 %d 层 / 共 %d 层\n"
        "累计积分   ：%d\n"
        "剩余时间   ：约 %d 分钟\n"
        "组队成员   ：%s\n"
        "\n",
        inst["layer"],
        inst["max_layers"],
        inst["score"],
        (inst["expires"] - time()) / 60,
        implode(inst["members"], "、")
    );
}

// ======== 奖励结算 ================================================

// 结算指定实例的奖励
// 返回结算结果文本
varargs string settle_instance(string inst_id, int forced)
{
    mapping inst, realm;
    string result;
    int total_exp, total_score, i;
    
    if (undefinedp(instances[inst_id]))
        return "该实例不存在。\n";
    
    inst = instances[inst_id];
    
    if (inst["status"] != SR_STATUS_ACTIVE && !forced)
        return "该实例无需结算。\n";
    
    if (undefinedp(realms[inst["realm_id"]]))
    {
        inst["status"] = SR_STATUS_CLOSED;
        return "关联的秘境定义已不存在。\n";
    }
    
    realm = realms[inst["realm_id"]];
    
    // 计算总奖励
    total_exp = 0;
    for (i = 0; i < sizeof(inst["rewards"]); i++)
    {
        if (mapp(inst["rewards"][i]))
            total_exp += inst["rewards"][i]["exp"];
    }
    
    // 层数完成度加成
    total_exp = total_exp * inst["layer"] / inst["max_layers"];
    
    // 难度加成
    total_exp = to_int(total_exp * (1 + realm[SR_FIELD_DIFFICULTY] * 0.5));
    
    total_score = inst["score"];
    
    inst["status"] = SR_STATUS_SETTLING;
    
    result = sprintf(
        HIW "╔══════════════════════════════╗\n" NOR
        HIW "║  " HIC "秘境结算 - %s" HIW "          ║\n" NOR
        HIW "╚══════════════════════════════╝\n" NOR
        "\n"
        "探索进度：第 %d 层 / 共 %d 层\n"
        "累计积分：%d\n"
        "获得经验：%d\n"
        "\n",
        realm[SR_FIELD_NAME],
        inst["layer"] - 1,  // 当前 layer 已自增，所以减 1
        total_score,
        total_exp
    );
    
    // 隐藏奖励判定（层数完成度高时触发）
    if (inst["layer"] >= inst["max_layers"])
    {
        result += HIG "你完成了全部秘境探索，触发隐藏奖励！\n" NOR;
        total_exp += 500;
        total_score += 100;
        result += sprintf("隐藏奖励：额外经验 +500，额外积分 +100\n\n");
    }
    
    // 发放奖励
    for (i = 0; i < sizeof(inst["members"]); i++)
    {
        object player;
        player = find_player(inst["members"][i]);
        
        if (objectp(player))
        {
            // 发放经验
            player->add("combat_exp", total_exp);
            // 记录积分
            player->set("secret_realm/score/" + inst["realm_id"],
                        player->query("secret_realm/score/" + inst["realm_id"]) + total_score);
            // 累加秘境积分
            realm_points[inst["members"][i]] += total_score;
        }
    }
    
    result += sprintf("奖励已发放至所有成员。\n");
    
    inst["status"] = SR_STATUS_CLOSED;
    inst["expires"] = time();
    
    return result;
}

// ======== 积分与排行榜 ============================================

// 查询玩家秘境总积分
int query_player_points(object me)
{
    string pid;
    
    pid = me->query("id");
    if (!stringp(pid) || undefinedp(realm_points[pid]))
        return 0;
    
    return realm_points[pid];
}

// 获取积分排行榜（前 N 名）
// 返回：({ ({ "player_id", points }), ... })
mixed get_ranking(int top_n)
{
    string *ids;
    mixed *sorted = ({});
    int i, j;
    
    if (top_n <= 0)
        top_n = 10;
    
    ids = keys(realm_points);
    
    // 简单排序（冒泡，数据量通常不大）
    for (i = 0; i < sizeof(ids); i++)
    {
        sorted += ({ ({ ids[i], realm_points[ids[i]] }) });
    }
    
    for (i = 0; i < sizeof(sorted); i++)
    {
        for (j = i + 1; j < sizeof(sorted); j++)
        {
            if (sorted[j][1] > sorted[i][1])
            {
                mixed tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }
    
    if (sizeof(sorted) > top_n)
        sorted = sorted[0..top_n - 1];
    
    return sorted;
}

// ======== 限时秘境调度 ============================================

int timed_realm_check()
{
    string *ids;
    int now, i;
    
    now = time();
    ids = keys(timed_schedule);
    
    for (i = 0; i < sizeof(ids); i++)
    {
        if (undefinedp(realms[ids[i]]))
        {
            map_delete(timed_schedule, ids[i]);
            continue;
        }
        
        if (realms[ids[i]][SR_FIELD_TYPE] != SR_TYPE_TIMED)
        {
            map_delete(timed_schedule, ids[i]);
            continue;
        }
        
        // 到达开启时间
        if (timed_schedule[ids[i]] <= now &&
            realms[ids[i]][SR_FIELD_STATUS] == SR_STATUS_CLOSED)
        {
            open_timed_realm(ids[i]);
        }
        
        // 秘境正在进行，且已过持续时间
        if (realms[ids[i]][SR_FIELD_STATUS] == SR_STATUS_ACTIVE)
        {
            // 检查所有关联实例是否都已过期
            // 简化处理：直接关闭
            close_timed_realm(ids[i]);
        }
    }
    
    return 1;
}

// 安排限时秘境开启
int schedule_timed_realm(string realm_id, int open_time)
{
    if (undefinedp(realms[realm_id]))
        return 0;
    
    if (realms[realm_id][SR_FIELD_TYPE] != SR_TYPE_TIMED)
        return 0;
    
    timed_schedule[realm_id] = open_time;
    return 1;
}

// 开启限时秘境
int open_timed_realm(string realm_id)
{
    mapping realm;
    
    if (undefinedp(realms[realm_id]))
        return 0;
    
    realm = realms[realm_id];
    realm[SR_FIELD_STATUS] = SR_STATUS_OPEN;
    
    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIG "【秘境公告】%s 现已开放！符合条件的道友可前往入口进入。\n" NOR,
                realm[SR_FIELD_NAME]));
    
    // 安排关闭时间
    timed_schedule[realm_id] = time() + realm[SR_FIELD_DURATION];
    
    return 1;
}

// 关闭限时秘境
int close_timed_realm(string realm_id)
{
    mapping realm;
    
    if (undefinedp(realms[realm_id]))
        return 0;
    
    realm = realms[realm_id];
    realm[SR_FIELD_STATUS] = SR_STATUS_CLOSED;
    
    CHANNEL_D->do_channel(this_object(), "sys",
        sprintf(HIW "【秘境公告】%s 已关闭，等待下一次开启。\n" NOR,
                realm[SR_FIELD_NAME]));
    
    return 1;
}

// ======== 内置秘境定义 ============================================

void load_default_realms()
{
    // ---- 剧情秘境：血色禁地 ----
    register_realm(([
        SR_FIELD_ID          : "blood_forbidden",
        SR_FIELD_NAME        : "血色禁地",
        SR_FIELD_TYPE        : SR_TYPE_STORY,
        SR_FIELD_DIFFICULTY  : SR_DIFFICULTY_NORMAL,
        SR_FIELD_RESET       : SR_RESET_ONCE,
        SR_FIELD_STATUS      : SR_STATUS_CLOSED,
        SR_FIELD_ENTRY       : "/d/yue/npc/obj/blood_forbidden_entry",
        SR_FIELD_EXIT        : "/d/yue/npc/obj/blood_forbidden_exit",
        SR_FIELD_MIN_LEVEL   : 5,    // 炼气期
        SR_FIELD_MAX_LEVEL   : 15,   // 筑基以下
        SR_FIELD_ITEM_REQ    : "",
        SR_FIELD_TEAM_REQ    : 1,
        SR_FIELD_DURATION    : 7200, // 2 小时
        SR_FIELD_CD          : SR_CD_WEEKLY,
        SR_FIELD_LAYERS      : 5,
        SR_FIELD_REWARDS     : ([
            "exp"       : 2000,
            "potential" : 500,
            "item"      : "/d/yue/obj/condensing_herb",
        ]),
    ]));

    // ---- 限时秘境：虚天殿 ----
    register_realm(([
        SR_FIELD_ID          : "xu_tian_hall",
        SR_FIELD_NAME        : "虚天殿",
        SR_FIELD_TYPE        : SR_TYPE_TIMED,
        SR_FIELD_DIFFICULTY  : SR_DIFFICULTY_HARD,
        SR_FIELD_RESET       : SR_RESET_MONTHLY,
        SR_FIELD_STATUS      : SR_STATUS_CLOSED,
        SR_FIELD_ENTRY       : "/d/luanxinghai/npc/obj/xutian_entry",
        SR_FIELD_EXIT        : "/d/luanxinghai/npc/obj/xutian_exit",
        SR_FIELD_MIN_LEVEL   : 25,   // 结丹期
        SR_FIELD_MAX_LEVEL   : 40,   // 元婴以下
        SR_FIELD_ITEM_REQ    : "",
        SR_FIELD_TEAM_REQ    : 3,
        SR_FIELD_DURATION    : 14400, // 4 小时
        SR_FIELD_CD          : SR_CD_WEEKLY,
        SR_FIELD_LAYERS      : 3,
        SR_FIELD_REWARDS     : ([
            "exp"       : 10000,
            "potential" : 2000,
            "item"      : "/d/luanxinghai/obj/xutian_ding",
        ]),
    ]));

    // ---- 多人秘境：妖兽岛猎妖 ----
    register_realm(([
        SR_FIELD_ID          : "beast_island",
        SR_FIELD_NAME        : "妖兽岛",
        SR_FIELD_TYPE        : SR_TYPE_CHALLENGE,
        SR_FIELD_DIFFICULTY  : SR_DIFFICULTY_NORMAL,
        SR_FIELD_RESET       : SR_RESET_DAILY,
        SR_FIELD_STATUS      : SR_STATUS_CLOSED,
        SR_FIELD_ENTRY       : "/d/tianxing/npc/obj/beast_island_entry",
        SR_FIELD_EXIT        : "/d/tianxing/npc/obj/beast_island_exit",
        SR_FIELD_MIN_LEVEL   : 15,   // 筑基期
        SR_FIELD_MAX_LEVEL   : 0,    // 无上限
        SR_FIELD_ITEM_REQ    : "",
        SR_FIELD_TEAM_REQ    : 5,
        SR_FIELD_DURATION    : 3600, // 1 小时
        SR_FIELD_CD          : SR_CD_DAILY,
        SR_FIELD_LAYERS      : 1,
        SR_FIELD_REWARDS     : ([
            "exp"       : 3000,
            "potential" : 1000,
            "item"      : "/d/luanxinghai/obj/demon_pill",
        ]),
    ]));

    // ---- 随机秘境：古修士洞府 ----
    register_realm(([
        SR_FIELD_ID          : "ancient_cave",
        SR_FIELD_NAME        : "古修士洞府",
        SR_FIELD_TYPE        : SR_TYPE_FORTUNE,
        SR_FIELD_DIFFICULTY  : SR_DIFFICULTY_EASY,
        SR_FIELD_RESET       : SR_RESET_ONCE,
        SR_FIELD_STATUS      : SR_STATUS_CLOSED,
        SR_FIELD_ENTRY       : "/d/yue/npc/obj/ancient_cave_entry",
        SR_FIELD_EXIT        : "/d/yue/npc/obj/ancient_cave_exit",
        SR_FIELD_MIN_LEVEL   : 5,
        SR_FIELD_MAX_LEVEL   : 0,
        SR_FIELD_ITEM_REQ    : "",
        SR_FIELD_TEAM_REQ    : 1,
        SR_FIELD_DURATION    : 1800, // 30 分钟
        SR_FIELD_CD          : SR_CD_FORTUNE,
        SR_FIELD_LAYERS      : 3,
        SR_FIELD_REWARDS     : ([
            "exp"       : 500,
            "potential" : 200,
            "item"      : "/d/yue/obj/ancient_relic",
        ]),
    ]));
}

// ======== 调试与维护 ==============================================

int clean_up()
{
    return 1;
}

// Wiz 指令：查看所有秘境的注册状态
string debug_status()
{
    string *ids;
    string result;
    int i;
    
    ids = keys(realms);
    result = sprintf("秘境系统状态\n");
    result += sprintf("═══════════════════════════════\n");
    result += sprintf("已注册秘境：%d 个\n", sizeof(ids));
    result += sprintf("活跃实例：%d 个\n", sizeof(instances) - cleanup_expired_instances());
    result += sprintf("积分记录：%d 个玩家\n\n", sizeof(realm_points));
    
    for (i = 0; i < sizeof(ids); i++)
    {
        result += sprintf("  [%s] %s (类型:%d, 状态:%d, 难度:%d)\n",
                          ids[i],
                          realms[ids[i]][SR_FIELD_NAME],
                          realms[ids[i]][SR_FIELD_TYPE],
                          realms[ids[i]][SR_FIELD_STATUS],
                          realms[ids[i]][SR_FIELD_DIFFICULTY]);
    }
    
    return result;
}
