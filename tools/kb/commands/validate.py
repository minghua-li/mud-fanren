from __future__ import annotations
import re
import sys
from pathlib import Path

from tools.kb.core.entry import scan_entries
from tools.kb.core.conventions import parse_conventions


def cmd_validate_main(root: Path, args) -> int:
    conv_path = root / "conventions.md"
    if not conv_path.exists():
        print("错误: conventions.md 不存在", file=sys.stderr)
        return 1

    conv = parse_conventions(conv_path)
    entries = scan_entries(root)
    errors = 0

    id_pattern = re.compile(r"^[a-z0-9][a-z0-9-]{2,60}$")

    for e in entries:
        # Validate id format
        if not id_pattern.match(e.id):
            print(f"  [{e.id}] id 格式不符合 kebab-case", file=sys.stderr)
            errors += 1

        # Validate id matches filename
        if e.id != e.path.stem:
            print(f"  [{e.id}] id 与文件名不一致 (stem={e.path.stem})", file=sys.stderr)
            errors += 1

        # Validate required fields
        if not e.claim:
            print(f"  [{e.id}] claim 为空", file=sys.stderr)
            errors += 1

        if not e.tags:
            print(f"  [{e.id}] tags 为空", file=sys.stderr)
            errors += 1

        if not e.modules:
            print(f"  [{e.id}] modules 为空", file=sys.stderr)
            errors += 1

        if not e.kind:
            print(f"  [{e.id}] kind 为空", file=sys.stderr)
            errors += 1

        if e.status not in ("current", "superseded", "historical"):
            print(f"  [{e.id}] status 无效: {e.status}", file=sys.stderr)
            errors += 1

        if not e.verified:
            print(f"  [{e.id}] verified 为空", file=sys.stderr)
            errors += 1

        # Validate tags against controlled vocab
        for tag in e.tags:
            if tag not in conv.get("tags", set()):
                print(f"  [{e.id}] tag '{tag}' 不在受控词表内", file=sys.stderr)
                errors += 1

        # Validate modules against controlled vocab
        for mod in e.modules:
            if mod not in conv.get("modules", set()):
                print(f"  [{e.id}] module '{mod}' 不在受控词表内", file=sys.stderr)
                errors += 1

        # Validate cluster
        if e.cluster and e.cluster not in conv.get("clusters", set()):
            print(f"  [{e.id}] cluster '{e.cluster}' 不在受控词表内", file=sys.stderr)
            errors += 1

        # Validate kind
        if e.kind and e.kind not in conv.get("kinds", set()):
            print(f"  [{e.id}] kind '{e.kind}' 不在受控词表内", file=sys.stderr)
            errors += 1

    if errors:
        print(f"\n校验完成: {len(entries)} 条目, {errors} 个错误", file=sys.stderr)
        return 1

    print(f"校验通过: {len(entries)} 条目, 0 错误")
    return 0
