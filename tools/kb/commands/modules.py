from __future__ import annotations
from collections import Counter
from pathlib import Path

from tools.kb.core.entry import scan_entries
from tools.kb.core.conventions import parse_conventions


def cmd_modules_main(root: Path, args) -> int:
    entries = scan_entries(root)
    counter: Counter[str] = Counter()
    for e in entries:
        for mod in e.modules:
            counter[mod] += 1

    conv = parse_conventions(root / "conventions.md")
    controlled = conv.get("modules", set())

    if args.unused:
        unused = controlled - set(counter.keys())
        if unused:
            print("未使用的 module:")
            for m in sorted(unused):
                print(f"  {m}")
        else:
            print("所有受控 module 均已使用")
        return 0

    if args.orphan:
        used = set(counter.keys())
        orphan = used - controlled
        if orphan:
            print("不在受控词表中的 module（orphan）:")
            for m in sorted(orphan):
                print(f"  {m} ({counter[m]} 次)")
        else:
            print("无 orphan module")
        return 0

    print(f"Module 统计 ({len(counter)} 个):")
    for mod, count in counter.most_common():
        mark = " *" if mod not in controlled else ""
        print(f"  {mod}: {count}{mark}")
    return 0
