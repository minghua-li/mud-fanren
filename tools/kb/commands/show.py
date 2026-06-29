from __future__ import annotations
from pathlib import Path
from tools.kb.core.entry import scan_entries, save_entry


def cmd_show_main(root: Path, args) -> int:
    entries = scan_entries(root)
    for e in entries:
        if e.id == args.id:
            rel = e.path.relative_to(root.parent.parent)
            print(f"id:         {e.id}")
            print(f"path:       {rel}")
            print(f"claim:      {e.claim}")
            print(f"tags:       {', '.join(e.tags)}")
            print(f"modules:    {', '.join(e.modules)}")
            print(f"cluster:    {e.cluster}")
            print(f"kind:       {e.kind}")
            print(f"status:     {e.status}")
            print(f"verified:   {e.verified}")
            if e.verify_freq:
                print(f"verify_freq: {e.verify_freq}")
            if e.supersedes:
                print(f"supersedes: {', '.join(e.supersedes)}")
            if e.related:
                print(f"related:    {', '.join(e.related)}")
            return 0

    print(f"未找到 entry: {args.id}", file=sys.stderr)
    return 1
