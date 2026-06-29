from __future__ import annotations
from pathlib import Path

from tools.kb.core.entry import scan_entries, save_entry


def cmd_supersede_main(root: Path, args) -> int:
    entries = {e.id: e for e in scan_entries(root)}

    old = entries.get(args.old)
    new = entries.get(args.new)

    if not old:
        print(f"未找到 entry: {args.old}", file=sys.stderr)
        return 1
    if not new:
        print(f"未找到 entry: {args.new}", file=sys.stderr)
        return 1

    old.status = "superseded"
    if args.new not in old.supersedes:
        old.supersedes.append(args.new)

    save_entry(old)
    print(f"已标记 '{args.old}' 为 superseded, 指向 '{args.new}'")
    return 0
