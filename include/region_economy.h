// region_economy.h
// 区域经济系统 —— 区域定义、物价差异、区域特产、贸易路线
// 对应设计文档 02-扩充内容/02-区域游戏玩法.md §5.2 + §6.5
// Created for ticket #22

#ifndef __REGION_ECONOMY_H__
#define __REGION_ECONOMY_H__

//=============================================================================
// 区域定义（对应设计文档 §5.1 区域准入境界门槛总表）
//=============================================================================
#define REGION_MIRROR_LAKE      "mirror_lake"       // 镜州江湖（凡人区域）
#define REGION_YUE_SECTS        "yue_sects"         // 越国七派（炼气）
#define REGION_TAI_NAN          "tai_nan"           // 太南谷（炼气）
#define REGION_BLOOD_FORBIDDEN  "blood_forbidden"   // 血色禁地（炼气）
#define REGION_LUANXING_ISLANDS "luanxing_islands"  // 乱星海岛屿（筑基）
#define REGION_TIAN_XING        "tian_xing"         // 天星城（结丹）
#define REGION_OUTER_SEA        "outer_sea"         // 外星海（结丹-元婴）
#define REGION_XU_TIAN          "xu_tian"           // 虚天殿（结丹-元婴）
#define REGION_NINE_NATIONS     "nine_nations"      // 九国盟（筑基-结丹）
#define REGION_MULAN            "mulan"             // 慕兰草原（结丹-元婴）
#define REGION_DA_JIN           "da_jin"            // 大晋帝国（元婴）
#define REGION_TIAN_YUAN        "tian_yuan"         // 天渊城（化神）
#define REGION_SAN_HUANG        "san_huang"         // 三皇境（化神-炼虚）
#define REGION_BARBARIAN        "barbarian"         // 蛮荒世界（化神-炼虚）
#define REGION_LEIMING          "leiming"           // 雷鸣大陆（炼虚）
#define REGION_GUANG_HAN        "guang_han"         // 广寒界（化神-合体）
#define REGION_DI_YUAN          "di_yuan"           // 地渊（化神-大乘）

//=============================================================================
// 区域名称
//=============================================================================
#define REGION_NAME ([ \
    REGION_MIRROR_LAKE     : "镜州江湖", \
    REGION_YUE_SECTS       : "越国七派", \
    REGION_TAI_NAN         : "太南谷", \
    REGION_BLOOD_FORBIDDEN : "血色禁地", \
    REGION_LUANXING_ISLANDS : "乱星海岛屿", \
    REGION_TIAN_XING       : "天星城", \
    REGION_OUTER_SEA       : "外星海", \
    REGION_XU_TIAN         : "虚天殿", \
    REGION_NINE_NATIONS    : "九国盟", \
    REGION_MULAN           : "慕兰草原", \
    REGION_DA_JIN          : "大晋帝国", \
    REGION_TIAN_YUAN       : "天渊城", \
    REGION_SAN_HUANG       : "三皇境", \
    REGION_BARBARIAN       : "蛮荒世界", \
    REGION_LEIMING         : "雷鸣大陆", \
    REGION_GUANG_HAN       : "广寒界", \
    REGION_DI_YUAN         : "地渊", \
])

//=============================================================================
// 区域物价修正系数（对应设计文档 §5.2 资源产出等级）
// 基础定价 = 1.0，低阶区域 < 1.0（物价低），高阶区域 > 1.0（物价高）
// 区域物价修正是物品在该区域的基础价格乘数
//=============================================================================
#define REGION_PRICE_MODIFIER ([ \
    REGION_MIRROR_LAKE     : 0.60,  /* 凡人区域，物价最低 */ \
    REGION_YUE_SECTS       : 0.80,  /* 新手区域，物价偏低 */ \
    REGION_TAI_NAN         : 0.85,  /* 坊市交易，物价适中 */ \
    REGION_BLOOD_FORBIDDEN : 1.20,  /* 秘境区域，物价偏高 */ \
    REGION_LUANXING_ISLANDS : 0.90, /* 海岛区域，物价适中 */ \
    REGION_TIAN_XING       : 1.10,  /* 经济中心，物价略高 */ \
    REGION_OUTER_SEA       : 1.30,  /* 高阶猎场，物价较高 */ \
    REGION_XU_TIAN         : 1.50,  /* 顶级秘境，物价很高 */ \
    REGION_NINE_NATIONS    : 1.00,  /* 战争地带，物价标准 */ \
    REGION_MULAN           : 1.20,  /* 异域区域，物价偏高 */ \
    REGION_DA_JIN          : 1.40,  /* 大晋繁华，物价较高 */ \
    REGION_TIAN_YUAN       : 1.80,  /* 灵界起点，物价昂贵 */ \
    REGION_SAN_HUANG       : 2.00,  /* 灵界腹地，物价昂贵 */ \
    REGION_BARBARIAN       : 2.50,  /* 蛮荒险地，物价极高 */ \
    REGION_LEIMING         : 3.00,  /* 雷鸣大陆，顶级物价 */ \
    REGION_GUANG_HAN       : 2.00,  /* 秘境区域，物价很高 */ \
    REGION_DI_YUAN         : 2.50,  /* 深渊区域，物价极高 */ \
])

//=============================================================================
// 区域特产（对应设计文档 §5.2 + §6.5）
// 每个区域有 1~3 种核心特产，在本地出售有加成
// 跨区域贸易可赚取差价（设计文档 §6.5 区域特产贸易路线）
//=============================================================================

// 商品类型前缀
#define REGION_SPECIAL_PREFIX  "region_special:"

// 区域特产定义：region_id -> ({ 特产商品type列表 })
#define REGION_SPECIAL_PRODUCTS ([ \
    REGION_MIRROR_LAKE     : ({ "herb_basic", "ore_basic", "hide_basic" }), \
    REGION_YUE_SECTS       : ({ "pill_qi", "furnace_low", "talisman_basic" }), \
    REGION_TAI_NAN         : ({ "talisman_basic", "pill_qi", "misc_basic" }), \
    REGION_BLOOD_FORBIDDEN : ({ "herb_condensing", "demon_pill_low", "ancient_relic" }), \
    REGION_LUANXING_ISLANDS : ({ "pearl", "coral_jade", "sea_herb" }), \
    REGION_TIAN_XING       : ({ "demon_pill_mid", "ancient_relic", "silk_spirit" }), \
    REGION_OUTER_SEA       : ({ "demon_pill_high", "beast_bone", "beast_hide_high" }), \
    REGION_XU_TIAN         : ({ "gubao_fragment", "herb_wan_nian", "core_high" }), \
    REGION_NINE_NATIONS    : ({ "weapon_military", "armor_military", "pill_battle" }), \
    REGION_MULAN           : ({ "herb_mulan", "talisman_fashi", "silk_special" }), \
    REGION_DA_JIN          : ({ "gubao_complete", "pill_top", "silk_imperial" }), \
    REGION_TIAN_YUAN       : ({ "pill_miechen", "material_spirit", "weapon_spirit" }), \
    REGION_SAN_HUANG       : ({ "herb_spirit_high", "material_spirit", "pill_spirit" }), \
    REGION_BARBARIAN       : ({ "beast_core", "hide_spirit", "bone_spirit" }), \
    REGION_LEIMING         : ({ "pill_leiming", "material_thunder", "weapon_thunder" }), \
    REGION_GUANG_HAN       : ({ "gubao_xuantian", "jinshu_page", "herb_divine" }), \
    REGION_DI_YUAN         : ({ "ore_dark", "beast_core_deep", "herb_abyss" }), \
])

//=============================================================================
// 区域特产价格加成（在产地区域出售时获得额外收益）
// 特产在原产地出售价 = 基准价 × 1.5（+50%）
// 特产在非产地区域出售价 = 基准价 × 0.8 ~ 1.2（取决于距离）
//=============================================================================
#define SPECIAL_SOURCE_BONUS   1.50   // 特产在原产地溢价
#define SPECIAL_DISTANT_PENALTY 0.80  // 特产在远方的最低折扣

//=============================================================================
// 区域商店折扣（基于声望）
//=============================================================================
#define REGION_REPUT_DISCOUNT ([ \
    0    : 1.00,  /* 陌路：原价 */ \
    100  : 1.00,  /* 相识：原价 */ \
    500  : 0.90,  /* 友善：9折 */ \
    2000 : 0.80,  /* 尊敬：8折 */ \
    5000 : 0.70,  /* 崇敬：7折 */ \
    10000: 0.50,  /* 崇拜：5折 */ \
])

//=============================================================================
// 贸易路线利润系数
// 设计文档 §6.5：区域特产贸易路线
// 利润系数越高的路线，跑商收益越高（也越危险）
//=============================================================================
#define TRADE_ROUTE_PROFIT ([ \
    /* 越国 → 太南谷 */ \
    REGION_YUE_SECTS "->" REGION_TAI_NAN       : 1.2, \
    /* 太南谷 → 天星城 */ \
    REGION_TAI_NAN "->" REGION_TIAN_XING       : 1.8, \
    /* 乱星海 → 天星城 */ \
    REGION_LUANXING_ISLANDS "->" REGION_TIAN_XING : 1.3, \
    /* 慕兰草原 → 天星城 */ \
    REGION_MULAN "->" REGION_TIAN_XING         : 2.0, \
    /* 天星城 → 大晋 */ \
    REGION_TIAN_XING "->" REGION_DA_JIN        : 2.5, \
    /* 天渊城 ↔ 三皇境 */ \
    REGION_TIAN_YUAN "->" REGION_SAN_HUANG     : 1.5, \
    /* 蛮荒世界 → 天渊城 */ \
    REGION_BARBARIAN "->" REGION_TIAN_YUAN     : 2.0, \
    /* 雷鸣大陆 → 天渊城 */ \
    REGION_LEIMING "->" REGION_TIAN_YUAN       : 3.0, \
])

#endif
