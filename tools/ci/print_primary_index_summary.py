"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            print_primary_index_summary.py                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:54:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 071f2f6199  2026-03-09  feat: add primary-docs index/inventory generator ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Print a Markdown summary of docs/_generated/primary_index.json for the
GitHub Actions step summary.

Usage:
    python3 tools/ci/print_primary_index_summary.py [INDEX_FILE]

Arguments:
    INDEX_FILE  Path to the JSON index (default: docs/_generated/primary_index.json)
"""

import json
import pathlib
import sys


def main() -> int:
    index_file = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(
        "docs/_generated/primary_index.json"
    )

    if not index_file.exists():
        print("No JSON index found (YAML format may have been selected).")
        return 0

    with index_file.open(encoding="utf-8") as f:
        data = json.load(f)

    print(f"**Generated at:** {data['generated_at']}")
    print(f"**Total files indexed:** {data['total_files']}")
    print(f"**Scan directories:** {', '.join(data['scan_dirs'])}")
    print()

    if data["entries"]:
        by_type: dict = {}
        for entry in data["entries"]:
            by_type.setdefault(entry["type"], []).append(entry["path"])

        print("| Type | Count | Files |")
        print("|------|-------|-------|")
        for doc_type, paths in sorted(by_type.items()):
            files = "<br>".join(f"`{p}`" for p in paths)
            print(f"| {doc_type} | {len(paths)} | {files} |")
    else:
        print("*No primary documentation files found in the scanned directories.*")

    return 0


if __name__ == "__main__":
    sys.exit(main())
