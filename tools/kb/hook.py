"""Pre-commit hook: validate KB entries and regenerate indexes."""
from __future__ import annotations
import subprocess
import sys
from pathlib import Path

from tools.kb.core.repo import kb_root
from tools.kb.commands.validate import cmd_validate_main
from tools.kb.commands.gen_indexes import cmd_gen_indexes_main


def _staged_kb_files(repo_root: Path) -> list[Path]:
    result = subprocess.run(
        ["git", "diff", "--cached", "--name-only"],
        capture_output=True, text=True, check=True,
        cwd=str(repo_root),
    )
    files = []
    for line in result.stdout.splitlines():
        p = repo_root / line
        if "document/knowledge/" in str(p) and p.suffix == ".md":
            if not p.name.startswith("INDEX-") and p.name != "conventions.md":
                files.append(p)
    return files


def main() -> int:
    try:
        root = kb_root()
        repo_root = root.parent.parent
    except Exception as e:
        print(f"KB hook: 无法找到 KB 根目录 - {e}", file=sys.stderr)
        return 1

    staged = _staged_kb_files(repo_root)
    if not staged:
        return 0

    # Validate
    class Args:
        pass

    args = Args()
    rc = cmd_validate_main(root, args)
    if rc != 0:
        print("KB hook: 校验失败，请修复后再 commit", file=sys.stderr)
        return rc

    # Regenerate indexes
    rc = cmd_gen_indexes_main(root, args)
    if rc != 0:
        return rc

    # Re-stage index files
    for idx in root.glob("INDEX-*.md"):
        subprocess.run(
            ["git", "add", str(idx)],
            check=True, cwd=str(repo_root),
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
