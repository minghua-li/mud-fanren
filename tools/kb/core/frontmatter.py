from __future__ import annotations
import yaml
from typing import Any, Tuple

_FM_RE = r"^---\s*$"


def split(text: str) -> Tuple[dict[str, Any], str]:
    """Split markdown text into (frontmatter_dict, body)."""
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return {}, text

    end = -1
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            end = i
            break

    if end == -1:
        return {}, text

    fm_lines = lines[1:end]
    body = "\n".join(lines[end + 1:]).strip()
    fm_text = "\n".join(fm_lines)

    try:
        meta = yaml.safe_load(fm_text)
    except yaml.YAMLError:
        meta = {}

    if not isinstance(meta, dict):
        meta = {}

    # Normalize verified to string
    if "verified" in meta and not isinstance(meta["verified"], str):
        meta["verified"] = str(meta["verified"])

    return meta, body


def dump(meta: dict[str, Any], body: str) -> str:
    """Join frontmatter dict + body back into markdown string."""
    fm_text = yaml.safe_dump(meta, allow_unicode=True, default_flow_style=False).strip()
    return f"---\n{fm_text}\n---\n\n{body.strip()}\n"
