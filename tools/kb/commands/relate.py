from __future__ import annotations
from pathlib import Path

from tools.kb.core.entry import scan_entries, save_entry


def cmd_relate_main(root: Path, args) -> int:
    entries = {e.id: e for e in scan_entries(root)}

    e1 = entries.get(args.id1)
    e2 = entries.get(args.id2)

    if not e1:
        print(f"未找到 entry: {args.id1}", file=sys.stderr)
        return 1
    if not e2:
        print(f"未找到 entry: {args.id2}", file=sys.stderr)
        return 1

    if args.unlink:
        if args.id2 in e1.related:
            e1.related.remove(args.id2)
        if args.id1 in e2.related:
            e2.related.remove(args.id1)
        print(f"已取消链接: {args.id1} <-> {args.id2}")
    else:
        if args.id2 not in e1.related:
            e1.related.append(args.id2)
        if args.id1 not in e2.related:
            e2.related.append(args.id1)
        print(f"已建立链接: {args.id1} <-> {args.id2}")

    save_entry(e1)
    save_entry(e2)
    return 0
