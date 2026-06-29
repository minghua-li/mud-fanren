---
id: data-persistence
claim: "数据通过 save_object()/restore_object() 序列化为 .o 文件，F_SAVE 要求对象实现 query_save_file() 返回路径，按首字母分片存储于 /data/"
tags: [data, dbase, player]
modules: [feature, adm-daemons]
cluster: architecture
kind: invariant
status: current
verified: "2026-06-29"
---

## Why

FluffOS 内建 `save_object(file)` 将对象的 `mapping`/`int`/`string`/`array` 变量写入 `file.o`（不包含 `nosave` 变量）。`F_SAVE` 封装了这一机制。

## How to apply

- 对象须定义 `query_save_file()` 返回不含扩展名的路径
- F_SAVE 的 `save()` 调用 `save_object(file)`，`restore()` 调用 `restore_object(file + ".o")`
- 数据分片规则：`/data/login/<首字母>/<id>.o` 和 `/data/user/<首字母>/<id>.o`
- `/data/` 下其他重要文件：
  - `board/*.o` — 公告板持久化
  - `familyd.o`, `examined.o` — 守护进程状态
  - `emoted.o` — 表情数据
  - `npc/*.o` — NPC 状态
- `nosave` 变量（如 `tmp_dbase`）不会被持久化
- 修改数据存储格式需写迁移逻辑（参考 `query_attr_version()` 方案）
