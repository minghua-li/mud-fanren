// pill.h
// 丹药炼制系统（#73：#64 子单，P5-4 法宝丹药炼制系统 1E 落地之丹药侧）
//
// 设计依据：.knowledge/items/1E-法宝丹药经济.md §2（丹药体系）+ §4.1（数据结构）
//           .knowledge/02-扩充内容/02-丹药体系详解.md §1.2（品级）/§3（核心丹药）/§4（炼制系统）
//
// 丹药实体属性（1E §4.1）：
//   pill_type    —— 丹药分类（见 PILL_TYPE_*）
//   stage        —— 对应修炼阶段（REALM_* 或 0=通用）
//   effect       —— 效果值（修为丹=修为量；突破丹=突破概率加成%）
//   quality      —— 品级（PILL_QUALITY_*，1=凡品 2=良品 3=上品）
//   side_effect  —— 副作用描述（如丹毒累积）
//   refine_level —— 所需炼丹术等级

#ifndef PILL_H
#define PILL_H

// 丹药分类（02-丹药体系详解 §1.1 六大类中的核心三类 + 通用）
#define PILL_TYPE_XIUWEI       "xiuwei"        // 修为丹：增加修为值
#define PILL_TYPE_BREAKTHROUGH "breakthrough"  // 突破丹：突破概率加成
#define PILL_TYPE_HEAL         "heal"          // 疗伤丹：回复气血

// 品级（1E §2.3 上/中/下三品；02 §1.2 效果系数）
#define PILL_QUALITY_FAN       1   // 凡品 ×1.0
#define PILL_QUALITY_LIANG     2   // 良品 ×1.5
#define PILL_QUALITY_SHANG     3   // 上品 ×2.0

// 丹方结构（PILL_D 内部数据）
//   ([ id: ([
//       "name"         : 丹方名（中文，供命令显示）
//       "pill"         : 成品丹药对象路径
//       "ingredients"  : ([ 材料id: 数量 ])，材料id 对齐坊市 material_id
//       "base_rate"    : 基础成功率（0-100）
//       "refine_level" : 所需炼丹术等级
//       "stage"        : 对应境界阶段（REALM_*）
//       "quality"      : 基准品级
//   ]) ])

// 炼丹术等级（玩家 DBASE）
#define PILL_REFINE_LEVEL      "pill_refine_level"   // int，炼丹术等级（0 起）
#define PILL_REFINE_EXP        "pill_refine_exp"     // int，累计炼制经验（成功 1 次 +1）
#define PILL_TOXIN             "pill_toxin"          // int，丹毒累积（副作用）

// 丹毒阈值（02 §5.2 简化：0-50 正常，>50 提示微毒，暂不做惩罚层）
#define PILL_TOXIN_WARN        50

// 突破丹叠加上限（02 §3.1 筑基丹可叠加最多 3 颗；结金丹同）
#define PILL_BREAK_MAX_STACK   3

#endif
