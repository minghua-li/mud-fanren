// formation.c
// 阵法系统特征（mixin）
// 继承此特征的 NPC/玩家可参与阵法
// Created for #29 子任务：技能组合与阵法系统

#include <ansi.h>

// 五行属性常量（与 skill_combo.h 一致）
#define ELEMENT_GOLD    1
#define ELEMENT_WOOD    2
#define ELEMENT_WATER   3
#define ELEMENT_FIRE    4
#define ELEMENT_EARTH   5
#define ELEMENT_THUNDER 6
#define ELEMENT_ICE     7
#define ELEMENT_WIND    8

// 阵型类型常量
#define FORMATION_NONE           0
#define FORMATION_JULING         1   // 聚灵阵（辅助）
#define FORMATION_DIANDAO        2   // 颠倒五行阵（防御+幻境）
#define FORMATION_TIANGANG       3   // 天罡北斗阵（防御）
#define FORMATION_DAGENG         4   // 大庚剑阵（攻击）
#define FORMATION_SANCAI         5   // 三才阵（3人组队）
#define FORMATION_SIXIANG        6   // 四象阵（4人组队）
#define FORMATION_WUXING         7   // 五行阵（5人组队）

// 阵型数据表
nosave mapping formation_data = ([
    FORMATION_JULING : ([
        "name"         : "聚灵阵",
        "type"         : "辅助",
        "setup_time"   : 3,         // 布阵所需回合数
        "effect"       : "每回合恢复法力5%%",
        "mana_regen"   : 5,         // 每回合法力恢复百分比
        "cost_per_tick": 1,         // 每回合灵石消耗
        "desc"         : "聚灵阵汇聚天地灵气，每回合自动恢复法力",
    ]),
    FORMATION_DIANDAO : ([
        "name"         : "颠倒五行阵",
        "type"         : "防御+幻境",
        "setup_time"   : 5,
        "effect"       : "五行伤害减免30%%，敌方位移受限",
        "wuxing_resist": 30,        // 五行伤害减免百分比
        "move_limit"   : 1,         // 限制敌方移动
        "cost_per_tick": 3,
        "desc"         : "颠倒五行，逆转阴阳，大幅削弱五行伤害并限制敌人行动",
    ]),
    FORMATION_TIANGANG : ([
        "name"         : "天罡北斗阵",
        "type"         : "防御",
        "setup_time"   : 2,
        "effect"       : "全员防御+20%%，分摊伤害",
        "defense_bonus": 20,        // 防御加成百分比
        "damage_share" : 1,         // 分摊伤害
        "cost_per_tick": 2,
        "desc"         : "天罡北斗，众星拱月，提升全员防御并分摊所受伤害",
    ]),
    FORMATION_DAGENG : ([
        "name"         : "大庚剑阵",
        "type"         : "攻击",
        "setup_time"   : 4,
        "effect"       : "对范围内敌人持续剑气伤害",
        "sword_damage" : 40,        // 每回合剑气伤害基数
        "cost_per_tick": 0,         // 无灵石消耗（占用飞剑）
        "desc"         : "大庚剑阵，万剑齐发，对敌人造成持续剑气伤害",
    ]),
    FORMATION_SANCAI : ([
        "name"         : "三才阵",
        "type"         : "组队-3人",
        "setup_time"   : 1,
        "effect"       : "前排分摊70%%伤害，后排专注输出/治疗",
        "min_members"  : 3,
        "max_members"  : 3,
        "front_share"  : 70,        // 前排分摊伤害百分比
        "back_bonus"   : 15,        // 后排伤害/治疗加成百分比
        "cost_per_tick": 1,
        "desc"         : "天地人三才定位，前排抵御、后排输出",
    ]),
    FORMATION_SIXIANG : ([
        "name"         : "四象阵",
        "type"         : "组队-4人",
        "setup_time"   : 1,
        "effect"       : "四方获得对应属性加成",
        "min_members"  : 4,
        "max_members"  : 4,
        "element_bonus": 15,        // 属性加成百分比
        "cost_per_tick": 2,
        "desc"         : "四象各守一方，金攻木愈水控火盾",
    ]),
    FORMATION_WUXING : ([
        "name"         : "五行阵",
        "type"         : "组队-5人",
        "setup_time"   : 2,
        "effect"       : "五行相生循环，全员属性增益连锁",
        "min_members"  : 5,
        "max_members"  : 5,
        "cycle_bonus"  : 10,        // 相生循环每环加成
        "cost_per_tick": 3,
        "desc"         : "五行相生，生生不息，全员获得属性增益连锁",
    ]),
]);

// 阵法状态变量
nosave int    formation_type;       // 当前阵型类型
nosave string formation_eye_id;     // 阵眼角色 ID
nosave int    formation_active;     // 阵法是否已激活
nosave int    formation_setup_ticks; // 布阵进度（剩余 tick）
nosave int    formation_tick_count; // 阵法已维持 tick 数
nosave object *formation_members;   // 阵内成员列表

// ===== 查询函数 =====

// 查询当前阵型类型
int query_formation_type()
{
    return formation_type;
}

// 查询当前阵型是否已激活
int is_formation_active()
{
    return formation_active;
}

// 查询布阵剩余时间
int query_formation_setup_ticks()
{
    return formation_setup_ticks;
}

// 查询阵眼角色 ID
string query_formation_eye()
{
    return formation_eye_id;
}

// 查询阵法成员
object *query_formation_members()
{
    return formation_members;
}

// 查询当前阵型数据
mapping query_formation_data()
{
    if ( !formation_type || formation_type == FORMATION_NONE )
        return 0;

    return formation_data[formation_type];
}

// ===== 布阵/撤阵 =====

// 开始布阵
// 返回 1 = 成功, 0 = 失败
int start_formation(int type, object *members)
{
    if ( !arrayp(members) || sizeof(members) < 1 )
        return 0;

    // 检查成员数量是否满足阵型要求
    mapping data = formation_data[type];
    if ( !mapp(data) )
        return 0;

    // 检查是否为多人阵型
    if ( data["min_members"] && sizeof(members) < data["min_members"] )
        return 0;

    if ( data["max_members"] && sizeof(members) > data["max_members"] )
        return 0;

    // 设置阵型
    formation_type = type;
    formation_members = members;
    formation_setup_ticks = data["setup_time"];
    formation_active = 0;
    formation_tick_count = 0;

    // 选取阵眼（默认由队长或队伍 leader 担任，也可手动指定）
    formation_eye_id = this_object()->query("id");

    // 通知全体成员
    string formation_name = data["name"];
    foreach ( object ob in members )
    {
        if ( objectp(ob) && living(ob) )
        {
            tell_object(ob, HIY "开始布设「" + formation_name + "」，需要 " + data["setup_time"] + " 回合完成！\n" NOR);
        }
    }

    return 1;
}

// 布阵进度更新（每 tick 由 COMBAT_D 或心跳调用）
// 返回 1 = 布阵完成, 0 = 布阵中, -1 = 布阵失败
int progress_formation()
{
    if ( !formation_type || formation_type == FORMATION_NONE )
        return -1;

    if ( formation_active )
        return 1;

    if ( formation_setup_ticks > 0 )
    {
        formation_setup_ticks--;

        if ( formation_setup_ticks <= 0 )
        {
            // 阵法激活！
            formation_active = 1;
            formation_setup_ticks = 0;
            activate_formation_effects();
            return 1;
        }
    }

    return 0;
}

// 撤阵
void dismiss_formation()
{
    if ( formation_type == FORMATION_NONE && !formation_active )
        return;

    // 移除阵法效果
    if ( formation_active )
        deactivate_formation_effects();

    // 通知成员
    mapping data = formation_data[formation_type];
    string formation_name = mapp(data) ? data["name"] : "未知阵法";

    if ( arrayp(formation_members) )
    {
        foreach ( object ob in formation_members )
        {
            if ( objectp(ob) && living(ob) )
            {
                tell_object(ob, HIY "阵法已撤！「" + formation_name + "」消散。\n" NOR);
            }
        }
    }

    // 清除状态
    formation_type = FORMATION_NONE;
    formation_eye_id = 0;
    formation_active = 0;
    formation_setup_ticks = 0;
    formation_tick_count = 0;
    formation_members = 0;
    this_object()->delete_temp("formation");
}

// ===== 阵法效果 =====

// 激活阵法效果
void activate_formation_effects()
{
    if ( !formation_active || !formation_type )
        return;

    mapping data = formation_data[formation_type];
    if ( !mapp(data) )
        return;

    string formation_name = data["name"];

    // 队内广播
    if ( arrayp(formation_members) )
    {
        foreach ( object ob in formation_members )
        {
            if ( !objectp(ob) || !living(ob) )
                continue;

            tell_object(ob, HIC "\n☆★☆ 「" + formation_name + "」已激活！☆★☆\n" NOR);
            tell_object(ob, data["effect"] + "\n");

            // 应用阵法效果到每个成员
            apply_formation_to_member(ob, data);
        }
    }

    // 阵眼额外公告
    tell_object(this_object(), HIW "你作为阵眼，主持「" + formation_name + "」的运转。\n" NOR);
}

// 对单个成员应用阵法效果
void apply_formation_to_member(object member, mapping data)
{
    if ( !objectp(member) )
        return;

    int type = formation_type;

    switch ( type )
    {
    case FORMATION_JULING:
        // 聚灵阵：法力恢复通过每 tick 处理，不设临时加成
        member->set_temp("formation/type", "juling");
        member->set_temp("formation/mana_regen", data["mana_regen"]);
        break;

    case FORMATION_DIANDAO:
        // 颠倒五行阵：五行抗性提升
        member->set_temp("formation/type", "diandao");
        member->set_temp("formation/wuxing_resist", data["wuxing_resist"]);
        break;

    case FORMATION_TIANGANG:
        // 天罡北斗阵：防御加成，伤害分摊标记
        member->set_temp("formation/type", "tiangang");
        member->set_temp("formation/defense_bonus", data["defense_bonus"]);
        member->set_temp("formation/damage_share", 1);
        // 实际防御加成通过 query_temp("apply/defense") 在 damage.h 中计算
        member->add_temp("apply/defense", member->query("defense") * data["defense_bonus"] / 100);
        break;

    case FORMATION_DAGENG:
        // 大庚剑阵：剑气伤害，找最近的敌人攻击
        member->set_temp("formation/type", "dageng");
        member->set_temp("formation/sword_damage", data["sword_damage"]);
        break;

    case FORMATION_SANCAI:
        // 三才阵：根据位置（前端/后排）给予不同加成
        member->set_temp("formation/type", "sancai");
        if ( member == this_object() || member == formation_members[0] || member == formation_members[1] )
        {
            // 前排
            member->set_temp("formation/position", "front");
            member->set_temp("formation/front_share", data["front_share"]);
        }
        else
        {
            // 后排
            member->set_temp("formation/position", "back");
            member->set_temp("apply/damage", member->query_temp("apply/damage") * data["back_bonus"] / 100);
        }
        break;

    case FORMATION_SIXIANG:
        // 四象阵：四方对应属性（金木水火），按成员顺序分配
        member->set_temp("formation/type", "sixiang");
        {
            int idx = member_array(member, formation_members);
            string *elements = ({ "gold", "wood", "water", "fire" });
            string elem = elements[idx % 4];
            member->set_temp("formation/element", elem);
            // 对应属性伤害加成
            member->set_temp("formation/element_bonus", data["element_bonus"]);
        }
        break;

    case FORMATION_WUXING:
        // 五行阵：五行相生连锁
        member->set_temp("formation/type", "wuxing");
        {
            int idx = member_array(member, formation_members);
            // 相生关系: 金生水, 水生木, 木生火, 火生土, 土生金
            string *cycle = ({ "gold_benefit", "water_benefit", "wood_benefit", "fire_benefit", "earth_benefit" });
            string benefit = cycle[idx % 5];
            member->set_temp("formation/cycle_benefit", data["cycle_bonus"]);
            member->set_temp("formation/cycle_type", benefit);
        }
        break;
    }
}

// 取消阵法效果
void deactivate_formation_effects()
{
    if ( !arrayp(formation_members) )
        return;

    foreach ( object ob in formation_members )
    {
        if ( !objectp(ob) )
            continue;

        // 恢复天罡北斗阵的防御加成
        int def_bonus = ob->query_temp("formation/defense_bonus");
        if ( def_bonus )
        {
            ob->add_temp("apply/defense", -(ob->query("defense") * def_bonus / 100));
        }

        // 恢复三才阵后排伤害加成
        if ( ob->query_temp("formation/position") == "back" )
        {
            int back_bonus = formation_data[FORMATION_SANCAI]["back_bonus"];
            ob->add_temp("apply/damage", -(ob->query_temp("apply/damage") * back_bonus / 100));
        }

        // 清除所有阵法临时数据
        ob->delete_temp("formation");
    }
}

// ===== 阵法心跳处理 =====

// 每 tick 阵法效果处理（由 COMBAT_D 战斗心跳调用）
void formation_tick(object me)
{
    if ( !formation_active || !formation_type || formation_type == FORMATION_NONE )
        return;

    mapping data = formation_data[formation_type];
    if ( !mapp(data) )
        return;

    formation_tick_count++;

    // 消耗灵石（如果阵型有消耗）
    int cost = data["cost_per_tick"];
    if ( cost > 0 )
    {
        // 阵眼消耗灵石
        if ( !this_object()->query("balance") || this_object()->query("balance") < cost )
        {
            tell_object(this_object(), HIR "灵石不足！阵法因缺乏灵力维持而消散！\n" NOR);
            dismiss_formation();
            return;
        }
        this_object()->add("balance", -cost);
    }

    // 各类阵法的 tick 效果
    switch ( formation_type )
    {
    case FORMATION_JULING:
        // 聚灵阵：恢复法力
        if ( arrayp(formation_members) )
        {
            foreach ( object ob in formation_members )
            {
                if ( !objectp(ob) || !living(ob) )
                    continue;
                int max_mana = ob->query("max_neili");
                int regen = max_mana * data["mana_regen"] / 100;
                ob->add("neili", regen);
                if ( ob->query("neili") > max_mana )
                    ob->set("neili", max_mana);
            }
        }
        break;

    case FORMATION_DAGENG:
        // 大庚剑阵：对敌人造成剑气伤害
        {
            object *enemies = me->query_enemy();
            if ( arrayp(enemies) && sizeof(enemies) > 0 )
            {
                int sword_dmg = data["sword_damage"];
                foreach ( object enemy in enemies )
                {
                    if ( !objectp(enemy) || !living(enemy) )
                        continue;
                    // 剑气伤害，无视部分防御
                    int dmg = sword_dmg + random(sword_dmg / 2);
                    enemy->receive_damage("qi", dmg, me);
                    message_vision(
                        HIC "「大庚剑阵」一道剑气划破长空，击中$n！\n" NOR,
                        me, enemy
                    );
                }
            }
        }
        break;

    case FORMATION_TIANGANG:
        // 天罡北斗阵：无需额外 tick 效果（防御加成已在激活时设置）
        break;

    case FORMATION_DIANDAO:
        // 颠倒五行阵：无需额外 tick 效果（抗性已设置）
        break;

    case FORMATION_SANCAI:
    case FORMATION_SIXIANG:
    case FORMATION_WUXING:
        // 组队阵型：各有特性在战斗结算时体现
        break;
    }
}

// ===== 阵眼判定 =====

// 设定阵眼
int set_formation_eye(string id)
{
    if ( !stringp(id) )
        return 0;

    // 检查该成员是否在阵内
    if ( arrayp(formation_members) )
    {
        int found = 0;
        foreach ( object ob in formation_members )
        {
            if ( objectp(ob) && ob->query("id") == id )
            {
                found = 1;
                break;
            }
        }
        if ( !found )
            return 0;
    }

    formation_eye_id = id;
    return 1;
}

// 判断某人是否为阵眼
int is_formation_eye(string id)
{
    return stringp(formation_eye_id) && formation_eye_id == id;
}

// 阵眼死亡/离线时的处理
void formation_eye_fallen()
{
    if ( !formation_active )
        return;

    tell_object(this_object(), HIR "阵眼失位！「" + formation_data[formation_type]["name"] + "」紊乱！\n" NOR);

    // 可选：尝试转移阵眼
    if ( arrayp(formation_members) )
    {
        foreach ( object ob in formation_members )
        {
            if ( objectp(ob) && ob != this_object() && living(ob) )
            {
                set_formation_eye(ob->query("id"));
                tell_object(this_object(), HIY "阵眼已转移至 " + ob->name() + "！\n" NOR);
                return;
            }
        }
    }

    // 无人可接替，撤阵
    dismiss_formation();
}

// ===== 伤害/效果计算辅助 =====

// 计算阵法对伤害的加成/减免
// 由 COMBAT_D 的 damage.h 等调用
int formation_damage_modify(object victim, int damage, int element_type)
{
    if ( !formation_active || !formation_type )
        return damage;

    int final_damage = damage;

    // 颠倒五行阵：五行伤害减免
    if ( formation_type == FORMATION_DIANDAO )
    {
        if ( element_type >= ELEMENT_GOLD && element_type <= ELEMENT_EARTH )
        {
            int resist = formation_data[FORMATION_DIANDAO]["wuxing_resist"];
            final_damage = final_damage * (100 - resist) / 100;
        }
    }

    // 天罡北斗阵：伤害分摊（由阵眼接收部分伤害）
    if ( formation_type == FORMATION_TIANGANG )
    {
        if ( victim != this_object() )
        {
            // 阵眼分担伤害
            int shared = final_damage * 30 / 100; // 30% 由阵眼分担
            final_damage -= shared;
            if ( objectp(this_object()) && living(this_object()) )
            {
                this_object()->receive_damage("qi", shared, victim);
                tell_object(this_object(), HIW "你作为阵眼，分担了 " + shared + " 点伤害。\n" NOR);
            }
        }
    }

    // 三才阵：前排分摊
    if ( formation_type == FORMATION_SANCAI )
    {
        string pos = victim->query_temp("formation/position");
        if ( pos == "front" )
        {
            // 前排已有减伤标记，不重复处理
        }
        else if ( pos == "back" )
        {
            // 后排额外保护：前排成员为其分担部分伤害
            if ( arrayp(formation_members) )
            {
                foreach ( object member in formation_members )
                {
                    if ( !objectp(member) || member == victim )
                        continue;
                    if ( member->query_temp("formation/position") == "front" )
                    {
                        int shared = final_damage * 30 / 100;
                        final_damage -= shared;
                        member->receive_damage("qi", shared, victim);
                        break;
                    }
                }
            }
        }
    }

    return final_damage;
}

// 获取阵法提供的属性加成（用于战斗计算）
mapping query_formation_bonus(object member)
{
    mapping bonus = ([]);

    if ( !formation_active || !formation_type )
        return bonus;

    switch ( formation_type )
    {
    case FORMATION_TIANGANG:
        bonus["defense"] = formation_data[FORMATION_TIANGANG]["defense_bonus"];
        break;

    case FORMATION_SANCAI:
        if ( member->query_temp("formation/position") == "back" )
            bonus["damage"] = formation_data[FORMATION_SANCAI]["back_bonus"];
        break;

    case FORMATION_SIXIANG:
        bonus["element"] = formation_data[FORMATION_SIXIANG]["element_bonus"];
        break;

    case FORMATION_WUXING:
        bonus["cycle"] = formation_data[FORMATION_WUXING]["cycle_bonus"];
        break;
    }

    return bonus;
}
