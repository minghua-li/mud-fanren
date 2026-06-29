from __future__ import annotations
import sys
from datetime import date
from pathlib import Path

from tools.kb.core.entry import load_entry, scan_entries
from tools.kb.core.conventions import parse_conventions


def cmd_new_main(root: Path, args) -> int:
    entry_id = args.id
    cluster = args.cluster
    today = date.today().isoformat()

    # Check conventions
    conv_path = root / "conventions.md"
    if conv_path.exists():
        conv = parse_conventions(conv_path)
        if cluster not in conv.get("clusters", set()):
            print(f"警告: cluster '{cluster}' 不在受控词表内", file=sys.stderr)

    # Check for duplicate id
    existing = scan_entries(root)
    if any(e.id == entry_id for e in existing):
        print(f"错误: entry id '{entry_id}' 已存在", file=sys.stderr)
        return 1

    dir_path = root / cluster
    dir_path.mkdir(parents=True, exist_ok=True)
    file_path = dir_path / f"{entry_id}.md"

    if file_path.exists():
        print(f"错误: 文件已存在 {file_path}", file=sys.stderr)
        return 1

    content = f"""---
id: {entry_id}
claim: TODO 替换为单句事实（含具体标识符）
tags: [TODO]
modules: [TODO]
cluster: {cluster}
kind: pattern
status: current
verified: "{today}"
---

## Why

TODO 一段说明为什么记录这条（背景、踩坑过程、决策动机）。

## How to apply

TODO 一段说明这条在新代码中怎么用（具体应用场景）。
"""
    file_path.write_text(encoding="utf-8", data=content.lstrip())
    print(f"创建: {file_path}")
    return 0
