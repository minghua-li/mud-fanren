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

// #61 境界体系门槛校验（realm_index/realm_sub 口径）：
// 大境界高于 min_idx 直接放行；同大境界要求子阶段 >= min_sub
// （0=初期 1=中期 2=后期），如 min_idx=2/min_sub=1 即「筑基中期起」。
// 境界读取统一走 ROOT_REFINE_D（#61 境界存储约定，见
// .knowledge/cultivation/realm-storage-convention），不另造读取。
int check_realm_gate(object me, int min_idx, int min_sub)
{
    int idx = ROOT_REFINE_D->query_player_realm_index(me);

    if (idx > min_idx)
        return 1;
    if (idx < min_idx)
        return 0;
    return ROOT_REFINE_D->query_player_sub_stage(me) >= min_sub;
}

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
    // 注册了 min_realm_index 的秘境走 #61 境界体系（realm_index/realm_sub，
    // 如血色禁地=筑基中期起）；未注册的沿用旧 level 数值口径。
    // 注意：缺键在 LPC 中取 0 且 intp(0) 为真，必须用 undefinedp 判存在，
    // 否则全部秘境误走 0=凡人档、门槛被放开。
    if (!undefinedp(realm[SR_FIELD_MIN_REALM_INDEX]))
    {
        if (!check_realm_gate(me, realm[SR_FIELD_MIN_REALM_INDEX],
                              realm[SR_FIELD_MIN_REALM_SUB]))
            return 10;  // 境界不足
    }
    else
    {
        min_lv = realm[SR_FIELD_MIN_LEVEL];
        max_lv = realm[SR_FIELD_MAX_LEVEL];
        cur_lv = me->query("level");

        if (cur_lv < min_lv)
            return 10;  // 境界不足

        if (max_lv > 0 && cur_lv > max_lv)
            return 11;  // 超出境界上限
    }
    
    // 检查物品要求
    item_req = realm[SR_FIELD_ITEM_REQ];
    if (stringp(item_req) && item_req != "")
    {
        string *parts;
        parts = explode(item_req, ":");
        if (sizeof(parts) >= 2)
        {
            item_count = me->query_amount(parts[0]);
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
    object team_leader;
    object *team;
    int i;
    
    if (undefinedp(realms[realm_id]))
        return 0;
    
    realm = realms[realm_id];
    inst_id = generate_instance_id();
    
    // 检查组队情况
    if (realm[SR_FIELD_TEAM_REQ] > 1)
    {
        team_leader = me->query_team_leader();
        if (objectp(team_leader))
        {
            team = team_leader->query_team_members();
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

// 按房间 sr_mobs 表生成真实怪物实体（#108 c2）：
// 房间持有 ({ 真实 NPC 文件路径: 数量 }) 表，已生成的跳过、只补差额。
// 不再使用程序化泛名占位生成（#31 旧 generate_layer_mobs 已移除）。
// 返回本次实际新生成的对象数组（可能为空）。
object *spawn_layer_mobs(object room)
{
    mapping mob_table;
    string *paths;
    string path;
    object *inv;
    object *spawned = ({});
    object ob;
    int i, j, have;

    if (!objectp(room))
        return spawned;

    mob_table = room->query("sr_mobs");
    if (!mapp(mob_table))
        return spawned;

    inv = all_inventory(room);
    paths = keys(mob_table);
    for (i = 0; i < sizeof(paths); i++)
    {
        path = paths[i];
        have = 0;
        for (j = 0; j < sizeof(inv); j++)
        {
            if (base_name(inv[j]) == path)
                have++;
        }
        for (j = have; j < mob_table[path]; j++)
        {
            ob = new(path);
            if (!objectp(ob))
                continue;
            ob->move(room);
            spawned += ({ ob });
        }
    }

    return spawned;
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

// 执行探索动作（#108 c4：改接真实战斗系统）
// 按玩家所在房间的 sr_mobs 表生成真实怪物实体，并对房间内全部试炼妖兽
// 发起真实战斗（F_ATTACK fight_ob 开局，回合/伤害/击杀结算走既有战斗系统
// —— #63 产物的 COMBAT_D 奖励链路，include/combat/reward.h）。
// 不再使用随机数模拟击杀数/经验（#31 旧模拟段已移除）。
// 返回探索结果文本
string do_explore(object me, string inst_id)
{
    mapping inst;
    object room;
    object *inv;
    object *mobs;
    object mob;
    int layer, i;
    string result;

    if (undefinedp(instances[inst_id]))
        return "你的秘境实例已不存在。\n";

    inst = instances[inst_id];

    if (inst["status"] != SR_STATUS_ACTIVE)
        return "该秘境当前无法探索。\n";

    // 检查玩家是否在实例成员中
    if (member_array(me->query("id"), inst["members"]) == -1)
        return "你并非这个秘境实例的成员。\n";

    room = environment(me);
    if (!objectp(room))
        return "你此刻并不在禁地之中。\n";

    layer = (int)room->query("sr_layer");
    if (layer <= 0)
        return "此处未挂接试炼路线，无法在此开始试炼。\n";

    // 记录到达的最深层（供结算进度使用）
    if (layer > (int)inst["layer"])
        inst["layer"] = layer;

    // 补全本房间缺失的真实怪物实体（房间 sr_mobs 表持有）
    spawn_layer_mobs(room);

    // 收集房间内全部本秘境的试炼妖兽（含此前生成未清完的）
    inv = all_inventory(room);
    mobs = ({});
    for (i = 0; i < sizeof(inv); i++)
    {
        if (inv[i]->query("sr_realm") == inst["realm_id"])
            mobs += ({ inv[i] });
    }

    if (sizeof(mobs) == 0)
        return "本层妖物已清。你可以继续深入，或退出秘境。\n";

    // 接真实战斗：对每只妖兽发起战斗（战斗回合、伤害公式、击杀奖励
    // 全部由既有战斗系统处理；击杀时 NPC 的 winner_reward 掉落真实物品
    // 并回调 on_mob_defeated 做试炼结算）
    result = HIY "你踏入了禁地深处的迷障，本层的妖兽拦住了去路：\n" NOR;
    for (i = 0; i < sizeof(mobs); i++)
    {
        mob = mobs[i];
        result += "  " + mob->name() + "\n";
    }
    for (i = 0; i < sizeof(mobs); i++)
    {
        mob = mobs[i];
        if (!mob->is_fighting(me))
            me->fight_ob(mob);
    }
    result += "\n";

    return result;
}

// 试炼妖兽被击杀时的结算挂接（#108 c6 写入端）
// 由 d/xuejin/npc/*.c 的 winner_reward（战斗系统战胜判定回调）调用：
// - 累加实例积分（sr_score）；
// - 击杀 BOSS（sr_boss=1，墨蛟）= 试炼存活 → 写入持久存活标记，
//   读取端：主线 mq_1_9 的 OBJ_SURVIVE 目标（main_quest_d quest_progress）
//   与九宗晋升门槛（sect_d promote）。
// 形参顺序契约（审查第 1 轮修复）：与调用方 winner_reward(killer, victim)
// 的传参顺序一致——实参 1=killer（击杀者/玩家），实参 2=victim（被击杀 NPC）。
// NPC 侧 winner_reward 由 reward.h 的 victim->winner_reward(killer, victim)
// 调用（killer=玩家，victim=NPC），故守护进程此处 killer 形参恒为玩家、
// victim 形参恒为 NPC（sr_realm/sr_score/sr_boss 均挂在 NPC 上）。
int on_mob_defeated(object killer, object victim)
{
    string realm_id;
    string inst_id;
    mapping inst;
    int score;

    if (!objectp(victim) || !objectp(killer))
        return 0;

    realm_id = victim->query("sr_realm");
    if (!stringp(realm_id) || realm_id == "")
        return 0;

    inst_id = query_player_instance(killer, realm_id);
    if (!stringp(inst_id) || inst_id == "")
        return 0;

    inst = instances[inst_id];
    score = (int)victim->query("sr_score");
    if (score <= 0)
        score = 10;
    inst["score"] += score;

    if ((int)victim->query("sr_boss") == 1)
        mark_trial_survivors(killer, victim, realm_id);

    return 1;
}

// 试炼存活标记写入（#108 c6）：击杀 BOSS 的存活玩家 + 同处一室的其他存活
// 玩家，均持久写入 "secret_realm/<realm_id>/survived" 时间戳标记。
void mark_trial_survivors(object killer, object victim, string realm_id)
{
    object room;
    object *inv;
    string flag;
    int i;

    flag = "secret_realm/" + realm_id + "/survived";

    if (objectp(killer) && userp(killer) && living(killer))
    {
        killer->set(flag, time());
        tell_object(killer, HIG "你活下来了。" + realms[realm_id][SR_FIELD_NAME] +
                    "的试炼刻进了你的修行——各派的长老都记得这份战功。\n" NOR);
    }

    room = environment(victim);
    if (!objectp(room))
        return;

    inv = all_inventory(room);
    for (i = 0; i < sizeof(inv); i++)
    {
        if (userp(inv[i]) && living(inv[i]) && inv[i] != killer)
        {
            inv[i]->set(flag, time());
            tell_object(inv[i], HIG "你活下来了。" + realms[realm_id][SR_FIELD_NAME] +
                        "的试炼刻进了你的修行。\n" NOR);
        }
    }
}

// 查询玩家在指定秘境的活跃实例 ID（无则返回空串）
string query_player_instance(object me, string realm_id)
{
    string *ids;
    int i;

    if (!objectp(me) || !stringp(realm_id))
        return "";

    ids = keys(instances);
    for (i = 0; i < sizeof(ids); i++)
    {
        mapping inst = instances[ids[i]];
        if (inst["realm_id"] == realm_id &&
            inst["status"] == SR_STATUS_ACTIVE &&
            member_array(me->query("id"), inst["members"]) != -1)
        {
            return ids[i];
        }
    }

    return "";
}

// 退出秘境（#108 c7）：结算并销毁实例，把玩家送回注册出口（真实房间路径）
string do_exit(object me, string realm_id)
{
    string inst_id, result = "";
    string exit_path;
    mapping realm;

    if (!objectp(me) || undefinedp(realms[realm_id]))
        return "该秘境不存在。\n";

    realm = realms[realm_id];
    exit_path = realm[SR_FIELD_EXIT];

    inst_id = query_player_instance(me, realm_id);
    if (stringp(inst_id) && inst_id != "")
    {
        result += settle_instance(inst_id);
        destroy_instance(inst_id);
    }

    if (stringp(exit_path) && exit_path != "" && objectp(load_object(exit_path)))
    {
        me->move(exit_path);
        result += "你回到了" + realm[SR_FIELD_NAME] + "的入口。\n";
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
    
    // 计算总奖励（#108：探索已改接真实战斗，不再逐层模拟经验；
    // 结算基础改按注册块奖励表，按层完成度与难度缩放）
    total_exp = (int)realm[SR_FIELD_REWARDS]["exp"];
    if (total_exp <= 0)
        total_exp = 500;

    // 层数完成度加成
    total_exp = total_exp * (int)inst["layer"] / (int)inst["max_layers"];

    // 难度加成
    total_exp = total_exp * (2 + realm[SR_FIELD_DIFFICULTY]) / 2;
    
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
        inst["layer"],  // #108：layer = 到达的最深层（不再自增）
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
    string reward_item = realm[SR_FIELD_REWARDS]["item"];
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
            // 发放物品（#108 c3：注册块奖励物品为真实实体路径，如黄龙草；
            // 路径无效时 new 返回 0，静默跳过，不影响其他奖励）
            if (stringp(reward_item) && reward_item != "")
            {
                object reward = new(reward_item);
                if (objectp(reward))
                    reward->move(player);
            }
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
    // #108 三层内容落地：
    // - 状态转正：register_realm 统一置 CLOSED，故注册后经既有开放机制
    //   set_realm_status 转为 SR_STATUS_OPEN（玩家可经入口真实进入）；
    // - 出口改指真实入口房间（/d/xuejin/rukou）；行走链路 入口→外围→中层→
    //   核心→巢窟，逐层步行可走回入口（巢窟回入口共 3 步，无一步直达出口）；
    // - 层数对齐 1G §3.2.1 三层环形结构（原 5 层为 #31 占位）；
    // - 冷却由周改日：一次性剧情试炼，死亡/退出后需当日可重试，周冷却会把主线卡死一周；
    // - 奖励物品改指真实实体黄龙草（#67 已合入主干，筑基丹主药，不造同名占位）。
    // 怪物表不在此注册：三层房间各自 set("sr_mobs", ...) 持有本层真实 NPC 实体表，
    // do_explore 按所在房间生成并接真实战斗（见 d/xuejin/ 各房间文件）。
    register_realm(([
        SR_FIELD_ID          : "blood_forbidden",
        SR_FIELD_NAME        : "血色禁地",
        SR_FIELD_TYPE        : SR_TYPE_STORY,
        SR_FIELD_DIFFICULTY  : SR_DIFFICULTY_NORMAL,
        SR_FIELD_RESET       : SR_RESET_ONCE,
        SR_FIELD_STATUS      : SR_STATUS_OPEN,
        SR_FIELD_ENTRY       : "/d/xuejin/rukou",
        SR_FIELD_EXIT        : "/d/xuejin/rukou",
        SR_FIELD_MIN_REALM_INDEX : 2,   // 筑基（#61 境界体系）
        SR_FIELD_MIN_REALM_SUB   : 1,   // 中期起
        SR_FIELD_ITEM_REQ    : "",
        SR_FIELD_TEAM_REQ    : 1,
        SR_FIELD_DURATION    : 7200, // 2 小时
        SR_FIELD_CD          : SR_CD_DAILY,
        SR_FIELD_LAYERS      : 3,
        SR_FIELD_REWARDS     : ([
            "exp"       : 2000,
            "potential" : 500,
            "item"      : "/d/yueguo/tainan/obj/huanglongcao",
        ]),
    ]));
    set_realm_status("blood_forbidden", SR_STATUS_OPEN);

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
