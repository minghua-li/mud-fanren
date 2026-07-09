mixed calc_damage(object me,object victim,object weapon,mapping my,mapping your,mapping action)
{
    int damage_bonus,defense_factor,damage;
    int deepj, weekj, defatk, absweekj;
    int att_elem, def_elem;
    float elem_mod;
    string result = "";
    string force_skill,martial_skill,attack_skill;
    mixed foo;

    attack_skill = choose_attack_skill(me, weapon);
    
    if ( me->query_temp("attack_val/skill") )
    {
        if ( objectp(weapon) )
        {
            martial_skill = me->query_temp("attack_val/skill/weapon");
        }
        else
        {
            martial_skill = me->query_temp("attack_val/skill/no_weapon");
        }
    }

    if ( !martial_skill || file_size(SKILL_D(martial_skill)+".c") < 0 )
        martial_skill = me->query_skill_mapped(attack_skill);

    //命中！计算伤害
    //增加单系伤害的支持，dmg_map下空手功夫统一置unarmed，其他为各兵器伤害
    if ( objectp(weapon) )
      damage = me->query_temp("apply/damage")+me->query_temp("apply/dmg_map/"+attack_skill);
    else
      damage = me->query_temp("apply/damage")+me->query_temp("apply/dmg_map/unarmed");
    
//防止外部加载的伤害为负数导致后续计算出现问题，统一将扣为负数的伤害认为没有damage。by seagate@pkuxkx
    if ( damage<0 )
    	damage=0;
    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 1: apply/damage is %d\n", damage));
    damage = (damage + random(damage)) / 2;      
    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 2: damage is %d\n", damage));
    if( action["damage"] )
        damage += action["damage"] * damage / 100;
    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 3: damage is %d\n", damage));      
    damage_bonus = me->query_str();      
    if( action["force"] )
        damage_bonus += action["force"] *damage_bonus / 100;
    
    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 4: damage_bonus is %d, action/force is %d\n", damage_bonus,action["force"]));
    
    // Let force skill take effect.
    if( my["jiali"] && (my["neili"] > my["jiali"]) ) {
        if( force_skill = me->query_skill_mapped("force") ) {
            foo = SKILL_D(force_skill)->hit_ob(me, victim, damage_bonus, my["jiali"]);          
            if ((me->query("env/combat")=="verbose") && wizardp(me))
              tell_object(me,sprintf("damage step 4.5: force & jiali foo is %d\n", foo));
            if( stringp(foo) )
                result += foo;
            else if( intp(foo) )
                damage_bonus += foo;
        }

        if (strwidth(result))
        {
            combat_msg(result, 2, me, victim);
            COMBAT_D->report_status(victim);
            result = "";
        }
    }
    
    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 5: damage_bonus is %d\n", damage_bonus));

    if( martial_skill )
    {
        foo = find_skill(martial_skill)->hit_ob(me, victim, damage_bonus);        
        if( stringp(foo) )
            result += foo;
        else if(intp(foo) )
        {
            damage_bonus += foo;
            if ((me->query("env/combat")=="verbose") && wizardp(me))
              tell_object(me,sprintf("damage step 6: martial skill foo is %d\n", foo));
        }

        if (strwidth(result))
        {
            combat_msg(result, 2, me, victim);
            COMBAT_D->report_status(victim);
            result = "";
        }
    }
    
    // Let weapon or monster have their special damage.
    if( objectp(weapon) ) 
    {
        foo = weapon->hit_ob(me, victim, damage_bonus);
        if( stringp(foo) ) result += foo;
        else if(intp(foo) ) damage_bonus += foo;
        if (strwidth(result))
        {
            combat_msg(result, 2, me, victim);
            COMBAT_D->report_status(victim);
            result = "";
        }
    } 
    else 
    {
        foo = me->hit_ob(me, victim, damage_bonus);
        if( stringp(foo) ) result += foo;
        else if(intp(foo) ) damage_bonus += foo;
        if (strwidth(result))
        {
            combat_msg(result, 2, me, victim);
            COMBAT_D->report_status(victim);
            result = "";
        }
    }
    
    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 7: weapon damage is %d\n", foo));
    
    if( damage_bonus > 0 )
        damage += (damage_bonus + random(damage_bonus))/2;

    if ((me->query("env/combat")=="verbose") && wizardp(me))
      tell_object(me,sprintf("damage step 8: damage after adding bonus is %d\n", damage));

    //提供额外的攻击增益和防御增益
    //apply/deep_injure  普通伤害增益，按照百分比增加普通攻击伤害，加深效果最多不超过400%
    //apply/week_injure  普通伤害抗性（%），按照百分比削弱普通攻击伤害，削减效果最多不超过75%
    //apply/defense_attack 普通伤害抗性（点），按照固定数字削弱普通攻击伤害，削弱数字不超过总伤害的50%
    //apply/abs_week_injure 普通伤害绝对抗性，最多不超过25%
    deepj=me->query_temp("apply/deep_injure");
    weekj=victim->query_temp("apply/week_injure");
    defatk=victim->query_temp("apply/defense_attack");
    absweekj=victim->query_temp("apply/abs_week_injure");
    if ( deepj>0 || weekj>0 || defatk>0 )
    {
    	if ( deepj<-75 )
    		deepj=-75;
    	
    	if ( deepj>400 )
    		deepj=400;
    	
    	if ( weekj<-400 )
    		weekj=-400;
    	
    	if ( weekj>75 )
    		weekj=75;
      
      if ( absweekj<=0 )
        absweekj=0;
      
      if ( absweekj>25 )
        absweekj=25;
    	
    	if ( defatk<0 )
    		defatk=0;
    	
    	if ( defatk>damage/2 )
    		defatk=damage/2;
    	
    	damage=(damage-defatk)*(100+deepj)/100*(100-weekj-absweekj)/100;
    }
    
    // ===== 五行克制修正 =====
    // 基于设计文档 2.1 节: 五行系数 (克制 1.5x / 被克 0.7x / 变异 1.2~2.0x)
    att_elem = ELE_NONE;
    def_elem = ELE_NONE;
    elem_mod = 1.0;
    {
        att_elem = query_character_element(me);
        def_elem = query_character_element(victim);
        
        if (att_elem != ELE_NONE && def_elem != ELE_NONE)
        {
            elem_mod = calc_element_modifier(att_elem, def_elem);
            damage = to_int(damage * elem_mod);
            
            string att_c = query_element_color(att_elem);
            string def_c = query_element_color(def_elem);
            string att_n = query_element_name(att_elem);
            string def_n = query_element_name(def_elem);
            
            if (elem_mod >= 1.4)
                result += HIR "【五行克制】" + att_c + att_n + NOR "克制" + def_c + def_n + NOR "，伤害大增！\n" NOR;
            else if (elem_mod <= 0.75)
                result += HIC "【五行被克】" + att_c + att_n + NOR "被" + def_c + def_n + NOR "克制，伤害大减！\n" NOR;
            else if (elem_mod > 1.01)
                result += HIY "【五行小优】" + att_c + att_n + NOR "对" + def_c + def_n + NOR "有微弱优势。\n" NOR;
            else if (elem_mod < 0.99)
                result += CYN "【五行小劣】" + att_c + att_n + NOR "被" + def_c + def_n + NOR "略微压制。\n" NOR;
        }
    }
    
    // ===== 境界系数修正 =====
    // 基于设计文档 2.1 节: 境界压制系数
    {
        int my_realm = me->query("realm_level");
        int your_realm = victim->query("realm_level");
        
        if (my_realm > 0 && your_realm > 0)
        {
            float realm_mod = 1.0;
            int realm_diff = my_realm - your_realm;
            
            if (realm_diff >= 10)        // 差 2 大境界以上
                realm_mod = 2.5;
            else if (realm_diff >= 5)    // 差 1 大境界
                realm_mod = 2.0;
            else if (realm_diff >= 2)    // 差 2-4 小层
                realm_mod = 1.2 + (realm_diff - 2) * 0.05;
            else if (realm_diff == 1)    // 差 1 小层
                realm_mod = 1.2;
            else if (realm_diff == 0)    // 同境界
                realm_mod = 1.0;
            else if (realm_diff == -1)   // 低 1 小层
                realm_mod = 0.85;
            else if (realm_diff >= -4)   // 低 2-4 小层
                realm_mod = 0.65;
            else if (realm_diff >= -9)   // 低 1 大境界
                realm_mod = 0.50;
            else                         // 低 2 大境界以上
                realm_mod = 0.25;
            
            damage = to_int(damage * realm_mod);
            
            if (realm_mod >= 2.0)
                result += HIR "【境界压制】你以绝对修为碾压对手，伤害大增！\n" NOR;
            else if (realm_mod >= 1.5)
                result += HIY "【境界优势】你的修为高于对方，伤害有所提升。\n" NOR;
            else if (realm_mod <= 0.30)
                result += HIB "【境界鸿沟】对方修为远超于你，你的攻击几乎无效！\n" NOR;
            else if (realm_mod <= 0.55)
                result += HIC "【境界劣势】对方修为高于你，伤害大幅降低。\n" NOR;
            else if (realm_mod < 0.9)
                result += CYN "【境界微劣】对方修为略高于你，伤害略有降低。\n" NOR;
        }
    }
    
    // ===== 灵根加成修正 =====
    // 基于设计文档 3.3 节: 灵根对法术伤害的影响
    {
        mapping sr = me->query("spirit_root");
        if (!mapp(sr))
            sr = me->query("spirit_root_data");
            
        if (mapp(sr))
        {
            string lingen_type = sr["type"];
            float lingen_mod = 1.0;
            
            if (stringp(lingen_type))
            {
                // 天灵根
                if (strsrch(lingen_type, "天灵根") >= 0)
                    lingen_mod = 1.5;
                // 真灵根
                else if (strsrch(lingen_type, "真灵根") >= 0)
                    lingen_mod = 1.0;
                // 变异灵根
                else if (strsrch(lingen_type, "变异灵根") >= 0 || strsrch(lingen_type, "变异") >= 0)
                    lingen_mod = 1.3;
                // 伪灵根
                else if (strsrch(lingen_type, "伪灵根") >= 0)
                    lingen_mod = 0.6;
            }
            
            // 属性匹配额外加成
            string main_elem = sr["main_element"];
            if (stringp(main_elem) && att_elem != ELE_NONE)
            {
                string att_name = query_element_name(att_elem);
                if (main_elem == att_name)
                {
                    // 灵根属性与技能属性一致，额外加成
                    int extra = 0;
                    if (lingen_type == "天灵根") extra = 30;
                    else if (lingen_type == "真灵根") extra = 15;
                    else if (lingen_type == "变异灵根") extra = 25;
                    else extra = 5;
                    
                    lingen_mod += extra / 100.0;
                    result += HIY "【灵根共鸣】" + main_elem + "灵根与技能属性共鸣，伤害 +" + extra + "%！\n" NOR;
                }
                else if (att_elem >= ELE_GOLD && att_elem <= ELE_EARTH)
                {
                    // 非对应属性有惩罚（仅针对天灵根和变异灵根）
                    if (lingen_type == "天灵根")
                    {
                        lingen_mod -= 0.15;
                        result += CYN "【灵根冲突】天灵根使用非对应属性法术，威力下降。\n" NOR;
                    }
                    else if (strsrch(lingen_type, "变异") >= 0)
                    {
                        lingen_mod -= 0.10;
                        result += CYN "【灵根冲突】变异灵根使用常规五行法术，威力微降。\n" NOR;
                    }
                }
            }
            
            damage = to_int(damage * lingen_mod);
        }
    }
    
    // ===== 阵法加成修正 =====
    // 由 formation_damage_modify 处理阵法对伤害的加成/减免
    if (function_exists("formation_damage_modify", victim))
    {
        int old_damage = damage;
        damage = victim->formation_damage_modify(victim, damage, att_elem);
        if (damage < old_damage)
            result += HIC "【阵法减伤】对方阵法削弱了你的攻击！\n" NOR;
    }
    
    // ===== 暴击/会心判定 =====
    // 基于设计文档 2.2 节
    {
        int crit_chance = 5;       // 基础暴击率 5%
        int crit_damage = 150;     // 基础暴击伤害 150%
        
        // 装备暴击率加成
        crit_chance += me->query_temp("apply/crit_rate");
        if (crit_chance > 50) crit_chance = 50;  // 暴击率上限 50%
        
        // 暴击伤害加成
        crit_damage += me->query_temp("apply/crit_damage");
        if (crit_damage > 300) crit_damage = 300;
        
        // 会心率（法术暴击）
        int hui_xin_chance = 0;
        int my_shenshi = me->query("shenshi");
        int your_shenshi = victim->query("shenshi");
        if (my_shenshi > 0 && your_shenshi > 0)
        {
            hui_xin_chance = my_shenshi * 10 / (my_shenshi + your_shenshi);
            if (hui_xin_chance > 15) hui_xin_chance = 15;
        }
        
        // 暴击判定
        if (random(100) < crit_chance)
        {
            damage = damage * crit_damage / 100;
            result += HIR "【暴击】致命一击！伤害 ×" + (crit_damage / 100) + "." + (crit_damage % 100 / 10) + "！\n" NOR;
        }
        // 会心判定（法术时触发）
        else if (random(100) < hui_xin_chance)
        {
            damage = damage * 200 / 100;  // 会心伤害 2.0x
            result += HIM "【会心一击】法术会心！伤害翻倍！\n" NOR;
        }
    }
    
    damage=special_armor_effect(victim,me,damage);
    //whuan,这里注意，调用对手的盔甲特效
    if( damage < 0 ) 
    {
  	  log_file( "wiz/combatd", sprintf("\n%s\n异常数据（伤害负值）：计算目标：%s(%s)，对方：%s(%s), 武器伤害：%d，伤害：%d，附加伤害：%d\n", 
  	    ctime(time()), me->query("name"), me->query("id"), victim->query("name"), victim->query("id"), me->query_temp("apply/damage"), damage,  damage_bonus) );
      damage = 0;
    }
    
    // Let combat exp take effect
    defense_factor = your["combat_exp"];
    if(!intp(my["combat_exp"]) || my["combat_exp"] <= 0) my["combat_exp"] = 0;
    while( random(defense_factor) > my["combat_exp"] ) {
        damage -= damage / 3;
        defense_factor /= 2;
    }
    return ({damage,result});
}