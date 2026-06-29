from __future__ import annotations
from pathlib import Path
from tools.kb.core.scan import scan_tolerant


def cmd_search_main(root: Path, args) -> int:
    entries = scan_tolerant(root)
    terms = [t.lower() for t in args.terms]

    results = []
    for e in entries:
        if args.tag and args.tag not in e.tags:
            continue
        if args.module and args.module not in e.modules:
            continue
        if args.kind and args.kind != e.kind:
            continue
        if terms:
            haystack = f"{e.id} {e.claim} {' '.join(e.tags)} {' '.join(e.modules)} {e.body}".lower()
            if not all(t in haystack for t in terms):
                continue
        results.append(e)

    results = results[:args.limit]

    if not results:
        print("无匹配结果")
        return 0

    for e in results:
        rel = e.path.relative_to(root.parent.parent)  # relative to repo root
        print(f"  [{e.id}] ({rel})")
        print(f"    claim: {e.claim}")
        print(f"    tags: {', '.join(e.tags)}  modules: {', '.join(e.modules)}")
        print()

    print(f"共 {len(results)} 条匹配")
    return 0
