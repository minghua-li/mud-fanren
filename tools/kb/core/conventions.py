from __future__ import annotations
from pathlib import Path
from typing import Set


def parse_conventions(conventions_md: Path) -> dict:
    """Parse conventions.md extracting tags, modules, clusters, kinds."""
    text = conventions_md.read_text(encoding="utf-8")
    result: dict[str, Set[str]] = {
        "tags": set(),
        "modules": set(),
        "clusters": set(),
        "kinds": set(),
    }

    current_section: str | None = None

    for line in text.splitlines():
        if line.startswith("## "):
            name = line[3:].strip()
            section_map = {
                "Tags": "tags",
                "Modules": "modules",
                "Clusters": "clusters",
                "Kinds": "kinds",
            }
            current_section = section_map.get(name)
            continue

        if current_section and line.startswith("|") and not line.startswith("|---"):
            parts = [p.strip() for p in line.split("|")]
            if len(parts) >= 2:
                slug = parts[1]
                if slug and slug not in ("tag", "module", "cluster", "kind", ""):
                    result[current_section].add(slug)

    return result
