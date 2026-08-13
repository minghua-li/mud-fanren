// cmds/usr/lianqi.c
// 炼器命令 —— 法宝炼制（1E §1.5 五步流程）
// 场所：化刀坞炼器工坊（d/yueguo/huadao/fac/lianqi，#60 门派设施）
// 依赖：FORGE_D（配方/炼制）、SECT_FACILITY_D（工坊设施加成 query_forge_bonus）、
//       SECT_D（境界判定 query_cultivation_tier/门派身份 query_player_sect）
// Created for ticket #74

#include <ansi.h>
#include <forge.h>
#include <globals.h>
#include <sect.h>

inherit F_CLEAN_UP;

void create() { seteuid(ROOT_UID); }

int help(object me);

int main(object me, string arg)
{
    int bonus;

    if (arg == "list")
    {
        write(FORGE_D->describe_formula_list());
        return 1;
    }

    if (!stringp(arg) || arg == "" || arg == "help")
    {
        help(me);
        return 1;
    }

    // 场所检查：必须在炼器工坊内（#60 设施房间按 base_name 匹配）
    if (SECT_FACILITY_D->query_current_facility(me) != "huadao_lianqi")
        return notify_fail("你必须先到化刀坞炼器工坊才能炼器。\n");

    // 化刀坞弟子自动激活工坊设施加成（消耗灵石/贡献换取炼器加成；无 buff 时）
    bonus = SECT_FACILITY_D->query_forge_bonus(me);
    if (bonus <= 0 && SECT_D->query_player_sect(me) == "huadao_dock")
    {
        SECT_FACILITY_D->use_facility(me, "huadao_lianqi");
        bonus = SECT_FACILITY_D->query_forge_bonus(me);
    }
    if (bonus > 0)
        tell_object(me, HIG "炼器工坊灵火加持，成功率 +" + bonus + "%\n" NOR);

    FORGE_D->forge(me, arg);
    return 1;
}

int help(object me)
{
    write(@HELP
炼器（lianqi）—— 炼制法宝

用法：
  lianqi list                查看可炼制的法宝配方
  lianqi <配方id>            按配方炼制法宝（须在化刀坞炼器工坊内）

炼制流程（1E §1.5）：
  材料采集 → 精炼提纯 → 器胚锻造 → 禁制铭刻 → 通灵开光
  任一步骤失败，材料尽数耗毁；成功则获得法宝成品。

说明：
  * 炼器材料可在太南谷坊市购置（在坊市内 buy tiejing/yinjing/jinjing/xuantie/gengjing，
    用 list 查看全部货物与价格）。
  * 炼制成功率受炼器术等级（lianqi-shu）与炼器工坊设施加成影响。
  * 化刀坞弟子在工坊内炼器可自动获得设施加成（消耗灵石/贡献）。
  * 法宝境界需求见配方说明，境界不足无法炼制与御使。

HELP
    );
    return 1;
}
