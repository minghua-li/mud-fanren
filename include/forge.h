// forge.h
// 炼器系统常量 —— 法宝品质/五步炼制流程/炼器场所
// 设计文档: .knowledge/items/1E-法宝丹药经济.md §1.5（炼器流程/材料表）
//          .knowledge/02-扩充内容/02-法宝与武器图鉴.md §4（炼制与进阶）
// 相关 daemon: adm/daemons/forge_d.c（FORGE_D）
// Created for ticket #74

#ifndef __FORGE_H__
#define __FORGE_H__

// -------- 炼器场所（化刀坞炼器工坊，#60 门派设施房间） --------
#define FORGE_ROOM              "/d/yueguo/huadao/fac/lianqi"

// -------- 法宝品质（1E §1.2 品质维度；level 字段取值） --------
#define FORGE_QUALITY_LOW       "下品"      // 倍率 1.00
#define FORGE_QUALITY_MID       "中品"      // 倍率 1.15
#define FORGE_QUALITY_HIGH      "上品"      // 倍率 1.30
#define FORGE_QUALITY_TOP       "极品"      // 倍率 1.50

// 品质 → 属性倍率（炼成时乘算 attack/defense 并取整）
#define FORGE_QUALITY_RATE ([ \
        FORGE_QUALITY_LOW  : 1.0,  \
        FORGE_QUALITY_MID  : 1.15, \
        FORGE_QUALITY_HIGH : 1.3,  \
        FORGE_QUALITY_TOP  : 1.5,  \
])

// 品质 → 禁制层数加成（禁制越多威能越大，1E §1.3）
#define FORGE_QUALITY_BAN ([ \
        FORGE_QUALITY_LOW  : 0, \
        FORGE_QUALITY_MID  : 1, \
        FORGE_QUALITY_HIGH : 2, \
        FORGE_QUALITY_TOP  : 3, \
])

// -------- 五步炼制流程（1E §1.5：材料采集→精炼提纯→器胚锻造→禁制铭刻→通灵开光） --------
// 注：材料采集在坊市/采集侧完成（进入炼器时以背包材料齐备为准），
//     工坊内实际执行后四步 + 通灵开光为最终判定步。
#define FORGE_STEPS ({ "精炼提纯", "器胚锻造", "禁制铭刻", "通灵开光" })

// -------- 成功率钳制边界（对齐 #61 突破概率钳制 [1,99] 口径） --------
#define FORGE_MIN_RATE  1
#define FORGE_MAX_RATE  99

#endif
