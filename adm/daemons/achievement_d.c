// achievement_d.c
// 成就系统守护进程
// 负责成就定义管理、进度追踪、解锁检测、奖励发放

inherit F_DBASE;

#include <ansi.h>
#include <achievement.h>

// ==================== 成就定义 ====================

// 所有成就定义
protected mapping achievements = ([]);

// 按分类索引
protected mapping by_category = ([]);

void create()
{
    seteuid(getuid());
    set("channel_id", HIG "成就系统" NOR);
    set_achievements();
}

// ==================== 成就定义初始化 ====================

void set_achievements()
{
    // ---------- 修炼成就 (12) ----------
    set_achievement("ach_cult_01", "初入修仙", ACH_CAT_CULTIVATION, "达到炼气期，踏入修仙之路",
        20, "ach_check_realm", ({ "炼气", "1层" }),
        ([ "title": HIG "修炼之始" NOR, "attr_practice": 2 ]));

    set_achievement("ach_cult_02", "凡人巅峰", ACH_CAT_CULTIVATION, "达到炼气十三层大圆满",
        20, "ach_check_realm", ({ "炼气", "13层" }),
        ([ "title": HIC "凡人巅峰" NOR, "attr_practice": 3 ]));

    set_achievement("ach_cult_03", "筑基有成", ACH_CAT_CULTIVATION, "成功筑基，迈入修士行列",
        20, "ach_check_realm", ({ "筑基", "初期" }),
        ([ "title": HIC "筑基修士" NOR, "attr_practice": 3 ]));

    set_achievement("ach_cult_04", "筑基圆满", ACH_CAT_CULTIVATION, "筑基期大圆满",
        20, "ach_check_realm", ({ "筑基", "后期" }),
        ([ "title": HIC "筑基圆满" NOR, "attr_practice": 4 ]));

    set_achievement("ach_cult_05", "金丹大成", ACH_CAT_CULTIVATION, "凝结金丹，成就真人果位",
        20, "ach_check_realm", ({ "结丹", "初期" }),
        ([ "title": HIM "金丹真人" NOR, "attr_practice": 5 ]));

    set_achievement("ach_cult_06", "金丹巅峰", ACH_CAT_CULTIVATION, "金丹期大圆满",
        20, "ach_check_realm", ({ "结丹", "后期" }),
        ([ "title": HIM "金丹巅峰" NOR, "attr_practice": 5 ]));

    set_achievement("ach_cult_07", "元婴出世", ACH_CAT_CULTIVATION, "碎丹成婴，元婴老怪",
        20, "ach_check_realm", ({ "元婴", "初期" }),
        ([ "title": HIR "元婴老怪" NOR, "attr_practice": 8 ]));

    set_achievement("ach_cult_08", "元婴大成", ACH_CAT_CULTIVATION, "元婴期大圆满",
        20, "ach_check_realm", ({ "元婴", "后期" }),
        ([ "title": HIR "元婴大成" NOR, "attr_practice": 8 ]));

    set_achievement("ach_cult_09", "化神归真", ACH_CAT_CULTIVATION, "化神期，神识化形",
        20, "ach_check_realm", ({ "化神", "初期" }),
        ([ "title": HIW "化神圣君" NOR, "attr_practice": 10 ]));

    set_achievement("ach_cult_10", "炼虚合道", ACH_CAT_CULTIVATION, "炼虚期，合道天地",
        20, "ach_check_realm", ({ "炼虚", "初期" }),
        ([ "title": HIY "炼虚尊者" NOR, "attr_practice": 12 ]));

    set_achievement("ach_cult_11", "合体归一", ACH_CAT_CULTIVATION, "合体期，身合大道",
        20, "ach_check_realm", ({ "合体", "初期" }),
        ([ "title": HBRED "合体大能" NOR, "attr_practice": 15 ]));

    set_achievement("ach_cult_12", "飞升上界", ACH_CAT_CULTIVATION, "成功渡飞升劫，飞升灵界",
        20, "ach_check_realm", ({ "大乘", "圆满" }),
        ([ "title": HBBLU "真仙降世" NOR, "attr_hp": 5000, "attr_mp": 5000 ]));

    // ---------- 任务成就 (15) ----------
    set_achievement("ach_task_01", "初出茅庐", ACH_CAT_TASK, "完成第一个任务",
        10, "ach_check_task_count", 1,
        ([ "title": HIW "初出茅庐" NOR, "attr_exp_rate": 2 ]));

    set_achievement("ach_task_02", "任务达人·初阶", ACH_CAT_TASK, "完成100个任务",
        10, "ach_check_task_count", 100,
        ([ "title": HIC "勤勉修士" NOR, "attr_exp_rate": 3 ]));

    set_achievement("ach_task_03", "任务达人·中阶", ACH_CAT_TASK, "完成500个任务",
        10, "ach_check_task_count", 500,
        ([ "title": HIG "任务狂人" NOR, "attr_exp_rate": 5 ]));

    set_achievement("ach_task_04", "任务达人·高阶", ACH_CAT_TASK, "完成2000个任务",
        10, "ach_check_task_count", 2000,
        ([ "title": HIY "天道酬勤" NOR, "attr_exp_rate": 8 ]));

    set_achievement("ach_task_05", "日常全勤·周", ACH_CAT_TASK, "连续7天完成所有日常任务",
        10, "ach_check_daily_streak", 7,
        ([ "title": HIG "日行一善" NOR ]));

    set_achievement("ach_task_06", "日常全勤·月", ACH_CAT_TASK, "连续30天完成所有日常任务",
        10, "ach_check_daily_streak", 30,
        ([ "title": HIY "雷打不动" NOR ]));

    set_achievement("ach_task_07", "支线收集者", ACH_CAT_TASK, "完成50条支线任务",
        10, "ach_check_side_quest", 50,
        ([ "title": HIC "博闻广记" NOR, "attr_side_reward": 10 ]));

    set_achievement("ach_task_08", "支线大师", ACH_CAT_TASK, "完成100条支线任务",
        10, "ach_check_side_quest", 100,
        ([ "title": HIM "支线大师" NOR, "attr_side_reward": 15 ]));

    set_achievement("ach_task_09", "主线推进", ACH_CAT_TASK, "完成第一章主线剧情",
        10, "ach_check_main_chapter", 1,
        ([ "title": HIC "越国风云" NOR ]));

    set_achievement("ach_task_10", "乱星海之主", ACH_CAT_TASK, "完成第二章主线剧情",
        10, "ach_check_main_chapter", 2,
        ([ "title": HIM "星海霸主" NOR ]));

    set_achievement("ach_task_11", "灵界传奇", ACH_CAT_TASK, "完成第三章主线剧情",
        10, "ach_check_main_chapter", 3,
        ([ "title": HIW "灵界传奇" NOR ]));

    set_achievement("ach_task_12", "飞升之路", ACH_CAT_TASK, "完成终章主线剧情",
        10, "ach_check_main_chapter", 4,
        ([ "title": HIY "飞升上界" NOR ]));

    set_achievement("ach_task_13", "成就猎手", ACH_CAT_TASK, "获得30个成就",
        10, "ach_check_achievement_count", 30,
        ([ "title": HIY "成就猎手" NOR, "attr_ach_reward": 20 ]));

    set_achievement("ach_task_14", "全成就者", ACH_CAT_TASK, "获得60个成就",
        10, "ach_check_achievement_count", 60,
        ([ "title": HBRED "完美主义" NOR, "attr_ach_reward": 30 ]));

    set_achievement("ach_task_15", "周常达人", ACH_CAT_TASK, "完成50个周常任务",
        10, "ach_check_weekly_count", 50,
        ([ "title": HIC "周常达人" NOR ]));

    // ---------- 战斗成就 (15) ----------
    set_achievement("ach_combat_01", "初经沙场", ACH_CAT_COMBAT, "累计击杀100个怪物",
        15, "ach_check_kill_count", 100,
        ([ "title": HIW "初经沙场" NOR, "attr_attack": 3 ]));

    set_achievement("ach_combat_02", "百战老兵", ACH_CAT_COMBAT, "累计击杀1000个怪物",
        15, "ach_check_kill_count", 1000,
        ([ "title": HIC "百战老兵" NOR, "attr_attack": 5 ]));

    set_achievement("ach_combat_03", "千战精英", ACH_CAT_COMBAT, "累计击杀5000个怪物",
        15, "ach_check_kill_count", 5000,
        ([ "title": HIG "千战精英" NOR, "attr_attack": 6 ]));

    set_achievement("ach_combat_04", "万战修罗", ACH_CAT_COMBAT, "累计击杀10000个怪物",
        15, "ach_check_kill_count", 10000,
        ([ "title": HIR "万战修罗" NOR, "attr_attack": 8 ]));

    set_achievement("ach_combat_05", "以一当十", ACH_CAT_COMBAT, "单场战斗击杀10个以上的怪物",
        15, "ach_check_mass_kill", 10,
        ([ "title": HIR "勇猛无双" NOR, "attr_critical": 2 ]));

    set_achievement("ach_combat_06", "越级挑战·初", ACH_CAT_COMBAT, "击败高1个大境界的敌人",
        15, "ach_check_cross_realm", 1,
        ([ "title": HIY "以下克上" NOR, "attr_cross_dmg": 5 ]));

    set_achievement("ach_combat_07", "越级挑战·极", ACH_CAT_COMBAT, "击败高2个大境界的敌人",
        15, "ach_check_cross_realm", 2,
        ([ "title": HIM "以弱胜强" NOR, "attr_cross_dmg": 10 ]));

    set_achievement("ach_combat_08", "PVP新星", ACH_CAT_COMBAT, "PVP连胜5场",
        15, "ach_check_pvp_streak", 5,
        ([ "title": HIC "竞技新星" NOR, "attr_pvp_dmg": 3 ]));

    set_achievement("ach_combat_09", "PVP霸主", ACH_CAT_COMBAT, "PVP连胜20场",
        15, "ach_check_pvp_streak", 20,
        ([ "title": HIR "竞技霸主" NOR, "attr_pvp_dmg": 8 ]));

    set_achievement("ach_combat_10", "副本先驱", ACH_CAT_COMBAT, "首次通关任意副本",
        15, "ach_check_first_dungeon", 1,
        ([ "title": HIG "开荒者" NOR, "attr_dungeon_drop": 5 ]));

    set_achievement("ach_combat_11", "副本专家", ACH_CAT_COMBAT, "通关全部主线副本",
        15, "ach_check_all_dungeons", 1,
        ([ "title": HIC "秘境探秘者" NOR, "attr_dungeon_drop": 10 ]));

    set_achievement("ach_combat_12", "猎妖者", ACH_CAT_COMBAT, "击杀100只妖兽",
        15, "ach_check_beast_kill", 100,
        ([ "title": HIW "猎妖者" NOR, "attr_beast_dmg": 5 ]));

    set_achievement("ach_combat_13", "屠魔者", ACH_CAT_COMBAT, "击杀50只魔物",
        15, "ach_check_demon_kill", 50,
        ([ "title": HIM "屠魔者" NOR, "attr_demon_dmg": 5 ]));

    set_achievement("ach_combat_14", "以一敌百", ACH_CAT_COMBAT, "单场战斗击杀20个以上的怪物",
        15, "ach_check_mass_kill", 20,
        ([ "title": HIR "万人敌" NOR, "attr_critical": 3 ]));

    set_achievement("ach_combat_15", "不败传说", ACH_CAT_COMBAT, "连续100场战斗未死亡",
        15, "ach_check_no_death", 100,
        ([ "title": HIY "不败传说" NOR, "attr_defense": 5 ]));

    // ---------- 收集成就 (10) ----------
    set_achievement("ach_collect_01", "藏书初成", ACH_CAT_COLLECTION, "收集10种不同的功法/秘籍",
        10, "ach_check_book_count", 10,
        ([ "title": HIW "初窥书海" NOR ]));

    set_achievement("ach_collect_02", "藏书万卷", ACH_CAT_COLLECTION, "收集100种不同的功法/秘籍",
        10, "ach_check_book_count", 100,
        ([ "title": HIC "博学多识" NOR, "attr_intelligence": 2 ]));

    set_achievement("ach_collect_03", "博物学家", ACH_CAT_COLLECTION, "收集50种不同的灵药",
        10, "ach_check_herb_count", 50,
        ([ "title": HIG "百草通" NOR, "attr_alchemy": 3 ]));

    set_achievement("ach_collect_04", "灵药大师", ACH_CAT_COLLECTION, "收集100种不同的灵药",
        10, "ach_check_herb_count", 100,
        ([ "title": HIY "灵药大师" NOR, "attr_alchemy": 5 ]));

    set_achievement("ach_collect_05", "矿石收藏家", ACH_CAT_COLLECTION, "收集30种不同的矿石",
        10, "ach_check_ore_count", 30,
        ([ "title": HIC "寻矿猎手" NOR, "attr_mining": 10 ]));

    set_achievement("ach_collect_06", "法宝收藏家", ACH_CAT_COLLECTION, "收集20件不同的法宝",
        10, "ach_check_item_count", 20,
        ([ "title": HIY "多宝道人" NOR, "attr_item_efficiency": 5 ]));

    set_achievement("ach_collect_07", "套装收集者", ACH_CAT_COLLECTION, "集齐一套完整套装",
        10, "ach_check_set_count", 1,
        ([ "title": HIR "套装收集者" NOR ]));

    set_achievement("ach_collect_08", "丹药大全", ACH_CAT_COLLECTION, "收集30种不同的丹药",
        10, "ach_check_pill_count", 30,
        ([ "title": HIM "丹药大全" NOR, "attr_pill_efficiency": 5 ]));

    set_achievement("ach_collect_09", "材料达人", ACH_CAT_COLLECTION, "收集50种不同的材料",
        10, "ach_check_material_count", 50,
        ([ "title": HIC "材料达人" NOR ]));

    set_achievement("ach_collect_10", "图鉴达人", ACH_CAT_COLLECTION, "收集200种不同的物品",
        10, "ach_check_total_collect", 200,
        ([ "title": HIY "图鉴达人" NOR, "attr_bag_space": 10 ]));

    // ---------- 探索成就 (8) ----------
    set_achievement("ach_explore_01", "初行天下", ACH_CAT_EXPLORATION, "访问10个不同的区域",
        10, "ach_check_area_count", 10,
        ([ "title": HIW "初行天下" NOR ]));

    set_achievement("ach_explore_02", "四海为家", ACH_CAT_EXPLORATION, "访问30个不同的区域",
        10, "ach_check_area_count", 30,
        ([ "title": HIC "行者无疆" NOR, "attr_move_speed": 5 ]));

    set_achievement("ach_explore_03", "大地行者", ACH_CAT_EXPLORATION, "访问80个不同的区域",
        10, "ach_check_area_count", 80,
        ([ "title": HIY "踏遍山河" NOR, "attr_travel_cost": 50 ]));

    set_achievement("ach_explore_04", "位面旅者", ACH_CAT_EXPLORATION, "访问人界、乱星海、灵界",
        10, "ach_check_realm_visit", 3,
        ([ "title": HIM "三界行者" NOR, "attr_reputation_rate": 5 ]));

    set_achievement("ach_explore_05", "秘境探索者", ACH_CAT_EXPLORATION, "进入5个不同的秘境",
        10, "ach_check_secret_count", 5,
        ([ "title": HIC "秘境探索者" NOR ]));

    set_achievement("ach_explore_06", "天涯海角", ACH_CAT_EXPLORATION, "访问所在世界的尽头区域",
        10, "ach_check_world_end", 1,
        ([ "title": HIG "天涯海角" NOR, "attr_move_speed": 3 ]));

    set_achievement("ach_explore_07", "洞天福地", ACH_CAT_EXPLORATION, "发现5处隐藏洞天",
        10, "ach_check_hidden_area", 5,
        ([ "title": HIY "洞天福地" NOR, "attr_practice_bonus": 2 ]));

    set_achievement("ach_explore_08", "地标收集者", ACH_CAT_EXPLORATION, "访问所有主要城市",
        10, "ach_check_all_cities", 1,
        ([ "title": HIR "地标收集者" NOR, "attr_travel_cost": 30 ]));

    // ---------- 社交成就 (6) ----------
    set_achievement("ach_social_01", "初交知己", ACH_CAT_SOCIAL, "添加第一位好友",
        10, "ach_check_friend_count", 1,
        ([ "title": HIW "初交知己" NOR ]));

    set_achievement("ach_social_02", "广结善缘", ACH_CAT_SOCIAL, "拥有10位好友",
        10, "ach_check_friend_count", 10,
        ([ "title": HIC "广结善缘" NOR, "attr_exp_team": 3 ]));

    set_achievement("ach_social_03", "众星捧月", ACH_CAT_SOCIAL, "拥有50位好友",
        10, "ach_check_friend_count", 50,
        ([ "title": HIG "众星捧月" NOR, "attr_exp_team": 5 ]));

    set_achievement("ach_social_04", "组队达人", ACH_CAT_SOCIAL, "组队完成100次战斗",
        10, "ach_check_team_count", 100,
        ([ "title": HIC "组队达人" NOR, "attr_team_bonus": 5 ]));

    set_achievement("ach_social_05", "商业巨贾", ACH_CAT_SOCIAL, "累计交易额达100,000灵石",
        10, "ach_check_trade_amount", 100000,
        ([ "title": HIY "商业巨贾" NOR, "attr_trade_discount": 5 ]));

    set_achievement("ach_social_06", "名师高徒", ACH_CAT_SOCIAL, "收徒3人以上",
        10, "ach_check_apprentice_count", 3,
        ([ "title": HIM "名师高徒" NOR, "attr_mentor_bonus": 5 ]));

    // ---------- 生活成就 (8) ----------
    set_achievement("ach_life_01", "炼丹入门", ACH_CAT_LIFE, "炼丹技艺达到50级",
        10, "ach_check_skill_level", ({ "alchemy", 50 }),
        ([ "title": HIW "炼丹入门" NOR, "attr_alchemy_rate": 3 ]));

    set_achievement("ach_life_02", "炼丹大师", ACH_CAT_LIFE, "炼丹技艺达到200级",
        10, "ach_check_skill_level", ({ "alchemy", 200 }),
        ([ "title": HIY "炼丹大师" NOR, "attr_alchemy_rate": 8 ]));

    set_achievement("ach_life_03", "炼器入门", ACH_CAT_LIFE, "炼器技艺达到50级",
        10, "ach_check_skill_level", ({ "forge", 50 }),
        ([ "title": HIW "炼器入门" NOR, "attr_forge_rate": 3 ]));

    set_achievement("ach_life_04", "炼器大师", ACH_CAT_LIFE, "炼器技艺达到200级",
        10, "ach_check_skill_level", ({ "forge", 200 }),
        ([ "title": HIY "炼器大师" NOR, "attr_forge_rate": 8 ]));

    set_achievement("ach_life_05", "采集达人", ACH_CAT_LIFE, "采集技艺达到100级",
        10, "ach_check_skill_level", ({ "gather", 100 }),
        ([ "title": HIC "采集达人" NOR, "attr_gather_rate": 10 ]));

    set_achievement("ach_life_06", "垂钓高手", ACH_CAT_LIFE, "钓鱼技艺达到100级",
        10, "ach_check_skill_level", ({ "fishing", 100 }),
        ([ "title": HIG "垂钓高手" NOR, "attr_fishing_bonus": 10 ]));

    set_achievement("ach_life_07", "百工之长", ACH_CAT_LIFE, "三项生活技艺均达到100级",
        10, "ach_check_multi_skill", 3,
        ([ "title": HIY "百工之长" NOR, "attr_craft_rate": 5 ]));

    set_achievement("ach_life_08", "生活大师", ACH_CAT_LIFE, "所有生活技艺均达到150级",
        10, "ach_check_all_life_skills", 150,
        ([ "title": HIR "生活大师" NOR, "attr_all_craft": 10 ]));

    // ---------- 隐藏成就 (6) ----------
    set_achievement("ach_hidden_01", "天选之人", ACH_CAT_HIDDEN, "创建角色时roll到天灵根",
        30, "ach_check_heaven_roots", 1,
        ([ "title": HIM "天命所归" NOR ]));

    set_achievement("ach_hidden_02", "逆天改命", ACH_CAT_HIDDEN, "伪灵根玩家成功结丹",
        30, "ach_check_pseudo_realm", ({ "结丹", "初期" }),
        ([ "title": HIR "逆天者" NOR, "attr_breakthrough": 5 ]));

    set_achievement("ach_hidden_03", "掌天瓶之主", ACH_CAT_HIDDEN, "完成掌天瓶全部觉醒事件",
        30, "ach_check_bottle_awaken", 1,
        ([ "title": HIM "瓶中人" NOR, "attr_bottle_efficiency": 10 ]));

    set_achievement("ach_hidden_04", "孤勇者", ACH_CAT_HIDDEN, "单人通关血色禁地",
        30, "ach_check_solo_dungeon", 1,
        ([ "title": HIY "独行侠" NOR, "attr_solo_bonus": 15 ]));

    set_achievement("ach_hidden_05", "富可敌国", ACH_CAT_HIDDEN, "累计交易额达1,000,000灵石",
        30, "ach_check_trade_million", 1000000,
        ([ "title": HIY "富可敌国" NOR, "attr_trade_discount": 10 ]));

    set_achievement("ach_hidden_06", "轮回者", ACH_CAT_HIDDEN, "完成所有支线任务和主线任务",
        30, "ach_check_all_quests", 1,
        ([ "title": HBRED "完美道标" NOR, "attr_all_stats": 2 ]));

    // 构建分类索引
    rebuild_index();
}

// 注册单个成就
void set_achievement(string id, string name, string category, string desc,
    int score, string check_func, mixed target, mapping rewards)
{
    achievements[id] = ([
        "id": id,
        "name": name,
        "category": category,
        "description": desc,
        "score": score,
        "check_func": check_func,
        "target": target,
        "rewards": rewards,
        "sort_order": sizeof(achievements),
    ]);
}

// 重建分类索引
void rebuild_index()
{
    string *ids, id, cat;
    by_category = ([]);

    ids = keys(achievements);
    for (int i = 0; i < sizeof(ids); i++)
    {
        id = ids[i];
        cat = achievements[id]["category"];
        if (!by_category[cat])
            by_category[cat] = ({});
        by_category[cat] += ({ id });
    }
}

// ==================== 查询接口 ====================

// 获取所有成就定义
mapping query_all_achievements()
{
    return achievements + ([]);  // 返回副本
}

// 获取某分类成就列表
string *query_achievements_by_category(string category)
{
    if (!by_category[category])
        return ({});
    return by_category[category] + ({});
}

// 获取单个成就定义
mapping query_achievement(string id)
{
    if (!achievements[id])
        return 0;
    return achievements[id] + ([]);
}

// 获取成就总分
int get_achievement_score(object player)
{
    return player->query("achievement/score");
}

// 获取已解锁成就列表
string *get_unlocked_achievements(object player)
{
    mapping ach_data = player->query("achievement");
    if (!ach_data || !ach_data["unlocked"])
        return ({});
    return ach_data["unlocked"];
}

// 获取成就数量
int get_achievement_count(object player)
{
    return sizeof(get_unlocked_achievements(player));
}

// 获取可解锁成就列表（未解锁但条件可能满足）
string *get_available_achievements(object player)
{
    string *all, *unlocked, *available;
    int i;
    mapping ach_data;

    all = keys(achievements);
    unlocked = get_unlocked_achievements(player);
    available = ({});

    for (i = 0; i < sizeof(all); i++)
    {
        if (member_array(all[i], unlocked) == -1)
            available += ({ all[i] });
    }
    return available;
}

// ==================== 核心检测 ====================

// 尝试检测并解锁指定成就（返回1=新解锁，0=未解锁或已解锁）
int try_unlock(object player, string ach_id)
{
    string *unlocked;
    string check_func;
    mixed target;
    int result;

    if (!player || !achievements[ach_id])
        return 0;

    // 已解锁则跳过
    unlocked = get_unlocked_achievements(player);
    if (member_array(ach_id, unlocked) != -1)
        return 0;

    check_func = achievements[ach_id]["check_func"];
    target = achievements[ach_id]["target"];

    // 调用检测函数
    result = call_other(this_object(), check_func, player, target);
    if (!result)
        return 0;

    // 解锁成就
    unlock_achievement(player, ach_id);
    return 1;
}

// 批量检测：检查玩家所有可解锁成就
int check_all(object player)
{
    string *all_ids, *unlocked;
    int i, new_count;

    if (!player)
        return 0;

    all_ids = keys(achievements);
    unlocked = get_unlocked_achievements(player);
    new_count = 0;

    for (i = 0; i < sizeof(all_ids); i++)
    {
        if (member_array(all_ids[i], unlocked) == -1)
        {
            if (try_unlock(player, all_ids[i]))
                new_count++;
        }
    }
    return new_count;
}

// 按分类批量检测
int check_category(object player, string category)
{
    string *cat_ids, *unlocked;
    int i, new_count;

    if (!player || !by_category[category])
        return 0;

    cat_ids = by_category[category];
    unlocked = get_unlocked_achievements(player);
    new_count = 0;

    for (i = 0; i < sizeof(cat_ids); i++)
    {
        if (member_array(cat_ids[i], unlocked) == -1)
        {
            if (try_unlock(player, cat_ids[i]))
                new_count++;
        }
    }
    return new_count;
}

// ==================== 成就解锁 ====================

// 解锁成就（内部）
void unlock_achievement(object player, string ach_id)
{
    mapping ach_data, reward, player_ach;
    string *unlocked;
    string msg;
    int score;

    if (!player || !achievements[ach_id])
        return;

    // 更新玩家数据
    player_ach = player->query("achievement");
    if (!player_ach)
        player_ach = ([]);

    unlocked = player_ach["unlocked"];
    if (!unlocked)
        unlocked = ({});

    if (member_array(ach_id, unlocked) != -1)
        return;

    unlocked += ({ ach_id });
    score = (player_ach["score"] || 0) + achievements[ach_id]["score"];

    player->set("achievement/unlocked", unlocked);
    player->set("achievement/score", score);
    player->set("achievement/last_unlock_time", time());

    // 发送解锁消息
    msg = HIG "\n╔══════════════════════════════════╗\n" NOR;
    msg += HIG "║        ★ 成就解锁！ ★            ║\n" NOR;
    msg += HIG "╠══════════════════════════════════╣\n" NOR;
    msg += sprintf("  成就：%s\n", achievements[ach_id]["name"]);
    msg += sprintf("  分类：%s\n", category_to_chinese(achievements[ach_id]["category"]));
    msg += sprintf("  描述：%s\n", achievements[ach_id]["description"]);
    msg += sprintf("  分值：%d 分\n", achievements[ach_id]["score"]);

    // 发放奖励
    reward = achievements[ach_id]["rewards"];
    if (reward)
    {
        msg += "  奖励：";
        if (reward["title"])
        {
            msg += reward["title"] + " ";
            // 发放称号
            distribute_title_reward(player, ach_id);
        }
        msg += "\n";
    }

    msg += HIG "╚══════════════════════════════════╝\n" NOR;
    tell_object(player, msg);

    // 设置通知标志
    player->set("achievement/new_notification", 1);

    // 广播（可选）
    if (achievements[ach_id]["score"] >= 20)
    {
        message("system", sprintf(HIG "【成就】%s完成了成就「%s」！\n" NOR,
            player->name(1), achievements[ach_id]["name"]), users());
    }

    // 检查段位奖励
    check_tier_rewards(player);
}

// ==================== 检查函数 ====================

// 检查境界
int ach_check_realm(object player, mixed target)
{
    string *req_realm = target;
    string player_realm, player_sub;

    player_realm = player->query("realm");
    player_sub = player->query("realm_sub");

    if (!player_realm || !player_sub)
        return 0;

    // 比较境界（简化版：按字符串比较）
    // 实际应使用境界索引比较，这里用简单方式
    return ach_realm_compare(player_realm, player_sub, req_realm[0], req_realm[1]);
}

// 境界比较辅助
int ach_realm_compare(string realm, string sub, string req_realm, string req_sub)
{
    // 境界顺序
    string *realm_order = ({ "凡人", "炼气", "筑基", "结丹", "元婴", "化神",
                            "炼虚", "合体", "大乘" });
    string *sub_order = ({ "初期", "中期", "后期", "巅峰", "大圆满", "圆满" });
    int ri, rs, req_ri, req_rs;

    ri = member_array(realm, realm_order);
    req_ri = member_array(req_realm, realm_order);
    if (ri < 0 || req_ri < 0) return 0;
    if (ri < req_ri) return 0;
    if (ri > req_ri) return 1;

    // 同一境界比较子境界
    rs = member_array(sub, sub_order);
    req_rs = member_array(req_sub, sub_order);
    if (rs < 0 || req_rs < 0) return 0;
    return rs >= req_rs;
}

// 检查任务完成数
int ach_check_task_count(object player, mixed target)
{
    int total = player->query("jobs/completed/total") ||
                player->query("quest/completed") || 0;
    return total >= (int)target;
}

// 检查日常连续天数
int ach_check_daily_streak(object player, mixed target)
{
    int streak = player->query("quest/daily_streak") || 0;
    return streak >= (int)target;
}

// 检查支线完成数
int ach_check_side_quest(object player, mixed target)
{
    int count = player->query("quest/side_completed") || 0;
    return count >= (int)target;
}

// 检查主线章节完成数
int ach_check_main_chapter(object player, mixed target)
{
    int chapter = player->query("quest/main_chapter") || 0;
    return chapter >= (int)target;
}

// 检查成就数量
int ach_check_achievement_count(object player, mixed target)
{
    int count = get_achievement_count(player);
    return count >= (int)target;
}

// 检查周常完成数
int ach_check_weekly_count(object player, mixed target)
{
    int count = player->query("quest/weekly_completed") || 0;
    return count >= (int)target;
}

// 检查击杀数
int ach_check_kill_count(object player, mixed target)
{
    int kills = player->query("combat/kills") || 0;
    return kills >= (int)target;
}

// 检查单场击杀数
int ach_check_mass_kill(object player, mixed target)
{
    int max_kill = player->query("combat/max_single_kill") || 0;
    return max_kill >= (int)target;
}

// 检查越级挑战
int ach_check_cross_realm(object player, mixed target)
{
    int cross = player->query("combat/max_cross_realm") || 0;
    return cross >= (int)target;
}

// 检查PVP连胜
int ach_check_pvp_streak(object player, mixed target)
{
    int streak = player->query("combat/pvp_max_streak") || 0;
    return streak >= (int)target;
}

// 检查首次通关副本
int ach_check_first_dungeon(object player, mixed target)
{
    int cleared = player->query("dungeon/cleared_count") || 0;
    return cleared >= 1;
}

// 检查全部副本
int ach_check_all_dungeons(object player, mixed target)
{
    int all = player->query("dungeon/all_cleared") || 0;
    return all >= 1;
}

// 检查妖兽击杀
int ach_check_beast_kill(object player, mixed target)
{
    int kills = player->query("combat/beast_kills") || 0;
    return kills >= (int)target;
}

// 检查魔物击杀
int ach_check_demon_kill(object player, mixed target)
{
    int kills = player->query("combat/demon_kills") || 0;
    return kills >= (int)target;
}

// 检查无死亡战斗数
int ach_check_no_death(object player, mixed target)
{
    int streak = player->query("combat/no_death_streak") || 0;
    return streak >= (int)target;
}

// 检查功法收集数
int ach_check_book_count(object player, mixed target)
{
    int count = player->query("collect/book_count") || 0;
    return count >= (int)target;
}

// 检查灵药收集数
int ach_check_herb_count(object player, mixed target)
{
    int count = player->query("collect/herb_count") || 0;
    return count >= (int)target;
}

// 检查矿石收集数
int ach_check_ore_count(object player, mixed target)
{
    int count = player->query("collect/ore_count") || 0;
    return count >= (int)target;
}

// 检查法宝收集数
int ach_check_item_count(object player, mixed target)
{
    int count = player->query("collect/item_count") || 0;
    return count >= (int)target;
}

// 检查套装收集
int ach_check_set_count(object player, mixed target)
{
    int count = player->query("collect/set_count") || 0;
    return count >= (int)target;
}

// 检查丹药收集数
int ach_check_pill_count(object player, mixed target)
{
    int count = player->query("collect/pill_count") || 0;
    return count >= (int)target;
}

// 检查材料收集数
int ach_check_material_count(object player, mixed target)
{
    int count = player->query("collect/material_count") || 0;
    return count >= (int)target;
}

// 检查总收集数
int ach_check_total_collect(object player, mixed target)
{
    int count = player->query("collect/total_count") || 0;
    return count >= (int)target;
}

// 检查区域访问数
int ach_check_area_count(object player, mixed target)
{
    int count = player->query("explore/area_count") || 0;
    return count >= (int)target;
}

// 检查访问的世界数
int ach_check_realm_visit(object player, mixed target)
{
    int count = player->query("explore/realm_visited") || 0;
    return count >= (int)target;
}

// 检查秘境数
int ach_check_secret_count(object player, mixed target)
{
    int count = player->query("explore/secret_count") || 0;
    return count >= (int)target;
}

// 检查世界尽头
int ach_check_world_end(object player, mixed target)
{
    return player->query("explore/world_end") || 0;
}

// 检查隐藏区域数
int ach_check_hidden_area(object player, mixed target)
{
    int count = player->query("explore/hidden_area_count") || 0;
    return count >= (int)target;
}

// 检查所有城市
int ach_check_all_cities(object player, mixed target)
{
    return player->query("explore/all_cities") || 0;
}

// 检查好友数
int ach_check_friend_count(object player, mixed target)
{
    int count = player->query("social/friend_count") || 0;
    return count >= (int)target;
}

// 检查组队次数
int ach_check_team_count(object player, mixed target)
{
    int count = player->query("social/team_count") || 0;
    return count >= (int)target;
}

// 检查交易额
int ach_check_trade_amount(object player, mixed target)
{
    int amount = player->query("social/trade_amount") || 0;
    return amount >= (int)target;
}

// 检查收徒数
int ach_check_apprentice_count(object player, mixed target)
{
    int count = player->query("social/apprentice_count") || 0;
    return count >= (int)target;
}

// 检查技能等级
int ach_check_skill_level(object player, mixed target)
{
    string skill = target[0];
    int level = target[1];
    int player_level = player->query_skill(skill, 1);
    return player_level >= level;
}

// 检查多项技能
int ach_check_multi_skill(object player, mixed target)
{
    int required = target;
    string *life_skills = ({ "alchemy", "forge", "gather", "fishing" });
    int count = 0;

    for (int i = 0; i < sizeof(life_skills); i++)
    {
        if (player->query_skill(life_skills[i], 1) >= 100)
            count++;
    }
    return count >= required;
}

// 检查所有生活技能
int ach_check_all_life_skills(object player, mixed target)
{
    int level = target;
    string *life_skills = ({ "alchemy", "forge", "gather", "fishing" });

    for (int i = 0; i < sizeof(life_skills); i++)
    {
        if (player->query_skill(life_skills[i], 1) < level)
            return 0;
    }
    return 1;
}

// 检查天灵根
int ach_check_heaven_roots(object player, mixed target)
{
    string roots = player->query("spirit_roots");
    return roots && strsrch(roots, "天灵根") >= 0;
}

// 检查伪灵根结丹
int ach_check_pseudo_realm(object player, mixed target)
{
    string roots = player->query("spirit_roots");
    if (!roots || strsrch(roots, "伪灵根") < 0)
        return 0;
    return ach_check_realm(player, target);
}

// 检查掌天瓶觉醒
int ach_check_bottle_awaken(object player, mixed target)
{
    return player->query("quest/bottle_awakened") || 0;
}

// 检查单人副本
int ach_check_solo_dungeon(object player, mixed target)
{
    return player->query("dungeon/solo_clear") || 0;
}

// 检查百万交易
int ach_check_trade_million(object player, mixed target)
{
    int amount = player->query("social/trade_amount") || 0;
    return amount >= (int)target;
}

// 检查全任务
int ach_check_all_quests(object player, mixed target)
{
    int main_done = player->query("quest/main_completed") || 0;
    int side_all = player->query("quest/all_side_completed") || 0;
    return main_done && side_all;
}

// ==================== 奖励发放 ====================

// 发放称号
void distribute_title_reward(object player, string ach_id)
{
    mapping reward = achievements[ach_id]["rewards"];
    string title;

    if (!reward || !reward["title"])
        return;

    title = reward["title"];
    // 设置称号（使用玩家称号系统）
    player->set("achievement/titles/" + ach_id, title);

    // 如果有成就系统专属称号位，注册到称号系统
    // 这里调用 RANK_D 或 TITLE_D 的接口
    // 具体实现取决于游戏称号系统
    tell_object(player, sprintf(HIG "你获得了称号：%s\n" NOR, title));
}

// 获取符合条件的段位
int query_tier(int score)
{
    if (score >= ACH_TIER_5) return 5;
    if (score >= ACH_TIER_4) return 4;
    if (score >= ACH_TIER_3) return 3;
    if (score >= ACH_TIER_2) return 2;
    if (score >= ACH_TIER_1) return 1;
    return 0;
}

// 检查段位奖励
void check_tier_rewards(object player)
{
    int score = get_achievement_score(player);
    int current_tier = query_tier(score);
    int last_tier = player->query("achievement/last_tier") || 0;

    if (current_tier > last_tier)
    {
        // 新段位达成
        for (int i = last_tier + 1; i <= current_tier; i++)
        {
            distribute_tier_reward(player, i);
        }
        player->set("achievement/last_tier", current_tier);
    }
}

// 发放段位奖励
void distribute_tier_reward(object player, int tier)
{
    mapping tier_info;
    string msg;

    switch (tier)
    {
    case 1:
        tier_info = ([
            "name": "成就新星",
            "attr": ([ "practice_speed": 2 ]),
        ]);
        break;
    case 2:
        tier_info = ([
            "name": "成就达人",
            "attr": ([ "all_stats": 1 ]),
        ]);
        break;
    case 3:
        tier_info = ([
            "name": "成就宗师",
            "attr": ([ "all_stats": 2 ]),
        ]);
        break;
    case 4:
        tier_info = ([
            "name": "成就传说",
            "attr": ([ "all_stats": 3 ]),
        ]);
        break;
    case 5:
        tier_info = ([
            "name": "全境圆满",
            "attr": ([ "all_stats": 5 ]),
        ]);
        break;
    default:
        return;
    }

    msg = sprintf(HIY "\n★★★ 成就段位突破：「%s」★★★\n" NOR, tier_info["name"]);
    tell_object(player, msg);

    // 记录段位称号
    player->set("achievement/tier_name", tier_info["name"]);
}

// ==================== 辅助函数 ====================

// 分类名转中文
string category_to_chinese(string category)
{
    switch (category)
    {
    case ACH_CAT_CULTIVATION:    return ACH_CAT_CN_CULTIVATION;
    case ACH_CAT_TASK:           return ACH_CAT_CN_TASK;
    case ACH_CAT_COMBAT:         return ACH_CAT_CN_COMBAT;
    case ACH_CAT_COLLECTION:     return ACH_CAT_CN_COLLECTION;
    case ACH_CAT_EXPLORATION:    return ACH_CAT_CN_EXPLORATION;
    case ACH_CAT_SOCIAL:         return ACH_CAT_CN_SOCIAL;
    case ACH_CAT_LIFE:           return ACH_CAT_CN_LIFE;
    case ACH_CAT_HIDDEN:         return ACH_CAT_CN_HIDDEN;
    default:                     return "未知";
    }
}

// 获取成就进度字符串（简化版 - 可用于显示进度条）
string get_achievement_progress(object player, string ach_id)
{
    mapping ach = achievements[ach_id];
    if (!ach) return "";

    string check_func = ach["check_func"];
    mixed target = ach["target"];
    int current = 0;
    int required = 0;

    // 根据检测函数类型获取当前进度
    if (check_func == "ach_check_kill_count" ||
        check_func == "ach_check_beast_kill" ||
        check_func == "ach_check_demon_kill")
    {
        if (check_func == "ach_check_kill_count")
            current = player->query("combat/kills") || 0;
        else if (check_func == "ach_check_beast_kill")
            current = player->query("combat/beast_kills") || 0;
        else
            current = player->query("combat/demon_kills") || 0;
        required = target;
    }
    else if (check_func == "ach_check_task_count")
    {
        current = player->query("quest/completed") || 0;
        required = target;
    }
    else if (check_func == "ach_check_side_quest")
    {
        current = player->query("quest/side_completed") || 0;
        required = target;
    }
    else if (check_func == "ach_check_area_count")
    {
        current = player->query("explore/area_count") || 0;
        required = target;
    }
    else if (check_func == "ach_check_friend_count")
    {
        current = player->query("social/friend_count") || 0;
        required = target;
    }
    else if (check_func == "ach_check_skill_level")
    {
        current = player->query_skill(target[0], 1);
        required = target[1];
    }
    else if (check_func == "ach_check_book_count" ||
             check_func == "ach_check_herb_count" ||
             check_func == "ach_check_item_count" ||
             check_func == "ach_check_pill_count" ||
             check_func == "ach_check_material_count" ||
             check_func == "ach_check_total_collect")
    {
        string field;
        if (strsrch(check_func, "book") >= 0) field = "book_count";
        else if (strsrch(check_func, "herb") >= 0) field = "herb_count";
        else if (strsrch(check_func, "ore") >= 0) field = "ore_count";
        else if (strsrch(check_func, "pill") >= 0) field = "pill_count";
        else if (strsrch(check_func, "material") >= 0) field = "material_count";
        else field = "total_count";
        current = player->query("collect/" + field) || 0;
        required = target;
    }
    else if (check_func == "ach_check_weekly_count")
    {
        current = player->query("quest/weekly_completed") || 0;
        required = target;
    }

    if (required > 0)
        return sprintf("%d/%d", current, required);
    return "";
}

// 获取成就完成状态
int is_achievement_unlocked(object player, string ach_id)
{
    string *unlocked = get_unlocked_achievements(player);
    return member_array(ach_id, unlocked) != -1;
}

// ==================== 事件钩子 ====================

// 玩家击杀怪物时调用
void on_kill(object player, string monster_type, int count)
{
    check_category(player, ACH_CAT_COMBAT);
    // 妖兽击杀
    if (monster_type == "beast")
    {
        check_category(player, ACH_CAT_COMBAT);
    }
}

// 完成任务时调用
void on_quest_complete(object player, string quest_type)
{
    check_category(player, ACH_CAT_TASK);
}

// 玩家境界突破时调用
void on_realm_breakthrough(object player)
{
    check_category(player, ACH_CAT_CULTIVATION);
}

// 玩家访问新区域时调用
void on_area_discover(object player)
{
    check_category(player, ACH_CAT_EXPLORATION);
}

// 玩家收集物品时调用
void on_collect(object player)
{
    check_category(player, ACH_CAT_COLLECTION);
}

// 玩家社交动作时调用
void on_social_action(object player)
{
    check_category(player, ACH_CAT_SOCIAL);
}

// 玩家生活技能升级时调用
void on_skill_upgrade(object player)
{
    check_category(player, ACH_CAT_LIFE);
}

// ==================== 调试与管理 ====================

// 列出所有成就
string list_achievements()
{
    string *ids = keys(achievements);
    string result = "成就列表:\n";
    string id;

    for (int i = 0; i < sizeof(ids); i++)
    {
        id = ids[i];
        result += sprintf("%-20s %-16s %s (分值:%d)\n",
            id, achievements[id]["name"],
            category_to_chinese(achievements[id]["category"]),
            achievements[id]["score"]);
    }
    return result;
}

// 统计信息
string query_statistics()
{
    string msg;
    int total = sizeof(achievements);
    mapping cat_count = ([]);
    string *cats, cat;

    cats = keys(by_category);
    for (int i = 0; i < sizeof(cats); i++)
    {
        cat = cats[i];
        cat_count[category_to_chinese(cat)] = sizeof(by_category[cat]);
    }

    msg = sprintf("成就统计:\n总数: %d\n", total);
    msg += "分类分布:\n";
    foreach (cat, int count in cat_count)
    {
        msg += sprintf("  %s: %d\n", cat, count);
    }
    return msg;
}
