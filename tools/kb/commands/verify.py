from __future__ import annotations
from datetime import date
from pathlib import Path

from tools.kb.core.entry import scan_entries, save_entry


def cmd_verify_main(root: Path, args) -> int:
    entries = scan_entries(root)
    target = None
    for e in entries:
        if e.id == args.id:
            target = e
            break

    if not target:
        print(f"未找到 entry: {args.id}", file=sys.stderr)
        return 1

    new_date = args.date if args.date else date.today().isoformat()
    target.verified = new_date
    save_entry(target)
    print(f"已更新 {args.id}.verified = {new_date}")
    return 0
