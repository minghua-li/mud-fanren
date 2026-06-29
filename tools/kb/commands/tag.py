from __future__ import annotations
from pathlib import Path

from tools.kb.core.entry import scan_entries, save_entry


def cmd_tag_main(root: Path, args) -> int:
    entries = scan_entries(root)
    target = None
    for e in entries:
        if e.id == args.id:
            target = e
            break

    if not target:
        print(f"未找到 entry: {args.id}", file=sys.stderr)
        return 1

    changes = args.changes
    for change in changes:
        if change.startswith("+"):
            tag = change[1:]
            if tag not in target.tags:
                target.tags.append(tag)
                print(f"  + {tag}")
        elif change.startswith("-"):
            tag = change[1:]
            if tag in target.tags:
                target.tags.remove(tag)
                print(f"  - {tag}")
        else:
            print(f"忽略: {change}（需 +tag 或 -tag 格式）")

    save_entry(target)
    print(f"已更新: {target.path}")
    return 0
