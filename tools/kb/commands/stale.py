from __future__ import annotations
from datetime import datetime, date
from pathlib import Path

from tools.kb.core.entry import scan_entries


def cmd_stale_main(root: Path, args) -> int:
    entries = [e for e in scan_entries(root) if e.status == "current"]
    today = date.today()
    threshold_months = args.months

    stale = []
    for e in entries:
        if not e.verified:
            stale.append((e, "无 verified 日期"))
            continue
        try:
            vd = datetime.strptime(e.verified, "%Y-%m-%d").date()
        except ValueError:
            stale.append((e, f"无法解析日期: {e.verified}"))
            continue
        months_map = {"monthly": 1.5, "quarterly": 4, "yearly": 13}
        actual = months_map.get(e.verify_freq, threshold_months) if e.verify_freq else threshold_months
        delta = (today - vd).days / 30
        if delta > actual:
            stale.append((e, f"{int(delta)} 月前"))

    if not stale:
        print("无超期 entry")
        return 0

    for e, reason in stale:
        print(f"  [{e.id}] {reason} — {e.claim}")
    print(f"\n共 {len(stale)} 条超期")
    return 0
