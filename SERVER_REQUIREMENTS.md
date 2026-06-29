# MUD 服务器改造需求

## 背景

AI MUD 客户端需要准确判断「服务器正在等待玩家输入」的时刻，以便在正确的时机进行下一轮决策。当前服务器不发送任何输入就绪信号，客户端只能靠时间 debounce + 文本猜测，不可靠。

## 核心需求

### 1. Telnet GO_AHEAD（必须）

在每次等待玩家输入时，服务器在提示符之后发送 `IAC GA`（`0xFF 0xF9`）。

**发送时机**：
- 登录/注册提示符后（如「您的英文名字：」）
- 游戏内命令提示符后（如 `> `）
- NPC 对话选项后
- yes/no 确认提示后
- 战斗中需要玩家操作时（如询问是否逃跑）

**不发送时机**：
- 战斗自动回合输出（服务器自动推进的战斗轮次）
- 系统公告、频道消息
- 任何不需要玩家立即输入的输出

**协议细节**：
- 客户端会对 `DO SGA` 回复 `WILL SGA`，但服务器仍应在提示符后发 GA
- 如果服务器当前有 SGA suppress 逻辑，需要改为：即使 suppress SGA 协商成功，仍然发送 GA
- GA 应该在提示符文本之后、同一个 TCP 包内发送

示例（十六进制）：
```
... 您您的英文ID： \xFF\xF9
... > \xFF\xF9
```

### 2. GMCP 结构化状态（建议）

通过 GMCP（Generic MUD Communication Protocol，Telnet 子协商选项 201）推送结构化游戏状态，替代文本解析。

**Hello 握手**：
```
IAC WILL 201
```

**客户端注册支持的 Package**：
```
IAC SB 201 "Core.Hello {\"Client\":\"AI-MUD\",\"Version\":\"0.2\"}" IAC SE
IAC SB 201 "Core.Supports [\"Char.Status 1\",\"Char.Vitals 1\",\"Room.Info 1\"]" IAC SE
```

**服务器推送的数据**：

Char.Vitals（气血/精神/内力，数值变化时推送）：
```json
{
  "hp": 100, "maxhp": 150,
  "jing": 80, "maxjing": 100,
  "qi": 60, "maxqi": 100
}
```

Char.Status（等级/经验/金钱，变化时推送）：
```json
{
  "level": 5,
  "exp": 1200,
  "gold": 50,
  "name": "铁憨憨"
}
```

Room.Info（进入新房间时推送）：
```json
{
  "name": "扬州客店",
  "exits": ["north", "south", "east"],
  "npcs": ["店小二"]
}
```

## 验证方法

改造后用以下命令验证 GA：
```bash
# 连接并抓取原始字节，搜索 FF F9
nc localhost 26666 | xxd | grep "ff f9"
```

如果能看到 `ff f9` 出现在提示符之后，说明 GA 已生效。
