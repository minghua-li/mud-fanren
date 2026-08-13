---
claim: tools/llm/gateway.py 是 Phase 0 外挂 LLM 网关：telnet 连 5555/6666 按端口选 GBK/UTF-8
  编码，ai 前缀输入经 LLM 解析、safety.py 分层过滤后回写执行
cluster: architecture
id: llm-gateway-poc
kind: architecture
modules:
- tools-llm
related:
- gb-to-utf8-noop
status: current
tags:
- command
- setup
verified: '2026-08-13'
---

## Why

「自然语言 → MUD 指令」体验验证（RFC #68 的 Phase 0）落地为 `tools/llm/` 外挂网关：
零驱动改动，纯标准库 Python，telnet 连游戏 5555/6666 端口，把玩家 `ai <自然语言>` 输入
解析为指令序列回写执行。它是 Phase 1（#70 驱动内 `llmd.c` + sidecar）的设计前提，
Phase 1 会复用本网关验证过的输出契约与安全模型，因此目录职责、编码约定、
安全分层、验证方法都必须沉淀。

## How to apply

### 目录职责（新增/修改先对齐，别自造轮子）

| 文件 | 职责 |
|---|---|
| `gateway.py` | 主网关：telnet 连接（IAC 协商）、输入转发、`ai` 拦截、逐条回写（间隔 0.4s） |
| `llm_client.py` | OpenAI 兼容 Chat Completions 客户端（纯标准库 urllib）+ 本地配置读取 |
| `safety.py` | 指令安全过滤（白名单三层 + 条数上限 + 字符集校验） |
| `mock_llm.py` | 无 API key 时的演示/自测解析器（真实效果靠 LLM） |
| `selftest.py` | 三条验收自测（fake MUD 服务器 + mock/注入替身，55 断言） |

### 端口编码约定（与 master-object-callbacks 一致）

- 5555 = GBK、6666 = UTF-8；网关按端口默认编码，`--encoding` 可覆盖。
- 6666 存在既有乱码坑（`simul_efun/chinese.c` 的 gb_to_utf8 空操作，
  见 pitfall/gb-to-utf8-noop）——遇乱码改用 5555。

### LLM 输出契约（对齐 RFC #68，Phase 1 沿用）

```json
{"commands": ["go east", "list", "pawn gold"], "confirm": ["kill npc"], "reason": "一句话"}
```

- `commands`：安全可直接执行，≤8 条；`confirm`：高危意图，PoC 默认拦截。
- prompt 限定动词白名单，且明确「不知道路线先输出 look」。

### 安全分层（safety.py，不可放宽）

- SAFE 白名单直放（go/look/list/pawn/buy/get/say 等）；DANGEROUS（kill/drop/give/quit 等）
  拦截；DENIED（shutdown/exec/update/eval/ls 等 wiz 调试命令）绝对拒绝。
- 每条指令字符集校验 `[a-z][a-z0-9_-]`，杜绝 `;` `|` `$()` 注入。
- LLM 调用失败 / 非 JSON / 未配 key → 安全中止，零注入。

### telnet 协商（踩过的坑）

`strip_telnet(raw)` 返回 **(数据, 需回应字节)** 二元组：对 DO/WILL 回应
WONT/DONT 的字节必须 `sendall` 回服务器，绝不能当数据打印（否则终端乱码）；
SB...SE 子协商整段跳过。收到无换行的登录提示（如「请输入你的 ID：」）时，
超时 1s 无新数据要把残留 buf flush 打印，否则玩家看不到提示。

### 验证方法（无 driver 环境）

本地起 fake MUD 服务器（模拟 telnet 协商 + 按行回显），注入假 LLM 响应或 mock
解析器即可全链路实证连接/转发/解析/回写/失败安全，不依赖真实游戏与真实 API。

### 环境事实（2026-08-13 探测，易变）

5555/6666 端口有 TCP listener 且回 telnet 协商，但容器内游戏未就绪：
收到任何数据后立即关闭连接，无法完成真实登录——真实端到端验证需等游戏服务恢复。
