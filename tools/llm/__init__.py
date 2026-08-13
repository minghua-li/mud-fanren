# tools.llm —— 自然语言 → MUD 指令解析网关（Phase 0 外挂式 PoC）
#
# 纯标准库实现，无第三方依赖（对齐 tools/kb 基建风格）。
# 入口：python -m tools.llm.gateway（或等价地 python -m tools.llm）
# 注意：从仓库根用 `python -m tools.llm.gateway` 运行；直接执行
#   `python tools/llm/gateway.py` 会因 sys.path 不含仓库根而导入失败。
