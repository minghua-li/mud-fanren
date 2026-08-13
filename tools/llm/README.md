# tools/llm —— 自然语言 → MUD 指令网关（Phase 0 外挂式 PoC）

把「自然语言」翻译成 MUD 指令并回写执行的外挂网关。**零驱动改动**，
纯 Python 脚本（纯标准库，无第三方依赖），telnet 连接游戏 5555/6666 端口。

对应 RFC #68 Phase 0、实施票 #69。

## 它能做什么

```
ai 去当铺把金条当了
```

网关拦截这条输入 → 调 LLM（或内置 mock）解析为指令序列 →
安全过滤 → 逐条回写连接执行，例如：

```
[网关] 意图解析：去当铺典当金条
[网关] 计划执行：
      1. go east
      2. go north
      3. list
      4. pawn gold
[网关] 逐条回写连接执行…
```

## 快速开始

### 1. 启动游戏服务

按项目既有方式启动（`driver config.ini`），游戏监听 5555（GBK）/ 6666（UTF-8）telnet 端口。

### 2. 配置你的 LLM API（在你自己主机上本地配置，密钥绝不进代码库）

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

> 网关只从环境变量 / 本地配置文件读取，代码库与提交中不含任何密钥。
> 不支持本地 API（如 ollama）时 `api_key` 可留空，网关仅凭 api_base 发起请求。

### 3. 运行网关

```bash
# 连 5555 端口（GBK 编码，自动转码）
python -m tools.llm.gateway --port 5555

# 连 6666 端口（UTF-8）
python -m tools.llm.gateway --port 6666

# 没有 API key 也想体验链路：mock 模式（内置规则解析，非真实 LLM）
python -m tools.llm.gateway --port 5555 --mock
```

### 4. 使用

- 先照常登录游戏（输入 id/密码等，网关原样透传）。
- 普通输入直接透传给游戏。
- 以 `ai ` 开头的输入被拦截并走 LLM 解析。
- `ai help` 看帮助；`/quit` 退出网关（**不**向游戏发送 quit）。

## 失败安全（验收 c3）

- **动词白名单**：只有白名单内安全动词（go/look/list/pawn/buy/get/say 等）
  会被回写；其余动词（含 wiz/admin/管理调试命令）一律拒绝。
- **高危意图拦截**：kill/hit/fight/steal/give/drop/quit 等出现在 LLM 输出中
  即被拦截，绝不注入连接（`--allow-confirm` 可强制放行，PoC 不推荐）。
- **指令条数上限**：单次最多 8 条，防多命令连续执行失控。
- **格式校验**：每条指令只允许 `[a-z][a-z0-9_-]` 字符，杜绝 `;`、`|`、`$()` 等注入。
- **LLM 异常降级**：调用失败 / 返回非 JSON / 未配置密钥 → 安全中止，零指令注入。

实现见 `safety.py`；全部行为有 `selftest.py` 实测覆盖（55 项断言全绿）。

## 自测

```bash
python tools/llm/selftest.py
```

不依赖真实游戏服务与真实 LLM API（fake MUD 服务器 + mock/注入替身），
可重复运行，退出码 0 = 全绿。覆盖：

- c1 网关可独立运行：连接、透传、`ai` 拦截、回写执行
- c2 `ai 去当铺把金条当了` → go/list/pawn 序列被回写执行
- c3 危险/无效输入拦截与降级（危险指令不注入、LLM 垃圾响应零注入、超限拒绝）

## 目录结构

| 文件 | 职责 |
|---|---|
| `gateway.py` | 主网关：telnet 连接（含 IAC 协商）、输入转发、`ai` 拦截、回写执行 |
| `llm_client.py` | OpenAI 兼容 API 客户端（纯标准库 urllib）+ 本地配置读取 |
| `safety.py` | 指令安全过滤：白名单 / 危险拦截 / 条数上限 / 格式校验 |
| `mock_llm.py` | mock 解析器（无 API key 的演示/自测替身） |
| `selftest.py` | 三条验收自测脚本 |
| `README.md` | 本说明 |

## 已知限制（Phase 0 边界，见 #68 RFC）

- LLM 不知道地图，路线不确定时输出 `look` 由玩家补上下文后再次发起。
- 游戏 6666 端口存在既有 GBK 乱码坑（`simul_efun/chinese.c` 的 gb_to_utf8 空操作，
  见知识库 `gb-to-utf8-noop`）——如遇乱码改用 5555 端口。
- 本网关是外挂 PoC，验证体验与 prompt 效果；Phase 1（#70）才把解析层
  下沉到驱动内 daemon + sidecar，并注入房间/背包 grounding。
