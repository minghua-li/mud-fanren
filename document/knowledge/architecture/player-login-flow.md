---
id: player-login-flow
claim: "登录流程分 login ob（验证）和 player ob（游戏体）两层，通过 exec() 转移交互连接；数据存于 /data/login/ 和 /data/user/ 按首字母分片"
tags: [player, dbase, data]
modules: [adm-daemons, adm-single, clone]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

LPMUD 标准设计：登录对象（`/clone/user/login.c`）只处理认证和角色选择，玩家对象（通过 `LOGIN_D->make_body()` 创建）负责游戏内状态。`exec(user, ob)` 将玩家交互从登录对象转移到玩家对象。

## How to apply

- 登录对象保存路径：`/data/login/<首字母>/<id>.o`
- 玩家对象保存路径：`/data/user/<首字母>/<id>.o`
- 任何需要持久化的对象须实现 `query_save_file()` 返回路径（不带扩展名），然后继承 F_SAVE 获得 `save()` / `restore()`
- F_SAVE 内部调用 `save_object(file)` / `restore_object(file + ".o")`
- `LOGIN_D->logon(ob)` 启动流程：显示欢迎 → 输入ID → 确认 → `make_body()` → `enter_world()`
- `query_save_file()` 中拼接路径时一定要用 `sprintf(DATA_DIR "login/%c/%s", id[0], id)` 模式
