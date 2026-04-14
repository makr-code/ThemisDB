"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            create_docs_audit_issues.py                        ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:45:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     456                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 237c9f5970  2026-02-23  feat(docs): add docs-audit issue creator script and issue... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Create GitHub Issues for the ThemisDB documentation audit.

Derived from the findings in:
  - docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md
  - docs/DOCUMENTATION_CLEANUP_ACTION_PLAN.md

Usage:
    python3 scripts/create_docs_audit_issues.py              # dry-run (print only)
    python3 scripts/create_docs_audit_issues.py --create     # create issues via gh CLI
    python3 scripts/create_docs_audit_issues.py --create --repo makr-code/ThemisDB

Requirements:
    gh CLI authenticated with write access to the target repository.
"""

import subprocess
import sys
import argparse
from typing import List, Dict


REPO = "makr-code/ThemisDB"
LABEL_DOCS_AUDIT = "area:docs-audit"
LABEL_DOCUMENTATION = "type:documentation"

# ─── Issue definitions ────────────────────────────────────────────────────────
# Each entry maps to one GitHub Issue.
# Sources:
#   - docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md §"Major Gaps"
#   - docs/DOCUMENTATION_CLEANUP_ACTION_PLAN.md

DOCS_AUDIT_ISSUES: List[Dict] = [
    # ── CRITICAL ──────────────────────────────────────────────────────────────
    {
        "title": "[docs-audit] Fix CMake Architecture documentation: modular directories don't exist",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:high"],
        "body": """\
## Audit Finding – Severity: CRITICAL

### Category
`architecture-gap`

### Affected File(s)
- `docs/architecture/CMAKE_MODULAR_ARCHITECTURE.md`
- `docs/architecture/MODULAR_ARCHITECTURE_ROADMAP.md`

### Problem Description
The architecture documentation describes a modular CMake system that does **not** exist in the codebase:
- `cmake/Features/` directory → **does not exist**
- `cmake/Targets/` directory → **does not exist**
- `LLM.cmake`, `GPU.cmake`, `gRPC.cmake` module files → **do not exist**
- Docs claim the main `cmake/CMakeLists.txt` was refactored to ~150 lines; the file is still ~3 000 lines.

This is actively misleading for contributors trying to understand or extend the build system.

### Evidence
- Gap analysis: `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §1.1 (table rows marked ❌ FALSE)

### Acceptance Criteria
- [ ] `CMAKE_MODULAR_ARCHITECTURE.md` updated to accurately reflect the current build structure
- [ ] Planned vs. implemented sections are clearly separated
- [ ] All claimed directory/file paths verified to exist or removed from docs
- [ ] CI link-check passes on updated file

### References
- `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md`
- `docs/DOCUMENTATION_CLEANUP_ACTION_PLAN.md`
""",
    },
    {
        "title": "[docs-audit] Clarify Plugin System documentation: template-stage vs production-ready",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:high"],
        "body": """\
## Audit Finding – Severity: CRITICAL

### Category
`stale-content` / `architecture-gap`

### Affected File(s)
- `docs/plugins/` (all files)
- `docs/USER_REGISTRATION_PLUGINS.md`

### Problem Description
The plugin system is documented as *production-ready runtime plugin loading*, but the actual implementation consists of template/example stub files only.  
Contributors following the documentation will not be able to build a working plugin.

### Evidence
- Gap analysis: `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §2.2 ("Plugin system: template/example files only")

### Acceptance Criteria
- [ ] All plugin docs clearly indicate whether the feature is production-ready or in-development
- [ ] A contributor guide for the current (stub) plugin interface is present
- [ ] README in `plugins/` is updated with accurate status

### References
- `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §2.2
""",
    },

    # ── HIGH ──────────────────────────────────────────────────────────────────
    {
        "title": "[docs-audit] Broken internal links: run link-check and fix all dead references",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:high"],
        "body": """\
## Audit Finding – Severity: HIGH

### Category
`broken-links`

### Affected File(s)
- `docs/` (full tree)
- `compendium/` (full tree)
- `README.md`

### Problem Description
Multiple documentation files contain broken internal links (references to moved, renamed, or deleted files).
The last link-check report is stale; a fresh run is needed before the cleanup phase.

### Steps to Reproduce
```bash
cd "$(git rev-parse --show-toplevel)"
# Run the existing lint script
python3 scripts/docs-lint.py docs compendium --format json -o /tmp/lint_report.json
# Or use the validate script
bash scripts/validate-docs.sh
```

### Acceptance Criteria
- [ ] Link-check executed; results saved to `docs/link_check_results.txt`
- [ ] All broken *internal* links fixed
- [ ] `.markdown-link-check.json` exclusion list created for known external/future links
- [ ] CI step added (or existing step updated) to fail on new broken internal links

### References
- `docs/DOCUMENTATION_CLEANUP_ACTION_PLAN.md` Task 1.1
- `scripts/docs-lint.py`
- `scripts/link-check.py`
""",
    },
    {
        "title": "[docs-audit] Archive stale implementation summaries (older than 3 months)",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:medium"],
        "body": """\
## Audit Finding – Severity: HIGH

### Category
`stale-content`

### Affected File(s)
Files matching: `docs/*IMPLEMENTATION_SUMMARY*.md`, `docs/*COMPLETE*.md`, `docs/*FINAL*.md`

Example candidates (not exhaustive):
- `docs/PHASE1_COMPLETE.md`
- `docs/PHASE2.1_COMPLETE.md`
- `docs/IMPLEMENTATION_COMPLETE.md`
- Dozens of `*_COMPLETE.md` and `*_SUMMARY.md` files at repo root

### Problem Description
The `docs/` directory contains 100+ historical implementation-summary files that clutter navigation, confuse new contributors, and produce false positives in documentation search.
These files were useful during active development but should now be archived.

### Steps
```bash
# Identify candidates older than 3 months
find docs -name "*IMPLEMENTATION_SUMMARY*" -o -name "*COMPLETE*" -o -name "*FINAL*" | \\
  xargs ls -lt 2>/dev/null | head -60

# Move to archive
mkdir -p docs/ARCHIVED/implementation_history
git mv docs/<file> docs/ARCHIVED/implementation_history/
```

### Acceptance Criteria
- [ ] Files older than 3 months and not actively referenced moved to `docs/ARCHIVED/`
- [ ] `docs/ARCHIVED/README.md` index created
- [ ] No broken links introduced (run link-check after)
- [ ] Root-level `*COMPLETE*.md` files at repo root reviewed and cleaned up

### References
- `docs/DOCUMENTATION_CLEANUP_ACTION_PLAN.md` Task 1.2
""",
    },

    # ── MEDIUM ────────────────────────────────────────────────────────────────
    {
        "title": "[docs-audit] API Documentation: add missing protocol API docs (WebSocket, MQTT, gRPC, MCP)",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:medium"],
        "body": """\
## Audit Finding – Severity: HIGH (MEDIUM priority for new work)

### Category
`api-coverage` / `missing-section`

### Affected File(s)
- `docs/api/` (missing: websocket.md, mqtt.md, grpc.md, mcp.md)
- `docs/bpmn-wire-protocol.md`

### Problem Description
ThemisDB supports WebSocket, MQTT, gRPC, and MCP protocol APIs.
Implementations exist in `src/server/` but none of these protocols has a comprehensive API reference in `docs/api/`.
Only the HTTP and PostgreSQL wire protocols are well documented.

| Protocol   | Implementation | API Doc |
|------------|---------------|---------|
| HTTP/REST  | ✅ | ✅ |
| PostgreSQL wire | ✅ | ✅ |
| WebSocket  | ✅ | ❌ Missing |
| gRPC       | ✅ | ❌ Missing |
| MQTT       | ⚠️ Experimental | ❌ Missing |
| MCP        | ⚠️ Experimental | ❌ Missing |

### Acceptance Criteria
- [ ] `docs/api/websocket.md` created with endpoint list and message format
- [ ] `docs/api/grpc.md` created with service/method reference
- [ ] `docs/api/mqtt.md` created (even if marked experimental)
- [ ] `docs/api/mcp.md` created (even if marked experimental)
- [ ] All new files linked from `docs/api/README.md`

### References
- `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §6
""",
    },
    {
        "title": "[docs-audit] Examples documentation: create index and quickstart for numbered examples (01-22+)",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:medium"],
        "body": """\
## Audit Finding – Severity: MAJOR

### Category
`examples-coverage` / `missing-section`

### Affected File(s)
- `examples/` directory (22+ numbered subdirectories)
- `docs/EXAMPLES_INDEX.md` (incomplete)

### Problem Description
The `examples/` directory contains 22+ numbered examples covering various ThemisDB features.
While each example has its own README, there is no:
- Overview document explaining what each example demonstrates
- Quickstart guide pointing users to the right example for their use case
- Category grouping (vector search, graph, LLM, etc.)

### Acceptance Criteria
- [ ] `docs/EXAMPLES_INDEX.md` (or `docs/examples/README.md`) updated with all examples
- [ ] Each example entry includes: title, description, key concepts, prerequisites
- [ ] Examples grouped by feature category
- [ ] Quickstart section recommending the best starting point for common scenarios

### References
- `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §3
""",
    },
    {
        "title": "[docs-audit] Tools documentation: create hub covering all 30+ scripts and .NET admin tools",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:medium"],
        "body": """\
## Audit Finding – Severity: MAJOR

### Category
`tools-coverage` / `missing-section`

### Affected File(s)
- `scripts/` directory (80+ scripts)
- `tools/` directory (.NET tools)
- `docs/TOOLS_INDEX.md` (stub)

### Problem Description
ThemisDB ships 80+ shell/Python/PowerShell scripts and multiple .NET admin tool GUIs.
Only the data-ingestion tool has any meaningful documentation.
The `docs/TOOLS_INDEX.md` file exists as a stub but contains no useful content.

| Tool category | Count | Documented |
|---------------|-------|------------|
| Build scripts | ~25 | 0% |
| Release scripts | ~15 | 0% |
| Documentation scripts | ~10 | 0% |
| .NET admin tools | ~5 | 10% |
| Validation/audit | ~10 | 20% |

### Acceptance Criteria
- [ ] `docs/TOOLS_INDEX.md` populated with all tool categories and brief descriptions
- [ ] Each major script/tool includes: purpose, usage example, prerequisites
- [ ] .NET admin tools documented with screenshots or feature list
- [ ] CI script usage documented in `docs/ci/` or linked from CI workflow files

### References
- `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §4
""",
    },
    {
        "title": "[docs-audit] English documentation completeness: translate/create missing EN counterparts",
        "labels": [LABEL_DOCUMENTATION, LABEL_DOCS_AUDIT, "priority:low"],
        "body": """\
## Audit Finding – Severity: MEDIUM

### Category
`translation`

### Affected File(s)
- `docs/en/` (missing several documents that exist only in German `docs/de/`)
- `docs/de/` (reference source)

### Problem Description
German documentation (`docs/de/`) is more comprehensive than the English documentation (`docs/en/`).
Several architecture decision records, implementation guides, and operational runbooks exist only in German.
There is also no English index file in `docs/en/`.

Key gaps (German only, no English equivalent):
- `docs/de/architecture/PHASE2_STORAGE_DI.md`
- `docs/de/features/CLOUD_STORAGE_IMPLEMENTATION_SUMMARY.md`
- `docs/de/llm/RAG_IMPLEMENTATION_GUIDE.md`
- Several other implementation and architecture docs

### Acceptance Criteria
- [ ] `docs/en/README.md` or `docs/en/index.md` created with navigation links
- [ ] At minimum, the 5 most frequently referenced DE-only docs translated to EN
- [ ] Translation process documented in `CONTRIBUTING.md`
- [ ] Language parity tracking table added to `docs/LANGUAGE_STRUCTURE.md`

### References
- `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §5.1
- `docs/LANGUAGE_STRUCTURE.md`
""",
    },
]


# ─── Helpers ─────────────────────────────────────────────────────────────────

def run(args: list) -> tuple[int, str, str]:
    """Execute a command (arg list) and return (returncode, stdout, stderr)."""
    result = subprocess.run(args, capture_output=True, text=True)
    return result.returncode, result.stdout.strip(), result.stderr.strip()


def create_issue(repo: str, title: str, labels: List[str], body: str) -> bool:
    """Create a GitHub issue using the gh CLI."""
    labels_str = ",".join(labels)

    cmd = [
        "gh", "issue", "create",
        "--repo", repo,
        "--title", title,
        "--label", labels_str,
        "--body", body,
    ]

    rc, stdout, stderr = run(cmd)
    if rc == 0:
        url = stdout.strip()
        print(f"  ✅  Created: {url if url else 'OK'}")
        return True
    else:
        print(f"  ❌  Failed: {stderr or 'unknown error'}")
        return False


def ensure_label(repo: str, label: str, color: str = "0075ca") -> None:
    """Create a GitHub label if it does not already exist."""
    list_rc, list_out, _ = run(["gh", "label", "list", "--repo", repo, "--json", "name", "-q", ".[].name"])
    if list_rc == 0 and label in list_out.splitlines():
        return  # label already exists
    run(["gh", "label", "create", label, "--repo", repo, "--color", color, "--force"])


# ─── Main ─────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create GitHub Issues for the ThemisDB documentation audit."
    )
    parser.add_argument(
        "--create",
        action="store_true",
        help="Actually create issues via gh CLI (default: dry-run / print only)",
    )
    parser.add_argument(
        "--repo",
        default=REPO,
        help=f"Target GitHub repository (default: {REPO})",
    )
    args = parser.parse_args()

    print("=" * 70)
    print("ThemisDB – Docs Audit Issue Creator")
    print("=" * 70)
    print(f"Repository : {args.repo}")
    print(f"Mode       : {'LIVE – creating issues' if args.create else 'DRY-RUN – printing only'}")
    print(f"Issues     : {len(DOCS_AUDIT_ISSUES)}")
    print("=" * 70)
    print()

    if args.create:
        # Ensure required labels exist
        print("Ensuring labels exist …")
        ensure_label(args.repo, LABEL_DOCS_AUDIT, "0075ca")
        ensure_label(args.repo, LABEL_DOCUMENTATION, "0075ca")
        print()

    created = 0
    failed = 0

    for i, issue in enumerate(DOCS_AUDIT_ISSUES, start=1):
        print(f"[{i}/{len(DOCS_AUDIT_ISSUES)}] {issue['title']}")

        if args.create:
            ok = create_issue(args.repo, issue["title"], issue["labels"], issue["body"])
            if ok:
                created += 1
            else:
                failed += 1
        else:
            # Dry-run: show labels
            print(f"  Labels: {', '.join(issue['labels'])}")

        print()

    print("=" * 70)
    if args.create:
        print(f"✅ Created : {created}")
        print(f"❌ Failed  : {failed}")
    else:
        print(f"Would create {len(DOCS_AUDIT_ISSUES)} issues.")
        print("Re-run with --create to actually create them.")
    print("=" * 70)

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
