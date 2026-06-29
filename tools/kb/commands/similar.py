from __future__ import annotations
from pathlib import Path

from tools.kb.core.entry import scan_entries


def cmd_similar_main(root: Path, args) -> int:
    entries = {e.id: e for e in scan_entries(root)}

    target = entries.get(args.id)
    if not target:
        print(f"未找到 entry: {args.id}", file=sys.stderr)
        return 1

    # Score by tag + module intersection
    target_tags = set(target.tags)
    target_modules = set(target.modules)

    scored = []
    for e in entries.values():
        if e.id == args.id:
            continue
        tag_overlap = len(target_tags & set(e.tags))
        mod_overlap = len(target_modules & set(e.modules))
        score = tag_overlap * 3 + mod_overlap * 2
        if score > 0:
            scored.append((score, e))

    scored.sort(key=lambda x: -x[0])

    print(f"与 '{args.id}' 相似的条目:")
    for score, e in scored[:args.limit]:
        print(f"  [{score}分] {e.id} — {e.claim[:80]}")
    return 0
