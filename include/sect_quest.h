// sect_quest.h
// 宗门任务链与宗门事件 —— 常量与类型定义
// 设计来源: .knowledge/factions/sects/ 九宗档案「宗门事件与任务链」节
//           02-扩充内容/02-任务链与奖励曲线.md（活跃度梯度）
// 相关 daemon: adm/daemons/sect_quest_d.c（宗门任务/事件落地）
//               adm/daemons/quest_chain_d.c（任务链框架）
//               adm/daemons/sect_d.c（门派/贡献） adm/daemons/reputation_d.c（声望）

#ifndef __SECT_QUEST__
#define __SECT_QUEST__

// ─── 宗门任务奖励扩展键（quest_chain.h 已有 exp/coin/reputation/items） ───
// rewards["contribution"]: 门派贡献值，结算走 SECT_D->add_contribution
// rewards["skills"]: ({ "skill_id", ... })，功法发放：
//   本门功法（SECT_D->query_sect_skill_info 命中）→ 写 sect/learned 免贡献解锁
//   通用功法（kungfu/skill 下真实存在）→ player->set_skill(id, 1) 直接发放

// ─── 宗门事件触发条件键 ───
#define EV_COND_REALM_MIN      "realm_min"       // 最低境界索引（intp 且 >=0 为真下限；缺失或 <0 视为不设下限）
#define EV_COND_REALM_MAX      "realm_max"       // 最高境界索引（intp 且 >=0 为真上限；缺失或 <0 为不限，(N,-1)=「N 期以上不限」）
#define EV_COND_SECT           "sect"            // 限定宗门 ID（缺省=事件所属宗门）
#define EV_COND_REP_MIN        "rep_min"         // 本门声望值下限
#define EV_COND_CONTRIB_MIN    "contrib_min"     // 门派贡献下限
#define EV_COND_QUEST          "quest"           // 前置任务 id（须已完成）
#define EV_COND_MALE_ONLY      "male_only"       // 1=仅男弟子可触发

// ─── 宗门事件奖励键 ───
#define EV_REWARD_REP          "reputation"      // 本门声望值
#define EV_REWARD_CONTRIB      "contribution"    // 门派贡献
#define EV_REWARD_EXP          "exp"             // 修为经验（combat_exp）
#define EV_REWARD_ITEMS        "items"           // ({ 物品路径, ... })
#define EV_REWARD_SKILLS       "skills"          // ({ 功法 id, ... })

// ─── 玩家数据路径 ───
#define SECT_QUEST_TRIGGERED   "sect_quest/triggered"        // 已触发事件 ([ event_id: time ])
#define SECT_QUEST_LAST_DAY    "quest_chain/last_active_day" // 上次活跃日 key（活跃度梯度）

// ─── 活跃度梯度参数（对齐 quest_chain.h 连续奖励加成） ───
#define SECT_QUEST_STREAK_DAY_BONUS   0.05  // 连续活跃每日 +5%（对齐 DAILY_STREAK_BONUS）
#define SECT_QUEST_STREAK_CAP_DAYS    7     // 连续活跃封顶天数（对齐 DAILY_STREAK_MAX_DAYS）
#define SECT_QUEST_STREAK_CAP_BONUS   1.5   // 封顶后奖励 ×1.5（对齐 DAILY_STREAK_VIP_BONUS）

// ─── 任务目标区域语义（OBJ_REACH / OBJ_TALK 的 target） ───
// target 为房间路径（前缀或完整路径），report 时按 base_name(environment(player))
// 前缀匹配判定到达；OBJ_TALK 在宗门驻地区域内视为已接谈。

#endif // __SECT_QUEST__
