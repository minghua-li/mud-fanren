// adm/daemons/spell_d.c
// 五行法术守护进程（SPELL_D）—— 法术施展链路
// 设计文档: .knowledge/combat/1F-法术剑诀阵法战斗.md §1.1（五行基础法术分级/各属性详情）、
//          §2.2（技能等级系数）、02-扩充内容/02-战斗机制与平衡.md §2.1-2.2（五行/境界/灵根系数）
// 依赖:
//   SECT_D->query_cultivation_tier   —— 修习/施展境界门槛（#61）
//   realm_power.h                    —— 境界压制 × 灵根加成组合系数（#78，与普攻同一套原语）
//   element.h                        —— 五行相克系数与元素名/颜色（#28）
// 数据单真源：每个法术 skill 的 query_spell_info() 返回威力/法力/冷却/效果，
//   本 daemon 只维护「别名索引 + 施法流程」，不复制数值。
// Created for ticket #76

#include <ansi.h>
#include <globals.h>
#include <spell.h>
#include <combat/realm_power.h>

inherit F_DBASE;

// -------- 法术注册索引 --------
// 技能 id 列表（与 kungfu/skill/<id>.c 一一对应）
nosave string *spell_ids = ({
    "huodan-shu", "huohua-shu", "chanrao-shu", "bingjian-shu",
    "liusha-shu", "dici-shu", "tufang-shu", "jinren-shu",
});

// 别名索引：中文名/简称/拼音/id → 技能 id
nosave mapping spell_alias = ([
    "火弹术": "huodan-shu", "火弹": "huodan-shu", "huodan": "huodan-shu",
    "火花术": "huohua-shu", "火花": "huohua-shu", "huohua": "huohua-shu",
    "缠绕术": "chanrao-shu", "缠绕": "chanrao-shu", "chanrao": "chanrao-shu",
    "冰箭术": "bingjian-shu", "冰箭": "bingjian-shu", "bingjian": "bingjian-shu",
    "流沙术": "liusha-shu", "流沙": "liusha-shu", "liusha": "liusha-shu",
    "地刺术": "dici-shu", "地刺": "dici-shu", "dici": "dici-shu",
    "土墙术": "tufang-shu", "土墙": "tufang-shu", "tufang": "tufang-shu",
    "金刃术": "jinren-shu", "金刃": "jinren-shu", "jinren": "jinren-shu",
]);

// -------- 查询接口 --------

string *query_spell_ids() { return spell_ids; }

// 别名 / 中文名 / id → 技能 id；未命中返回 0
string resolve_spell_id(string arg)
{
    int i;
    mapping info;

    if (!stringp(arg) || arg == "") return 0;
    if (member_array(arg, spell_ids) != -1) return arg;
    // 注意：mapping 取不到的键返回 0，undefinedp(0) 为假——必须用 stringp 判断
    if (stringp(spell_alias[arg])) return spell_alias[arg];

    // 中文名兜底（skill 文件的 query_spell_info()["name"]）
    for (i = 0; i < sizeof(spell_ids); i++)
    {
        info = query_spell_info(spell_ids[i]);
        if (mapp(info) && info["name"] == arg) return spell_ids[i];
    }
    return 0;
}

// 单个法术元数据（读 skill 文件的 query_spell_info）
mapping query_spell_info(string id)
{
    mapping info;

    if (!stringp(id) || id == "") return 0;
    if (member_array(id, spell_ids) == -1) return 0;

    info = SKILL_D(id)->query_spell_info();
    if (!mapp(info)) return 0;
    return info;
}

// 玩家已学的法术列表（含等级）
mapping *query_player_spells(object me)
{
    mapping *list = ({});
    mapping info;
    int i, lv;

    if (!objectp(me)) return list;
    for (i = 0; i < sizeof(spell_ids); i++)
    {
        lv = me->query_skill(spell_ids[i], 1);
        if (lv <= 0) continue;
        info = query_spell_info(spell_ids[i]);
        if (!mapp(info)) continue;
        info["skill_level"] = lv;
        list += ({ info });
    }
    return list;
}

// 法术当前冷却剩余秒数（0 = 可施展）
int query_cooldown(object me, string id)
{
    int until;

    if (!objectp(me) || !stringp(id)) return 0;
    until = me->query_temp("spell/cooldown/" + id);
    if (!intp(until)) return 0;
    if (until <= time()) return 0;
    return until - time();
}

// 法术所需法力（境界越高法力池越大，1F §2.3 境界消耗系数的简化实现：
// 消耗按 tier 递减，筑基/结丹递减比例与 PILL/forge 体系保持整数口径）
int query_spell_cost(object me, mapping info)
{
    int cost, tier;

    if (!mapp(info)) return 0;
    cost = info["cost"];
    if (!intp(cost) || cost < 1) cost = 1;
    if (!objectp(me)) return cost;

    tier = SECT_D->query_cultivation_tier(me);
    if (tier >= 6)       cost = cost * 70 / 100;    // 结丹期
    else if (tier >= 3)  cost = cost * 85 / 100;    // 筑基期
    if (cost < 1) cost = 1;
    return cost;
}

// 法术伤害（千分比口径全部取整，避免浮点漂移）
//   功率 × 技能等级系数（SPELL_COEF_BASE + 等级×SPELL_COEF_PER_LEVEL）→
//   境界压制 × 灵根加成（#78 combat_power_permille）→ 五行相克（#28）→ 破甲
int query_spell_damage(object me, object victim, mapping info, int skill_level)
{
    int dmg, coef, pm, elem, elem_pm;
    float elem_mod;

    if (!objectp(me) || !objectp(victim) || !mapp(info)) return 0;

    dmg = info["power"];
    if (!intp(dmg) || dmg <= 0) return 0;

    coef = SPELL_COEF_BASE + skill_level * SPELL_COEF_PER_LEVEL;
    if (coef > SPELL_COEF_MAX) coef = SPELL_COEF_MAX;
    if (coef < 1) coef = 1;

    elem = spell_element_id(info["element"]);
    pm = combat_power_permille(me, victim, 1, elem);

    elem_mod = calc_element_modifier(elem, query_character_element(victim));
    elem_pm = to_int(elem_mod * 1000);

    dmg = dmg * coef / 1000;
    dmg = dmg * pm / 1000;
    dmg = dmg * elem_pm / 1000;

    if (info["fx"] == SPELL_FX_PIERCE)
        dmg = dmg * (1000 + SPELL_PIERCE_RATIO) / 1000;

    if (dmg < 1) dmg = 1;
    return dmg;
}

// -------- 施法主流程 --------

// 返回 1 成功施展，0 失败（失败时已 tell_object 说明原因）
int cast_spell(object me, string id, object target)
{
    mapping info;
    string name, elem_name;
    int lv, tier, cost, elem, dmg, cd, fx;

    if (!objectp(me)) return 0;

    info = query_spell_info(id);
    if (!mapp(info))
    {
        tell_object(me, "没有这种法术。输入 shifa 查看已学法术。\n");
        return 0;
    }
    name = info["name"];

    lv = me->query_skill(id, 1);
    if (lv <= 0)
    {
        tell_object(me, "你尚未修习「" + name + "」。\n");
        return 0;
    }

    tier = SECT_D->query_cultivation_tier(me);
    if (tier < spell_tier_need(info["level"]))
    {
        tell_object(me, "你修为不足，尚不能施展「" + name + "」（需" +
                    (info["level"] == SPELL_HIGH ? "炼气后期" :
                     info["level"] == SPELL_MID ? "炼气中期" : "炼气初期") + "以上）。\n");
        return 0;
    }

    if (me->is_busy())
    {
        tell_object(me, "你正手忙脚乱，无暇掐诀施法。\n");
        return 0;
    }

    cd = query_cooldown(me, id);
    if (cd > 0)
    {
        tell_object(me, "「" + name + "」尚需 " + cd + " 秒方能再次施展。\n");
        return 0;
    }

    cost = query_spell_cost(me, info);
    if (me->query("neili") < cost)
    {
        tell_object(me, "你法力不足，施展「" + name + "」至少需 " + cost + " 点法力。\n");
        return 0;
    }

    // 防御/辅助法术只能对自己
    if (info["type"] == SPELL_TYPE_DEFEND) target = me;
    if (!objectp(target))
    {
        tell_object(me, "这里没有你要攻击的对象。\n");
        return 0;
    }
    if (target != me && environment(target) != environment(me))
    {
        tell_object(me, "对方已不在你眼前。\n");
        return 0;
    }

    // 扣法力、进冷却、施法硬直
    me->add("neili", -cost);
    me->set_temp("spell/cooldown/" + id, time() + info["cooldown"]);
    me->start_busy(SPELL_CAST_BUSY);

    elem = spell_element_id(info["element"]);
    elem_name = info["element"];
    fx = info["fx"];

    if (target == me)
    {
        // ---- 自身增益（土墙术等）----
        message_vision(HIY "\n$N口中低诵法诀，周身" + query_element_color(elem) +
                       elem_name + NOR + HIY "灵光一闪，凝成护体屏障。\n" NOR, me);
        if (fx == SPELL_FX_GUARD)
        {
            me->add_temp("apply/defense_attack", SPELL_GUARD_DEFENSE);
            call_out("remove_spell_debuff", SPELL_GUARD_DURATION, me,
                     "apply/defense_attack", SPELL_GUARD_DEFENSE);
            tell_object(me, HIC "【土墙护体】厚土之力附于周身，可挡外伤 " +
                        SPELL_GUARD_DEFENSE + " 点，持续 " + SPELL_GUARD_DURATION + " 秒。\n" NOR);
        }
        me->improve_skill(id, 1);
        return 1;
    }

    // ---- 攻击法术 ----
    dmg = query_spell_damage(me, target, info, lv);

    message_vision(HIY "\n$N掐诀引动" + query_element_color(elem) + elem_name + NOR +
                   HIY "灵之气，" + name + "呼啸而出！\n" NOR, me);

    if (dmg > 0)
    {
        int real;

        real = target->receive_damage("qi", dmg, me);
        if (real > 0 && random(100) < 30)
            target->receive_wound("qi", real / 2, me);

        message_vision(HIR "「" + name + "」正中$n！\n" NOR, me, target);
        COMBAT_D->report_status(target);

        // 五行克制提示（与普攻同一套系数，便于玩家感知）
        if (calc_element_modifier(elem, query_character_element(target)) >= 1.4)
            tell_object(me, HIG "【五行克制】" + elem_name + "法正克对方属性，威力大增。\n" NOR);
        else if (calc_element_modifier(elem, query_character_element(target)) <= 0.75)
            tell_object(me, CYN "【五行被克】" + elem_name + "法被对方属性所制，威力打了折扣。\n" NOR);

        me->improve_skill(id, 1 + dmg * SPELL_IMPROVE_RATIO / 1000);
    }

    // ---- 附加效果 ----
    switch (fx)
    {
    case SPELL_FX_BURN:
        call_out("burn_tick", SPELL_BURN_INTERVAL, target, me,
                 dmg * SPELL_BURN_RATIO / 1000, SPELL_BURN_TICKS);
        tell_object(me, HIY "火气已透入" + target->name() + "体内，仍在灼烧。\n" NOR);
        break;
    case SPELL_FX_SLOW:
        target->add_temp("apply/dodge", -SPELL_SLOW_DODGE);
        call_out("remove_spell_debuff", SPELL_SLOW_DURATION, target,
                 "apply/dodge", -SPELL_SLOW_DODGE);
        tell_object(me, HIB "寒气/流沙缠住" + target->name() + "脚步，身法大减。\n" NOR);
        break;
    case SPELL_FX_BIND:
        target->add_temp("apply/dodge", -SPELL_BIND_DODGE);
        target->add_temp("apply/parry", -SPELL_BIND_PARRY);
        target->start_busy(1);
        call_out("remove_spell_debuff", SPELL_BIND_DURATION, target,
                 "apply/dodge", -SPELL_BIND_DODGE);
        call_out("remove_spell_debuff", SPELL_BIND_DURATION, target,
                 "apply/parry", -SPELL_BIND_PARRY);
        tell_object(me, HIG "藤蔓缠住" + target->name() + "四肢，它一时难以动弹。\n" NOR);
        break;
    case SPELL_FX_PIERCE:
        tell_object(me, HIW "金刃锋锐，直透护体真气。\n" NOR);
        break;
    }

    // 若尚未开战，法术命中即开战
    if (!me->is_fighting(target) && living(target))
    {
        me->fight_ob(target);
        if (!target->is_fighting(me) && living(target))
            target->fight_ob(me);
    }

    return 1;
}

// -------- 附加效果回调 --------

// 灼烧：每 SPELL_BURN_INTERVAL 秒造成一次伤害
void burn_tick(object target, object caster, int dmg, int ticks)
{
    int real;

    if (!objectp(target) || ticks <= 0) return;
    if (!living(target)) return;

    real = target->receive_damage("qi", dmg, caster);
    if (real > 0)
        tell_object(target, HIR "灼烧之气在体内翻涌，你痛得闷哼一声（-" +
                    real + "）。\n" NOR);

    if (ticks > 1)
        call_out("burn_tick", SPELL_BURN_INTERVAL, target, caster, dmg, ticks - 1);
}

// 撤销临时增益/减益（target 已销毁时静默返回）
void remove_spell_debuff(object target, string key, int amount)
{
    if (!objectp(target)) return;
    target->add_temp(key, -amount);
}
