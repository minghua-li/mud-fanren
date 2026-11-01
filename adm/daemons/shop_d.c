// shop_d.c
// 声望商店系统 - 商店守护进程
// 设计文档: 02-扩充内容/02-声望与互动玩法.md 第3章
// 基于声望等级的门派/势力商店管理

#include <ansi.h>
#include <reputation.h>
#include <globals.h>

inherit F_DBASE;
inherit F_SAVE;

// 商店物品定义（扩展版）
// 格式: ({ item_id, name, type, path, price, rep_req, rep_cost, tier, faction, desc, stock })
//   item_id: 唯一ID
//   name: 显示名
//   type: item/weapon/armor/pill/skill
//   path: 对象路径
//   price: 灵石价格
//   rep_req: 所需声望等级
//   rep_cost: 购买消耗的声望值
//   tier: 商店层级
//   faction: 所属势力ID
//   desc: 说明
//   stock: 库存(-1=无限)
// #64/#97 目录清理（2026-10-31，逐条判定留痕见 #97 票面）：
//   - 改指 21 条悬空引用：武器 8 → /clone/weapon/*；护甲 4 → /clone/armor/*；
//     法宝杂物 9 → /d/yueguo/obj/treasure 参数化法宝基类（交付时由目录补 name/id/value）
//   - 移除 12 条（无对应实体，本阶段不补新物品实体）：技能书 7（对应功法内容
//     未落地，/clone/book 既有书籍教授的是无关旧技能，改指会货不对板）+ 符箓/
//     空间袋/入场券 5（符箓、灵石袋、拍卖令类实体本仓不存在）
//   - 丹药 13 条维持 #73 改指 /clone/pill/* 不动
// #64 收口补丁（2026-10-31，复审指出「货不对板」）：
//   - 庚精改指 /d/yueguo/tainan/obj/gengjing：真实材料实体（is_material=1，
//     坊市 fangshi.c 在售，FORGE_D 配方 gengjingfeijian 用料），原指 treasure 基类无 is_material、不能入炼器配方
//   - 天玄石移除：全仓无对应真实材料实体（仅本目录一行提及），treasure 基类非材料，同名挂接即货不对板
nosave mixed *item_catalog = ({
  // ===================== 黄枫谷 =====================
  // -- 基础 --
  ({"hfg_hp1", "止血草", "pill", "/clone/pill/cao", 100, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "huangfeng_valley", "快速止血的草药", -1}),
  ({"hfg_pill1", "聚气丹", "pill", "/clone/pill/juqi", 300, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "huangfeng_valley", "回复少量灵力", -1}),
  // -- 中级 --
  ({"hfg_swd1", "黄枫制式长剑", "weapon", "/clone/weapon/changjian", 5000, REP_LEVEL_FRIENDLY, 100, SHOP_TIER_INTERMEDIATE, "huangfeng_valley", "黄枫谷弟子标配法器", 50}),
  ({"hfg_arm1", "黄枫法袍", "armor", "/clone/armor/cloth", 5000, REP_LEVEL_FRIENDLY, 100, SHOP_TIER_INTERMEDIATE, "huangfeng_valley", "黄枫谷制式法袍", 50}),
  ({"hfg_pill2", "筑基丹", "pill", "/clone/pill/zhuji", 8000, REP_LEVEL_FRIENDLY, 200, SHOP_TIER_INTERMEDIATE, "huangfeng_valley", "筑基期突破辅助丹药", 20}),
  // -- 高级 --
  ({"hfg_swd2", "中阶飞剑", "weapon", "/clone/weapon/jian", 30000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "huangfeng_valley", "中阶法器飞剑", 10}),
  ({"hfg_pill3", "凝丹丸", "pill", "/clone/pill/ningdan", 25000, REP_LEVEL_TRUST, 400, SHOP_TIER_ADVANCED, "huangfeng_valley", "筑基期辅助丹药", 15}),
  ({"hfg_item1", "灵晶", "item", "/d/yueguo/obj/treasure", 20000, REP_LEVEL_TRUST, 300, SHOP_TIER_ADVANCED, "huangfeng_valley", "蕴含精纯灵力的水晶", 20}),
  // -- 核心 --
  ({"hfg_mat2", "庚精", "item", "/d/yueguo/tainan/obj/gengjing", 150000, REP_LEVEL_RESPECT, 3000, SHOP_TIER_CORE, "huangfeng_valley", "稀有炼器至宝", 5}),
  ({"hfg_item2", "秘境传送卷", "item", "/d/yueguo/obj/treasure", 100000, REP_LEVEL_RESPECT, 2000, SHOP_TIER_CORE, "huangfeng_valley", "通往黄枫谷秘境的卷轴", 3}),
  // -- 秘密 --
  ({"hfg_swd3", "大衍神剑", "weapon", "/clone/weapon/yitianjian", 500000, REP_LEVEL_ADORE, 20000, SHOP_TIER_SECRET, "huangfeng_valley", "大衍神君传承飞剑", 1}),
  ({"hfg_item3", "古传送阵秘钥", "item", "/d/yueguo/obj/treasure", 300000, REP_LEVEL_ADORE, 10000, SHOP_TIER_SECRET, "huangfeng_valley", "黄枫谷古传送阵通行密钥", 1}),

  // ===================== 掩月宗 =====================
  ({"yym_hp1", "月华草", "pill", "/clone/pill/yuehua_cao", 100, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "yanyue_sect", "掩月宗特产疗伤灵草", -1}),
  ({"yym_pill1", "月华丹", "pill", "/clone/pill/yuehua_dan", 5000, REP_LEVEL_FRIENDLY, 100, SHOP_TIER_INTERMEDIATE, "yanyue_sect", "掩月宗月华秘制丹药", 30}),
  ({"yym_swd1", "月华刃", "weapon", "/clone/weapon/blade/wandao", 40000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "yanyue_sect", "蕴含月华之力的法器", 8}),
  ({"yym_arm1", "月影法袍", "armor", "/clone/armor/pifeng", 60000, REP_LEVEL_TRUST, 800, SHOP_TIER_ADVANCED, "yanyue_sect", "可融入月影的隐匿法袍", 5}),

  // ===================== 灵兽山 =====================
  ({"lsm_hp1", "灵兽丸", "pill", "/clone/pill/shouwan", 150, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "lingshou_mountain", "驯兽用灵丹", -1}),
  ({"lsm_item1", "御兽环", "item", "/d/yueguo/obj/treasure", 6000, REP_LEVEL_FRIENDLY, 150, SHOP_TIER_INTERMEDIATE, "lingshou_mountain", "控制妖兽的法器", 20}),
  ({"lsm_arm1", "灵兽甲", "armor", "/clone/armor/pifeng", 35000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "lingshou_mountain", "以妖兽皮所制法袍", 8}),
  ({"lsm_pill1", "兽王丹", "pill", "/clone/pill/shouwang", 50000, REP_LEVEL_TRUST, 800, SHOP_TIER_ADVANCED, "lingshou_mountain", "大幅提升灵兽战力", 5}),

  // ===================== 星宫 =====================
  ({"spa_hp1", "星辰丹", "pill", "/clone/pill/xingchen", 200, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "star_palace", "引星辰之力炼制的灵丹", -1}),
  ({"spa_item1", "星罗盘", "item", "/d/yueguo/obj/treasure", 8000, REP_LEVEL_FRIENDLY, 200, SHOP_TIER_INTERMEDIATE, "star_palace", "乱星海导航法器", 15}),
  ({"spa_swd1", "星辰剑", "weapon", "/clone/weapon/sword/qingfeng", 50000, REP_LEVEL_TRUST, 800, SHOP_TIER_ADVANCED, "star_palace", "引星辰之力的飞剑", 5}),
  ({"spa_item2", "星核碎片", "item", "/d/yueguo/obj/treasure", 100000, REP_LEVEL_ADORE, 8000, SHOP_TIER_SECRET, "star_palace", "星辰核心的碎片，蕴含恐怖能量", 2}),

  // ===================== 逆星盟 =====================
  ({"ram_pill1", "匿息丹", "pill", "/clone/pill/nixi", 250, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "rebel_alliance", "隐藏气息的丹药", -1}),
  ({"ram_swd1", "暗影匕", "weapon", "/clone/weapon/bishou", 45000, REP_LEVEL_TRUST, 600, SHOP_TIER_ADVANCED, "rebel_alliance", "逆星盟刺客专用匕首", 5}),

  // ===================== 广源斋 =====================
  ({"gyp_item4", "虚空珠", "item", "/d/yueguo/obj/treasure", 300000, REP_LEVEL_RESPECT, 5000, SHOP_TIER_CORE, "guangyuan_pavilion", "可跨界传送的稀世珍宝", 2}),

  // ===================== 天渊城 =====================
  ({"tyc_hp1", "军需丹", "pill", "/clone/pill/junxu", 300, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "tianyuan_city", "天渊城制式军用丹药", -1}),
  ({"tyc_arm1", "天渊战甲", "armor", "/clone/armor/tiejia", 25000, REP_LEVEL_FRIENDLY, 300, SHOP_TIER_INTERMEDIATE, "tianyuan_city", "天渊城制式战甲", 20}),
  ({"tyc_swd1", "灭魔剑", "weapon", "/clone/weapon/gangjian", 60000, REP_LEVEL_TRUST, 1000, SHOP_TIER_ADVANCED, "tianyuan_city", "克制魔族的法器", 5}),

  // ===================== 九国盟 =====================
  ({"ngm_pill1", "行军丹", "pill", "/clone/pill/xingjun", 200, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "nine_nations_alliance", "九国盟军用丹药", -1}),
  ({"ngm_swd1", "九国制式长刀", "weapon", "/clone/weapon/gangdao", 35000, REP_LEVEL_TRUST, 500, SHOP_TIER_ADVANCED, "nine_nations_alliance", "九国盟制式法器", 10}),

  // ===================== 慕兰法士 =====================
  ({"mlf_pill1", "法士灵液", "pill", "/clone/pill/lingye", 300, REP_LEVEL_NEUTRAL, 0, SHOP_TIER_BASIC, "mulan_legalists", "慕兰法士秘制灵液", -1}),
});

// 运行时库存（用于有限物品跟踪）
nosave mapping runtime_stock = ([]);

void create()
{
    seteuid(getuid());
    restore();

    // 初始化运行时库存
    if (sizeof(runtime_stock) == 0)
        init_stock();
}

void init_stock()
{
    runtime_stock = ([]);
    for (int i = 0; i < sizeof(item_catalog); i++)
    {
        mixed *item = item_catalog[i];
        string id = item[0];
        int stock = item[10]; // stock field
        runtime_stock[id] = stock;
    }
}

// ======== 查询接口 ========

// 获取指定势力指定层级的物品列表
mixed *query_tier_items(string faction, int tier)
{
    mixed *result = ({});
    for (int i = 0; i < sizeof(item_catalog); i++)
    {
        mixed *item = item_catalog[i];
        if (item[8] == faction && item[7] == tier)
            result += ({ item });
    }
    return result;
}

// 获取玩家可查看的所有物品（按声望等级筛选）
mixed *query_available_items(object player, string faction)
{
    int level = REPUTATION_D->query_reputation_level(player, faction);
    mixed *result = ({});

    for (int i = 0; i < sizeof(item_catalog); i++)
    {
        mixed *item = item_catalog[i];
        if (item[8] != faction) continue;
        if (!file_exists(item[3] + ".c")) continue;   // 源文件不存在的条目不展示
        if (level >= item[5])   // 声望等级满足
            result += ({ item });
    }
    return result;
}

// 按物品ID查找
mixed *get_item_by_id(string item_id)
{
    for (int i = 0; i < sizeof(item_catalog); i++)
    {
        if (item_catalog[i][0] == item_id)
            return item_catalog[i];
    }
    return 0;
}

// 获取物品当前库存
int query_stock(string item_id)
{
    if (runtime_stock[item_id])
        return runtime_stock[item_id];
    // 不在运行时库存中的视为无限
    return -1;
}

// ======== 购买逻辑 ========

// 尝试购买物品
// 返回值: 1=成功, -1=声望等级不足, -2=声望值不足, -3=灵石不足, -4=库存不足, -5=死敌无法交易,
//         -6=对象生成失败(商品数据异常,不扣费), -7=对象生成成功但交付失败(整体回滚,无净扣费)
varargs int buy_item(object player, string item_id, int quantity)
{
    mixed *item = get_item_by_id(item_id);
    if (!item) return 0;

    if (quantity < 1) quantity = 1;

    string faction = item[8];
    string item_name = item[1];
    int price = item[4];
    int req_level = item[5];
    int rep_cost = item[6];
    int stock = query_stock(item_id);

    // 1. 检查声望等级
    int level = REPUTATION_D->query_reputation_level(player, faction);
    if (level < req_level) return -1;

    // 2. 检查死敌
    float discount = REPUTATION_D->query_discount(faction, player);
    if (discount < 0) return -5;

    // 3. 检查库存
    if (stock != -1 && stock < quantity)
        return -4;

    // 4. 检查声望消耗
    int current_rep = player->query(REP_PATH_FACTION + "/" + faction);
    if (current_rep < rep_cost * quantity) return -2;

    // 5. 计算价格并检查灵石
    int final_price = to_int(price * discount) * quantity;
    int balance = player->query("balance");
    if (balance < final_price) return -3;

    // 6. 预生成对象：生成失败则不扣费、不交付（-6）
    object *obs = allocate(quantity);
    for (int i = 0; i < quantity; i++)
    {
        obs[i] = new(item[3]);
        if (!obs[i])
        {
            for (int j = 0; j < i; j++)
                destruct(obs[j]);
            return -6;
        }
    }

    // 7. 执行购买
    // 扣声望
    if (rep_cost > 0)
        REPUTATION_D->deduct_reputation(player, faction, rep_cost * quantity);

    // 扣灵石
    player->add("balance", -final_price);

    // 扣库存
    if (stock != -1)
        runtime_stock[item_id] -= quantity;

    // 8. 交付给玩家：参数化法宝基类（如 /d/yueguo/obj/treasure）的
    //    名称/ID/价值由商品目录补上；交付失败则整体回滚扣减（-7，无净扣费）
    for (int i = 0; i < quantity; i++)
    {
        if (!stringp(obs[i]->query("name")) || !obs[i]->query("name"))
            obs[i]->set_name(item[1], ({ item[0] }));
        if (!obs[i]->query("value"))
            obs[i]->set("value", price);
        if (!obs[i]->move(player))
        {
            for (int j = 0; j < quantity; j++)
                destruct(obs[j]);
            if (rep_cost > 0)
                REPUTATION_D->refund_reputation(player, faction, rep_cost * quantity,
                        sprintf("商店购买失败回滚 %s x%d", item_name, quantity));
            player->add("balance", final_price);
            if (stock != -1)
                runtime_stock[item_id] += quantity;
            return -7;
        }
    }

    // 记录最后互动
    player->set(REP_PATH_LAST_INTERACT + "/" + faction, time());

    // 更新购买统计
    player->add("reputation/purchase_count/" + faction, quantity);

    return 1;
}

// ======== 管理接口 ========

// 补货
void restock_item(string item_id, int amount)
{
    if (runtime_stock[item_id] && runtime_stock[item_id] != -1)
    {
        runtime_stock[item_id] += amount;
        save();
    }
}

// 批量补货
void restock_all()
{
    init_stock();
    save();
}

// 获取物品类型的中文名
string get_item_type_name(string type)
{
    switch (type)
    {
    case "weapon": return "武器";
    case "armor":  return "防具";
    case "pill":   return "丹药";
    case "skill":  return "功法";
    case "item":   return "物品";
    default:       return "未知";
    }
}

// 保存数据
string query_save_file()
{
    return "/data/shop_d";
}
