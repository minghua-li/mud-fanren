---
claim: '法宝炼制链路由 FORGE_D（adm/daemons/forge_d.c）配方驱动承载，成品由通用基类 d/yueguo/obj/treasure.c（继承
  EQUIP）运行时生成，境界限制走 SECT_D->query_cultivation_tier，设施加成接 SECT_FACILITY_D->query_forge_bonus，材料供给接
  #67 坊市 goods 表与 ECONOMY_D 定价'
cluster: architecture
id: forge-crafting-system
kind: pattern
modules:
- adm-daemons
- cmds
- include
- d-areas
related:
- sect-facility-system
status: current
tags:
- daemon
- dbase
- command
- mapping
- lpc-syntax
- pitfall
verified: '2026-08-13'
---

## Why

#74 落地法宝炼制链路（P5-4 1E §1.5 落地）时确立的架构约定：炼制系统不把逻辑散落在房间/NPC 里，而是由配置驱动的守护进程承载——FORGE_D（adm/daemons/forge_d.c）的 forge_formula 声明全部配方（材料组合/境界/炼器术/成功率），玩家在化刀坞炼器工坊（#60 设施房间，FORGE_ROOM）执行 lianqi 命令，逻辑全部走 FORGE_D。法宝成品统一由通用基类 /d/yueguo/obj/treasure.c（继承 EQUIP=ITEM+F_EQUIP）运行时生成——新增配方只加一条配置，命令层/实体层零改动。核心接缝：境界限制必须走 #61 修炼系统交付的 SECT_D->query_cultivation_tier（tier=境界索引*3+小阶段，SECT_TIER_* 常量在 include/sect.h）；设施加成消费 #60 交付的 SECT_FACILITY_D->query_forge_bonus（该钩子此前全库零调用方，本票为首个消费者）；材料供给接 #67 坊市（fangshi.c goods 表 + ECONOMY_D register_goods 定价，1 灵石=100 文）。

## How to apply

- 配方配置在 `adm/daemons/forge_d.c` 的 `forge_formula` mapping：每条含 name/desc/treasure_type（法器/法宝/古宝/通天灵宝/玄天之宝，1E §1.1）/ element（金木水火土风雷空间时间，1E §1.3）/ forbidden_count（禁制基数）/ attack+defense（**只能单侧非零**——F_EQUIP 的 wear() 会拒绝带 weapon_prop 的对象，攻防兼备法宝无法同时 wield+wear）/ special / require_tier（SECT_TIER_*，1E §1.1 适用境界）/ materials（材料 id→数量，材料对象 material_id 对齐）/ min_skill（炼器术等级，02 图鉴 §4.3：法器≥10、法宝≥50）/ base_rate（基准成功率 %）/ value（法宝价值，文）。新增配方=加一条配置，无需改命令/实体。
- 炼制流程（1E §1.5 五步）：材料采集为确定性步骤（背包材料齐备即通过，按 material_id 遍历 all_inventory 计数、逐对象 destruct 扣减——材料不堆叠、每对象一份）；工坊内四步（精炼提纯/器胚锻造/禁制铭刻/通灵开光，FORGE_STEPS）每步独立判定，步概率=综合成功率^(1/4)（pow 几何均摊），合成概率即综合成功率。综合成功率 = base_rate + 炼器术等级/2（每 2 级 +1%）+ query_forge_bonus（百分比直接加），钳制 [1,99]（FORGE_MIN_RATE/MAX_RATE，对齐 #61 突破概率钳制口径）。
- 品质判定（FORGE_QUALITY_* 常量在 include/forge.h）：roll_quality(rate) 按 roll<rate*25%/50%/80% 分极品(×1.5)/上品(×1.3)/中品(×1.15)/下品(×1.0)，倍率乘算 attack/defense（to_int 取整），禁制层数=基数+FORGE_QUALITY_BAN[品质]。法宝名=配方名+「（品质）」，id=配方 id（+fabao/treasure 通用别名），wield/wear 用 id 即可。
- 法宝基类 `d/yueguo/obj/treasure.c`：继承 EQUIP（获得/持有走 F_MOVE 背包、装备走 F_EQUIP、交易走 value）；setup() 按 attack/defense 单侧生成 weapon_prop（wield 御器）或 armor_prop（wear 护身）+ 装备消息；**override wield()/wear() 先做境界检查**（SECT_D->query_cultivation_tier(owner) < query("require_level") → notify_fail 拒绝，然后 ::wield()/::wear() 调父类——`::` 调用父类实现有先例 kungfu/skill/huagong-dafa.c）；extra_long() 动态展示品阶/属性/禁制/境界需求/特殊效果（F_NAME long() 自动追加）。
- 命令 `cmds/usr/lianqi.c`：`lianqi list` 任意位置可用（FORGE_D->describe_formula_list，配方 id+材料+技能+境界+成功率）；`lianqi <配方id|配方名>` 须在炼器工坊内（场所检查 SECT_FACILITY_D->query_current_facility(me) == "huadao_lianqi"）；化刀坞弟子且无 forge buff 时自动 SECT_FACILITY_D->use_facility 激活设施加成（耗灵石/贡献），非本派弟子无加成也可炼（base_rate）。配方名/配方 id 双向解析在 FORGE_D->resolve_formula（对齐 #60 resolve_seed_id 先例）。
- 材料来源：d/yueguo/tainan/obj/ 下每材料一文件（inherit ITEM + is_material + material_id + value），对齐 #67 tiejing.c 先例；坊市 fangshi.c goods 表加条目 + create() 里 ECONOMY_D->register_goods(<类型>, <基准灵石价>, <周转量>) 注册经济定价（类型如 ore_silver/ore_gold/ore_xuantie/ore_gengjing，herb_huanglong 先例为自定义类型合法）。材料稀有度↔价格：铁精2/银精5/金精15/玄铁30/庚精80 灵石（1E §1.5 材料表）。
- 验证（可复跑）：`python3 tools/check/forge_verify.py`（148 断言 exit 0：括号状态机/接口签名/配方字段+攻防单侧/材料↔坊市引用完整性/register_goods 全覆盖/FORGE_D 宏/help 前向原型 + LPC 原文守卫（forge 函数体：境界/技能/材料检查、扣减、query_success_rate 接线、FORGE_STEPS、roll_quality、new treasure、tier_name；treasure.check_realm；lianqi 场所检查）+ 同构模拟 8 场景（材料不足不扣/境界拒绝/技能拒绝/成功属性齐全/品质倍率/失败耗材/端到端装备境界/1E 数据口径）+ 4 组突变实证（删境界检查/场所改指/删材料扣减→守卫红）。**守卫匹配字符串字面量必须用保留字符串的函数体提取器**（strip_lpc 会剥掉 "lianqi-shu"/"huadao_lianqi" 使守卫失配——本票踩坑）。模拟脚本解析 LPC mapping 字面量时：行首缩进去平（eval 顶层不容缩进）、`__DIR__"obj/x"` 拼接先替换成纯字符串、宏名（SECT_TIER_*）提供 namespace 值映射；LPC `([` 转 Python `{` 时先整体捕获含外括号的块再转换（只取内文会丢 dict 边界）。
- 环境边界：无 fluffos driver 无法运行时编译/运行（与 #57/#60/#61 同）；lpcc 与项目 UTF-8 不兼容（#67 实证）。法宝 `::` 父类调用与 pow float 运算均为 FluffOS v2019 支持语法（既有先例 toptend.c/mdfived.c），但运行时行为仍需人工装驱动复验。

## 档位与 move 判定（审查第 1 轮修订）

基准成功率档位（02 图鉴 §4.3，审查第 1 轮对齐）：法器 75%、法宝 35%——青钢剑/赤铜盾 base_rate=75，玄铁重剑/庚精飞剑=35；实际综合成功率 = base + 炼器术/2 + 工坊加成，故高级炼器术（≥50 级）炼制法宝仍有可观成功率。另：F_MOVE->move 失败返回 notify_fail 字符串（truthy）而非 0，成品 move 判断必须用 != 1（背包满落地面分支），!move() 会漏判致法宝悬空丢失。
