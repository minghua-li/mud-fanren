// mansion.h
// 洞府经营系统 —— 类型定义、常量、宏、接口声明
// Created for ticket #32

#ifndef __MANSION_H__
#define __MANSION_H__

//=============================================================================
// 洞府等级（Lv.1 ~ Lv.5，对应设计文档 §3.4.1）
//=============================================================================
#define MANSION_LV1           1   // 草庐/石室：免费（门派配给）
#define MANSION_LV2           2   // 普通洞府：灵石购买/宗门贡献
#define MANSION_LV3           3   // 灵脉洞府：高额灵石/特殊贡献
#define MANSION_LV4           4   // 顶级洞府：声望+稀有材料
#define MANSION_LV5           5   // 圣地级：特殊条件+天价材料

// 洞府等级名称
#define MANSION_LEVEL_NAME ([ \
    MANSION_LV1 : "草庐", \
    MANSION_LV2 : "普通洞府", \
    MANSION_LV3 : "灵脉洞府", \
    MANSION_LV4 : "顶级洞府", \
    MANSION_LV5 : "圣地洞府", \
])

//=============================================================================
// 建筑类型（对应设计文档 §3.2 功能模块）
//=============================================================================
#define MANSION_BLDG_CULTIVATE   1   // 打坐室（修炼速度加成）
#define MANSION_BLDG_STORAGE     2   // 储物室（物品存放）
#define MANSION_BLDG_REST        3   // 休息室（HP/MP恢复）
#define MANSION_BLDG_GARDEN      4   // 药圃（种植灵药）
#define MANSION_BLDG_FORGE       5   // 炼器室（炼器成功率+）
#define MANSION_BLDG_ALCHEMY     6   // 炼丹室（炼丹成功率+）
#define MANSION_BLDG_BEAST       7   // 灵兽室（驯养灵兽）
#define MANSION_BLDG_BARRIER     8   // 护府禁制（防御）
#define MANSION_BLDG_WARN        9   // 预警法阵（入侵警报）
#define MANSION_BLDG_PORTAL     10   // 传送锚点（快速回府）

// 建筑名称
#define MANSION_BLDG_NAME ([ \
    MANSION_BLDG_CULTIVATE : "打坐室", \
    MANSION_BLDG_STORAGE   : "储物室", \
    MANSION_BLDG_REST      : "休息室", \
    MANSION_BLDG_GARDEN    : "药圃", \
    MANSION_BLDG_FORGE     : "炼器室", \
    MANSION_BLDG_ALCHEMY   : "炼丹室", \
    MANSION_BLDG_BEAST     : "灵兽室", \
    MANSION_BLDG_BARRIER   : "护府禁制", \
    MANSION_BLDG_WARN      : "预警法阵", \
    MANSION_BLDG_PORTAL    : "传送锚点", \
])

// 各建筑可建造的最低洞府等级
#define MANSION_BLDG_UNLOCK ([ \
    MANSION_BLDG_CULTIVATE : MANSION_LV1, \
    MANSION_BLDG_STORAGE   : MANSION_LV1, \
    MANSION_BLDG_REST      : MANSION_LV1, \
    MANSION_BLDG_GARDEN    : MANSION_LV2, \
    MANSION_BLDG_FORGE     : MANSION_LV3, \
    MANSION_BLDG_ALCHEMY   : MANSION_LV3, \
    MANSION_BLDG_BEAST     : MANSION_LV3, \
    MANSION_BLDG_BARRIER   : MANSION_LV2, \
    MANSION_BLDG_WARN      : MANSION_LV2, \
    MANSION_BLDG_PORTAL    : MANSION_LV3, \
])

//=============================================================================
// 洞府修炼加成系数（对应设计文档 §3.4.2）
//=============================================================================
#define MANSION_CULTIVATE_SPEED ([ \
    MANSION_LV1 : 1.0, \
    MANSION_LV2 : 1.2, \
    MANSION_LV3 : 1.5, \
    MANSION_LV4 : 2.0, \
    MANSION_LV5 : 2.5, \
])

//=============================================================================
// 洞府升级成本（对应设计文档 §3.4.1，单位：灵石）
//=============================================================================
#define MANSION_UPGRADE_COST ([ \
    MANSION_LV1 : 0, \
    MANSION_LV2 : 5000, \
    MANSION_LV3 : 50000, \
    MANSION_LV4 : 200000, \
    MANSION_LV5 : 1000000, \
])

//=============================================================================
// 药圃土地品质（对应设计文档 §3.3）
//=============================================================================
#define LAND_NORMAL     1   // 凡土：默认，×1.0，低阶，2块
#define LAND_SPIRIT     2   // 灵土：门派贡献，×1.5，中阶，5块
#define LAND_SPRING     3   // 灵泉浇灌灵土：任务/购买，×2.0，高阶，8块
#define LAND_ANCIENT    4   // 万年灵土：秘境/天价，×3.0，顶级，12块

#define LAND_NAME ([ \
    LAND_NORMAL : "凡土", \
    LAND_SPIRIT : "灵土", \
    LAND_SPRING : "灵泉浇灌灵土", \
    LAND_ANCIENT : "万年灵土", \
])

// 土地种植速度系数
#define LAND_SPEED ([ \
    LAND_NORMAL : 1.0, \
    LAND_SPIRIT : 1.5, \
    LAND_SPRING : 2.0, \
    LAND_ANCIENT : 3.0, \
])

// 各品质土地的最大地块数
#define LAND_MAX_PLOTS ([ \
    LAND_NORMAL : 2, \
    LAND_SPIRIT : 5, \
    LAND_SPRING : 8, \
    LAND_ANCIENT : 12, \
])

// 土地升级成本（从当前品质升到下一级，灵石）
#define LAND_UPGRADE_COST ([ \
    LAND_NORMAL : 1000, \
    LAND_SPIRIT : 10000, \
    LAND_SPRING : 100000, \
    LAND_ANCIENT : 0, \
])

//=============================================================================
// 药圃种植状态
//=============================================================================
#define PLOT_EMPTY      0   // 空闲
#define PLOT_SEEDED     1   // 已播种
#define PLOT_GROWING    2   // 生长中
#define PLOT_MATURE     3   // 成熟（可收获）
#define PLOT_FALLOW     4   // 休整中

//=============================================================================
// 洞府灵气浓度类型
//=============================================================================
#define AURA_NONE       0   // 无灵脉
#define AURA_LOW        1   // 下品灵脉
#define AURA_MIDDLE     2   // 中品灵脉
#define AURA_HIGH       3   // 上品灵脉
#define AURA_SUPREME    4   // 极品灵脉

#define AURA_NAME ([ \
    AURA_NONE : "无", \
    AURA_LOW : "下品灵脉", \
    AURA_MIDDLE : "中品灵脉", \
    AURA_HIGH : "上品灵脉", \
    AURA_SUPREME : "极品灵脉", \
])

// 灵脉品质系数（设计文档 §3.4.2）
#define AURA_FACTOR ([ \
    AURA_NONE : 1.0, \
    AURA_LOW : 1.2, \
    AURA_MIDDLE : 1.5, \
    AURA_HIGH : 2.0, \
    AURA_SUPREME : 3.0, \
])

//=============================================================================
// 洞府维护成本（对应设计文档 §3.4.3，每 7 天）
//=============================================================================
#define MANSION_MAINTENANCE ([ \
    MANSION_LV1 : 0, \
    MANSION_LV2 : 100, \
    MANSION_LV3 : 500, \
    MANSION_LV4 : 2000, \
    MANSION_LV5 : 10000, \
])

// 禁制充能成本（每 30 天）
#define BARRIER_CHARGE ([ \
    MANSION_LV1 : 0, \
    MANSION_LV2 : 500, \
    MANSION_LV3 : 2000, \
    MANSION_LV4 : 10000, \
    MANSION_LV5 : 50000, \
])

// 药圃维护成本（每次种植）
#define GARDEN_MAINTENANCE ([ \
    LAND_NORMAL : 10, \
    LAND_SPIRIT : 50, \
    LAND_SPRING : 200, \
    LAND_ANCIENT : 1000, \
])

//=============================================================================
// 建筑效果参数（加成百分比，如 5 = +5%）
//=============================================================================
#define FORGE_BONUS_BASE      5   // 炼器室基础加成 %
#define ALCHEMY_BONUS_BASE    5   // 炼丹室基础加成 %
#define FORGE_BONUS_PER_LEVEL 3   // 建筑每级递增 %
#define ALCHEMY_BONUS_PER_LEVEL 3
#define BEAST_TRAIN_SPEED     1.2 // 灵兽室驯养速度系数
#define REST_HP_RECOVER       1.5 // 休息室气血恢复系数
#define REST_MP_RECOVER       1.5 // 休息室法力恢复系数

//=============================================================================
// 建筑等级上限（随洞府等级开放）
//=============================================================================
#define MAX_BLDG_LEVEL(lv) (lv * 2)  // 洞府 Lv.N 时建筑最高可升到 N*2 级

//=============================================================================
// 种植常量（灵药种子相关）
//=============================================================================
// 生长阶段时长基数（秒），实际用时 = 基数 / 土地速度系数
#define GROWTH_BASE_SHORT   3600     // 短周期（1小时）
#define GROWTH_BASE_MEDIUM  14400    // 中周期（4小时）
#define GROWTH_BASE_LONG    86400    // 长周期（24小时）

//=============================================================================
// 洞府数据字段路径宏（供 mansion_d 内部查询使用）
//=============================================================================
#define MNS_FIELD_ID         "id"          // 玩家 ID
#define MNS_FIELD_LEVEL      "level"       // 洞府等级
#define MNS_FIELD_AURA       "aura"        // 灵脉品质
#define MNS_FIELD_CREATED    "created"     // 创建时间
#define MNS_FIELD_BLDGS      "buildings"   // 建筑列表 mapping
// building entry: bldg_id -> ([ "level": N, "built": time() ])
#define MNS_FIELD_GARDEN     "garden"      // 药圃数据
// garden: ([ "land": LAND_*, "plots": ([ idx: plot_data ]) ])
#define MNS_FIELD_MAINT      "last_maint"  // 上次维护时间
#define MNS_FIELD_BARRIER_CHARGE "last_barrier_charge"

//=============================================================================
// NPC/怪物常量（洞府守护相关）
//=============================================================================
#define MANSION_GUARD_ID     "mansion_guard"
#define MANSION_GUARD_NAME   "洞府守卫"

#endif
