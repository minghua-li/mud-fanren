from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, List

from tools.kb.core import frontmatter


@dataclass
class Entry:
    id: str
    claim: str
    tags: list[str]
    modules: list[str]
    kind: str
    status: str
    verified: str
    path: Path
    body: str
    cluster: str = ""
    verify_freq: str | None = None
    supersedes: list[str] = field(default_factory=list)
    related: list[str] = field(default_factory=list)
    extra: dict[str, Any] = field(default_factory=dict)


def load_entry(path: Path) -> Entry | None:
    """Load a single entry from a .md file."""
    if not path.suffix == ".md":
        return None
    # Skip index files and conventions
    if path.name.startswith("INDEX-") or path.name == "conventions.md":
        return None

    text = path.read_text(encoding="utf-8")
    meta, body = frontmatter.split(text)

    if not meta or "id" not in meta:
        return None

    entry_id = meta["id"]
    if entry_id != path.stem:
        # Allow mismatch but warn - use file stem as canonical
        entry_id = path.stem

    cluster = path.parent.name if path.parent.name != "knowledge" else ""

    return Entry(
        id=entry_id,
        claim=meta.get("claim", ""),
        tags=meta.get("tags", []),
        modules=meta.get("modules", []),
        kind=meta.get("kind", ""),
        status=meta.get("status", "current"),
        verified=meta.get("verified", ""),
        cluster=meta.get("cluster", cluster),
        verify_freq=meta.get("verify_freq"),
        supersedes=meta.get("supersedes", []),
        related=meta.get("related", []),
        extra={k: v for k, v in meta.items() if k not in (
            "id", "claim", "tags", "modules", "kind", "status",
            "verified", "cluster", "verify_freq", "supersedes", "related",
        )},
        path=path,
        body=body,
    )


def scan_entries(kb_dir: Path) -> list[Entry]:
    """Scan all .md files in kb_dir recursively and load entries."""
    entries: list[Entry] = []
    for path in sorted(kb_dir.rglob("*.md")):
        entry = load_entry(path)
        if entry:
            entries.append(entry)
    return entries


def save_entry(entry: Entry) -> None:
    """Write entry back to disk."""
    meta = {
        "id": entry.id,
        "claim": entry.claim,
        "tags": entry.tags,
        "modules": entry.modules,
        "kind": entry.kind,
        "status": entry.status,
        "verified": entry.verified,
    }
    if entry.cluster:
        meta["cluster"] = entry.cluster
    if entry.verify_freq:
        meta["verify_freq"] = entry.verify_freq
    if entry.supersedes:
        meta["supersedes"] = entry.supersedes
    if entry.related:
        meta["related"] = entry.related
    meta.update(entry.extra)

    text = frontmatter.dump(meta, entry.body)
    entry.path.write_text(encoding="utf-8", data=text)
