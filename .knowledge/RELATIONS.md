# 知识点关联边表

格式：`<源slug> -> <目标slug> : <关联理由>`

## 核心关联

WORLD_DESIGN -> CULTIVATION_SYSTEM : 世界格局决定了修仙体系的边界条件（人界灵气稀薄限制化神以上存在）
WORLD_DESIGN -> FACTIONS : 不同地理区域存在不同的势力分布
FACTIONS -> CHARACTERS : 每个势力下有对应的核心角色
CULTIVATION_SYSTEM -> COMBAT_SYSTEM : 功法/境界差异直接映射为战斗力的数值与机制
CULTIVATION_SYSTEM -> ITEMS_AND_ECONOMY : 境界越高可接触和使用的高阶物品越多
ITEMS_AND_ECONOMY -> QUESTS : 副本/奇遇产出是物品的主要来源
ITEMS_AND_ECONOMY -> CULTIVATION_SYSTEM : 灵石/丹药直接支撑修炼进程
COMBAT_SYSTEM -> CHARACTERS : 角色的战斗风格由其所学功法决定
QUESTS -> WORLD_DESIGN : 任务链按剧本的行历路线设计
QUESTS -> CULTIVATION_SYSTEM : 任务以境界门槛划分

## 跨界关联

CHARACTERS -> CULTIVATION_SYSTEM : 韩立是唯一完整遍历全部境界的角色
COMBAT_SYSTEM -> ITEMS_AND_ECONOMY : 阵法/傀儡/符箓等都是可制造消耗品
FACTIONS -> COMBAT_SYSTEM : 阵营战争是后期最大的战斗场景
ITEMS_AND_ECONOMY -> FACTIONS : 经济资源分配影响势力间的博弈
QUESTS -> FACTIONS : 许多任务依赖特定势力声望解锁
1C-修仙境界功法 -> CULTIVATION_SYSTEM : 子任务文档，以剧本原文为据补充境界/灵根/功法的详细数值与机制设计
1C-修仙境界功法 -> ITEMS_AND_ECONOMY : 境界突破依赖丹药体系（筑基丹/结金丹/补天丹等）
1C-修仙境界功法 -> COMBAT_SYSTEM : 功法层数与神通直接影响战斗数值
WORLD_DESIGN -> QUESTS : 界面结构（人界→灵界→仙界）决定了任务链的阶段性分段
1A-人界地理 -> WORLD_DESIGN : 人界地理设计的详细展开，覆盖了 WORLD_DESIGN 中人界部分的细化
1A-人界地理 -> FACTIONS : 每个地理区域都有对应的势力分布
1A-人界地理 -> QUESTS : 区域连接通路和进入门槛决定了任务线的推进路线
1D-门派种族声望 -> FACTIONS : 对 FACTIONS 中势力的详细展开，补充魔道六宗/九国盟/大晋势力/妖兽体系等细节
1D-门派种族声望 -> WORLD_DESIGN : 各势力的地理分布关联三界结构
1D-门派种族声望 -> CHARACTERS : 各角色的势力归属与声望关系
1D-门派种族声望 -> COMBAT_SYSTEM : 势力战争(正魔之战/法士战争/界面战争)是核心战斗场景
1D-门派种族声望 -> QUESTS : 声望驱动的任务解锁逻辑
1G-任务副本奇遇 -> QUESTS : 1G 覆盖并扩展了 QUESTS 的内容，是 QUESTS 的超集
1G-任务副本奇遇 -> CULTIVATION_SYSTEM : 任务以境界门槛划分，奖励曲线对齐修炼体系
1G-任务副本奇遇 -> 1C-修仙境界功法 : 任务难度阶梯参照1C的详细境界数值
1G-任务副本奇遇 -> WORLD_DESIGN : 任务链按行历路线设计，副本入口映射到具体区域
1G-任务副本奇遇 -> FACTIONS : 势力声望驱动专属任务链解锁
1G-任务副本奇遇 -> CHARACTERS : 剧情NPC对话触发任务/奇遇
1G-任务副本奇遇 -> ITEMS_AND_ECONOMY : 副本/奇遇产出是高级物品的主要来源
1G-任务副本奇遇 -> COMBAT_SYSTEM : 副本怪物AI与技能配置引用战斗体系
1G-任务副本奇遇 -> 1A-人界地理 : 人界区域连接通路决定任务推进路线
