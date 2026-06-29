---
id: quest-system
claim: "任务系统在 /quest/ 下按门派/区域分目录组织，标准模式是守护进程（aquest/bquest）加数字后缀，区域级任务直接放在命名目录下"
tags: [quest, daemon]
modules: [quest]
cluster: architecture
kind: architecture
status: current
verified: "2026-06-29"
---

## Why

北大侠客行任务数量多且持续增长，按门派/区域组织任务代码是维护性的关键架构决策。

## How to apply

目录布局：
```
/quest/
├── aquest/           A类任务守护进程（带数字后缀: aquest1.c, aquest2.c...）
├── bquest/           B类任务守护进程
├── changan/          长安任务
├── emei/  gaibang/  各门派任务
├── escort/           护送任务
├── new_quest/        较新的任务系统（独立子目录）
└── quest.h           任务相关头文件
```

- 引用宏：`AQUEST_D(n)` → `/quest/aquest/aquest<n>`，`BQUEST_D(n)` → `/quest/bquest/bquest<n>`
- `ZQUEST_D` → `/obj/quest.c`（任务通用对象）
- 新任务优先放到对应门派/区域目录下，避免继续膨胀 `aquest/bquest`
- 任务守护进程通常继承 F_DBASE 持有一个全局任务列表 mapping
