from __future__ import annotations
import sys
from pathlib import Path

from tools.kb.core.entry import scan_entries, save_entry


def cmd_append_main(root: Path, args) -> int:
    entries = scan_entries(root)
    target = None
    for e in entries:
        if e.id == args.id:
            target = e
            break

    if not target:
        print(f"未找到 entry: {args.id}", file=sys.stderr)
        return 1

    section_title = args.section
    content = sys.stdin.read().strip()

    if not content:
        print("错误: 请通过 stdin 提供内容", file=sys.stderr)
        return 1

    target.body += f"\n\n## {section_title}\n\n{content}\n"
    save_entry(target)
    print(f"已追加 '## {section_title}' 到 {args.id}")
    return 0
