from __future__ import annotations
from pathlib import Path
import subprocess


def find_repo_root() -> Path:
    """Find git repo root from CWD."""
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True, text=True, check=True,
    )
    return Path(result.stdout.strip())


def kb_root() -> Path:
    """Return the KB directory (document/knowledge) under repo root."""
    return find_repo_root() / "document" / "knowledge"
