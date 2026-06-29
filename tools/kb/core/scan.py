"""Fault-tolerant scan for read-only commands."""
from __future__ import annotations
from pathlib import Path
from tools.kb.core.entry import load_entry


def scan_tolerant(kb_dir: Path) -> list:
    """Scan entries, skipping unparseable files silently."""
    entries = []
    for path in sorted(kb_dir.rglob("*.md")):
        try:
            entry = load_entry(path)
            if entry:
                entries.append(entry)
        except Exception:
            continue
    return entries
