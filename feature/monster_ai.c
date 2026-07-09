// monster_ai.c
// PVE 怪物 AI 系统特征（mixin）
// 继承此特征的 NPC 获得智能行为决策能力
// 基于《凡人修仙传》设定，4级 AI 分级
// Created for #21 核心：战斗机制增强

#include <ansi.h>
#include <element.h>

// ===== AI 等级常量 =====
#define AI_LEVEL_BASIC      1   // 低级：随机攻击最近目标
#define AI_LEVEL_TACTICAL   2   // 中级：优先低防、技能搭配
#define AI_LEVEL_ADVANCED   3   // 高级：判断局势、技能组合
#define AI_LEVEL_BOSS       4   // BOSS级：多阶段、召唤、护盾、狂暴

// AI 行动类型
#define AI_ACTION_ATTACK    1   // 普通攻击
#define AI_ACTION_SKILL     2   // 使用技能
#define AI_ACTION_DEFEND    3   // 防御/恢复
#define AI_ACTION_MOVE      4   // 移动
#define AI_ACTION_FLEE      5   // 逃跑
#define AI_ACTION_SPECIAL   6   // 特殊行动（BOSS阶段切换等）

// ===== AI 配置 =====
// 存放在 NPC 的 dbase 中 query("monster_ai")
// 若未设置，默认使用 AI_LEVEL_BASIC

nosave mapping ai_status = ([]);  // AI 运行时状态

// ===== 初始化 AI =====

// 设置 NPC 的 AI 等级
// 应在 NPC 的 create() 中调用
int set_ai_level(int level)
{
    if (level < AI_LEVEL_BASIC || level > AI_LEVEL_BOSS)
        return 0;
    set("monster_ai/level", level);
    return 1;
}

// 获取 AI 等级
int query_ai_level()
{
    int level = query("monster_ai/level");
    if (level < AI_LEVEL_BASIC || level > AI_LEVEL_BOSS)
    {
        // 默认低级 AI
        set_ai_level(AI_LEVEL_BASIC);
        return AI_LEVEL_BASIC;
    }
    return level;
}

// ===== 核心 AI 决策 =====

// 选择攻击目标（被 combatd.c 的 choose_offensive_target 调用）
// 返回要攻击的 object，或 0 表示让系统默认选择
object choose_offensive_target()
{
    object me = this_object();
    int ai_level = query_ai_level();

    if (ai_level < AI_LEVEL_TACTICAL)
        return 0;  // 低级 AI 让系统默认选择

    object *enemies = me->query_enemy();
    if (!arrayp(enemies) || sizeof(enemies) < 1)
        return 0;

    // 中级以上 AI：分析目标
    object *candidates = ({});

    foreach (object ob in enemies)
    {
        if (!objectp(ob) || !living(ob))
            continue;
        if (environment(ob) != environment(me))
            continue;

        candidates += ({ ob });
    }

    if (sizeof(candidates) < 1)
        return 0;

    // 根据不同 AI 等级选择目标
    switch (ai_level)
    {
    case AI_LEVEL_TACTICAL:
        // 中级：优先攻击防御最低的敌人
        return select_lowest_defense(candidates);

    case AI_LEVEL_ADVANCED:
        // 高级：优先集火低血量、被控制的敌人
        {
            object focus = select_focus_target(candidates);
            if (focus)
                return focus;
            return select_lowest_defense(candidates);
        }

    case AI_LEVEL_BOSS:
        // BOSS：攻击威胁最大的敌人
        {
            object threat = select_highest_threat(candidates);
            if (threat)
                return threat;
            return select_lowest_defense(candidates);
        }
    }

    return 0;
}

// 选防御最低的目标
object select_lowest_defense(object *candidates)
{
    object target = 0;
    int min_def = 999999;

    foreach (object ob in candidates)
    {
        int def = ob->query_temp("apply/defense");
        if (def < min_def)
        {
            min_def = def;
            target = ob;
        }
    }

    return target;
}

// 选集火目标（低血量或受控）
object select_focus_target(object *candidates)
{
    // 优先找血量低于 30% 的目标
    foreach (object ob in candidates)
    {
        int qi = ob->query("qi");
        int max_qi = ob->query("max_qi");
        if (max_qi > 0 && qi * 100 / max_qi < 30)
        {
            return ob;  // 低血量，优先击杀
        }
    }

    // 找被控制的目标
    foreach (object ob in candidates)
    {
        if (ob->is_busy())
        {
            return ob;  // 被控制，无法还手
        }
    }

    return 0;
}

// 选威胁最高的目标（BOSS 用）
object select_highest_threat(object *candidates)
{
    object target = 0;
    int max_threat = 0;

    foreach (object ob in candidates)
    {
        int threat = 0;
        threat += ob->query_temp("apply/damage");       // 攻击力
        threat += ob->query_skill("force") / 10;        // 内力修为
        threat += ob->query("combat_exp") / 10000;      // 经验

        // 五行克制修正：如果目标被自身属性克制，威胁值提高
        int my_elem = query_character_element(this_object());
        int target_elem = query_character_element(ob);
        if (my_elem != ELE_NONE && target_elem != ELE_NONE)
        {
            float elem_mod = calc_element_modifier(my_elem, target_elem);
            threat = to_int(threat * elem_mod);
        }

        if (threat > max_threat)
        {
            max_threat = threat;
            target = ob;
        }
    }

    return target;
}

// ===== 战斗行动决策 =====

// AI 每回合行动决策（可由 heart_beat 或 combat_tick 调用）
// 返回建议的行动类型
int decide_action()
{
    object me = this_object();
    int ai_level = query_ai_level();

    if (ai_level < AI_LEVEL_TACTICAL)
        return AI_ACTION_ATTACK;

    // 血量状态检测
    int qi = me->query("qi");
    int max_qi = me->query("max_qi");
    int qi_ratio = max_qi > 0 ? qi * 100 / max_qi : 100;

    object *enemies = me->query_enemy();
    int enemy_count = arrayp(enemies) ? sizeof(enemies) : 0;

    switch (ai_level)
    {
    case AI_LEVEL_TACTICAL:
        // 中级 AI
        // 低血量时恢复
        if (qi_ratio < 30 && random(100) < 40)
            return AI_ACTION_DEFEND;

        // 有技能就使用技能
        if (has_offensive_skill() && random(100) < 60)
            return AI_ACTION_SKILL;

        return AI_ACTION_ATTACK;

    case AI_LEVEL_ADVANCED:
    case AI_LEVEL_BOSS:
        {
            // 高级 / BOSS AI

            // 濒死狂暴（仅 BOSS）
            if (ai_level == AI_LEVEL_BOSS && qi_ratio < 20 && random(100) < 70)
                return AI_ACTION_SPECIAL;

            // 低血量恢复
            if (qi_ratio < 30)
            {
                // BOSS 在低血量时狂暴而非逃跑
                if (ai_level == AI_LEVEL_BOSS && random(100) < 50)
                    return AI_ACTION_SPECIAL;  // BOSS 狂暴
                if (random(100) < 50)
                    return AI_ACTION_DEFEND;
            }

            // 被包围时使用 AOE
            if (enemy_count >= 3 && random(100) < 60)
                return AI_ACTION_SKILL;  // AOE 技能

            // 如果有低血量可秒杀的目标
            foreach (object ob in enemies)
            {
                if (!objectp(ob)) continue;
                int target_qi = ob->query("qi");
                int target_max = ob->query("max_qi");
                if (target_max > 0 && target_qi * 100 / target_max < 20)
                    return AI_ACTION_SKILL;  // 终结技
            }

            // 常规：使用技能
            if (has_offensive_skill() && random(100) < 65)
                return AI_ACTION_SKILL;

            // BOSS 阶段切换检测
            if (ai_level == AI_LEVEL_BOSS)
                check_boss_phase();

            return AI_ACTION_ATTACK;
        }
    }

    return AI_ACTION_ATTACK;
}

// ===== 技能使用 =====

// 检查怪物是否有攻击技能
int has_offensive_skill()
{
    // 检查是否配置了技能列表
    mixed *skills = query("monster_ai/skills");
    return arrayp(skills) && sizeof(skills) > 0;
}

// 选择要使用的技能（基于当前局势）
// 返回技能 ID 字符串，或 0 表示普攻
mixed choose_skill()
{
    object me = this_object();
    mixed *skills = query("monster_ai/skills");

    if (!arrayp(skills) || sizeof(skills) < 1)
        return 0;

    int ai_level = query_ai_level();

    if (ai_level == AI_LEVEL_BASIC)
    {
        // 低级：随机选一个
        return skills[random(sizeof(skills))];
    }

    // 中级以上：根据局势选择
    int qi = me->query("qi");
    int max_qi = me->query("max_qi");
    int qi_ratio = max_qi > 0 ? qi * 100 / max_qi : 100;

    object *enemies = me->query_enemy();

    // 若技能是按优先级排序的（如 [{id:..., priority:...}]）
    // 选择第一个满足条件的技能
    foreach (mixed sk in skills)
    {
        string sk_id;
        int priority = 5;
        string usage = "offensive";

        // 支持两种格式：字符串或 mapping
        if (stringp(sk))
        {
            sk_id = sk;
        }
        else if (mapp(sk))
        {
            sk_id = sk["id"];
            if (sk["priority"]) priority = sk["priority"];
            if (sk["usage"]) usage = sk["usage"];
        }
        else
        {
            continue;
        }

        // 检查技能冷却
        if (function_exists("query_skill_cooldown"))
        {
            if (query_skill_cooldown(me, sk_id) > 0)
                continue;
        }

        // 根据使用场景选择
        if (usage == "heal" && qi_ratio < 40)
            return sk_id;

        if (usage == "aoe" && arrayp(enemies) && sizeof(enemies) >= 3)
            return sk_id;

        if (usage == "finisher")
        {
            // 检查是否有低血量目标
            foreach (object ob in enemies)
            {
                if (!objectp(ob)) continue;
                int t_qi = ob->query("qi");
                int t_max = ob->query("max_qi");
                if (t_max > 0 && t_qi * 100 / t_max < 20)
                    return sk_id;
            }
        }

        // 常规攻击技能
        if (usage == "offensive" && random(10) < priority)
            return sk_id;
    }

    // 默认返回第一个技能
    return skills[random(sizeof(skills))];
}

// ===== BOSS 阶段系统 =====

// BOSS 阶段数据
// NPC 应配置 master_ai/phases 数组

// 检查是否应切换阶段
void check_boss_phase()
{
    object me = this_object();
    mixed *phases = query("monster_ai/phases");

    if (!arrayp(phases) || sizeof(phases) < 1)
        return;

    int current_phase = query_temp("monster_ai/phase");
    if (current_phase < 1) current_phase = 1;

    int qi = me->query("qi");
    int max_qi = me->query("max_qi");
    int qi_ratio = max_qi > 0 ? qi * 100 / max_qi : 100;

    // 遍历阶段定义，检查是否满足切换条件
    foreach (mixed phase in phases)
    {
        if (!mapp(phase))
            continue;

        int phase_num = phase["phase"];
        int hp_threshold = phase["hp_threshold"];

        // 如果当前血量 <= 阈值，且还没切到这个阶段
        if (qi_ratio <= hp_threshold && current_phase < phase_num)
        {
            enter_boss_phase(phase_num);
            return;
        }
    }
}

// 进入 BOSS 阶段
void enter_boss_phase(int phase_num)
{
    object me = this_object();
    mixed *phases = query("monster_ai/phases");

    if (!arrayp(phases))
        return;

    // 找到对应阶段定义
    mapping phase_data = 0;
    foreach (mixed p in phases)
    {
        if (mapp(p) && p["phase"] == phase_num)
        {
            phase_data = p;
            break;
        }
    }

    if (!mapp(phase_data))
        return;

    set_temp("monster_ai/phase", phase_num);

    string phase_name = phase_data["name"];
    tell_room(environment(me), HIR "\n※ " + me->name() + " 进入了 " + phase_name + "！※\n" NOR);

    // 执行阶段效果
    // 狂暴：提升攻速和伤害
    if (phase_data["berserk"])
    {
        me->add_temp("apply/damage", me->query_temp("apply/damage") * phase_data["berserk"] / 100);
        tell_room(environment(me), HIY me->name() + "陷入了狂暴状态！\n" NOR);
    }

    // 护盾：临时减伤
    if (phase_data["shield"])
    {
        me->set_temp("apply/abs_week_injure", phase_data["shield"]);
        tell_room(environment(me), HIC me->name() + "周身浮现出一道护盾！\n" NOR);
    }

    // 召唤
    if (phase_data["summon"])
    {
        string *summon_list = phase_data["summon"];
        int count = phase_data["summon_count"] ? phase_data["summon_count"] : 1;
        for (int i = 0; i < count; i++)
        {
            string summon_file = summon_list[random(sizeof(summon_list))];
            object summon = new(summon_file);
            if (summon)
            {
                summon->move(environment(me));
                tell_room(environment(me), HIM me->name() + "召唤了" + summon->name() + "！\n" NOR);
            }
        }
    }

    // 解锁新技能
    if (phase_data["unlock_skills"])
    {
        mixed *new_skills = phase_data["unlock_skills"];
        mixed *current_skills = query("monster_ai/skills");
        if (!arrayp(current_skills))
            current_skills = ({});
        foreach (mixed sk in new_skills)
        {
            current_skills += ({ sk });
        }
        set("monster_ai/skills", current_skills);
        tell_room(environment(me), HIW me->name() + "解锁了新的力量！\n" NOR);
    }

    // 播放阶段消息
    if (phase_data["message"])
    {
        tell_room(environment(me), phase_data["message"] + "\n");
    }
}

// ===== 逃跑判定 =====

// 判断是否应该逃跑
// 低级 AI：HP < 20% 时 30% 概率逃跑
// 中级以上：根据局势
int should_flee()
{
    object me = this_object();
    int ai_level = query_ai_level();

    // BOSS 不逃跑
    if (ai_level == AI_LEVEL_BOSS)
        return 0;

    int qi = me->query("qi");
    int max_qi = me->query("max_qi");
    int qi_ratio = max_qi > 0 ? qi * 100 / max_qi : 100;

    int flee_chance = 0;

    switch (ai_level)
    {
    case AI_LEVEL_BASIC:
        // 低级：HP < 20% 时 30% 概率
        if (qi_ratio < 20)
            flee_chance = 30;
        break;

    case AI_LEVEL_TACTICAL:
        // 中级：HP < 20% 时 50% 概率，HP < 10% 时 80% 概率
        if (qi_ratio < 10)
            flee_chance = 80;
        else if (qi_ratio < 20)
            flee_chance = 50;
        break;

    case AI_LEVEL_ADVANCED:
        // 高级：HP < 10% 时 30% 概率（更顽强）
        if (qi_ratio < 10)
            flee_chance = 30;
        break;
    }

    return random(100) < flee_chance;
}

// 执行逃跑
void perform_flee()
{
    object me = this_object();
    object *enemies = me->query_enemy();

    message_vision(HIW "$N虚晃一招，转身就逃！\n" NOR, me);

    // 清除所有敌人
    me->remove_all_enemy();
    me->remove_all_killer();
    me->start_busy(0);
}

// ===== 语音互动 =====

// 战斗中的随机喊话
void ai_say(string msg)
{
    object me = this_object();
    if (living(me) && !me->is_busy())
    {
        message_vision(me->name() + "：" + msg + "\n", me);
    }
}

// BOSS 入场台词
void boss_enter_say()
{
    string *enter_lines = query("monster_ai/enter_lines");
    if (arrayp(enter_lines) && sizeof(enter_lines) > 0)
    {
        ai_say(enter_lines[random(sizeof(enter_lines))]);
    }
}

// BOSS 死亡台词
void boss_death_say()
{
    string *death_lines = query("monster_ai/death_lines");
    if (arrayp(death_lines) && sizeof(death_lines) > 0)
    {
        ai_say(death_lines[random(sizeof(death_lines))]);
    }
}

// ===== AI 配置辅助 =====

// 为 NPC 配置标准的怪物 AI
// 用法：在 NPC 的 create() 中调用 setup_monster_ai(AI_LEVEL_BASIC)
void setup_monster_ai(int level)
{
    set_ai_level(level);

    // 根据等级自动配置基础属性
    switch (level)
    {
    case AI_LEVEL_BASIC:
        // 最低级怪物：没有特殊技能
        set("monster_ai/skills", ({}));
        break;

    case AI_LEVEL_TACTICAL:
        // 中级怪物：有 1-2 个技能
        set("monster_ai/skills", ({
            ([ "id": "skill_1", "priority": 6, "usage": "offensive" ]),
            ([ "id": "skill_2", "priority": 3, "usage": "heal" ]),
        }));
        break;

    case AI_LEVEL_ADVANCED:
        // 高级怪物：多个技能
        set("monster_ai/skills", ({
            ([ "id": "skill_1", "priority": 7, "usage": "offensive" ]),
            ([ "id": "skill_2", "priority": 5, "usage": "aoe" ]),
            ([ "id": "skill_3", "priority": 4, "usage": "heal" ]),
            ([ "id": "skill_4", "priority": 3, "usage": "finisher" ]),
        }));
        break;

    case AI_LEVEL_BOSS:
        // BOSS：全技能 + 阶段切换
        set("monster_ai/skills", ({
            ([ "id": "skill_1", "priority": 8, "usage": "offensive" ]),
            ([ "id": "skill_2", "priority": 7, "usage": "aoe" ]),
            ([ "id": "skill_3", "priority": 6, "usage": "heal" ]),
            ([ "id": "skill_4", "priority": 5, "usage": "finisher" ]),
        }));
        set("monster_ai/phases", ({
            ([
                "phase": 2,
                "name": "第二阶段·狂暴",
                "hp_threshold": 60,
                "berserk": 30,
                "message": HIR "怪物怒吼一声，进入了狂暴状态！" NOR,
            ]),
            ([
                "phase": 3,
                "name": "第三阶段·濒死",
                "hp_threshold": 25,
                "shield": 15,
                "berserk": 20,
                "message": HIM "怪物发出最后的嘶吼，不顾一切地发动攻击！" NOR,
            ]),
        }));
        break;
    }
}
