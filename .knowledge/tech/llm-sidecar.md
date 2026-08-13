# LLM 自然语言解析 sidecar 协议契约（#70 Phase 1）

状态：已落地（#70 交付，2026-08-13）

## 一句话

LPC 侧 `adm/daemons/llmd.c`（LLM_D）+ 玩家命令 `cmds/usr/ai.c` 负责采集
grounding 上下文与异步注入；本地 Python sidecar `tools/llm/sidecar.py` 负责
prompt 组装 / LLM 调用 / safety 过滤（复用 #69 llm_client/safety/mock）。
两者经 **TCP 127.0.0.1 新行分隔 JSON、每请求一连接** 通信。

## 职责边界（架构评估 5276954767 口径）

- **LPC 侧（LLM_D）只做**：采集 grounding（房间 short/long/exits/可见对象/背包/
  境界）→ socket 转发 → 异步回调（socket + call_out）→ 逐条 `me->force_me` 注入
  + 上下文变化校验（玩家死亡/离线中止后续）→ 失败安全兜底（watchdog 15s 零注入）。
- **sidecar 侧做**：prompt 组装、LLM 调用、JSON 解析、safety 过滤
  （不重写 #69 资产，DRY）。
- **集成点 = `cmds/usr/ai.c` 标准命令**（不走 command_hook 失败分支）：
  天然继承命令路径限速，失败 return 0 落到原失败提示。
- **TCP 是唯一选项**：LPC socket efuns 只支持 TCP/UDP，无 unix socket；
  LPC 无 spawn/popen → stdin/stdout 不可行。

## 协议契约

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

- `commands`：白名单安全动词，可直接注入（最多 8 条）
- `confirm`：仅 DANGEROUS 动词（check_confirm_command 校验），LPC 侧请玩家
  `ai confirm yes/no` 二次确认后注入
- `blocked`：被拦截条目（管理命令/非白名单/超限/格式非法）
- `error`：非空 = 解析失败（LLM 失败/超时/未配 key），`commands` 必为空

## 关键坑（必踩）

1. **两级超时**：sidecar 内 LLM 调用超时默认 12s（`--timeout`）**必须短于**
   LPC 侧 watchdog 15s（`LLM_WATCHDOG_TIME`）；watchdog 是权威兜底——
   超时清理 fd + pending，迟到响应因 fd 已关进不来，零注入。
2. **json_parse / json_encode efun**：FluffOS v2019 内置但**本仓无使用先例**；
   llmd.c 用 `catch(json_parse(...))` 包住，解析失败走零注入兜底；
   真实 driver 上需复验一次。
3. **LPC 侧白名单防御**：即使 sidecar 被替换/配置错误，llmd.c `valid_inject_cmd`
   也只在白名单内注入——`LLM_SAFE_VERBS / LLM_DANGEROUS_VERBS / LLM_DENIED_VERBS`
   三张动词表**与 tools/llm/safety.py 保持一致**（跨语言复制，改 safety 时须同步）。
4. **开关双闸**：全局默认关闭（LLM_D `set("llm_enabled", 0)`，管理员
   `set_llm_enabled(1)` 开启）+ 玩家 `set llm on`（env/llm，复用 set.c 通用 env
   机制；注意 set.c 的 `sscanf(data,"%d",data)` 使 `set llm on` 存字符串 "on"、
   `set llm` 存 int 1，ai.c 判断需兼容 "on"/"1"/1/yes/true）。
5. **新命令可见性**：新增 ai.c 后需 `rehash /cmds/usr`（COMMAND_D 缓存目录列表）
   或重启驱动。
6. **sidecar 只 bind 127.0.0.1**（绝不 0.0.0.0）；端口 37777 与 globals.h
   `LLM_SIDECAR_PORT` 一致。
7. **每请求一连接**：read_callback 按 `\n` 定界缓冲（TCP 分片是常态），
   未收完整行等下次回调；响应后 cleanup_fd（socket_close + map_delete）。
8. **LPC 无 catch/remove_call_out 先例**：本实现用 `catch(expr)` 单表达式 +
   watchdog 靠 pending 表存在性空跑（响应已处理则 watchdog 自然失效），
   不依赖 remove_call_out(handle)。
9. **force_me 前提**：LLM_D 需 `seteuid(ROOT_UID)`（master.c valid_seteuid 返 1
   允许），feature/command.c 的 force_me 检查 previous_object euid == ROOT_UID。

## 部署要求

1. 起 sidecar：`python -m tools.llm.sidecar --port 37777`（或 `--mock` 演示）
2. 管理员开全局：`call /adm/daemons/llmd set_llm_enabled 1`
3. 玩家 `set llm on` → `ai <自然语言>`；`ai confirm yes/no` 处理高危确认
4. 密钥只从环境变量 / `~/.config/fanren-mud/llm.json` 读取（#69 红线延续），
   代码库/ticket 零密钥

## 验证

`python tools/llm/sidecar_selftest.py`（87 断言 + 4 组突变验证，exit 0）：
mock 解析链路 / LLM 替身危险指令拦截 / 慢 LLM 超时 / 协议容错 / 未配 key /
LPC 静态校验（括号配对 + 关键模式）/ LPC 逻辑忠实翻译模拟（正常注入、watchdog
零注入、迟到响应忽略、LPC 白名单、confirm 流程、非法响应、断线、限流）/
端到端（真实 TCP + fake LPC 客户端）/ 密钥零硬编码。

环境无 fluffos driver：LPC 侧为静态校验 + Python 翻译模拟（作用域已如实标注），
真实驱动端到端待装驱动后复验（先例 #61/#69）。
