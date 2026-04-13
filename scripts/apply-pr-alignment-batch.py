"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            apply-pr-alignment-batch.py                        ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:22:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8452353dc5  2026-03-12  Add unit tests for sync-issues-from-roadmap.py ║
    • bd46fdcaf1  2026-03-12  Refactor issue and PR reconciliation documents; update Po... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = "makr-code/ThemisDB"


def run(cmd: list[str], timeout_sec: int = 30) -> tuple[int, str, str]:
    try:
        p = subprocess.run(
            cmd,
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=timeout_sec,
        )
        return p.returncode, p.stdout.strip(), p.stderr.strip()
    except subprocess.TimeoutExpired:
        return 124, "", f"timeout after {timeout_sec}s"


def main() -> int:
    ap = argparse.ArgumentParser(description="Apply PR milestone alignment proposals in batches.")
    ap.add_argument("--input", default="artifacts/issues-prs-doc-reconcile-2026-03-11.json")
    ap.add_argument("--offset", type=int, default=0)
    ap.add_argument("--limit", type=int, default=100)
    ap.add_argument("--confidence", default="high", choices=["high", "medium"])
    ap.add_argument("--out", default="artifacts/pr-alignment-batch-result-2026-03-12.json")
    args = ap.parse_args()

    src = ROOT / args.input
    if not src.exists():
        raise SystemExit(f"Input not found: {src}")

    data = json.loads(src.read_text(encoding="utf-8"))
    proposals = [
        p for p in data.get("pr_milestone_alignment_proposals", [])
        if p.get("confidence") == args.confidence
    ]
    batch = proposals[args.offset : args.offset + args.limit]

    result = {
        "input": args.input,
        "offset": args.offset,
        "limit": args.limit,
        "confidence": args.confidence,
        "selected": len(batch),
        "ok": 0,
        "fail": 0,
        "failures": [],
    }

    result["processed"] = 0
    try:
        for p in batch:
            pr_number = int(p["pr_number"])
            suggested = p["suggested_pr_milestone"]
            code, out, err = run([
                "gh", "pr", "edit", str(pr_number),
                "-R", REPO,
                "--milestone", suggested,
            ], timeout_sec=25)
            result["processed"] += 1
            if code == 0:
                result["ok"] += 1
            else:
                result["fail"] += 1
                result["failures"].append({
                    "pr_number": pr_number,
                    "current": p.get("current_pr_milestone"),
                    "suggested": suggested,
                    "stdout": out,
                    "stderr": err,
                })
    except KeyboardInterrupt:
        result["interrupted"] = True

    out_path = ROOT / args.out
    out_path.write_text(json.dumps(result, indent=2), encoding="utf-8")

    print(f"OUT={out_path.relative_to(ROOT)}")
    print(f"SELECTED={result['selected']}")
    print(f"PROCESSED={result['processed']}")
    print(f"OK={result['ok']}")
    print(f"FAIL={result['fail']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
