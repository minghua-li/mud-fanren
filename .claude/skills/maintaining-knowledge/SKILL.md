---
name: maintaining-knowledge
description: 维护 HAE 知识库 (document/knowledge/)；踩坑沉淀、文档失实修订、跨 issue 流程沉淀、用户明示捕获时使用。判断（何时、何 cluster、何 tag、是否升 CLAUDE.md）；执行（调 tools/kb 命令）；不手写 YAML。
---

# Maintaining Knowledge

HAE 知识库 (`document/knowledge/`) 维护 skill。判断 + 执行两步走，把 KB 视为可检索深度而非"写完就忘"。

## When 触发条件

满足以下任一即应调用本 skill：

- 踩到一个非 obvious 坑，预期会再发生（"这个我下次还会忘"）
- 发现 KB 已有 entry 与代码现实不符（架构变了 / API 改了 / bug 修了）
- 完成一个跨 issue 的新流程（PR 流程、测试约定、新分发模式）
- 用户明示"把这个记下来"

不触发的情况：
- 任务本身已有现成 entry 完整覆盖（先 `kb search` 确认）
- 单次性、不预期复发的内部修复（commit message 已足够）
- 代码注释能讲清的，不必单开 entry

## How 标准执行链路

### Step 1: 先搜，避免重复

```bash
python3 -m tools.kb search <topic-keywords> --tag <相关 tag>
```

- 命中现有 entry → 走「更新」分支（下面 Step 2a）
- 未命中 → 走「新增」分支（Step 2b）

按 tag 搜更精准。tag 列表：`python3 -m tools.kb tags`。

### Step 2a: 更新已有 entry

按变化类型选命令（不要 Read 全文件改 YAML——用增量命令）：

| 场景 | 命令 |
|---|---|
| 加 / 删 tag | `kb tag <id> +新tag -旧tag` |
| 双向链接 | `kb relate <id1> <id2>` 或 `--unlink` |
| 人工核验完 bump 日期 | `kb verify <id>` |
| 追加新一节正文 | `echo "..." \| kb append <id> --section "新发现"` |
| 事实被新实现取代 | `kb supersede <new> <old>` |
| 整段改写正文 | 直接 Edit 工具改 .md（仍是合法 markdown） |

### Step 2b: 新增 entry

```bash
# 1. scaffold 模板（自动生成 frontmatter + Why / How to apply 两段）
python3 -m tools.kb new <kebab-case-id> --cluster <cluster-name>

# 2. 编辑生成的文件，填：
#    - claim: 单句、含具体标识符（方法/字段/文件名）
#    - tags: 必须在 conventions.md 受控词表内（`kb tags` 查）
#    - modules: 必须在受控词表内（`kb modules` 查）
#    - 正文 Why（为什么记） + How to apply（怎么用）
```

### Step 3: 决定是否升 CLAUDE.md

如同时满足四个条件，propose 加入 CLAUDE.md：

- 跨子系统（≥3 个领域都要知道）
- 不知道就出 bug（不是"知道更好"）
- 半年稳定（不依赖某个 issue 进度）
- 一行说清

不满足任一 → 留在 KB 即可。

### Step 4: Commit

```bash
git add document/knowledge/<your-changes>
git commit -m "docs(kb): ..."
```

pre-commit hook 自动跑 `kb validate` 校验 schema + 重生成 INDEX-*.md。

如 hook 失败 → 修 frontmatter 错误再 commit，不要 `--no-verify` 绕。

## Tool reference 命令速查

15 个 subcommand 速查表（详见 `python3 -m tools.kb <cmd> --help`）：

| 命令 | 用途 |
|---|---|
| `kb search <terms>` | 多轴检索（tag/module/kind/status/in/case/verified-after/follow-related/limit/sort/count/format）|
| `kb show <id>` | 输出 entry frontmatter + claim |
| `kb new <id> --cluster <c>` | scaffold 新 entry |
| `kb validate` | schema / 词表 / 死链 / 重复 id 校验 |
| `kb gen-indexes` | 生成 5 个多视图索引（hook 自动跑）|
| `kb stale [--months N]` | 列超期 entry（综合 verify_freq）|
| `kb tags [--unused\|--orphan]` | 列受控 tag 词表 + 命中数 |
| `kb modules [--unused\|--orphan]` | 列受控 module 词表 + 命中数 |
| `kb graph <id> [--depth N] [--format dot\|ascii]` | 关系图（related + supersedes 双向追溯）|
| `kb similar <id> [--limit N]` | 找相似 entry（tag/module 交集 + 正文 TF）|
| `kb supersede <old> <new>` | 建立 supersession 双向链 |
| `kb tag <id> +X -Y` | 原子增删 tag |
| `kb relate <id1> <id2> [--unlink]` | 双向 related 链接维护 |
| `kb verify <id> [--date YYYY-MM-DD]` | bump verified 日期 |
| `kb append <id> --section <h>` | 给正文追加 section（stdin 读内容）|

## Style rules

详见 `document/knowledge/conventions.md`：

- **claim**：单句、事实在前、含具体标识符、不引用 issue # 作主体、60-150 字符
- **tags**：必在受控词表内；新 tag 须先加入 conventions.md（单独 PR）
- **modules**：必在受控词表内
- **kind**：枚举 7 选 1（pitfall / architecture / pattern / invariant / policy / environment / testing）
- **正文结构**：Why（背景动机）+ How to apply（怎么用），段落清晰

## Don'ts 防膨胀边界

- 不手写 YAML 改 frontmatter（用 `kb tag/relate/verify/append` 等命令，避免格式错）
- 不在 skill 内决定 cluster / tag 词表（由 conventions.md 收敛）
- 不修代码（只动 `document/knowledge/` 文件 + commit）
- 不大规模 audit（> 30 条改动时，本 skill 内部 dispatch 一个 subagent，不在主流程跑）
- 不跳过 hook（hook 失败修问题再 commit，不 --no-verify 绕）

## Refs

- Schema + 词表真相源：`document/knowledge/conventions.md`
- 工具：`tools/kb/`（15 subcommand）
- 设计稿：`docs/superpowers/specs/2026-05-12-knowledge-base-restructure-design.md`
- 实施 plan：`docs/superpowers/plans/2026-05-12-kb-restructure-phase1.md`
