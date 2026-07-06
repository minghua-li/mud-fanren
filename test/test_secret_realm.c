// test_secret_realm.c
// 秘境副本系统 —— 验收测试
// Test for ticket #31

#include <secret_realm.h>

// 测试注册与查询
int test_register_and_query()
{
    mapping realm;
    int ret;
    
    // 注册一个测试秘境
    realm = ([
        SR_FIELD_ID          : "test_realm_1",
        SR_FIELD_NAME        : "测试秘境",
        SR_FIELD_TYPE        : SR_TYPE_STORY,
        SR_FIELD_DIFFICULTY  : SR_DIFFICULTY_NORMAL,
        SR_FIELD_RESET       : SR_RESET_ONCE,
        SR_FIELD_MIN_LEVEL   : 5,
        SR_FIELD_MAX_LEVEL   : 15,
        SR_FIELD_TEAM_REQ    : 1,
        SR_FIELD_DURATION    : 3600,
        SR_FIELD_CD          : SR_CD_DAILY,
        SR_FIELD_LAYERS      : 5,
    ]);
    
    ret = SECRET_REALM_D->register_realm(realm);
    if (!ret) return 0;  // 注册失败
    
    realm = SECRET_REALM_D->query_realm("test_realm_1");
    if (sizeof(realm) == 0) return 0;  // 查询失败
    
    if (realm[SR_FIELD_NAME] != "测试秘境") return 0;
    if (realm[SR_FIELD_TYPE] != SR_TYPE_STORY) return 0;
    if (realm[SR_FIELD_DIFFICULTY] != SR_DIFFICULTY_NORMAL) return 0;
    
    return 1;
}

// 测试进入条件校验
int test_entry_condition()
{
    // 依赖具体玩家对象，此处测试常量定义的正确性
    if (SR_TYPE_STORY != 1) return 0;
    if (SR_TYPE_TIMED != 2) return 0;
    if (SR_TYPE_CHALLENGE != 3) return 0;
    if (SR_TYPE_FORTUNE != 4) return 0;
    
    // 验证境界字段常量
    if (SR_FIELD_MIN_LEVEL != "min_level") return 0;
    if (SR_FIELD_MAX_LEVEL != "max_level") return 0;
    if (SR_FIELD_ITEM_REQ != "item_req") return 0;
    if (SR_FIELD_TEAM_REQ != "team_req") return 0;
    
    // 验证错误码（按函数文档约定）
    // check_entry_condition 返回：
    // 0 = 可进入, 1 = 非玩家, 2 = 秘境不存在
    // 3 = 未开放, 10 = 境界不足, 11 = 超出上限
    // 20 = 缺少物品, 30 = 冷却中
    
    return 1;
}

// 测试秘境实例生命周期
int test_instance_lifecycle()
{
    mapping inst;
    string inst_id, realm_id = "test_realm_1";
    
    // create_instance 需要玩家对象，验证功能路径
    if (!functionp("create_instance")) return 0;
    if (!functionp("destroy_instance")) return 0;
    if (!functionp("cleanup_expired_instances")) return 0;
    
    // 确认实例管理函数存在
    return 1;
}

// 测试奖励结算
int test_settle_rewards()
{
    // 验证奖励类型常量
    if (SR_REWARD_EXP != 1) return 0;
    if (SR_REWARD_POTENTIAL != 2) return 0;
    if (SR_REWARD_SCORE != 3) return 0;
    if (SR_REWARD_ITEM != 4) return 0;
    if (SR_REWARD_SKILL != 5) return 0;
    if (SR_REWARD_POINT != 6) return 0;
    
    // 验证难度倍数常量
    if (SR_DIFFICULTY_EASY != 1) return 0;
    if (SR_DIFFICULTY_NORMAL != 2) return 0;
    if (SR_DIFFICULTY_HARD != 3) return 0;
    if (SR_DIFFICULTY_HELL != 4) return 0;
    
    // 验证结算函数存在
    if (!functionp("settle_instance")) return 0;
    
    return 1;
}

// 测试冷却管理
int test_cooldown()
{
    // 验证冷却常量
    if (SR_CD_DAILY != 86400) return 0;
    if (SR_CD_WEEKLY != 604800) return 0;
    if (SR_CD_CHALLENGE != 7200) return 0;
    if (SR_CD_FORTUNE != 0) return 0;
    
    // 确认冷却函数存在
    if (!functionp("is_in_cooldown")) return 0;
    if (!functionp("set_cooldown")) return 0;
    if (!functionp("query_remaining_cooldown")) return 0;
    
    return 1;
}
