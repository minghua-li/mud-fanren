# KB Conventions

北大侠客行 (pkuxkx-utf8) 知识库约定。所有 entry 必须遵守以下 schema 和受控词表。

## Schema

| 字段 | 必填 | 含义 |
|---|---|---|
| id | 是 | kebab-case slug，与文件名一致；正则 `^[a-z0-9][a-z0-9-]{2,60}$` |
| claim | 是 | 单句、事实在前、含具体标识符（方法/字段/文件名）；60-150 字符 |
| tags | 是 | array；受控词表见 ## Tags |
| modules | 是 | array；触及代码模块；受控词表见 ## Modules |
| cluster | 是 | 目录分组名；受控词表见 ## Clusters |
| kind | 是 | 性质；枚举见 ## Kinds |
| status | 是 | `current` / `superseded` / `historical` |
| verified | 是 | 最近人工核验日期；YAML 字符串 `"YYYY-MM-DD"`（需引号） |
| verify_freq | 否 | `monthly` / `quarterly` / `yearly` / `evergreen` |
| supersedes | 否 | 取代的旧 entry id list |
| related | 否 | 横向链接 id list |

## Tags

| tag | 触发场景 |
|---|---|
| inherit | 继承/多重继承模式 |
| dbase | 属性系统 set/query/add/delete |
| combat | 战斗系统 COMBAT_D |
| lpc-syntax | LPC 语法特性（varargs, nosave, mapping 等） |
| room | 房间创建/出口/重置 |
| npc | NPC 创建/行为/AI |
| skill | 武功/技能系统 |
| command | 玩家/巫师命令 |
| daemon | 守护进程单例 |
| init | init/add_action 命令注册 |
| move | 移动/行走系统 |
| message | 消息系统 tell/write/message_vision |
| data | 数据持久化/磁盘读寫 |
| mapping | mapping 操作技巧 |
| reset | reset/clean_up 生命周期 |
| callout | call_out 定时器/延迟调用 |
| player | 玩家对象相关 |
| quest | 任务系统 |
| setup | 环境搭建/配置 |
| pitfall | 常见踩坑 |

## Modules

| module | 说明 |
|---|---|
| adm-daemons | `/adm/daemons/` 守护进程 |
| adm-single | `/adm/single/` master + simul_efun |
| feature | `/feature/` F_ 特性 mixin |
| inherit | `/inherit/` 基类（ROOM/NPC/CHARACTER 等） |
| cmds | `/cmds/` 命令系统 |
| kungfu | `/kungfu/` 武功/门派/绝招 |
| d-areas | `/d/` 游戏区域 |
| clone | `/clone/` 可克隆对象 |
| quest | `/quest/` 任务系统 |
| include | `/include/` 头文件 |
| obj | `/obj/` 杂项对象 |
| tools-kb | `tools/kb/` 知识库工具自身 |

## Clusters

| cluster | 说明 |
|---|---|
| lpc | LPC 语言特性 |
| architecture | 系统架构决策 |
| combat | 战斗系统 |
| room | 房间/区域 |
| npc | NPC 系统 |
| skill | 武功系统 |
| dbase | DBASE 属性系统 |
| pitfall | 常见踩坑记录 |
| tool | 工具相关 |

## Kinds

| kind | 说明 |
|---|---|
| pitfall | 常见踩坑（"这个我下次还会忘"） |
| architecture | 架构决策与设计 |
| pattern | 推荐代码模式 |
| invariant | 必须遵守的不变约束 |
| policy | 约定/策略/规范 |
| environment | 环境/配置/搭建 |
| testing | 测试相关 |

## Claim 风格规则

- 单句、事实在前、含具体标识符（方法名/字段名/文件名）
- 不引用 issue # 作主体
- 60-150 字符
- 示例：`ROOM 的 create() 中必须在 setup() 之前调用 set("exits", ...)`

## Status 状态机

```
current ──→ superseded
  │
  └──→ historical
```

- `current`：当前有效
- `superseded`：被另一条 entry 取代（须填写 `supersedes`）
- `historical`：不再适用但保留记录

## Verify Freq 阈值

| verify_freq | 超期阈值 |
|---|---|
| monthly | 45 天 |
| quarterly | 4 月 |
| yearly | 13 月 |
| evergreen | 永不超期 |
| (未设置) | 6 月 |
