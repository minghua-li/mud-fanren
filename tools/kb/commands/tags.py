from __future__ import annotations
from collections import Counter
from pathlib import Path

from tools.kb.core.entry import scan_entries
from tools.kb.core.conventions import parse_conventions


def cmd_tags_main(root: Path, args) -> int:
    entries = scan_entries(root)
    counter: Counter[str] = Counter()
    for e in entries:
        for tag in e.tags:
            counter[tag] += 1

    conv = parse_conventions(root / "conventions.md")
    controlled = conv.get("tags", set())

    if args.unused:
        unused = controlled - set(counter.keys())
        if unused:
            print("未使用的 tag:")
            for t in sorted(unused):
                print(f"  {t}")
        else:
            print("所有受控 tag 均已使用")
        return 0

    if args.orphan:
        used = set(counter.keys())
        orphan = used - controlled
        if orphan:
            print("不在受控词表中的 tag（orphan）:")
            for t in sorted(orphan):
                print(f"  {t} ({counter[t]} 次)")
        else:
            print("无 orphan tag")
        return 0

    print(f"Tag 统计 ({len(counter)} 个):")
    for tag, count in counter.most_common():
        mark = " *" if tag not in controlled else ""
        print(f"  {tag}: {count}{mark}")
    return 0
