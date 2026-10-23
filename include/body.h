// body.h
// 体修体系常量（#79：COMBAT_SYSTEM §三 体修体系 / 1F §4.7 技能树）
//   四段路径（COMBAT_SYSTEM §三 表）：基础炼体（象甲功）→ 法体双修（梵圣真魔功）→
//     极限炼体（涅槃圣体）→ 毒体修炼（万毒混元身）
//   机制：淬体（cuina）积累「淬体点数」，点数换永久 max_qi / 减伤 / 空手伤害；
//     阶段越高点数上限越高，形成「肉身成长曲线」。
//   数据落点（全部由 BODY_D 维护，读方直接读 dbase，避免战斗热路径调 daemon）：
//     body_refine/points   淬体点数
//     body_refine/stage    体修阶段 0-4（见 BODY_STAGE_*）
//     body_refine/qi       永久 max_qi 加成（feature/attribute.c query_max_qi 消费）
//     body_refine/defense  永久固定减伤（include/combat/damage.h defatk 消费）
//     body_refine/fist     空手/重剑体修加成（damage.h 消费：空手全额、重剑半额）

#ifndef __BODY_H__
#define __BODY_H__

// ===== 体修阶段（COMBAT_SYSTEM §三）=====
#define BODY_STAGE_NONE       0   // 未淬体
#define BODY_STAGE_BASIC      1   // 基础炼体（象甲功）
#define BODY_STAGE_DUAL       2   // 法体双修（梵圣真魔功）
#define BODY_STAGE_ULTIMATE   3   // 极限炼体（涅槃圣体）
#define BODY_STAGE_POISON     4   // 毒体修炼（万毒混元身）

// ===== 阶段门槛 =====
// 每段要求上一段功法达到 BODY_PREREQ_SKILL 级（「先炼体、再法体双修」的梯度保证）
#define BODY_PREREQ_SKILL     50
// 四段功法（skill id）
#define BODY_SKILL_BASIC      "xiangjia-gong"          // 象甲功
#define BODY_SKILL_DUAL       "fansheng-zhenmo-gong"   // 梵圣真魔功
#define BODY_SKILL_ULTIMATE   "niepan-shengti"         // 涅槃圣体
#define BODY_SKILL_POISON     "wandu-hunyuan-shen"     // 万毒混元身

// ===== 淬体点数上限（按阶段）=====
#define BODY_CAP_BASIC        200
#define BODY_CAP_DUAL         450
#define BODY_CAP_ULTIMATE     800
#define BODY_CAP_POISON       1200

// ===== 点数换算（每点收益）=====
#define BODY_QI_PER_POINT     3    // 每点 +3 max_qi
#define BODY_DEF_DIVISOR      3    // 每 3 点 +1 固定减伤
#define BODY_FIST_DIVISOR     3    // 每 3 点 +1 空手伤害
#define BODY_SWORD_DIVISOR    6    // 重剑（体剑双修）每 6 点 +1 伤害（= 空手的一半）

// ===== 淬体（训练）消耗 =====
#define BODY_COST_NEILI       60
#define BODY_COST_JING        40
#define BODY_GAIN_BASE        1    // 每次淬体基础点数
#define BODY_GAIN_PER_LEVEL   20   // 每 20 级最高体修功法多 1 点
#define BODY_GAIN_MAX         8    // 单次最多 8 点
#define BODY_TRAIN_BUSY       2

// ===== 毒体（万毒混元身）=====
#define BODY_POISON_RESIST    30     // 毒体抗毒加成（叠加到 AntiPoisonRate）
#define BODY_POISON_ID        "wudu_poison"   // 空手出手自带之毒（feature/poison.c 既有毒种）
#define BODY_POISON_DURATION  10     // 带毒发作时长（empoison 内部按 PoisonDurLit=10 钳制）

// ===== 三头六臂法相（梵圣真魔功法体双修神通）=====
#define BODY_FAXIANG_BUFF     "active_buffer/fansheng-zhenmo-gong.santou_liubi"
#define BODY_FAXIANG_NEILI    200   // 法相内力消耗
#define BODY_FAXIANG_ATTACK   30    // apply/attack 加成
#define BODY_FAXIANG_DAMAGE   40    // apply/damage 加成
#define BODY_FAXIANG_LEVEL    20    // 基础持续秒数
#define BODY_FAXIANG_PER_LEVEL 4    // 每 10 级 +4 秒
#define BODY_FAXIANG_MAX_TIME  60   // 最长持续秒数

#endif  // __BODY_H__
