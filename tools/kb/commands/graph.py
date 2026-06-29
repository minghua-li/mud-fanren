from __future__ import annotations
from collections import deque
from pathlib import Path

from tools.kb.core.entry import scan_entries


def cmd_graph_main(root: Path, args) -> int:
    entries = {e.id: e for e in scan_entries(root)}

    start = entries.get(args.id)
    if not start:
        print(f"未找到 entry: {args.id}", file=sys.stderr)
        return 1

    visited: set[str] = set()
    queue: deque[tuple[str, int]] = deque()
    queue.append((args.id, 0))
    visited.add(args.id)

    if args.format == "ascii":
        while queue:
            eid, depth = queue.popleft()
            if depth > args.depth:
                continue
            prefix = "  " * depth + ("└─ " if depth > 0 else "")
            e = entries[eid]
            print(f"{prefix}{eid} ({e.kind}) — {e.claim[:60]}")

            for rel_id in e.related + e.supersedes:
                if rel_id not in visited and rel_id in entries:
                    visited.add(rel_id)
                    queue.append((rel_id, depth + 1))
    else:
        # dot format
        print("digraph KB {")
        print(f'  "{args.id}" [label="{args.id}"];')
        while queue:
            eid, depth = queue.popleft()
            if depth > args.depth:
                continue
            e = entries[eid]
            for rel_id in e.related:
                if rel_id in entries and rel_id not in visited:
                    visited.add(rel_id)
                    queue.append((rel_id, depth + 1))
                    print(f'  "{eid}" -> "{rel_id}";')
            for sup_id in e.supersedes:
                if sup_id in entries and sup_id not in visited:
                    visited.add(sup_id)
                    queue.append((sup_id, depth + 1))
                    print(f'  "{eid}" -> "{sup_id}" [style=dashed];')
        print("}")

    return 0
