// sect_d.c
// 门派系统守护进程 —— 九宗入宗/晋升/贡献/功法学习
// 设计文档: .knowledge/factions/sects/ 九宗档案（仕途晋升/功法/境界成长线）
//           1D-门派种族声望.md §三 门派等级体系
// 依赖: REPUTATION_D（声望调整与互斥）

#include <ansi.h>
#include <sect.h>
#include <reputation.h>
#include <globals.h>

inherit F_DBASE;

// -------- 境界分层常量 --------
// tier = 境界索引 * 3 + 小阶段（0初期/1中期/2后期）
// 境界索引对齐 quest_chain.h REALM_NAMES: 0炼气 1筑基 2结丹 3元婴 4化神 5炼虚
nosave string *realm_names = ({ "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘" });
nosave string *stage_names = ({ "初期", "中期", "后期" });

// -------- 九宗配置 --------
// ranks   : 阶位称号数组（下标即阶位）
// promote : 晋升门槛数组，promote[i-1] = 升到第 i 阶的 ({ tier, 贡献 })
// skills  : 功法 ([ skill_id: ([ name/rank/cost/desc ]) ])
// 门槛依据各档案「仕途晋升」节；默认：内门=炼气后期+贡献，真传=筑基，长老=结丹
nosave mapping sect_config = ([
  "yanyue_sect": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老", "太上长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,      SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,      SECT_CONTRIB_ELDER }),
      ({ SECT_TIER_YING,     SECT_CONTRIB_LEADER }),
    }),
    "skills": ([
      "shuangxiu-zhishu": ([ "name": "双修之术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "合修功法，阴阳相济，精进法力" ]),
      "xuanyue-xiyin-gong": ([ "name": "玄月吸阴功", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "阴系双修秘术，与合欢宗同源" ]),
    ]),
  ]),
  "huangfeng_valley": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老", "副宗主", "宗主" }),
    "promote": ({
      ({ SECT_TIER_ZHU,      SECT_CONTRIB_INNER }),   // 黄枫谷内门需筑基
      ({ SECT_TIER_ZHU_LATE, SECT_CONTRIB_TRUE }),    // 真传需筑基后期
      ({ SECT_TIER_JIE,      SECT_CONTRIB_ELDER }),
      ({ SECT_TIER_JIE_LATE, SECT_CONTRIB_DEPUTY }),
      ({ SECT_TIER_YING,     SECT_CONTRIB_LEADER }),
    }),
    "skills": ([
      "changchun-gong":  ([ "name": "长春功", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "木系基础功法，洗髓开智，1-13层" ]),
      "qingyuan-jianjue": ([ "name": "青元剑诀", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "剑修功法，残本9层，每三层一门神通" ]),
      "zhenyang-jue":    ([ "name": "真阳诀", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "火系顶阶功法，三阳之体如鱼得水" ]),
      "xuanbing-jue":    ([ "name": "玄冰诀", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "水属性辅助功法，修炼容易瓶颈易破" ]),
      "guiyuan-gong":    ([ "name": "归元功", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "防御功法，归元灵甲" ]),
      "huanling-jue":    ([ "name": "幻灵决", "rank": 2, "cost": SECT_SKILL_COST_ELITE, "desc": "幻术功法，幻影分身" ]),
      "ningyuan-gong":   ([ "name": "凝元功", "rank": 2, "cost": SECT_SKILL_COST_ELITE, "desc": "辅助功法，聚灵术" ]),
    ]),
  ]),
  "lingshou_mountain": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
    }),
    "skills": ([
      "yushou-shu":  ([ "name": "御兽术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "驯养灵兽灵虫，驭使作战" ]),
      "yichong-shu": ([ "name": "役虫术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "操控灵虫作战" ]),
      "kuilei-shu":  ([ "name": "傀儡术", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "炼制傀儡，越国七派中较突出" ]),
    ]),
  ]),
  "qingxu_sect": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
    }),
    "skills": ([
      "daomen-shufa":    ([ "name": "道门术法", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "清净无为，术法为主" ]),
      "qingxu-jian-dian": ([ "name": "清虚剑典", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "剑符双修，道剑路线" ]),
    ]),
  ]),
  "huadao_dock": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
    }),
    "skills": ([
      "daofa-chuancheng": ([ "name": "刀法传承", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "刀意速攻，刀法凶悍" ]),
      "lianqi-shu":       ([ "name": "炼器术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "炼器工艺七派突出" ]),
    ]),
  ]),
  "tianque_fort": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
    }),
    "skills": ([
      "zhubao-shu": ([ "name": "筑堡术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "筑堡建州，城防建造" ]),
      "zhenfa-shu": ([ "name": "阵法术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "门派阵法布设" ]),
    ]),
  ]),
  "jujian_gate": ([
    "male_only": 1,   // 巨剑门全男弟子
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
    }),
    "skills": ([
      "zhongjian-jianfa": ([ "name": "重剑剑法", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "银色巨剑，一劈可破上品法器护罩" ]),
      "jianxiu-chuancheng": ([ "name": "剑修传承", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "剑修立派，体剑双修" ]),
    ]),
  ]),
  "guiling_sect": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老", "副门主", "门主" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
      ({ SECT_TIER_JIE_LATE, SECT_CONTRIB_DEPUTY }),
      ({ SECT_TIER_YING,    SECT_CONTRIB_LEADER }),
    }),
    "skills": ([
      "guidao-gongfa": ([ "name": "鬼道功法", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "驱鬼役妖，操控鬼物" ]),
      "dushu":         ([ "name": "毒术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "用毒之术" ]),
      "anshu":         ([ "name": "暗术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "潜行暗杀类功法" ]),
      "lianshi-shu":   ([ "name": "炼尸术", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "炼制尸傀" ]),
      "xueling-dafa":  ([ "name": "血灵大法", "rank": 2, "cost": SECT_SKILL_COST_APEX, "desc": "《万灵真经》第一魔功，需天灵根暗灵根双修" ]),
    ]),
  ]),
  "yuling_sect": ([
    "male_only": 0,
    "ranks": ({ "外门弟子", "内门弟子", "真传弟子", "长老" }),
    "promote": ({
      ({ SECT_TIER_QI_LATE, SECT_CONTRIB_INNER }),
      ({ SECT_TIER_ZHU,     SECT_CONTRIB_TRUE }),
      ({ SECT_TIER_JIE,     SECT_CONTRIB_ELDER }),
    }),
    "skills": ([
      "yushou-shu":  ([ "name": "御兽术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "驭使灵兽战斗，与灵兽山同源" ]),
      "yichong-shu": ([ "name": "役虫术", "rank": 0, "cost": SECT_SKILL_COST_BASIC, "desc": "操控灵虫，虫兽双修" ]),
      "wangu-jue":   ([ "name": "万蛊诀", "rank": 1, "cost": SECT_SKILL_COST_INNER, "desc": "养蛊役虫" ]),
    ]),
  ]),
]);

void create()
{
    seteuid(getuid());
    set("name", "门派系统");
    set("id", "sect_d");
}

// ======== 查询接口 ========

string *query_sects()
{
    return keys(sect_config);
}

mapping query_sect_config(string sect_id)
{
    return sect_config[sect_id];
}

// 宗门名称（从声望系统取）
string query_sect_name(string sect_id)
{
    mapping fi = REPUTATION_D->get_faction_info(sect_id);

    if (mapp(fi) && stringp(fi["name"]))
        return fi["name"];
    return sect_id;
}

string *query_sect_ranks(string sect_id)
{
    mapping cfg = sect_config[sect_id];

    if (!mapp(cfg)) return ({});
    return cfg["ranks"];
}

// 玩家所属门派（无门派返回 0）
string query_player_sect(object player)
{
    if (!objectp(player)) return 0;
    return player->query(SECT_PATH_ID);
}

int query_rank(object player)
{
    int rank;

    if (!objectp(player)) return 0;
    rank = player->query(SECT_PATH_RANK);
    if (!intp(rank)) return 0;
    return rank;
}

string query_rank_name(object player)
{
    string sect_id = query_player_sect(player);
    int rank = query_rank(player);
    string *ranks;

    if (!stringp(sect_id)) return 0;
    ranks = query_sect_ranks(sect_id);
    if (rank < 0 || rank >= sizeof(ranks)) return 0;
    return ranks[rank];
}

int query_contribution(object player)
{
    int contrib;

    if (!objectp(player)) return 0;
    contrib = player->query(SECT_PATH_CONTRIB);
    if (!intp(contrib)) return 0;
    return contrib;
}

// 门派功法列表
string *query_sect_skills(string sect_id)
{
    mapping cfg = sect_config[sect_id];

    if (!mapp(cfg) || !mapp(cfg["skills"])) return ({});
    return keys(cfg["skills"]);
}

mapping query_sect_skill_info(string sect_id, string skill_id)
{
    mapping cfg = sect_config[sect_id];

    if (!mapp(cfg) || !mapp(cfg["skills"])) return 0;
    return cfg["skills"][skill_id];
}

// ======== 境界判定 ========

// 经验折算境界 tier（realm 属性缺失时的兜底）
// 阈值与 reputation_d.c query_daily_cap 一致
int exp_to_tier(int exp)
{
    if (exp < 100000)     return 0;   // 炼气初期
    if (exp < 1000000)    return 3;   // 筑基初期
    if (exp < 10000000)   return 6;   // 结丹初期
    if (exp < 50000000)   return 9;   // 元婴初期
    if (exp < 200000000)  return 12;  // 化神初期
    return 15;                        // 炼虚初期
}

// 提取境界字符串中的层数（如"炼气十三层"→13），无数字返回 0
int extract_layer(string realm)
{
    int i, len, start, end;

    if (!stringp(realm)) return 0;
    len = strlen(realm);
    start = -1;
    for (i = 0; i < len; i++)
    {
        int ch = realm[i];
        if (ch >= 48 && ch <= 57)
        {
            start = i;
            break;
        }
    }
    if (start == -1) return 0;

    end = start;
    while (end < len)
    {
        int ch = realm[end];
        if (ch < 48 || ch > 57) break;
        end++;
    }
    return to_int(realm[start..end-1]);
}

// 解析境界字符串 → ({ 境界索引, 层数 })；层数 0 表示无层数信息
mixed *parse_realm(string realm)
{
    int index, layer;

    if (!stringp(realm) || realm == "") return ({ 0, 0 });

    index = 0;
    for (int i = 0; i < sizeof(realm_names); i++)
        if (strsrch(realm, realm_names[i]) != -1)
        {
            index = i;
            break;
        }

    layer = extract_layer(realm);
    return ({ index, layer });
}

// 玩家境界 tier（境界索引*3 + 小阶段 0初/1中/2后）
// 层数折算：1-3层初期、4-6层中期、7层以上后期（1D §三 内门=炼气7层）
int query_cultivation_tier(object player)
{
    string realm;
    mixed *parsed;
    int index, layer, stage;

    if (!objectp(player)) return 0;

    realm = player->query("realm");
    if (!stringp(realm) || realm == "")
        return exp_to_tier(player->query("combat_exp"));

    parsed = parse_realm(realm);
    index = parsed[0];
    layer = parsed[1];

    stage = 1;   // 默认中期
    if (strsrch(realm, "初期") != -1) stage = 0;
    else if (strsrch(realm, "后期") != -1) stage = 2;
    else if (layer > 0)
    {
        if (layer <= 3) stage = 0;
        else if (layer <= 6) stage = 1;
        else stage = 2;
    }

    return index * 3 + stage;
}

// tier → 中文境界名
string tier_name(int tier)
{
    int idx = tier / 3;
    int stage = tier % 3;

    if (idx < 0) idx = 0;
    if (idx >= sizeof(realm_names)) idx = sizeof(realm_names) - 1;
    if (stage < 0) stage = 0;
    if (stage > 2) stage = 2;

    if (idx == 0)
        return realm_names[0] + stage_names[stage];  // 炼气初期等
    return realm_names[idx] + stage_names[stage];
}

// 玩家境界展示名（优先 realm 属性）
string query_realm_display(object player)
{
    string realm;

    if (!objectp(player)) return "炼气初期";
    realm = player->query("realm");
    if (stringp(realm) && realm != "")
        return realm;
    return tier_name(exp_to_tier(player->query("combat_exp")));
}

// ======== 入宗校验与执行 ========

// 检查入宗条件；返回 0=可入，否则返回拒绝原因字符串
string check_join(object player, string sect_id)
{
    mapping cfg;
    mapping betrayed;
    string cur_sect;
    string realm;
    mixed *parsed;

    if (!objectp(player)) return "无效的玩家对象";
    cfg = sect_config[sect_id];
    if (!mapp(cfg)) return "不存在的门派：" + sect_id;

    cur_sect = query_player_sect(player);
    if (stringp(cur_sect))
        return "你已是" + query_sect_name(cur_sect) + "弟子，请先退出或叛门（sect help）";

    betrayed = player->query(SECT_PATH_BETRAYED);
    if (mapp(betrayed) && betrayed[sect_id])
        return "你有叛出" + query_sect_name(sect_id) + "的记录，该派拒收";

    // 修为要求：炼气三层以上（1D §三）
    realm = player->query("realm");
    if (stringp(realm) && realm != "")
    {
        parsed = parse_realm(realm);
        if (parsed[0] == 0 && parsed[1] > 0 && parsed[1] < 3)
            return "修为不足：入宗需炼气三层以上（当前" + realm + "）";
    }

    // 巨剑门只收男弟子
    if (cfg["male_only"])
    {
        string gender = player->query("gender");
        if (gender != "男性" && gender != "male")
            return "巨剑门全为男弟子，不收女弟子";
    }

    return 0;
}

// 入宗成功返回 1
int join_sect(object player, string sect_id)
{
    string reason;

    if (!objectp(player)) return 0;

    reason = check_join(player, sect_id);
    if (stringp(reason))
    {
        tell_object(player, HIR + reason + "\n" + NOR);
        return 0;
    }

    player->set(SECT_PATH_ID, sect_id);
    player->set(SECT_PATH_RANK, 0);
    player->set(SECT_PATH_CONTRIB, 0);
    player->set(SECT_PATH_JOIN_TIME, time());

    // 入门声望：直接累加（一次性事件不占每日上限），再触发互斥
    player->add(REP_PATH_FACTION + "/" + sect_id, SECT_JOIN_REP_GAIN);
    REPUTATION_D->apply_mutex(player, sect_id, SECT_JOIN_REP_GAIN);

    log_file("sect", sprintf("%s %s join %s\n",
              ctime(time()), player->query("id"), sect_id));

    tell_object(player, HIG "\n恭喜！你正式拜入" + query_sect_name(sect_id) +
                "门下，成为一名外门弟子。\n" NOR);
    tell_object(player, HIC "门派声望 +" + SECT_JOIN_REP_GAIN + "。输入 " HIY "sect" HIC " 查看门派面板。\n" NOR);
    return 1;
}

// 内部：退出/叛门处理（betray: 1=叛门 0=正常退出）
void do_leave(object player, string sect_id, int betray)
{
    mapping betrayed;
    int penalty;

    if (!objectp(player) || !stringp(sect_id)) return;

    penalty = betray ? SECT_BETRAY_REP_PENALTY : SECT_LEAVE_REP_PENALTY;
    player->add(REP_PATH_FACTION + "/" + sect_id, penalty);

    if (betray)
    {
        betrayed = player->query(SECT_PATH_BETRAYED);
        if (!mapp(betrayed)) betrayed = ([]);
        betrayed[sect_id] = time();
        player->set(SECT_PATH_BETRAYED, betrayed);
        log_file("sect", sprintf("%s %s betray %s\n",
                  ctime(time()), player->query("id"), sect_id));
    }
    else
        log_file("sect", sprintf("%s %s leave %s\n",
                  ctime(time()), player->query("id"), sect_id));

    player->delete(SECT_PATH_ID);
    player->delete(SECT_PATH_RANK);
    player->delete(SECT_PATH_CONTRIB);
    player->delete(SECT_PATH_JOIN_TIME);
}

// 正常退出门派
int leave_sect(object player)
{
    string sect_id = query_player_sect(player);

    if (!objectp(player) || !stringp(sect_id)) return 0;

    tell_object(player, HIY "你离开了" + query_sect_name(sect_id) +
                "，门派声望 " + SECT_LEAVE_REP_PENALTY + "，贡献清空。\n" NOR);
    do_leave(player, sect_id, 0);
    return 1;
}

// 叛门
int betray_sect(object player)
{
    string sect_id = query_player_sect(player);

    if (!objectp(player) || !stringp(sect_id)) return 0;

    tell_object(player, HIR "你叛出了" + query_sect_name(sect_id) +
                "！门派声望 " + SECT_BETRAY_REP_PENALTY + "，被全派追杀！\n" NOR);
    do_leave(player, sect_id, 1);
    return 1;
}

// ======== 贡献 ========

int add_contribution(object player, int amount, string reason)
{
    int new_val;

    if (!objectp(player) || !stringp(query_player_sect(player))) return 0;

    new_val = player->add(SECT_PATH_CONTRIB, amount);
    if (stringp(reason) && reason != "")
        log_file("sect", sprintf("%s %s contrib %+d %s\n",
                  ctime(time()), player->query("id"), amount, reason));
    return new_val;
}

// ======== 晋升 ========

// 下一阶位的门槛 ([ tier, 贡献 ])；已到顶返回 0
mixed *query_next_rank_require(object player)
{
    string sect_id = query_player_sect(player);
    int rank = query_rank(player);
    mapping cfg;

    if (!stringp(sect_id)) return 0;
    cfg = sect_config[sect_id];
    if (!mapp(cfg)) return 0;

    mixed *promote = cfg["promote"];
    if (!arrayp(promote) || rank >= sizeof(promote))
        return 0;
    return promote[rank];
}

// 尝试晋升；成功返回 1
int promote(object player)
{
    string sect_id = query_player_sect(player);
    int rank = query_rank(player);
    mixed *require;
    int tier, need_contrib, contrib;
    mapping cfg;

    if (!objectp(player) || !stringp(sect_id))
    {
        tell_object(player, "你尚未拜入任何门派。\n");
        return 0;
    }

    cfg = sect_config[sect_id];
    if (!mapp(cfg))
    {
        tell_object(player, "你尚未拜入任何门派。\n");
        return 0;
    }
    require = query_next_rank_require(player);
    if (!arrayp(require))
    {
        tell_object(player, "你已位极本门，无更高阶位。\n");
        return 0;
    }

    tier = require[0];
    need_contrib = require[1];
    contrib = query_contribution(player);

    if (query_cultivation_tier(player) < tier)
    {
        tell_object(player, sprintf(HIR "修为不足：晋升" + cfg["ranks"][rank + 1] +
                    "需%s（当前%s）。\n" NOR, tier_name(tier), query_realm_display(player)));
        return 0;
    }

    if (contrib < need_contrib)
    {
        tell_object(player, sprintf(HIR "贡献不足：晋升" + cfg["ranks"][rank + 1] +
                    "需门派贡献%d（当前%d）。\n" NOR, need_contrib, contrib));
        return 0;
    }

    player->set(SECT_PATH_RANK, rank + 1);
    log_file("sect", sprintf("%s %s promote %s %d\n",
              ctime(time()), player->query("id"), sect_id, rank + 1));

    tell_object(player, HIG "\n【晋升】你已晋升为" + query_sect_name(sect_id) +
                cfg["ranks"][rank + 1] + "！\n" NOR);
    return 1;
}

// ======== 功法学习 ========

// 尝试学习本门功法；成功返回 1
int learn_skill(object player, string skill_id)
{
    string sect_id = query_player_sect(player);
    mapping skill_info;
    mapping learned;
    int rank, cost, contrib;

    if (!objectp(player) || !stringp(sect_id))
    {
        tell_object(player, "你尚未拜入任何门派。\n");
        return 0;
    }

    skill_info = query_sect_skill_info(sect_id, skill_id);
    if (!mapp(skill_info))
    {
        tell_object(player, "本门无此功法，输入 sect skills 查看功法列表。\n");
        return 0;
    }

    learned = player->query(SECT_PATH_LEARNED);
    if (mapp(learned) && learned[skill_id])
    {
        tell_object(player, "你已学过「" + skill_info["name"] + "」。\n");
        return 0;
    }

    rank = query_rank(player);
    if (rank < skill_info["rank"])
    {
        tell_object(player, sprintf("阶位不足：学习「%s」需达到%s（当前%s）。\n",
                    skill_info["name"],
                    query_sect_ranks(sect_id)[skill_info["rank"]],
                    query_rank_name(player)));
        return 0;
    }

    cost = skill_info["cost"];
    contrib = query_contribution(player);
    if (contrib < cost)
    {
        tell_object(player, sprintf("贡献不足：学习「%s」需门派贡献%d（当前%d）。\n",
                    skill_info["name"], cost, contrib));
        return 0;
    }

    player->add(SECT_PATH_CONTRIB, -cost);
    if (!mapp(learned)) learned = ([]);
    learned[skill_id] = time();
    player->set(SECT_PATH_LEARNED, learned);

    log_file("sect", sprintf("%s %s learn %s %s\n",
              ctime(time()), player->query("id"), sect_id, skill_id));

    tell_object(player, HIG "你耗门派贡献" + cost + "，习得「" +
                skill_info["name"] + "」。\n" NOR);
    return 1;
}

// 已学功法
string *query_learned_skills(object player)
{
    mapping learned;

    if (!objectp(player)) return ({});
    learned = player->query(SECT_PATH_LEARNED);
    if (!mapp(learned)) return ({});
    return keys(learned);
}

// ======== 总览（调试） ========

string dump_sect_info()
{
    string output = "====== 九宗门派 ======\n";

    foreach (string sid, mapping cfg in sect_config)
    {
        output += sprintf("%-20s | 阶位%d级 | 功法%d门%s\n",
                          query_sect_name(sid), sizeof(cfg["ranks"]),
                          sizeof(cfg["skills"]),
                          cfg["male_only"] ? " | 仅男弟子" : "");
    }
    return output;
}
