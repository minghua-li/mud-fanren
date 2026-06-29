# AGENTS.md

项目目标是参考北大侠客行的架构与实现方式，以 ./document/凡人修仙传.txt 为剧本，创建一个凡人修仙传MUD游戏。

## 项目概况

- 基于 **FluffOS v2019** 驱动的 **LPMUD**，
- 使用 **LPC** (LPMUD C) 语言，文件扩展名 `.c` / `.h`
- 约 22,000 个 `.c` 文件，150 万行代码
- 启动方式：`driver config.ini`（需安装 fluffos v2019）
- 三端口：5555(GBK), 6666(UTF-8), 8888(WebSocket)

## 技术栈

| 项目 | 说明 |
|---|---|
| 语言 | LPC (类 C 的脚本语言，解释执行) |
| 驱动 | FluffOS v2019 |
| 编码 | 项目文件为 UTF-8，运行时支持 GBK/UTF-8 websocket |
| 数据存储 | LPC mapping 序列化到文件（`/data/` 目录） |

## 架构关键概念

### 核心目录

| 目录 | 用途 |
|---|---|
| `adm/daemons/` | 守护进程（单例服务），如 COMBAT_D, CHANNEL_D, LOGIN_D |
| `adm/single/` | 全局单例：`master.c`（驱动入口）、`simul_efun.c`（全局函数） |
| `feature/` | Mixin 特性，以 `F_` 前缀命名，如 `F_DBASE`, `F_ATTACK` |
| `inherit/` | 可继承基类，如 `ROOM`, `NPC`, `CHARACTER`, `SKILL` |
| `include/` | 头文件，`globals.h` 会被驱动自动 include |
| `cmds/` | 命令，按权限分 `adm/` `arch/` `wiz/` `imm/` `usr/` `std/` |
| `kungfu/` | 武功系统：`skill/` 技能, `class/` 门派, `special/` 绝招 |
| `d/` | 游戏区域，含房间和 NPC |
| `clone/` | 可克隆对象模板（武器、盔甲、钱、食物等） |
| `quest/` | 任务系统 |

### 核心机制

1. **DBASE 属性系统**：通过 `set()`/`query()`/`add()`/`delete()` 操作对象的属性（mapping 存储），支持路径式访问如 `"apply/attack"`
2. **继承组合**：对象通过多重继承 `feature/` 获得能力（如 `inherit F_DBASE; inherit F_MOVE; inherit F_COMBAT;`）
3. **守护进程**：`/adm/daemons/` 下的单例，游戏全局服务
4. **宏驱动**：`/include/globals.h` 定义所有关键路径宏

## LPC 语法要点

### 类型
- `int`, `string`, `object`, `mapping`（映射/字典）, `mixed`（任意类型）
- 数组后缀 `*`，如 `string *list`, `object *inv`
- `nosave` 修饰变量表示不序列化到磁盘

### 常用模式
```lpc
// 创建对象
void create() { seteuid(getuid()); setup(); }

// 属性操作
set("name", "某物");
query("combat_exp");
add("qi", -10);

// 对象操作
object ob = new(PATH);
ob->move(this_object());
destruct(ob);

// 定时器
call_out("function_name", delay_in_seconds, arg);

// 消息
write("发给玩家的文字\n");
message_vision("$N做了一件事。\n", this_player());
tell_object(me, "私聊消息\n");

// 命令（在 init() 中注册）
add_action("do_enter", "enter");

// 类型检查
objectp(ob), stringp(str), mapp(map), functionp(fp)

// mapping 字面量
mapping m = ([ "key": "value", "key2": "value2" ]);

// 数组操作
string *arr = ({ "a", "b", "c" });
```

### 常用宏（来自 globals.h）
- `ROOM` → `/inherit/room/room`
- `NPC` → `/inherit/char/npc`
- `CHARACTER` → `/inherit/char/char`
- `__DIR__` → 当前文件所在目录

## 代码规范

1. **缩进**：使用空格（配置中 `warn tab: 0` 但应避免 tab）
2. **命名**：`snake_case`，路径用小写
3. **继承**：统一在文件顶部
4. **create()**：构造函数模式
5. **setup()**：后初始化

## 维护注意事项

1. **废弃文件多**：存在大量 `.old` `.bak` 和带时间戳后缀的备份文件，修改前确认目标文件
2. **全中文命名**：变量、注释、描述均为中文
3. **无测试覆盖**：修改后需人工验证
4. **全局可变状态**：守护进程持有全局 mapping，注意并发和状态一致性
5. **eval cost 限制**：配置 `maximum evaluation cost: 30000000`，避免死循环或超大运算
6. **不要删除无用存档文件**：除非明确要求，否则保留 `.old` `.bak` 等历史文件

## 知识库维护

本项目的知识库在 `document/knowledge/` 目录，由 `tools/kb/` Python 工具管理。

### 触发条件

满足以下任一即应记录知识条目：
- 踩到一个非 obvious 坑，预期会再发生
- 发现 KB 已有条目与代码现实不符（架构变了 / API 改了 / bug 修了）
- 完成一个新的跨模块流程
- 用户明示"把这个记下来"

### 操作命令

所有操作通过 `python -m tools.kb <子命令>` 执行，**禁止手写 YAML 改 frontmatter**。

| 场景 | 命令 |
|---|---|
| 新增条目 | `python -m tools.kb new <kebab-id> --cluster <cluster名>` |
| 搜索条目 | `python -m tools.kb search <关键词> --tag <tag> --module <module>` |
| 加/删 tag | `python -m tools.kb tag <id> +新tag -旧tag` |
| 关联条目 | `python -m tools.kb relate <id1> <id2>` |
| 标记过时 | `python -m tools.kb supersede <旧id> <新id>` |
| 追加正文 | `echo "内容" \| python -m tools.kb append <id> --section "新章节"` |
| 更新验证 | `python -m tools.kb verify <id>` |
| 校验全部 | `python -m tools.kb validate` |
| 重生成索引 | `python -m tools.kb gen-indexes` |
| 查超期条目 | `python -m tools.kb stale` |

### 规则

- tag/module/cluster/kind 必须使用 `conventions.md` 中的受控词表（`kb tags` / `kb modules` 可查）
- 新增受控词需先更新 `conventions.md`
- claim 须为单句事实、含具体标识符、60-150 字符
- 提交前 `kb validate` 会自动校验；hook 失败时修问题再 commit，不要 `--no-verify`
