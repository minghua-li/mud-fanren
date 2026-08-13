# tools/llm —— 自然语言 → MUD 指令解析（Phase 0 外挂网关 + Phase 1 sidecar）

把「自然语言」翻译成 MUD 指令并执行的两代方案：

- **Phase 0（gateway.py）**：外挂式 PoC 网关，**零驱动改动**，telnet 连接游戏
  5555/6666 端口，拦截 `ai ` 前缀输入走 LLM 解析后回写连接。
- **Phase 1（sidecar.py + LPC llmd.c）**：解析层下沉到驱动内 daemon
  （`adm/daemons/llmd.c` + 玩家命令 `cmds/usr/ai.c`），sidecar 只做本地
  TCP 翻译器，复用本目录的 llm_client / safety / mock_llm。

对应 RFC #68、实施票 #69（Phase 0）、#70（Phase 1）。

## 它能做什么

```
ai 去当铺把金条当了
```

在游戏里输入（Phase 1，需先 `set llm on` 并开启全局开关）→
LPC 采集房间/背包 grounding → TCP 发给 sidecar → sidecar 调 LLM（或 mock）
解析为指令序列 → safety 过滤 → LPC 异步注入执行，例如：

```
你正在理解你的意图，请稍候……
AI 意图解析：去当铺典当金条
（自动执行 go east → go north → list → pawn gold）
```

Phase 0 网关模式下为终端内回写（见下方说明）。

## Phase 1：sidecar（当前推荐形态）

### 启动

```bash
# 真实 LLM 模式（需先配置密钥，见「配置你的 LLM API」）
python -m tools.llm.sidecar --port 37777

# 无 key 演示/自测
python -m tools.llm.sidecar --port 37777 --mock
```

- 只监听 `127.0.0.1`（绝不 0.0.0.0）；端口默认 37777，与 `include/globals.h`
  的 `LLM_SIDECAR_PORT` 一致。
- **每请求一连接、新行分隔 JSON**：LPC 侧发一行请求、收一行响应。
- LLM 调用超时默认 12s（`--timeout`），**必须短于 LPC 侧 watchdog 15s**。

### 协议契约

请求（LPC → sidecar，单行 JSON）：

```json
{"player": "hanli", "language": "去当铺把金条当了",
 "grounding": {"room_short": "青牛镇", "room_long": "...", "exits": ["east"],
               "objects": ["小贩"], "inventory": ["金条"], "realm": "炼气1层"}}
```

响应（sidecar → LPC，单行 JSON）：

```json
{"commands": ["go east", "go north", "list", "pawn gold"],
 "confirm": [], "blocked": [], "reason": "去当铺典当金条", "error": null}
```

- `commands`：可直接注入的安全指令（白名单已过滤）
- `confirm`：高危指令（仅 DANGEROUS 动词），由 LPC 侧请玩家 `ai confirm yes/no`
  二次确认后执行
- `blocked`：被拦截条目（管理命令/非白名单/超限/格式非法）
- `error`：非空表示解析失败（LLM 失败/超时/未配 key），`commands` 必为空

### 游戏内使用（Phase 1）

1. 游戏侧需开启全局开关（管理员）：`call /adm/daemons/llmd set_llm_enabled 1`
2. 玩家 `set llm on`（opt-in，复用 set 命令 env 机制）
3. `ai <自然语言>` 发起请求；`ai confirm yes/no` 处理高危确认；`ai help` 看帮助
4. 新命令需 `rehash /cmds/usr` 或重启驱动后才可见

## Phase 0：外挂网关（PoC，已收尾 #69）

```bash
python -m tools.llm.gateway --port 5555 --mock   # 或 --port 6666
```

连 5555（GBK）/ 6666（UTF-8）telnet 端口，`ai ` 前缀输入被拦截解析后
逐条回写连接执行，其余输入原样透传。适合体验 prompt 效果与验证流程。

## 配置你的 LLM API（在你自己主机上本地配置，密钥绝不进代码库）

**方式 A：环境变量**（推荐）

```bash
export LLM_API_BASE="https://api.openai.com/v1"     # 或本地 ollama: http://localhost:11434/v1
export LLM_API_KEY="sk-你的密钥"
export LLM_MODEL="gpt-4o-mini"
```

**方式 B：本地配置文件**（git 之外，自己手写）

创建 `~/.config/fanren-mud/llm.json`：

```json
{
  "api_base": "https://api.openai.com/v1",
  "api_key": "sk-你的密钥",
  "model": "gpt-4o-mini"
}
```

> 网关/sidecar 只从环境变量 / 本地配置文件读取，代码库与提交中不含任何密钥。
> 本地 API（如 ollama，`http://localhost:11434/v1`）不需要密钥：
> 只配置 `LLM_API_BASE` 即可，会把「自定义了 API 地址」视为已配置。

## 失败安全（验收 c3 / #70 六条）

- **动词白名单**：只有白名单内安全动词（go/look/list/pawn/buy/get/say 等）
  会被执行；其余动词（含 wiz/admin/管理调试命令）一律拒绝。
- **高危意图二次确认**：kill/hit/fight/steal/give/drop/quit 等出现在 LLM 输出
  中即被拦截，绝不自动执行——玩家 `ai confirm yes` 显式确认才注入；
  管理命令（shutdown/exec/update 等）无论确认与否都绝对拒绝。
- **指令条数上限**：单次最多 8 条，防多命令连续执行失控。
- **格式校验**：每条指令只允许 `[a-z][a-z0-9_-]` 字符，杜绝 `;`、`|`、`$()` 等注入。
- **超时零注入**：LPC 侧 watchdog 15s 超时（sidecar 内 LLM 调用 12s 超时先行），
  超时/连接失败/响应非法一律不注入任何指令。
- **LLM 异常降级**：调用失败 / 返回非 JSON / 未配置密钥 → 安全中止，零指令注入。
- **LPC 侧白名单防线**：即使 sidecar 被替换/配置错误，llmd.c 也只注入
  白名单指令（动词表与 safety.py 保持一致，跨语言复制）。

实现见 `safety.py`（#69 资产）+ `sidecar.py` 的 `filter_payload` + `llmd.c` 的
`valid_inject_cmd`；全部行为有 `selftest.py`（#69）+ `sidecar_selftest.py`（#70）
实测覆盖。

## 自测

```bash
python tools/llm/selftest.py          # #69：网关 85 断言
python tools/llm/sidecar_selftest.py  # #70：sidecar+llmd+ai 87 断言
```

不依赖真实游戏服务与真实 LLM API（fake MUD 服务器 / fake LPC 客户端 +
mock/注入替身），可重复运行，退出码 0 = 全绿。#70 自测覆盖：

- c1 sidecar：解析（mock 当铺链路）/ 安全拦截（LLM 替身输出危险指令）/ 超时（慢 LLM）
- c2 llmd.c：TCP 每请求一连接、watchdog 15s 超时零注入（LPC 逻辑忠实翻译模拟）
- c3 开关：默认关闭 + set llm on opt-in（静态校验）
- c4 ai.c 集成链路（请求→sidecar→响应→注入；超时不注入半截指令）
- c5 端到端：sidecar 真实 TCP 服务 + fake LPC 客户端全链路
- c6 密钥仅本地配置、DRY 复用 #69

## 目录结构

| 文件 | 职责 |
|---|---|
| `gateway.py` | Phase 0 外挂网关：telnet 连接（含 IAC 协商）、`ai` 拦截、回写执行 |
| `sidecar.py` | Phase 1 sidecar：TCP 127.0.0.1 每请求一连接，prompt 组装 + LLM/mock + safety |
| `llm_client.py` | OpenAI 兼容 API 客户端（纯标准库 urllib）+ 本地配置读取（#69） |
| `safety.py` | 指令安全过滤：白名单 / 危险拦截 / 条数上限 / 格式校验（#69） |
| `mock_llm.py` | mock 解析器（无 API key 的演示/自测替身，#69） |
| `selftest.py` | #69 验收自测（85 断言） |
| `sidecar_selftest.py` | #70 验收自测（87 断言 + 4 组突变验证） |
| `README.md` | 本说明 |

## 已知限制（Phase 0/1 边界，见 #68 RFC）

- LLM 不知道地图，路线不确定时输出 `look` 由玩家补上下文后再次发起；
  Phase 1 会注入当前房间出口/可见之物/背包 grounding，降低猜路线概率。
- 游戏 6666 端口存在既有 GBK 乱码坑（`simul_efun/chinese.c` 的 gb_to_utf8 空操作，
  见知识库 `gb-to-utf8-noop`）——如遇乱码改用 5555 端口。
- Phase 1 的 LPC 侧（llmd.c/ai.c）因环境无 fluffos driver，采用静态验收 +
  Python 忠实翻译模拟验证；真实驱动端到端待驱动安装后复验（与 #61/#69 同边界）。
- `json_parse`/`json_encode` efun 本仓无使用先例（FluffOS v2019 内置），
  解析失败路径已按零注入兜底设计；真实 driver 上需复验一次。

