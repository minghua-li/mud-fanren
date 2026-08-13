// sect_quest_d.c
// 宗门任务链与宗门事件守护进程 —— 九宗档案「宗门事件与任务链」节落地
//
// 职责：
//   1. 注册九宗任务链（每宗 3 个串行任务）到 QUEST_CHAIN_D（任务链框架）
//   2. 维护九宗宗门事件（每宗 3 个，触发境界/条件与档案一致）
//   3. 提供接取/交任务/触发事件的宗门侧接口（#58 场景挂接 + #57 宗门对象）
//
// 奖励接入（c4）：
//   声望     → REPUTATION_D->add_reputation（含正魔互斥）
//   门派贡献 → SECT_D->add_contribution
//   修为     → combat_exp（境界缩放 + 活跃度梯度加成）
//   物品     → new() + move(player)（真实物品路径）
//   功法     → 本门功法写 sect/learned 免贡献；通用功法 set_skill(id,1)
//
// 设计来源: .knowledge/factions/sects/ 九宗档案「宗门事件与任务链」节
//           02-扩充内容/02-任务链与奖励曲线.md（活跃度梯度）
// 依赖: QUEST_CHAIN_D / SECT_D / REPUTATION_D / MONEY_D

#include <ansi.h>
#include <quest_chain.h>
#include <sect_quest.h>
#include <sect.h>

inherit F_DBASE;

// ─── 宗门驻地目录（#58 场景挂接点：越国七派 + 天罗魔道两宗） ───
nosave mapping sect_areas = ([
    "yanyue_sect"     : "/d/yueguo/yanyue",
    "huangfeng_valley": "/d/yueguo/huangfeng",
    "lingshou_mountain": "/d/yueguo/lingshou",
    "qingxu_sect"     : "/d/yueguo/qingxu",
    "huadao_dock"     : "/d/yueguo/huadao",
    "tianque_fort"    : "/d/yueguo/tianque",
    "jujian_gate"     : "/d/yueguo/jujian",
    "guiling_sect"    : "/d/tianluo/guiling",
    "yuling_sect"     : "/d/tianluo/yuling",
]);

// ─── 宗门任务链定义（九宗 × 3 串行，取材档案事件表） ───
// 字段对齐 quest_chain_d.register_quest 模板格式，另加 "sect" 归属键
nosave mapping quest_defs = ([
    // ── 掩月宗：双修结道侣线 ──
    "yanyue_quest_1": ([
        "id": "yanyue_quest_1", "sect": "yanyue_sect", "chain_id": "chain_yanyue",
        "name": "入门考核", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/yanyue/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "yanyue_sect", "value": 50 ]) }), "contribution": 200 ]),
        "description": "掩月宗门规最严，入门先过灵根与心性考核：前往掩月宗大殿受考。",
    ]),
    "yanyue_quest_2": ([
        "id": "yanyue_quest_2", "sect": "yanyue_sect", "chain_id": "chain_yanyue",
        "name": "血禁试炼", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 1 }),
        "prerequisites": ([ "quests": ({ "yanyue_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/yanyue/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 100, "reputation": ({ ([ "faction": "yanyue_sect", "value": 100 ]) }), "contribution": 500, "items": ({ "/clone/drug/baicao-dan" }) ]),
        "description": "入血色禁地采药换筑基丹，归来后往传功房向长老复命，换取试炼凭证。",
    ]),
    "yanyue_quest_3": ([
        "id": "yanyue_quest_3", "sect": "yanyue_sect", "chain_id": "chain_yanyue",
        "name": "双修结道侣", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "yanyue_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/yanyue/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "yanyue_sect", "value": 200 ]) }), "contribution": 1000, "skills": ({ "shuangxiu-zhishu" }) ]),
        "description": "择道侣共修，阴阳相济加速精进。前往山门与宗门长老定下双修之约，习得双修之术。",
    ]),

    // ── 黄枫谷：药园栽培 → 血禁试炼 ──
    "huangfeng_quest_1": ([
        "id": "huangfeng_quest_1", "sect": "huangfeng_valley", "chain_id": "chain_huangfeng",
        "name": "药园栽培", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/yuexudian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "huangfeng_valley", "value": 50 ]) }), "contribution": 200 ]),
        "description": "黄枫谷以炼丹立派。前往月须殿领灵田种药，交药换取贡献。",
    ]),
    "huangfeng_quest_2": ([
        "id": "huangfeng_quest_2", "sect": "huangfeng_valley", "chain_id": "chain_huangfeng",
        "name": "岳麓殿守卫", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 1 }),
        "prerequisites": ([ "quests": ({ "huangfeng_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 100, "reputation": ({ ([ "faction": "huangfeng_valley", "value": 100 ]) }), "contribution": 500, "items": ({ "/clone/drug/lingzhi" }) ]),
        "description": "轮值守藏经阁，防御入侵。前往大殿领守卫职司，获赐灵芝灵药。",
    ]),
    "huangfeng_quest_3": ([
        "id": "huangfeng_quest_3", "sect": "huangfeng_valley", "chain_id": "chain_huangfeng",
        "name": "血禁试炼", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "huangfeng_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huangfeng/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "huangfeng_valley", "value": 200 ]) }), "contribution": 1000, "skills": ({ "qingyuan-jianjue" }) ]),
        "description": "入禁地采玉髓芝/紫猴花换筑基丹。归来回传功房，长老传你青元剑诀残本。",
    ]),

    // ── 灵兽山：驯兽 → 暗桩抉择 ──
    "lingshou_quest_1": ([
        "id": "lingshou_quest_1", "sect": "lingshou_mountain", "chain_id": "chain_lingshou",
        "name": "驯兽入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/lingshou/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "lingshou_mountain", "value": 50 ]) }), "contribution": 200 ]),
        "description": "灵兽山以驭兽立派。前往大殿驯养低阶灵虫灵兽，获赐兽宠与贡献。",
    ]),
    "lingshou_quest_2": ([
        "id": "lingshou_quest_2", "sect": "lingshou_mountain", "chain_id": "chain_lingshou",
        "name": "警戒巡逻", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "lingshou_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/lingshou/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 1500, "coin": 80, "reputation": ({ ([ "faction": "lingshou_mountain", "value": 100 ]) }), "contribution": 500 ]),
        "description": "维护警戒虫网，防御外敌。前往山门布防巡哨。",
    ]),
    "lingshou_quest_3": ([
        "id": "lingshou_quest_3", "sect": "lingshou_mountain", "chain_id": "chain_lingshou",
        "name": "暗桩抉择", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "lingshou_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/lingshou/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "lingshou_mountain", "value": 300 ]) }), "contribution": 1000, "skills": ({ "kuilei-shu" }) ]),
        "description": "宗门暗桩疑云丛生，长老召你往传功房剖明心迹。抉择已定，获授傀儡术以固本门。",
    ]),

    // ── 清虚门：论道 ──
    "qingxu_quest_1": ([
        "id": "qingxu_quest_1", "sect": "qingxu_sect", "chain_id": "chain_qingxu",
        "name": "清修入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qingxu/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "qingxu_sect", "value": 50 ]) }), "contribution": 200 ]),
        "description": "清虚门清净无为，道门基础修习。前往大殿听道长讲道。",
    ]),
    "qingxu_quest_2": ([
        "id": "qingxu_quest_2", "sect": "qingxu_sect", "chain_id": "chain_qingxu",
        "name": "论道大会", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 1 }),
        "prerequisites": ([ "quests": ({ "qingxu_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qingxu/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 100, "reputation": ({ ([ "faction": "qingxu_sect", "value": 150 ]) }), "contribution": 500 ]),
        "description": "与各派修士论道悟性。前往山门赴论道之约。",
    ]),
    "qingxu_quest_3": ([
        "id": "qingxu_quest_3", "sect": "qingxu_sect", "chain_id": "chain_qingxu",
        "name": "大道参悟", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "qingxu_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/qingxu/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "qingxu_sect", "value": 200 ]) }), "contribution": 1000, "skills": ({ "daomen-shufa" }) ]),
        "description": "论道胜出者入传功房参悟道门术法。",
    ]),

    // ── 化刀坞：刀意淬炼 ──
    "huadao_quest_1": ([
        "id": "huadao_quest_1", "sect": "huadao_dock", "chain_id": "chain_huadao",
        "name": "刀法入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huadao/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "huadao_dock", "value": 50 ]) }), "contribution": 200 ]),
        "description": "化刀坞刀修汇聚。前往大殿习基础刀法，淬炼刀意。",
    ]),
    "huadao_quest_2": ([
        "id": "huadao_quest_2", "sect": "huadao_dock", "chain_id": "chain_huadao",
        "name": "炼器坊劳作", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "huadao_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huadao/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 1500, "coin": 150, "reputation": ({ ([ "faction": "huadao_dock", "value": 100 ]) }), "contribution": 500 ]),
        "description": "参与炼器积累手艺。前往山门坊场出力，得灵石报酬。",
    ]),
    "huadao_quest_3": ([
        "id": "huadao_quest_3", "sect": "huadao_dock", "chain_id": "chain_huadao",
        "name": "刀剑之争", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "huadao_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/huadao/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "huadao_dock", "value": 200 ]) }), "contribution": 1000, "skills": ({ "daofa-chuancheng" }) ]),
        "description": "与巨剑门试剑争锋。传功房长老见你刀意有成，传你刀法传承。",
    ]),

    // ── 天阙堡：筑堡布阵 ──
    "tianque_quest_1": ([
        "id": "tianque_quest_1", "sect": "tianque_fort", "chain_id": "chain_tianque",
        "name": "筑堡入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/tianque/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "tianque_fort", "value": 50 ]) }), "contribution": 200 ]),
        "description": "天阙堡筑堡建州。前往大殿参与城防建造。",
    ]),
    "tianque_quest_2": ([
        "id": "tianque_quest_2", "sect": "tianque_fort", "chain_id": "chain_tianque",
        "name": "阵法修习", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 1 }),
        "prerequisites": ([ "quests": ({ "tianque_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/tianque/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 1500, "coin": 80, "reputation": ({ ([ "faction": "tianque_fort", "value": 100 ]) }), "contribution": 500 ]),
        "description": "护山大阵维护。前往山门布阵巡逻，积攒阵法经验。",
    ]),
    "tianque_quest_3": ([
        "id": "tianque_quest_3", "sect": "tianque_fort", "chain_id": "chain_tianque",
        "name": "要塞坚守", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "tianque_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/tianque/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "tianque_fort", "value": 200 ]) }), "contribution": 1000, "skills": ({ "zhenfa-shu" }) ]),
        "description": "正魔大战守城立下战功。传功房授你阵法术真传。",
    ]),

    // ── 巨剑门：试剑 ──
    "jujian_quest_1": ([
        "id": "jujian_quest_1", "sect": "jujian_gate", "chain_id": "chain_jujian",
        "name": "剑法入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/jujian/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "jujian_gate", "value": 50 ]) }), "contribution": 200 ]),
        "description": "巨剑门体剑双修。前往大殿习基础剑法。",
    ]),
    "jujian_quest_2": ([
        "id": "jujian_quest_2", "sect": "jujian_gate", "chain_id": "chain_jujian",
        "name": "试剑大会", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 1 }),
        "prerequisites": ([ "quests": ({ "jujian_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/jujian/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 100, "reputation": ({ ([ "faction": "jujian_gate", "value": 150 ]) }), "contribution": 500 ]),
        "description": "门内比剑试锋。前往山门演武场应战。",
    ]),
    "jujian_quest_3": ([
        "id": "jujian_quest_3", "sect": "jujian_gate", "chain_id": "chain_jujian",
        "name": "殿后血战", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "jujian_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/yueguo/jujian/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "jujian_gate", "value": 200 ]) }), "contribution": 1000, "skills": ({ "zhongjian-jianfa" }) ]),
        "description": "正魔大战巨剑门殿后，血战存身。传功房授你重剑剑法。",
    ]),

    // ── 鬼灵门：驱鬼炼尸 → 血灵双修 ──
    "guiling_quest_1": ([
        "id": "guiling_quest_1", "sect": "guiling_sect", "chain_id": "chain_guiling",
        "name": "驱鬼入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/tianluo/guiling/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "guiling_sect", "value": 50 ]) }), "contribution": 200 ]),
        "description": "鬼灵门驱鬼役妖。前往大殿受驭鬼基础之任。",
    ]),
    "guiling_quest_2": ([
        "id": "guiling_quest_2", "sect": "guiling_sect", "chain_id": "chain_guiling",
        "name": "燕家联姻", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 1 }),
        "prerequisites": ([ "quests": ({ "guiling_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/tianluo/guiling/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 2000, "coin": 100, "reputation": ({ ([ "faction": "guiling_sect", "value": 150 ]) }), "contribution": 500 ]),
        "description": "与燕家结姻亲、共修血灵大法。前往山门迎燕家使者。",
    ]),
    "guiling_quest_3": ([
        "id": "guiling_quest_3", "sect": "guiling_sect", "chain_id": "chain_guiling",
        "name": "血灵双修", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 1, 2 }),
        "prerequisites": ([ "quests": ({ "guiling_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/tianluo/guiling/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "guiling_sect", "value": 200 ]) }), "contribution": 1000, "skills": ({ "xueling-dafa" }) ]),
        "description": "传功房传《万灵真经》第一魔功血灵大法，从此双修精进。",
    ]),

    // ── 御灵宗：兽潮推进 ──
    "yuling_quest_1": ([
        "id": "yuling_quest_1", "sect": "yuling_sect", "chain_id": "chain_yuling",
        "name": "驯兽入门", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 0, 0 }),
        "prerequisites": ([ ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/tianluo/yuling/dadian", "amount": 1 ]) }),
        "rewards": ([ "exp": 500, "coin": 50, "reputation": ({ ([ "faction": "yuling_sect", "value": 50 ]) }), "contribution": 200 ]),
        "description": "御灵宗万灵归宗。前往大殿驯养低阶灵兽。",
    ]),
    "yuling_quest_2": ([
        "id": "yuling_quest_2", "sect": "yuling_sect", "chain_id": "chain_yuling",
        "name": "魔道争霸", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 2 }),
        "prerequisites": ([ "quests": ({ "yuling_quest_1" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/tianluo/yuling/shanmen", "amount": 1 ]) }),
        "rewards": ([ "exp": 3000, "coin": 150, "reputation": ({ ([ "faction": "yuling_sect", "value": 150 ]) }), "contribution": 600 ]),
        "description": "魔道六宗内部角逐。前往山门听候兽潮调令。",
    ]),
    "yuling_quest_3": ([
        "id": "yuling_quest_3", "sect": "yuling_sect", "chain_id": "chain_yuling",
        "name": "兽潮推进", "type": QUEST_TYPE_SIDE, "refresh": REFRESH_ONCE,
        "realm_range": ({ 2, 3 }),
        "prerequisites": ([ "quests": ({ "yuling_quest_2" }) ]),
        "objectives": ({ ([ "type": OBJ_REACH, "target": "/d/tianluo/yuling/chuangong", "amount": 1 ]) }),
        "rewards": ([ "exp": 5000, "coin": 200, "reputation": ({ ([ "faction": "yuling_sect", "value": 200 ]) }), "contribution": 1000, "skills": ({ "wangu-jue" }) ]),
        "description": "驱兽潮推进越国，立下赫赫战功。传功房授你万蛊诀。",
    ]),
]);

// ─── 宗门事件定义（九宗 × 3，触发境界/条件与档案一致） ───
// 档案「宗门事件与任务链」节：事件/触发/内容/奖励方向
nosave mapping event_defs = ([
    // ── 掩月宗 ──
    "yanyue_ev_join": ([
        "id": "yanyue_ev_join", "sect": "yanyue_sect",
        "name": "入门考核", "desc": "灵根测试 + 基础任务，掩月宗门规最严。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "yanyue_ev_xuejin": ([
        "id": "yanyue_ev_xuejin", "sect": "yanyue_sect",
        "name": "血禁试炼", "desc": "入血色禁地采药换筑基丹。",
        "conditions": ([ EV_COND_REALM_MIN: 1, EV_COND_REALM_MAX: 1 ]),
        "rewards": ([ EV_REWARD_EXP: 1000, EV_REWARD_REP: 100, EV_REWARD_CONTRIB: 300, EV_REWARD_ITEMS: ({ "/clone/drug/baicao-dan" }) ]),
    ]),
    "yanyue_ev_war": ([
        "id": "yanyue_ev_war", "sect": "yanyue_sect",
        "name": "正魔大战", "desc": "掩月宗殿后/决策，立战功。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 黄枫谷 ──
    "huangfeng_ev_yaoyuan": ([
        "id": "huangfeng_ev_yaoyuan", "sect": "huangfeng_valley",
        "name": "药园栽培", "desc": "灵田种药、交药换贡献。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "huangfeng_ev_shouwei": ([
        "id": "huangfeng_ev_shouwei", "sect": "huangfeng_valley",
        "name": "岳麓殿守卫", "desc": "轮值守藏经阁，防御入侵。",
        "conditions": ([ EV_COND_REALM_MIN: 1, EV_COND_REALM_MAX: 1 ]),
        "rewards": ([ EV_REWARD_EXP: 1000, EV_REWARD_REP: 100, EV_REWARD_CONTRIB: 300, EV_REWARD_ITEMS: ({ "/clone/drug/lingzhi" }) ]),
    ]),
    "huangfeng_ev_waimai": ([
        "id": "huangfeng_ev_waimai", "sect": "huangfeng_valley",
        "name": "丹药外销", "desc": "炼丹供给坊市，打通商路。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 灵兽山 ──
    "lingshou_ev_xunshou": ([
        "id": "lingshou_ev_xunshou", "sect": "lingshou_mountain",
        "name": "驯兽入门", "desc": "驯养低阶灵虫/灵兽，获兽宠。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "lingshou_ev_xunluo": ([
        "id": "lingshou_ev_xunluo", "sect": "lingshou_mountain",
        "name": "警戒巡逻", "desc": "维护警戒虫网，防御外敌。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 1 ]),
        "rewards": ([ EV_REWARD_EXP: 800, EV_REWARD_REP: 100, EV_REWARD_CONTRIB: 300 ]),
    ]),
    "lingshou_ev_anzhuang": ([
        "id": "lingshou_ev_anzhuang", "sect": "lingshou_mountain",
        "name": "暗桩抉择", "desc": "是否回归御灵宗——阵营分支，声望巨变。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 清虚门 ──
    "qingxu_ev_xiuxing": ([
        "id": "qingxu_ev_xiuxing", "sect": "qingxu_sect",
        "name": "清修入门", "desc": "道门基础修习。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "qingxu_ev_lundao": ([
        "id": "qingxu_ev_lundao", "sect": "qingxu_sect",
        "name": "论道大会", "desc": "与各派修士论道，悟性与声望齐增。",
        "conditions": ([ EV_COND_REALM_MIN: 1, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 1000, EV_REWARD_REP: 150, EV_REWARD_CONTRIB: 300 ]),
    ]),
    "qingxu_ev_war": ([
        "id": "qingxu_ev_war", "sect": "qingxu_sect",
        "name": "正魔大战", "desc": "智囊侦查、退守北凉。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 化刀坞 ──
    "huadao_ev_daofa": ([
        "id": "huadao_ev_daofa", "sect": "huadao_dock",
        "name": "刀法入门", "desc": "基础刀法修习。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "huadao_ev_lianqi": ([
        "id": "huadao_ev_lianqi", "sect": "huadao_dock",
        "name": "炼器坊劳作", "desc": "参与炼器，积累手艺。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 1 ]),
        "rewards": ([ EV_REWARD_EXP: 800, EV_REWARD_REP: 100, EV_REWARD_CONTRIB: 300 ]),
    ]),
    "huadao_ev_daojian": ([
        "id": "huadao_ev_daojian", "sect": "huadao_dock",
        "name": "刀剑之争", "desc": "与巨剑门试剑，战功声望。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 天阙堡 ──
    "tianque_ev_zhubao": ([
        "id": "tianque_ev_zhubao", "sect": "tianque_fort",
        "name": "筑堡入门", "desc": "参与城防/建筑。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "tianque_ev_zhenfa": ([
        "id": "tianque_ev_zhenfa", "sect": "tianque_fort",
        "name": "阵法修习", "desc": "布阵、护山大阵维护。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 1 ]),
        "rewards": ([ EV_REWARD_EXP: 800, EV_REWARD_REP: 100, EV_REWARD_CONTRIB: 300 ]),
    ]),
    "tianque_ev_shoucheng": ([
        "id": "tianque_ev_shoucheng", "sect": "tianque_fort",
        "name": "要塞坚守", "desc": "正魔大战守城，战功声望。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 巨剑门 ──
    "jujian_ev_jianfa": ([
        "id": "jujian_ev_jianfa", "sect": "jujian_gate",
        "name": "剑法入门", "desc": "基础剑法修习。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "jujian_ev_shijian": ([
        "id": "jujian_ev_shijian", "sect": "jujian_gate",
        "name": "试剑大会", "desc": "门内比剑，战功声望。",
        "conditions": ([ EV_COND_REALM_MIN: 1, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 1000, EV_REWARD_REP: 150, EV_REWARD_CONTRIB: 300 ]),
    ]),
    "jujian_ev_dianhou": ([
        "id": "jujian_ev_dianhou", "sect": "jujian_gate",
        "name": "殿后血战", "desc": "正魔大战巨剑门殿后。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 鬼灵门 ──
    "guiling_ev_qugui": ([
        "id": "guiling_ev_qugui", "sect": "guiling_sect",
        "name": "驱鬼入门", "desc": "驭鬼基础任务。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "guiling_ev_yanjia": ([
        "id": "guiling_ev_yanjia", "sect": "guiling_sect",
        "name": "燕家联姻", "desc": "与燕家结姻亲、共修血灵大法。",
        "conditions": ([ EV_COND_REALM_MIN: 1, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 1000, EV_REWARD_REP: 150, EV_REWARD_CONTRIB: 300 ]),
    ]),
    "guiling_ev_mozheng": ([
        "id": "guiling_ev_mozheng", "sect": "guiling_sect",
        "name": "魔道争霸", "desc": "六宗内部角逐，战功声望。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),

    // ── 御灵宗 ──
    "yuling_ev_xunshou": ([
        "id": "yuling_ev_xunshou", "sect": "yuling_sect",
        "name": "驯兽入门", "desc": "驯养低阶灵兽。",
        "conditions": ([ EV_COND_REALM_MIN: 0, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 300, EV_REWARD_REP: 30, EV_REWARD_CONTRIB: 100 ]),
    ]),
    "yuling_ev_lingyu": ([
        "id": "yuling_ev_lingyu", "sect": "yuling_sect",
        "name": "灵兽山抉择", "desc": "是否召回灵兽山暗桩——阵营分支，声望巨变。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),
    "yuling_ev_shouchao": ([
        "id": "yuling_ev_shouchao", "sect": "yuling_sect",
        "name": "兽潮推进", "desc": "兽潮推进越国，战功领土。",
        "conditions": ([ EV_COND_REALM_MIN: 2, EV_COND_REALM_MAX: 0 ]),
        "rewards": ([ EV_REWARD_EXP: 5000, EV_REWARD_REP: 300, EV_REWARD_CONTRIB: 800 ]),
    ]),
]);

// ─── 任务链分组（串行） ───
nosave mapping chain_defs = ([
    "chain_yanyue"    : ({ "yanyue_quest_1", "yanyue_quest_2", "yanyue_quest_3" }),
    "chain_huangfeng" : ({ "huangfeng_quest_1", "huangfeng_quest_2", "huangfeng_quest_3" }),
    "chain_lingshou"  : ({ "lingshou_quest_1", "lingshou_quest_2", "lingshou_quest_3" }),
    "chain_qingxu"    : ({ "qingxu_quest_1", "qingxu_quest_2", "qingxu_quest_3" }),
    "chain_huadao"    : ({ "huadao_quest_1", "huadao_quest_2", "huadao_quest_3" }),
    "chain_tianque"   : ({ "tianque_quest_1", "tianque_quest_2", "tianque_quest_3" }),
    "chain_jujian"    : ({ "jujian_quest_1", "jujian_quest_2", "jujian_quest_3" }),
    "chain_guiling"   : ({ "guiling_quest_1", "guiling_quest_2", "guiling_quest_3" }),
    "chain_yuling"    : ({ "yuling_quest_1", "yuling_quest_2", "yuling_quest_3" }),
]);

// ═══════════════════════════════════════════
//  初始化
// ═══════════════════════════════════════════

void create()
{
    seteuid(ROOT_UID);
    set("channel_id", "宗门任务系统");

    register_all_quests();
    CHANNEL_D->do_channel(this_object(), "sys", "宗门任务与事件系统已经启动。");
}

int clean_up()
{
    return 1;
}

// 注册九宗任务链到 QUEST_CHAIN_D（任务模板 + 串行链）
void register_all_quests()
{
    string *qids = keys(quest_defs);
    int i;
    string *cids = keys(chain_defs);
    int j;

    for (i = 0; i < sizeof(qids); i++)
        QUEST_CHAIN_D->register_quest(quest_defs[qids[i]]);

    for (j = 0; j < sizeof(cids); j++)
        QUEST_CHAIN_D->register_chain(cids[j], CHAIN_SERIAL, chain_defs[cids[j]], ([]));
}

// ═══════════════════════════════════════════
//  查询接口
// ═══════════════════════════════════════════

string *query_sects()
{
    return keys(sect_areas);
}

string query_sect_area(string sect_id)
{
    return sect_areas[sect_id];
}

mapping query_quest(string quest_id)
{
    return quest_defs[quest_id];
}

mapping query_event(string event_id)
{
    return event_defs[event_id];
}

// 玩家是否位于本宗驻地场景（#58 场景挂接：房间路径前缀匹配）
int in_sect_area(object player, string sect_id)
{
    string area;
    string here;

    if (!objectp(player)) return 0;
    area = sect_areas[sect_id];
    if (!stringp(area)) return 0;
    here = base_name(environment(player));
    if (!stringp(here)) return 0;
    return strsrch(here, area) == 0;
}

// 本宗可接任务（含玩家状态：0 未解锁/1 可接/2 进行中/3 已完成）
mapping *query_sect_quests(object player)
{
    mapping *result = ({});
    string psect;
    string *qids;
    int i;

    if (!objectp(player)) return result;
    psect = SECT_D->query_player_sect(player);
    if (!stringp(psect)) return result;

    qids = keys(quest_defs);
    for (i = 0; i < sizeof(qids); i++)
    {
        mapping t = quest_defs[qids[i]];
        mapping completed = player->query(QUEST_CHAIN_COMPLETED);
        int status;

        if (t["sect"] != psect) continue;

        if (QUEST_CHAIN_D->get_player_active_quest(player, qids[i]))
            status = QUEST_STATUS_ACTIVE;
        else if (mapp(completed) && completed[qids[i]])
            status = QUEST_STATUS_COMPLETED;
        else if (QUEST_CHAIN_D->is_quest_available(qids[i], player))
            status = QUEST_STATUS_AVAILABLE;
        else
            status = QUEST_STATUS_LOCKED;
        result += ({ t + ([ "status": status ]) });
    }
    return result;
}

// 本宗事件列表（含触发状态：0 未触发/1 已触发）
mapping *query_sect_events(object player)
{
    mapping *result = ({});
    string psect;
    string *eids;
    mapping triggered;
    int i;

    if (!objectp(player)) return result;
    psect = SECT_D->query_player_sect(player);
    if (!stringp(psect)) return result;

    triggered = player->query(SECT_QUEST_TRIGGERED);

    eids = keys(event_defs);
    for (i = 0; i < sizeof(eids); i++)
    {
        mapping ev = event_defs[eids[i]];
        int done = 0;

        if (ev["sect"] != psect) continue;
        if (mapp(triggered) && triggered[eids[i]]) done = 1;
        result += ({ ev + ([ "triggered": done ]) });
    }
    return result;
}

// ═══════════════════════════════════════════
//  任务接取 / 交任务
// ═══════════════════════════════════════════

// 接取宗门任务
// 前置（c5 端到端）：已入本宗 + 位于本宗驻地场景 + 任务链顺序（框架 is_quest_available 检查前置）
int accept_quest(object player, string quest_id)
{
    mapping template;
    string sect_id, psect;

    if (!objectp(player) || !userp(player))
        return 0;

    template = quest_defs[quest_id];
    if (!mapp(template))
    {
        tell_object(player, "没有这个宗门任务。\n");
        return 0;
    }

    sect_id = template["sect"];
    psect = SECT_D->query_player_sect(player);
    if (sect_id != psect)
    {
        tell_object(player, "此任务非你所在宗门发布。\n");
        return 0;
    }

    if (!in_sect_area(player, sect_id))
    {
        tell_object(player, "请先回到" + SECT_D->query_sect_name(sect_id) + "驻地再领取任务。\n");
        return 0;
    }

    if (QUEST_CHAIN_D->assign_quest(player, quest_id))
    {
        tell_object(player, HIG "你接取了宗门任务「" + template["name"] + "」。\n" NOR);
        tell_object(player, template["description"] + "\n");
        return 1;
    }

    tell_object(player, "当前无法接取此任务（可能已接取、已完成或条件未满足）。\n");
    return 0;
}

// 推进任务目标进度（OBJ_REACH/OBJ_TALK 按当前位置判定）
// 返回目标是否全部达成
int quest_progress(object player, string quest_id)
{
    mapping active;
    mapping template;
    mapping objectives;
    mapping progress;
    string here;
    int i, done;

    if (!objectp(player)) return 0;

    active = QUEST_CHAIN_D->get_player_active_quest(player, quest_id);
    if (!mapp(active)) return 0;

    template = quest_defs[quest_id];
    if (!mapp(template)) return 0;

    objectives = template["objectives"];
    progress = active["progress"];
    if (!mapp(progress)) progress = ([]);

    here = base_name(environment(player));
    done = 1;

    for (i = 0; i < sizeof(objectives); i++)
    {
        mapping obj = objectives[i];
        string key = "obj_" + i;
        int cur = progress[key];
        int amount = obj["amount"];
        string target = obj["target"];

        if (!amount) amount = 1;
        if (!cur) cur = 0;

        // 到达类目标：所在房间路径前缀匹配目标即达成
        if ((obj["type"] == OBJ_REACH || obj["type"] == OBJ_TALK) &&
            stringp(target) && target != "" && stringp(here) &&
            strsrch(here, target) == 0)
        {
            cur = amount;
            progress[key] = cur;
        }

        if (cur < amount)
            done = 0;
    }

    active["progress"] = progress;
    player->set(QUEST_CHAIN_ACTIVE, active);
    QUEST_CHAIN_D->save_player_quest_state(player);

    return done;
}

// 交任务（目标达成 → 框架 complete_quest 结算奖励）
int report_quest(object player, string quest_id)
{
    mapping template;
    string sect_id, psect;

    if (!objectp(player) || !userp(player))
        return 0;

    template = quest_defs[quest_id];
    if (!mapp(template))
    {
        tell_object(player, "没有这个宗门任务。\n");
        return 0;
    }

    sect_id = template["sect"];
    psect = SECT_D->query_player_sect(player);
    if (sect_id != psect)
    {
        tell_object(player, "此任务非你所在宗门发布。\n");
        return 0;
    }

    if (!in_sect_area(player, sect_id))
    {
        tell_object(player, "请先回到" + SECT_D->query_sect_name(sect_id) + "驻地再交任务。\n");
        return 0;
    }

    if (quest_progress(player, quest_id))
    {
        if (QUEST_CHAIN_D->complete_quest(player, quest_id))
        {
            tell_object(player, HIG "宗门任务「" + template["name"] + "」完成！\n" NOR);
            return 1;
        }
        tell_object(player, "你当前没有进行中的此任务。\n");
        return 0;
    }

    tell_object(player, "任务目标尚未完成，请按任务指引继续。\n");
    return 0;
}

// ═══════════════════════════════════════════
//  宗门事件
// ═══════════════════════════════════════════

// 检查事件触发条件；通过返回 0，否则返回拒绝原因字符串
string check_event_conditions(object player, mapping ev)
{
    mapping conds;
    string sect_id;

    if (!objectp(player) || !mapp(ev)) return "事件不存在。\n";

    conds = ev["conditions"];
    sect_id = ev["sect"];
    if (!mapp(conds)) return 0;

    {
        int realm_idx = QUEST_CHAIN_D->get_player_realm_index(player);

        if (conds[EV_COND_REALM_MIN] && realm_idx < conds[EV_COND_REALM_MIN])
            return "你的境界尚未达到触发此事件的要求。\n";
        if (conds[EV_COND_REALM_MAX] && realm_idx > conds[EV_COND_REALM_MAX])
            return "你的境界已超出此事件的范围。\n";
    }

    if (conds[EV_COND_REP_MIN])
    {
        int rep = REPUTATION_D->query_reputation_value(player, sect_id);
        if (rep < conds[EV_COND_REP_MIN])
            return "你在本门的声望不足，事件暂未对你开放。\n";
    }

    if (conds[EV_COND_CONTRIB_MIN])
    {
        int contrib = SECT_D->query_contribution(player);
        if (contrib < conds[EV_COND_CONTRIB_MIN])
            return "你的门派贡献不足。\n";
    }

    if (conds[EV_COND_QUEST])
    {
        mapping completed = player->query(QUEST_CHAIN_COMPLETED);
        if (!mapp(completed) || !completed[conds[EV_COND_QUEST]])
            return "你尚未完成前置任务。\n";
    }

    if (conds[EV_COND_MALE_ONLY] && player->query("gender") != "男性")
        return "此事件仅限男弟子参与。\n";

    return 0;
}

// 触发宗门事件（条件满足 → 结算奖励 → 记录已触发）
int trigger_event(object player, string event_id)
{
    mapping ev;
    mapping triggered;
    string sect_id, psect, err;

    if (!objectp(player) || !userp(player))
        return 0;

    ev = event_defs[event_id];
    if (!mapp(ev))
    {
        tell_object(player, "没有这个宗门事件。\n");
        return 0;
    }

    sect_id = ev["sect"];
    psect = SECT_D->query_player_sect(player);
    if (sect_id != psect)
    {
        tell_object(player, "此事件与你所在宗门无关。\n");
        return 0;
    }

    triggered = player->query(SECT_QUEST_TRIGGERED);
    if (mapp(triggered) && triggered[event_id])
    {
        tell_object(player, "你已参与过此事件。\n");
        return 0;
    }

    err = check_event_conditions(player, ev);
    if (stringp(err))
    {
        tell_object(player, err);
        return 0;
    }

    grant_event_rewards(player, ev);

    if (!mapp(triggered)) triggered = ([]);
    triggered[event_id] = time();
    player->set(SECT_QUEST_TRIGGERED, triggered);

    return 1;
}

// 结算事件奖励（活跃度梯度加成：连续活跃递增、断档衰减）
void grant_event_rewards(object player, mapping ev)
{
    mapping rewards = ev["rewards"];
    string sect_id = ev["sect"];
    int i;
    float streak_bonus;

    if (!mapp(rewards)) return;
    streak_bonus = QUEST_CHAIN_D->calc_daily_bonus(
                       player->query(QUEST_CHAIN_DAILY_STREAK));

    tell_object(player, HIM "◇ 宗门事件「" + ev["name"] + "」达成！\n" NOR);

    // 修为经验（活跃度加成）
    int exp = rewards[EV_REWARD_EXP];
    if (exp)
    {
        exp = to_int(exp * streak_bonus);
        player->add("combat_exp", exp);
        tell_object(player, HIW "  修为经验 +" + exp + "\n" NOR);
    }

    // 门派贡献
    int contrib = rewards[EV_REWARD_CONTRIB];
    if (contrib)
    {
        SECT_D->add_contribution(player, contrib, "宗门事件 " + ev["id"]);
        tell_object(player, HIG "  门派贡献 +" + contrib + "\n" NOR);
    }

    // 本门声望（含正魔互斥）
    int rep = rewards[EV_REWARD_REP];
    if (rep)
    {
        REPUTATION_D->add_reputation(player, sect_id, rep, "宗门事件 " + ev["id"]);
        tell_object(player, HIM "  「" + SECT_D->query_sect_name(sect_id) + "」声望 +" + rep + "\n" NOR);
    }

    // 物品
    mixed items = rewards[EV_REWARD_ITEMS];
    if (arrayp(items))
    {
        for (i = 0; i < sizeof(items); i++)
        {
            if (stringp(items[i]) && items[i] != "")
            {
                object ob = new(items[i]);
                if (objectp(ob))
                {
                    if (ob->move(player))
                        tell_object(player, "  你获得了" + ob->name() + "。\n");
                    else
                        destruct(ob);
                }
            }
        }
    }

    // 功法
    mixed skills = rewards[EV_REWARD_SKILLS];
    if (arrayp(skills))
    {
        for (i = 0; i < sizeof(skills); i++)
        {
            if (stringp(skills[i]) && skills[i] != "")
                QUEST_CHAIN_D->grant_skill(player, sect_id, skills[i]);
        }
    }
}

// ═══════════════════════════════════════════
//  调试
// ═══════════════════════════════════════════

string dump_sect_quest_info()
{
    string output = "====== 宗门任务与事件 ======\n";

    foreach (string sid, string area in sect_areas)
        output += sprintf("%-20s %s\n", SECT_D->query_sect_name(sid), area);

    output += sprintf("任务模板：%d 条，事件：%d 个\n",
                      sizeof(quest_defs), sizeof(event_defs));
    return output;
}
