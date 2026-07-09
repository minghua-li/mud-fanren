// daily_task_d.c
// 日常任务守护进程 — 任务池管理、刷新逻辑、进度追踪、奖励发放
// Created for #36-A (#39) 日常任务系统

#include <ansi.h>
#include <daily_task.h>
#include <localtime.h>

inherit F_DBASE;

// ──────────────────────────────────────────────
// 全局任务模板池
// ──────────────────────────────────────────────

// 8 种日常任务模板，按类型分组
// 每个模板包含：id, name, type, quality, min_realm, max_realm,
//               objectives, rewards, desc, time_limit
nosave mapping task_templates = ([]);

// 境界标签名（用于显示）
nosave string *realm_names = ({
        "凡人", "炼气前期", "炼气中期", "炼气后期",
        "筑基前期", "筑基中期", "筑基后期",
        "结丹前期", "结丹中期", "结丹后期",
        "元婴前期", "元婴中期", "元婴后期",
        "化神期", "炼虚期", "合体期", "大乘期"
});

// 境界基准奖励值
nosave int *base_rewards = ({
        BASE_REWARD_MORTAL, BASE_REWARD_QI, BASE_REWARD_QI, BASE_REWARD_QI,
        BASE_REWARD_ZHU, BASE_REWARD_ZHU, BASE_REWARD_ZHU,
        BASE_REWARD_JIE, BASE_REWARD_JIE, BASE_REWARD_JIE,
        BASE_REWARD_YING, BASE_REWARD_YING, BASE_REWARD_YING,
        BASE_REWARD_HUA, BASE_REWARD_LIAN, BASE_REWARD_HE, BASE_REWARD_DA
});

// 每日任务上限
nosave int *daily_limits = ({
        DAILY_LIMIT_MORTAL, DAILY_LIMIT_QI, DAILY_LIMIT_QI, DAILY_LIMIT_QI,
        DAILY_LIMIT_ZHU, DAILY_LIMIT_ZHU, DAILY_LIMIT_ZHU,
        DAILY_LIMIT_JIE, DAILY_LIMIT_JIE, DAILY_LIMIT_JIE,
        DAILY_LIMIT_YING, DAILY_LIMIT_YING, DAILY_LIMIT_YING,
        DAILY_LIMIT_HIGH, DAILY_LIMIT_HIGH, DAILY_LIMIT_HIGH, DAILY_LIMIT_HIGH
});

// 稀有任务概率
nosave int *rare_chances = ({
        RARE_CHANCE_QI, RARE_CHANCE_QI, RARE_CHANCE_QI, RARE_CHANCE_QI,
        RARE_CHANCE_ZHU, RARE_CHANCE_ZHU, RARE_CHANCE_ZHU,
        RARE_CHANCE_JIE, RARE_CHANCE_JIE, RARE_CHANCE_JIE,
        RARE_CHANCE_YING, RARE_CHANCE_YING, RARE_CHANCE_YING,
        RARE_CHANCE_HIGH, RARE_CHANCE_HIGH, RARE_CHANCE_HIGH, RARE_CHANCE_HIGH
});

// ──────────────────────────────────────────────
// 初始化
// ──────────────────────────────────────────────

void create()
{
        seteuid(getuid());
        set("channel_id", "日常任务精灵");
        set("name", "日常任务系统");

        // 初始化任务模板池
        init_task_templates();
        init_weekly_templates();

        // 计算到下次 0 点的时间，注册每日重置 call_out
        schedule_daily_reset();
        schedule_weekly_reset();

        CHANNEL_D->do_channel(this_object(), "sys",
                "日常/周常任务系统已经启动。\n");
}

// ──────────────────────────────────────────────
// 任务模板定义
// ──────────────────────────────────────────────

void init_task_templates()
{
        // ---- 1. 杀怪任务 ----
        task_templates["kill_daily_1"] = ([
                "id":         "kill_daily_1",
                "name":       "猎杀妖兽",
                "type":       TASK_KILL,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_JIE_HIGH,
                "desc":       "在野外区域猎杀指定数量的妖兽，获取妖兽材料。",
                "time_limit": 7200,
                "objectives": ([
                        "target": "妖兽",
                        "amount": 10
                ]),
                "rewards":    ([
                        "exp_ratio":  100,
                        "coin_ratio": 100
                ])
        ]);

        task_templates["kill_daily_2"] = ([
                "id":         "kill_daily_2",
                "name":       "清剿妖兽",
                "type":       TASK_KILL,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_YING_HIGH,
                "desc":       "清剿特定区域的强力妖兽，维护一方安宁。",
                "time_limit": 10800,
                "objectives": ([
                        "target": "精英妖兽",
                        "amount": 5
                ]),
                "rewards":    ([
                        "exp_ratio":  150,
                        "coin_ratio": 150
                ])
        ]);

        // ---- 2. 采集任务 ----
        task_templates["collect_daily_1"] = ([
                "id":         "collect_daily_1",
                "name":       "采集灵药",
                "type":       TASK_COLLECT,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_JIE_HIGH,
                "desc":       "在指定野外区域采集灵药，供炼丹使用。",
                "time_limit": 7200,
                "objectives": ([
                        "target": "灵药",
                        "amount": 15
                ]),
                "rewards":    ([
                        "exp_ratio":  100,
                        "coin_ratio": 80
                ])
        ]);

        task_templates["collect_daily_2"] = ([
                "id":         "collect_daily_2",
                "name":       "采集矿石",
                "type":       TASK_COLLECT,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_YING_HIGH,
                "desc":       "在矿脉中采集指定数量的矿石，供炼器使用。",
                "time_limit": 7200,
                "objectives": ([
                        "target": "矿石",
                        "amount": 10
                ]),
                "rewards":    ([
                        "exp_ratio":  120,
                        "coin_ratio": 150
                ])
        ]);

        // ---- 3. 拜访任务 ----
        task_templates["visit_daily_1"] = ([
                "id":         "visit_daily_1",
                "name":       "拜访前辈",
                "type":       TASK_VISIT,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_DA,
                "desc":       "前往指定地点拜访某位前辈高人，聆听教诲。",
                "time_limit": 3600,
                "objectives": ([
                        "target": "NPC",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  80,
                        "coin_ratio": 50
                ])
        ]);

        task_templates["visit_daily_2"] = ([
                "id":         "visit_daily_2",
                "name":       "巡查坊市",
                "type":       TASK_VISIT,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_JIE_HIGH,
                "desc":       "巡查指定坊市的经营情况，向商会汇报。",
                "time_limit": 5400,
                "objectives": ([
                        "target": "坊市",
                        "amount": 3
                ]),
                "rewards":    ([
                        "exp_ratio":  100,
                        "coin_ratio": 120
                ])
        ]);

        // ---- 4. 送信任务 ----
        task_templates["deliver_daily_1"] = ([
                "id":         "deliver_daily_1",
                "name":       "送信跑腿",
                "type":       TASK_DELIVER,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_JIE_HIGH,
                "desc":       "将信件或物品从一处送往另一处，交给指定 NPC。",
                "time_limit": 3600,
                "objectives": ([
                        "target": "信件",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  80,
                        "coin_ratio": 100
                ])
        ]);

        task_templates["deliver_daily_2"] = ([
                "id":         "deliver_daily_2",
                "name":       "紧急传讯",
                "type":       TASK_DELIVER,
                "quality":    QUALITY_RARE,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_YING_HIGH,
                "desc":       "紧急传讯！需要将重要情报快速送达多个地点。",
                "time_limit": 4800,
                "objectives": ([
                        "target": "情报",
                        "amount": 3
                ]),
                "rewards":    ([
                        "exp_ratio":  200,
                        "coin_ratio": 250
                ])
        ]);

        // ---- 5. 护送任务 ----
        task_templates["escort_daily_1"] = ([
                "id":         "escort_daily_1",
                "name":       "护送商队",
                "type":       TASK_ESCORT,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_QI_MID,
                "max_realm":  REALM_JIE_HIGH,
                "desc":       "护送一支商队安全通过危险区域，抵御沿途妖兽。",
                "time_limit": 7200,
                "objectives": ([
                        "target": "商队",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  150,
                        "coin_ratio": 200
                ])
        ]);

        task_templates["escort_daily_2"] = ([
                "id":         "escort_daily_2",
                "name":       "护送同门",
                "type":       TASK_ESCORT,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_ZHU_HIGH,
                "desc":       "护送一位同门师弟/师妹前往指定地点。",
                "time_limit": 3600,
                "objectives": ([
                        "target": "同门",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  100,
                        "coin_ratio": 80
                ])
        ]);

        // ---- 6. 捐献任务 ----
        task_templates["donate_daily_1"] = ([
                "id":         "donate_daily_1",
                "name":       "捐献灵石",
                "type":       TASK_DONATE,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_HUA,
                "desc":       "向所属势力捐献一定数量的灵石，获取贡献度。",
                "time_limit": 3600,
                "objectives": ([
                        "target": "灵石",
                        "amount": 500
                ]),
                "rewards":    ([
                        "exp_ratio":  50,
                        "coin_ratio": 0
                ])
        ]);

        task_templates["donate_daily_2"] = ([
                "id":         "donate_daily_2",
                "name":       "捐献物资",
                "type":       TASK_DONATE,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_YING_HIGH,
                "desc":       "向所属势力捐献指定物资，支援门派发展。",
                "time_limit": 7200,
                "objectives": ([
                        "target": "物资",
                        "amount": 3
                ]),
                "rewards":    ([
                        "exp_ratio":  80,
                        "coin_ratio": 50
                ])
        ]);

        // ---- 7. 修炼任务 ----
        task_templates["practice_daily_1"] = ([
                "id":         "practice_daily_1",
                "name":       "日常修炼",
                "type":       TASK_PRACTICE,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_DA,
                "desc":       "完成一定次数的修炼循环，提升修为。",
                "time_limit": 14400,
                "objectives": ([
                        "target": "修炼循环",
                        "amount": 10
                ]),
                "rewards":    ([
                        "exp_ratio":  150,
                        "coin_ratio": 30
                ])
        ]);

        task_templates["practice_daily_2"] = ([
                "id":         "practice_daily_2",
                "name":       "门派杂务",
                "type":       TASK_PRACTICE,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_ZHU_HIGH,
                "desc":       "协助门派完成日常杂务（打扫、巡逻、跑腿等）。",
                "time_limit": 3600,
                "objectives": ([
                        "target": "杂务",
                        "amount": 5
                ]),
                "rewards":    ([
                        "exp_ratio":  60,
                        "coin_ratio": 30
                ])
        ]);

        // ---- 8. 副本任务 ----
        task_templates["dungeon_daily_1"] = ([
                "id":         "dungeon_daily_1",
                "name":       "挑战副本",
                "type":       TASK_DUNGEON,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_YING_HIGH,
                "desc":       "完成指定副本或挑战，获取珍稀奖励。",
                "time_limit": 14400,
                "objectives": ([
                        "target": "副本",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  200,
                        "coin_ratio": 200
                ])
        ]);

        task_templates["dungeon_daily_2"] = ([
                "id":         "dungeon_daily_2",
                "name":       "秘境探索",
                "type":       TASK_DUNGEON,
                "quality":    QUALITY_RARE,
                "min_realm":  REALM_JIE_LOW,
                "max_realm":  REALM_HUA,
                "desc":       "探索一处秘境，揭开其中奥秘并取得宝物。",
                "time_limit": 21600,
                "objectives": ([
                        "target": "秘境",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  300,
                        "coin_ratio": 300
                ])
        ]);
}

// ──────────────────────────────────────────────
// 周常任务模板定义
// ──────────────────────────────────────────────

// 周常任务使用与日常任务相同的 daemon，但刷新周期为每周
// 周常模板存放在 weekly_task_templates 映射中

nosave mapping weekly_task_templates = ([]);

void init_weekly_templates()
{
        // ---- 1. 门派守卫战 ----
        weekly_task_templates["weekly_guard_1"] = ([
                "id":         "weekly_guard_1",
                "name":       "门派守卫战",
                "type":       TASK_KILL,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_YING_HIGH,
                "desc":       "参与每周一次的门派防御事件，抵御来犯之敌。",
                "time_limit": 86400,
                "objectives": ([
                        "target": "入侵敌人",
                        "amount": 20
                ]),
                "rewards":    ([
                        "exp_ratio":  500,
                        "coin_ratio": 400
                ])
        ]);

        // ---- 2. 势力任务周常 ----
        weekly_task_templates["weekly_faction_1"] = ([
                "id":         "weekly_faction_1",
                "name":       "势力任务周常",
                "type":       TASK_KILL,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_DA,
                "desc":       "完成所属势力发布的多个任务，提升势力声望。",
                "time_limit": 604800,
                "objectives": ([
                        "target": "势力任务",
                        "amount": 5
                ]),
                "rewards":    ([
                        "exp_ratio":  300,
                        "coin_ratio": 300
                ])
        ]);

        // ---- 3. 炼丹/炼器周常 ----
        weekly_task_templates["weekly_craft_1"] = ([
                "id":         "weekly_craft_1",
                "name":       "炼丹/炼器周常",
                "type":       TASK_DONATE,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_DA,
                "desc":       "提交指定数量和品质的丹药或法器，获得丰厚奖励。",
                "time_limit": 604800,
                "objectives": ([
                        "target": "成品",
                        "amount": 3
                ]),
                "rewards":    ([
                        "exp_ratio":  400,
                        "coin_ratio": 500
                ])
        ]);

        // ---- 4. 资金周常 ----
        weekly_task_templates["weekly_fund_1"] = ([
                "id":         "weekly_fund_1",
                "name":       "资金周常",
                "type":       TASK_DONATE,
                "quality":    QUALITY_NORMAL,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_DA,
                "desc":       "通过交易/跑商完成一定灵石流通量。",
                "time_limit": 604800,
                "objectives": ([
                        "target": "灵石流通",
                        "amount": 10000
                ]),
                "rewards":    ([
                        "exp_ratio":  200,
                        "coin_ratio": 600
                ])
        ]);

        // ---- 5. 战斗周常 ----
        weekly_task_templates["weekly_pvp_1"] = ([
                "id":         "weekly_pvp_1",
                "name":       "战斗周常",
                "type":       TASK_KILL,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_DA,
                "desc":       "在PVP/切磋中取得一定场次的胜利。",
                "time_limit": 604800,
                "objectives": ([
                        "target": "胜利",
                        "amount": 10
                ]),
                "rewards":    ([
                        "exp_ratio":  350,
                        "coin_ratio": 300
                ])
        ]);

        // ---- 6. 收集周常 ----
        weekly_task_templates["weekly_collect_1"] = ([
                "id":         "weekly_collect_1",
                "name":       "收集周常",
                "type":       TASK_COLLECT,
                "quality":    QUALITY_GOOD,
                "min_realm":  REALM_QI_LOW,
                "max_realm":  REALM_DA,
                "desc":       "提交指定种类的灵药/材料。",
                "time_limit": 604800,
                "objectives": ([
                        "target": "材料",
                        "amount": 20
                ]),
                "rewards":    ([
                        "exp_ratio":  250,
                        "coin_ratio": 350
                ])
        ]);

        // ---- 7. 秘境周常 ----
        weekly_task_templates["weekly_dungeon_1"] = ([
                "id":         "weekly_dungeon_1",
                "name":       "秘境周常",
                "type":       TASK_DUNGEON,
                "quality":    QUALITY_RARE,
                "min_realm":  REALM_ZHU_LOW,
                "max_realm":  REALM_DA,
                "desc":       "完成指定秘境副本，获取稀有奖励。",
                "time_limit": 604800,
                "objectives": ([
                        "target": "秘境",
                        "amount": 1
                ]),
                "rewards":    ([
                        "exp_ratio":  800,
                        "coin_ratio": 600
                ])
        ]);
}

// ═══════════════════════════════════════════
// 周常任务函数
// ═══════════════════════════════════════════

// 获取周常任务模板列表
mapping query_all_weekly_templates()
{
        return weekly_task_templates;
}

// 计算到下周一的秒数
int seconds_to_next_monday()
{
        mixed *tm;
        int days_till_monday;

        tm = localtime(time());
        // LT_WDAY: 0=周日, 1=周一, ..., 6=周六
        // 目标：下周一 0:00
        if (tm[LT_WDAY] == 0)
                days_till_monday = 1;  // 周日->周一
        else if (tm[LT_WDAY] == 1)
                days_till_monday = 7;  // 周一->下周一
        else
                days_till_monday = 8 - tm[LT_WDAY];

        return days_till_monday * 86400
               - tm[LT_HOUR] * 3600
               - tm[LT_MIN] * 60
               - tm[LT_SEC];
}

void schedule_weekly_reset()
{
        int delay;

        delay = seconds_to_next_monday();
        if (delay < 60)
                delay += 604800;

        remove_call_out("weekly_reset");
        call_out("weekly_reset", delay);
}

void weekly_reset()
{
        set("last_weekly_reset", get_today_date_weekly());

        log_file("daily_task", sprintf("[%s] 周常任务已重置。\n", ctime(time())));

        schedule_weekly_reset();
}

int get_today_date_weekly()
{
        mixed *tm;
        int year, week_num;

        tm = localtime(time());
        year = tm[LT_YEAR];
        // 粗略计算周数（ISO风格的简化版）
        week_num = (tm[LT_YDAY] / 7) + 1;
        return year * 100 + week_num;
}

// 获取玩家周常任务数据
mapping get_weekly_player_data(object player)
{
        mapping data;

        data = player->query("weekly_task");
        if (!mapp(data))
        {
                data = ([
                        "tasks":          ({}),
                        "reset_week":     0,
                        "completed_ids":  ({}),
                ]);
                player->set("weekly_task", data);
        }
        return data;
}

void save_weekly_player_data(object player, mapping data)
{
        player->set("weekly_task", data);
}

// 获取某境界可用的周常任务模板
string *get_available_weekly_templates(int realm_index)
{
        string *available;
        string id;

        available = ({});
        foreach (id, mapping tmpl in weekly_task_templates)
        {
                if (realm_index >= tmpl["min_realm"] &&
                    realm_index <= tmpl["max_realm"])
                {
                        available += ({ id });
                }
        }
        return available;
}

// 刷新玩家周常任务
int refresh_weekly_player_tasks(object player)
{
        mapping data;
        int week_num, realm;
        string *available, *selected, task_id;
        int max_weekly = 4;  // 每周4个周常

        data = get_weekly_player_data(player);
        week_num = get_today_date_weekly();

        if (data["reset_week"] == week_num)
                return 0;

        data["tasks"] = ({});
        data["completed_ids"] = ({});
        data["reset_week"] = week_num;

        realm = estimate_realm_index(player);
        available = get_available_weekly_templates(realm);

        // 随机选最多4个
        selected = select_random_tasks(available, max_weekly);

        foreach (task_id in selected)
        {
                mapping tmpl = weekly_task_templates[task_id];
                if (!mapp(tmpl)) continue;

                mapping task_entry = ([
                        "id":           task_id,
                        "name":         tmpl["name"],
                        "type":         tmpl["type"],
                        "quality":      tmpl["quality"],
                        "desc":         tmpl["desc"],
                        "time_limit":   tmpl["time_limit"],
                        "deadline":     time() + tmpl["time_limit"],
                        "progress":     ([
                                "current":  0,
                                "target":   tmpl["objectives"]["amount"]
                        ]),
                        "status":       TASK_STATUS_ACTIVE,
                        "reward_ratio": tmpl["rewards"],
                        "objective":    tmpl["objectives"]["target"]
                ]);

                data["tasks"] += ({ task_entry });
        }

        save_weekly_player_data(player, data);
        return 1;
}

// 查询玩家周常任务
mapping *query_weekly_player_tasks(object player)
{
        mapping data;

        refresh_weekly_player_tasks(player);
        data = get_weekly_player_data(player);
        return data["tasks"];
}

// ──────────────────────────────────────────────
// 每日重置调度
// ──────────────────────────────────────────────

// 计算到下次 0 点的秒数
int seconds_to_midnight()
{
        mixed *tm;

        tm = localtime(time());
        // 下一天 0 点 = 今天 0 点 + 86400
        // 今天 0 点 = time() - tm[LT_HOUR]*3600 - tm[LT_MIN]*60 - tm[LT_SEC]
        return 86400 - tm[LT_HOUR] * 3600 - tm[LT_MIN] * 60 - tm[LT_SEC];
}

void schedule_daily_reset()
{
        int delay;

        delay = seconds_to_midnight();
        // 如果不到 60 秒就重置（接近 0 点），延迟到明天
        if (delay < 60)
                delay += 86400;

        remove_call_out("daily_reset");
        call_out("daily_reset", delay);
}

// 每日重置——全局维护一个重置日期标记即可
// 各玩家的刷新在每次查询时按需触发
void daily_reset()
{
        // 更新全局重置日期
        set("last_reset_date", get_today_date());

        // 日志
        log_file("daily_task", sprintf("[%s] 日常任务已重置。\n", ctime(time())));

        // 调度下一次
        schedule_daily_reset();
}

// ──────────────────────────────────────────────
// 日期工具函数
// ──────────────────────────────────────────────

int get_today_date()
{
        mixed *tm;

        tm = localtime(time());
        return tm[LT_YEAR] * 10000 + (tm[LT_MON] + 1) * 100 + tm[LT_MDAY];
}

// ──────────────────────────────────────────────
// 境界相关工具函数
// ──────────────────────────────────────────────

// 根据 combat_exp 估算玩家境界索引
// 使用经验值区间映射到境界定义
int estimate_realm_index(object player)
{
        int exp;

        exp = player->query("combat_exp");
        if (exp < 1000)     return REALM_MORTAL;
        if (exp < 10000)    return REALM_QI_LOW;
        if (exp < 50000)    return REALM_QI_MID;
        if (exp < 200000)   return REALM_QI_HIGH;
        if (exp < 500000)   return REALM_ZHU_LOW;
        if (exp < 1000000)  return REALM_ZHU_MID;
        if (exp < 3000000)  return REALM_ZHU_HIGH;
        if (exp < 8000000)  return REALM_JIE_LOW;
        if (exp < 20000000) return REALM_JIE_MID;
        if (exp < 50000000) return REALM_JIE_HIGH;
        if (exp < 100000000) return REALM_YING_LOW;
        if (exp < 200000000) return REALM_YING_MID;
        if (exp < 500000000) return REALM_YING_HIGH;
        if (exp < 1000000000) return REALM_HUA;
        if (exp < 3000000000) return REALM_LIAN;
        if (exp < 8000000000) return REALM_HE;
        return REALM_DA;
}

// 获取境界名称
string get_realm_name(int realm_index)
{
        if (realm_index < 0 || realm_index > REALM_MAX)
                return "未知境界";
        return realm_names[realm_index];
}

// 获取境界基准奖励
int get_base_reward(int realm_index)
{
        if (realm_index < 0 || realm_index > REALM_MAX)
                return base_rewards[0];
        return base_rewards[realm_index];
}

// 获取玩家每日任务上限
int get_daily_limit(object player)
{
        int realm;

        realm = estimate_realm_index(player);
        if (realm < 0 || realm > REALM_MAX)
                return MIN_DAILY_TASKS;
        return daily_limits[realm];
}

// ──────────────────────────────────────────────
// 玩家任务数据访问
// ──────────────────────────────────────────────

// 获取玩家日常任务数据
mapping get_player_data(object player)
{
        mapping data;

        data = player->query(DAILY_DATA_PREFIX);
        if (!mapp(data))
        {
                data = ([
                        "tasks":          ({}),    // 当前已接取的日常任务列表
                        "reset_date":     0,       // 上次重置日期
                        "abandon_count":  0,       // 今日已放弃次数
                        "completed_ids":  ({}),    // 今日已完成的任务 ID 列表
                        "streak":         0,        // 连续完成日常天数
                        "last_streak_date": 0,      // 上次计入连击的日期
                ]);
                player->set(DAILY_DATA_PREFIX, data);
        }
        return data;
}

// 保存玩家任务数据
void save_player_data(object player, mapping data)
{
        player->set(DAILY_DATA_PREFIX, data);
}

// ──────────────────────────────────────────────
// 任务池管理
// ──────────────────────────────────────────────

// 获取某境界段可用的任务模板列表
string *get_available_templates(int realm_index)
{
        string *available;
        string id;

        available = ({});
        foreach (id, mapping tmpl in task_templates)
        {
                if (realm_index >= tmpl["min_realm"] &&
                    realm_index <= tmpl["max_realm"])
                {
                        available += ({ id });
                }
        }
        return available;
}

// 从可用的任务模板中随机选择 count 个
// 保证不同类型的多样性
string *select_random_tasks(string *available, int count)
{
        int i, j, len;
        string *selected, *pool, id;
        mapping type_counts;

        if (sizeof(available) <= count)
                return available;

        selected = ({});
        type_counts = ([]);

        // 先挑不同类型的，确保多样性
        pool = copy(available);

        while (sizeof(selected) < count && sizeof(pool) > 0)
        {
                // 打乱 pool
                for (i = 0; i < sizeof(pool); i++)
                {
                        j = random(sizeof(pool));
                        if (i != j)
                        {
                                id = pool[i];
                                pool[i] = pool[j];
                                pool[j] = id;
                        }
                }

                // 尽量选不同类型
                for (i = 0; i < sizeof(pool); i++)
                {
                        id = pool[i];
                        if (sizeof(selected) >= count)
                                break;

                        // 同类型不超过 2 个
                        if (type_counts[task_templates[id]["type"]] >= 2)
                                continue;

                        selected += ({ id });
                        type_counts[task_templates[id]["type"]] =
                                type_counts[task_templates[id]["type"]] + 1;
                        pool[i] = pool[sizeof(pool) - 1];
                        pool = pool[0..sizeof(pool) - 2];
                        i--;
                }
        }

        return selected;
}

// 根据玩家境界决定任务品质
int determine_quality(object player)
{
        int realm, chance, roll;

        realm = estimate_realm_index(player);
        if (realm < 0 || realm > REALM_MAX)
                chance = RARE_CHANCE_QI;
        else
                chance = rare_chances[realm];

        roll = random(100);

        // 稀有 3.0x：概率 = rare_chance * 0.3
        if (roll < chance * 30 / 100)
                return QUALITY_RARE;

        // 优秀 1.5x：概率 = rare_chance * 0.5
        if (roll < chance * 80 / 100)
                return QUALITY_GOOD;

        return QUALITY_NORMAL;
}

// ──────────────────────────────────────────────
// 核心逻辑：刷新玩家日常任务
// ──────────────────────────────────────────────

// 检查玩家是否需要每日刷新，并执行
// 返回 1 表示已刷新（或首次生成），0 表示无需刷新
int refresh_player_tasks(object player)
{
        mapping data;
        int today, realm, task_count;
        string *available, *selected, task_id;
        mapping task_entry;

        data = get_player_data(player);
        today = get_today_date();

        // 已经刷新过今天的
        if (data["reset_date"] == today)
                return 0;

        // 检查连击状态
        // 如果上次连击日期不是昨天，重置连击
        if (data["last_streak_date"] > 0 &&
            data["last_streak_date"] < today - 1)
        {
                data["streak"] = 0;
        }

        // 重置每日状态
        data["tasks"] = ({});
        data["abandon_count"] = 0;
        data["completed_ids"] = ({});
        data["reset_date"] = today;

        // 获取该玩家境界可用的任务池
        realm = estimate_realm_index(player);
        task_count = get_daily_limit(player);
        available = get_available_templates(realm);

        // 随机筛选
        selected = select_random_tasks(available, task_count);

        // 为每个选中的模板创建玩家任务条目
        foreach (task_id in selected)
        {
                mapping tmpl;
                int quality;

                tmpl = task_templates[task_id];
                if (!mapp(tmpl))
                        continue;

                // 确定品质
                quality = determine_quality(player);

                task_entry = ([
                        "id":           task_id,
                        "name":         tmpl["name"],
                        "type":         tmpl["type"],
                        "quality":      quality,
                        "desc":         tmpl["desc"],
                        "time_limit":   tmpl["time_limit"],
                        "deadline":     time() + tmpl["time_limit"],
                        "progress":     ([
                                "current":  0,
                                "target":   tmpl["objectives"]["amount"]
                        ]),
                        "status":       TASK_STATUS_ACTIVE,
                        "reward_ratio": tmpl["rewards"],
                        "objective":    tmpl["objectives"]["target"]
                ]);

                data["tasks"] += ({ task_entry });
        }

        save_player_data(player, data);
        return 1;
}

// ──────────────────────────────────────────────
// 玩家当前任务查询
// ──────────────────────────────────────────────

// 获取玩家当前活跃的任务列表
// 在查询时自动触发每日刷新
mapping *query_player_tasks(object player)
{
        mapping data;

        // 先刷新（如果未刷新的话）
        refresh_player_tasks(player);

        data = get_player_data(player);
        return data["tasks"];
}

// ──────────────────────────────────────────────
// 进度追踪
// ──────────────────────────────────────────────

// 更新玩家某一任务的进度
// type_hint: 任务类型提示（TASK_KILL 等）
// target_hint: 目标名称提示（可选）
// amount: 本次增加量
// 返回 1 表示有任务完成，0 表示无变化
int update_progress(object player, int type_hint, string target_hint, int amount)
{
        mapping data;
        mapping *tasks;
        int i, completed;
        string *new_completed;

        data = get_player_data(player);
        tasks = data["tasks"];
        completed = 0;

        for (i = 0; i < sizeof(tasks); i++)
        {
                if (tasks[i]["status"] != TASK_STATUS_ACTIVE)
                        continue;

                // 如果任务已完成，跳过
                if (tasks[i]["progress"]["current"] >= tasks[i]["progress"]["target"])
                        continue;

                // 类型匹配（且目标匹配或目标为空）
                if (tasks[i]["type"] != type_hint)
                        continue;

                if (target_hint && tasks[i]["objective"] != target_hint)
                        continue;

                // 更新进度
                tasks[i]["progress"]["current"] += amount;

                // 检查是否完成
                if (tasks[i]["progress"]["current"] >= tasks[i]["progress"]["target"])
                {
                        tasks[i]["progress"]["current"] = tasks[i]["progress"]["target"];
                        tasks[i]["status"] = TASK_STATUS_COMPLETED;
                        completed = 1;
                }
        }

        data["tasks"] = tasks;

        // 如果有完成的，加入 completed_ids
        if (completed)
        {
                new_completed = data["completed_ids"];
                for (i = 0; i < sizeof(tasks); i++)
                {
                        if (tasks[i]["status"] == TASK_STATUS_COMPLETED &&
                            member_array(tasks[i]["id"], new_completed) == -1)
                        {
                                new_completed += ({ tasks[i]["id"] });
                        }
                }
                data["completed_ids"] = new_completed;
        }

        save_player_data(player, data);
        return completed;
}

// 快捷函数：杀怪进度更新
// 由战斗系统在击杀怪物时调用
void on_kill_monster(object player, string monster_name)
{
        update_progress(player, TASK_KILL, "妖兽", 1);
        update_progress(player, TASK_KILL, "精英妖兽", 1);
}

// 快捷函数：采集进度更新
void on_collect_item(object player, string item_name)
{
        update_progress(player, TASK_COLLECT, "灵药", 1);
        update_progress(player, TASK_COLLECT, "矿石", 1);
}

// 快捷函数：拜访进度更新
void on_visit_npc(object player, string npc_name)
{
        update_progress(player, TASK_VISIT, "NPC", 1);
}

// 快捷函数：修炼循环
void on_practice_cycle(object player)
{
        update_progress(player, TASK_PRACTICE, "修炼循环", 1);
        update_progress(player, TASK_PRACTICE, "杂务", 1);
}

// ──────────────────────────────────────────────
// 任务完成与奖励
// ──────────────────────────────────────────────

// 获取连击加成百分比
// streak: 连续天数
// 返回：百分比值（如 25 表示 +25%）
int get_streak_bonus(int streak)
{
        int bonus;

        bonus = streak * STREAK_BONUS_PER_DAY;
        if (bonus > MAX_STREAK_BONUS)
                bonus = MAX_STREAK_BONUS;
        return bonus;
}

// 获取境界加成百分比
// 境界比任务建议境界高时奖励衰减，低时也可做但奖励打折
// 当前境界与任务目标境界的差距
int get_realm_bonus(int player_realm, int task_min_realm)
{
        int diff;

        diff = player_realm - task_min_realm;

        // 境界越高，每层的境界加成递增
        if (diff >= 0)
                return diff * REALM_BONUS_STEP;

        // 低于建议境界：衰减（不鼓励越级）
        // 但保持最低 30%
        return diff * REALM_BONUS_STEP;  // 负数
}

// 计算任务奖励
// 返回：([ "exp": N, "coin": N ])
// 使用全整数运算，避免 LPC 浮点数问题
mapping calc_reward(object player, mapping task_entry)
{
        int realm, base_reward, quality_mult, streak;
        int realm_bonus, streak_bonus, total_mult;
        int exp_reward, coin_reward;
        mapping reward_ratio;

        realm = estimate_realm_index(player);
        base_reward = get_base_reward(realm);

        // 品质倍率
        switch (task_entry["quality"])
        {
        case QUALITY_RARE:
                quality_mult = 3;
                break;
        case QUALITY_GOOD:
                quality_mult = 2;
                break;
        default:
                quality_mult = 1;
        }

        // 连击加成
        streak = query_streak(player);
        streak_bonus = get_streak_bonus(streak);

        // 境界加成
        realm_bonus = get_realm_bonus(realm,
                task_templates[task_entry["id"]]["min_realm"]);

        // 最终倍率（百分制）
        // reward = base * quality * (100 + realm_bonus + streak_bonus) / 100
        total_mult = 100 + realm_bonus + streak_bonus;

        // 确保不低于 30%
        if (total_mult < 30)
                total_mult = 30;

        // 任务有自己的 exp_ratio 和 coin_ratio（百分制）
        reward_ratio = task_entry["reward_ratio"];

        // exp_reward = base_reward * quality * total_mult/100 * exp_ratio/100
        // = base_reward * quality * total_mult * exp_ratio / 10000
        exp_reward = base_reward * quality_mult * total_mult *
                     reward_ratio["exp_ratio"] / 10000;

        // coin_reward = base_reward/5 * quality * total_mult/100 * coin_ratio/100
        // = base_reward * quality * total_mult * coin_ratio / 500000
        coin_reward = base_reward * quality_mult * total_mult *
                      reward_ratio["coin_ratio"] / 500000;

        // 最低奖励保障
        if (exp_reward < 1)  exp_reward = 1;
        if (coin_reward < 1) coin_reward = 1;

        return ([
                "exp":  exp_reward,
                "coin": coin_reward
        ]);
}

// 提交完成指定的任务
// task_index: 玩家任务列表中的索引（0-based）
// 返回：1=成功，-1=任务不存在，-2=未完成，-3=已领取
int submit_task(object player, int task_index)
{
        mapping data, *tasks, reward;
        string task_id;
        int streak_updated;

        data = get_player_data(player);
        tasks = data["tasks"];

        if (task_index < 0 || task_index >= sizeof(tasks))
                return -1;

        if (tasks[task_index]["status"] != TASK_STATUS_COMPLETED)
                return -2;

        task_id = tasks[task_index]["id"];

        // 检查是否已领取过奖励
        if (member_array(task_id, data["completed_ids"]) == -1)
        {
                // 已完成但未加入 completed_ids（异常）
                data["completed_ids"] += ({ task_id });
        }

        // 计算奖励
        reward = calc_reward(player, tasks[task_index]);

        // 发放奖励
        player->add("combat_exp", reward["exp"]);
        // 灵石通过 moneyd 发放或直接记录
        // 假设玩家有 deposit 余额或通过 other_coin 记录
        // 也可以直接用玩家身上的金币
        MONEY_D->pay_player(player, reward["coin"]);

        // 更新连击
        streak_updated = update_streak(player);

        // 从活跃列表中移除（标记为已领取）
        tasks[task_index]["status"] = TASK_STATUS_FAILED;  // 不再可用
        data["tasks"] = tasks;
        save_player_data(player, data);

        // 发送消息
        tell_object(player, sprintf(
                HIG "你完成了日常任务「%s」！\n" NOR
                "获得经验：%d\n"
                "获得灵石：%d\n",
                tasks[task_index]["name"],
                reward["exp"],
                reward["coin"]));

        return 1;
}

// ──────────────────────────────────────────────
// 放弃任务
// ──────────────────────────────────────────────

// 放弃指定的任务
// task_index: 玩家任务列表索引
// 返回：1=成功，-1=任务不存在，-2=已达今日放弃上限，-3=冷却中
int abandon_task(object player, int task_index)
{
        mapping data, *tasks, now;

        data = get_player_data(player);
        tasks = data["tasks"];

        if (task_index < 0 || task_index >= sizeof(tasks))
                return -1;

        if (tasks[task_index]["status"] != TASK_STATUS_ACTIVE)
                return -1;

        // 检查放弃次数
        if (data["abandon_count"] >= MAX_ABANDON_PER_DAY)
        {
                tell_object(player, "你今天已经放弃了太多任务，无法继续放弃。\n");
                return -2;
        }

        // 检查冷却（如果设置了放弃时间）
        if (data["last_abandon_time"] &&
            time() - data["last_abandon_time"] < ABANDON_CD)
        {
                int remain;

                remain = ABANDON_CD - (time() - data["last_abandon_time"]);
                tell_object(player, sprintf(
                        "你刚刚放弃过任务，请等待 %d 秒后再试。\n", remain));
                return -3;
        }

        // 标记任务为放弃
        tasks[task_index]["status"] = TASK_STATUS_ABANDONED;
        data["tasks"] = tasks;
        data["abandon_count"] = data["abandon_count"] + 1;
        data["last_abandon_time"] = time();

        save_player_data(player, data);

        tell_object(player, sprintf(
                "你放弃了日常任务「%s」。\n" HIY "今日还可放弃 %d 次。\n" NOR,
                tasks[task_index]["name"],
                MAX_ABANDON_PER_DAY - data["abandon_count"]));

        return 1;
}

// ──────────────────────────────────────────────
// 连击管理
// ──────────────────────────────────────────────

// 获取玩家当前连击天数
int query_streak(object player)
{
        mapping data;

        data = get_player_data(player);
        return data["streak"];
}

// 更新连击（完成一个任务后调用）
int update_streak(object player)
{
        mapping data;
        int today;

        data = get_player_data(player);
        today = get_today_date();

        // 如果今天还没更新过连击
        if (data["last_streak_date"] < today)
        {
                // 如果是连续（昨天有完成）
                if (data["last_streak_date"] == today - 1 ||
                    data["streak"] == 0)
                {
                        data["streak"] = data["streak"] + 1;
                }
                else
                {
                        // 不连续，重置
                        data["streak"] = 1;
                }
                data["last_streak_date"] = today;
        }

        save_player_data(player, data);
        return data["streak"];
}

// ──────────────────────────────────────────────
// 调试与信息函数
// ──────────────────────────────────────────────

// 获取所有任务模板列表
mapping query_all_templates()
{
        return task_templates;
}

// 获取某个任务模板
mapping query_template(string id)
{
        if (!mapp(task_templates[id]))
                return 0;
        return copy(task_templates[id]);
}

// 查询玩家日常任务详细状态
string query_player_status(object player)
{
        mapping data;
        string msg;

        data = get_player_data(player);

        msg = sprintf(
                "日常任务状态：\n"
                "  重置日期：%d\n"
                "  今日已接：%d 个\n"
                "  今日已完成：%d 个\n"
                "  今日放弃：%d 次（上限 %d 次）\n"
                "  连击天数：%d 天\n",
                data["reset_date"],
                sizeof(data["tasks"]),
                sizeof(data["completed_ids"]),
                data["abandon_count"],
                MAX_ABANDON_PER_DAY,
                data["streak"]);

        return msg;
}

// ──────────────────────────────────────────────
// 全局定时重置（守护进程自动执行）
// ──────────────────────────────────────────────

protected void reset_daemon()
{
        // 每日重置由 schedule_daily_reset / daily_reset 处理
}

// ──────────────────────────────────────────────
// 品质对应的颜色与名称
// ──────────────────────────────────────────────

string quality_name(int quality)
{
        switch (quality)
        {
        case QUALITY_NORMAL:
                return HIG "普通" NOR;
        case QUALITY_GOOD:
                return HIB "优秀" NOR;
        case QUALITY_RARE:
                return HIM "稀有" NOR;
        default:
                return "未知";
        }
}

string quality_color(int quality)
{
        switch (quality)
        {
        case QUALITY_NORMAL:
                return HIG;
        case QUALITY_GOOD:
                return HIB;
        case QUALITY_RARE:
                return HIM;
        default:
                return NOR;
        }
}
