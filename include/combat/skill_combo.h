// skill_combo.h
// 技能组合/连招系统 + 冷却管理
// 用于 COMBAT_D 战斗流程中的组合技判定与冷却处理
// Created for #29 子任务：技能组合与阵法系统

#ifndef __SKILL_COMBO_H__
#define __SKILL_COMBO_H__

#include <ansi.h>

// 五行属性常量（与设计文档一致）
#define ELEMENT_GOLD    1
#define ELEMENT_WOOD    2
#define ELEMENT_WATER   3
#define ELEMENT_FIRE    4
#define ELEMENT_EARTH   5
#define ELEMENT_THUNDER 6  // 变异雷
#define ELEMENT_ICE     7  // 变异冰
#define ELEMENT_WIND    8  // 变异风

// 冷却类型
#define COOLDOWN_PUBLIC    1  // 公共冷却
#define COOLDOWN_INDIE     2  // 独立冷却
#define COOLDOWN_COMBO     3  // 连招冷却（受连招减免）

// 冷却时间常量（单位：心跳 tick）
#define CD_PUBLIC_DEFAULT      2   // 公共冷却，默认 2 tick
#define CD_BASIC_SPELL         0   // 基础法术，无冷却
#define CD_MID_SPELL           3   // 中级法术，3 tick
#define CD_HIGH_SPELL          6   // 高级法术，6 tick
#define CD_ULTIMATE_SPELL      12  // 绝招/剑阵，12 tick
#define CD_FORMATION_ONCE      0   // 阵法布设，每场战斗仅一次（特殊标记）
#define CD_ESCAPE_SKILL        30  // 保命技，30 tick

// 连招组合定义
// 每个组合：{ 前置技能ID, 后置技能ID, 额外效果倍率, 冷却减免( tick ), 境界要求, 描述 }
// 境界要求: 1=炼气前期, 2=炼气中期, 3=炼气后期, 4=筑基, 5=结丹...
nosave mapping combo_table = ([
    "冰封→金刃" : ([
        "pre_skill"    : "freezing",
        "post_skill"   : "goldblade",
        "damage_bonus" : 1.5,       // +150% 破冰伤害
        "armor_ignore" : 30,        // 无视 30% 防御
        "cd_reduce"    : 1,         // 冷却减免 1 tick
        "realm_req"    : 2,         // 炼气中期
        "desc"         : "冰冻术命中后接金刃术，破冰伤害+150%，无视30%防御",
    ]),
    "缠绕→火弹" : ([
        "pre_skill"    : "entangle",
        "post_skill"   : "fireball",
        "damage_bonus" : 1.0,       // 基础加成
        "burn_dot"     : 3,         // 灼烧持续 3 回合
        "cd_reduce"    : 1,
        "realm_req"    : 3,         // 炼气后期
        "desc"         : "缠绕术命中后接火弹术，灼烧额外持续3回合",
    ]),
    "御风→剑芒" : ([
        "pre_skill"    : "windwalk",
        "post_skill"   : "swordflash",
        "extra_attack" : 1,         // 攻击次数 +1
        "cd_reduce"    : 1,
        "realm_req"    : 5,         // 筑基中期
        "desc"         : "御风诀状态下施展剑芒，攻击次数+1",
    ]),
    "五行轮转" : ([
        "pre_skill"    : "__cycle__", // 特殊标记：连续使用金木水火土
        "elements"     : ({ ELEMENT_GOLD, ELEMENT_WOOD, ELEMENT_WATER, ELEMENT_FIRE, ELEMENT_EARTH }),
        "all_resist"   : -20,        // 全属性抗性 -20%
        "cd_reduce"    : 3,
        "realm_req"    : 6,         // 结丹期
        "desc"         : "连续使用金→木→水→火→土五行法术，最后一击附带五行崩坏",
    ]),
]);

// 技能冷却记录结构（存储在玩家临时数据中）
// query_temp("skill_cd/[skill_id]") = 剩余 tick 数
// query_temp("public_cd") = 公共冷却剩余 tick 数
// query_temp("combo_history") = ({ "skill1", "skill2", ... }) 最近技能历史
// query_temp("last_combo") = "组合名" 最近触发的组合技名称

// 检查一个技能是否在冷却中
// 返回 0 = 可用，>0 = 剩余冷却 tick 数
varargs int query_skill_cooldown(object me, string skill_id)
{
    int indie_cd, public_cd;

    // 检查独立冷却
    indie_cd = me->query_temp("skill_cd/" + skill_id);
    if ( indie_cd > 0 )
        return indie_cd;

    // 检查公共冷却（基础法术除外）
    if ( skill_id != "basic_attack" )
    {
        public_cd = me->query_temp("public_cd");
        if ( public_cd > 0 )
            return public_cd;
    }

    return 0;
}

// 设置技能冷却
// cooldown_type: COOLDOWN_PUBLIC / COOLDOWN_INDIE / COOLDOWN_COMBO
void set_skill_cooldown(object me, string skill_id, int ticks, int cooldown_type)
{
    int final_ticks = ticks;

    // 连招冷却减免：如果刚触发了连招，减免一部分冷却
    if ( cooldown_type == COOLDOWN_COMBO )
    {
        string last_combo = me->query_temp("last_combo");
        if ( stringp(last_combo) && !undefinedp(combo_table[last_combo]) )
        {
            int reduce = combo_table[last_combo]["cd_reduce"];
            final_ticks -= reduce;
            if ( final_ticks < 1 )
                final_ticks = 1;
        }
    }

    // 设置公共冷却
    if ( cooldown_type == COOLDOWN_PUBLIC || cooldown_type == COOLDOWN_COMBO )
    {
        // 公共冷却固定较短
        me->set_temp("public_cd", CD_PUBLIC_DEFAULT);
    }

    // 设置技能独立冷却
    me->set_temp("skill_cd/" + skill_id, final_ticks);
}

// 心跳冷却递减（由 COMBAT_D 每 tick 调用）
void tick_cooldowns(object me)
{
    mapping cds;
    string *skills;
    int public_cd;

    // 递减公共冷却
    public_cd = me->query_temp("public_cd");
    if ( public_cd > 0 )
    {
        me->set_temp("public_cd", public_cd - 1);
        if ( me->query_temp("public_cd") < 0 )
            me->delete_temp("public_cd");
    }

    // 递减所有技能独立冷却
    cds = me->query_temp("skill_cd");
    if ( mapp(cds) )
    {
        skills = keys(cds);
        foreach ( string sk in skills )
        {
            int cd = cds[sk];
            if ( cd > 0 )
            {
                cd--;
                if ( cd <= 0 )
                    me->delete_temp("skill_cd/" + sk);
                else
                    me->set_temp("skill_cd/" + sk, cd);
            }
        }
    }
}

// 检查技能组合/连招
// me: 施法者, target: 目标, skill_id: 当前释放的技能 ID
// 返回值: 命中组合的描述 mapping（若无组合返回 0）
mixed check_combo(object me, object target, string skill_id)
{
    string *history;
    string last_skill;
    string combo_name;

    // 获取技能历史
    history = me->query_temp("combo_history");
    if ( !arrayp(history) )
        history = ({});

    // 获取上一个技能
    last_skill = sizeof(history) > 0 ? history[sizeof(history) - 1] : 0;

    // 当前技能加入历史（最多保留最近 5 个）
    history += ({ skill_id });
    if ( sizeof(history) > 5 )
        history = history[sizeof(history) - 5..];
    me->set_temp("combo_history", history);

    // 遍历组合表，检查是否匹配
    foreach ( combo_name, mapping combo in combo_table )
    {
        // 五行轮转特殊判定
        if ( combo["pre_skill"] == "__cycle__" )
        {
            if ( sizeof(history) >= 5 )
            {
                string *last5 = history[sizeof(history)-5..];
                // 检查是否连续使用了金木水火土（检查顺序中是否包含全部五行属性技能）
                // 简化实现：若最近 5 个技能是不同属性，视为五行轮转
                // 实际游戏应检查技能对应的五行属性
                if ( sizeof(last5) == 5 )
                {
                    int *elem_hit = ({0,0,0,0,0});
                    for ( int i = 0; i < 5; i++ )
                    {
                        int elem = me->query_temp("skill_element/" + last5[i]);
                        if ( elem >= ELEMENT_GOLD && elem <= ELEMENT_EARTH )
                            elem_hit[elem - 1] = 1;
                    }
                    // 五种属性全部命中
                    if ( elem_hit[0] && elem_hit[1] && elem_hit[2] && elem_hit[3] && elem_hit[4] )
                    {
                        me->set_temp("last_combo", combo_name);
                        me->set_temp("combo_active", combo_name);
                        return combo;
                    }
                }
            }
            continue;
        }

        // 常规连招：前置技能匹配，当前技能后置
        if ( stringp(last_skill) && last_skill == combo["pre_skill"] && skill_id == combo["post_skill"] )
        {
            // 检查境界要求
            if ( me->query("realm_level") < combo["realm_req"] )
                continue;

            me->set_temp("last_combo", combo_name);
            me->set_temp("combo_active", combo_name);
            return combo;
        }
    }

    return 0;
}

// 应用连招额外伤害/效果
// 在 do_attack 伤害计算后调用，传入基础伤害，返回增强后的伤害
varargs int apply_combo_bonus(object me, object target, int base_damage, mapping combo, string skill_id)
{
    if ( !mapp(combo) )
        return base_damage;

    int final_damage = base_damage;
    float bonus = 1.0;

    // 伤害加成
    if ( combo["damage_bonus"] )
        bonus = to_float(combo["damage_bonus"]);

    // 破甲效果
    if ( combo["armor_ignore"] )
    {
        // 降低目标临时防御
        int armor_ignore = combo["armor_ignore"];
        int def_reduce = target->query_temp("apply/defense") * armor_ignore / 100;
        // 在本次攻击中忽略部分防御（通过临时修改战斗参数）
        me->set_temp("combo/armor_ignore", def_reduce);
    }

    // 灼烧 DOT
    if ( combo["burn_dot"] )
    {
        int burn_ticks = combo["burn_dot"];
        target->set_temp("status/dot_burn", burn_ticks);
        target->set_temp("status/dot_burn_damage", base_damage * 30 / 100); // 灼烧每回合 30% 初始伤害
    }

    // 额外攻击次数
    if ( combo["extra_attack"] )
    {
        me->set_temp("combo/extra_attack", combo["extra_attack"]);
    }

    // 五行轮转 - 全属性抗性降低
    if ( combo["all_resist"] )
    {
        int resist_penalty = combo["all_resist"];
        // 对目标施加 debuff
        target->add_temp("apply/all_resist", resist_penalty);
        // 持续一定 tick 后用 call_out 恢复
    }

    if ( bonus > 1.0 )
        final_damage = to_int(final_damage * bonus);

    return final_damage;
}

// 清除连招状态（每次攻击后调用）
void clear_combo_temp(object me)
{
    me->delete_temp("combo/armor_ignore");
    me->delete_temp("combo/extra_attack");
    me->delete_temp("combo_active");
}

#endif
