#!/usr/bin/env python3
"""
create-docs-audit-issues.py
----------------------------
Batch-creates GitHub Issues for the ThemisDB documentation audit based on
findings from docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md and the
docs/audit-reports/v1.4.1/ review.

Usage:
    GITHUB_TOKEN=<token> GITHUB_REPOSITORY=makr-code/ThemisDB python3 create-docs-audit-issues.py

Environment variables:
    GITHUB_TOKEN       – Personal access token with `repo` scope
    GITHUB_REPOSITORY  – Owner/repo (e.g. makr-code/ThemisDB)
    DRY_RUN            – Set to "1" to print issues without creating them (default: "0")
"""

import json
import os
import sys
import time
from urllib.request import Request, urlopen
from urllib.error import HTTPError

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN", "")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB")
DRY_RUN = os.environ.get("DRY_RUN", "0") == "1"

# ---------------------------------------------------------------------------
# Issue definitions
# Each entry maps directly to a GitHub issue body and metadata.
# Source: docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md (Major Gaps section)
#         docs/audit-reports/v1.4.1/FINDINGS_TRACKER.md
# ---------------------------------------------------------------------------

DOCS_AUDIT_ISSUES = [
    {
        "title": "[docs] Fix stale CMake modular architecture documentation",
        "labels": ["type:documentation", "priority:high", "status:open"],
        "body": """\
## Summary

`docs/architecture/CMAKE_MODULAR_ARCHITECTURE.md` describes a modular CMake
architecture (`cmake/Features/`, `cmake/Targets/`, `LLM.cmake`, `GPU.cmake`,
`gRPC.cmake`) that **does not exist** in the repository. This actively misleads
contributors trying to extend the build system.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-001
- **Severity:** High
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §1.1

## Affected Files

- `docs/architecture/CMAKE_MODULAR_ARCHITECTURE.md` – describes non-existent structure
- `docs/00_DOCUMENTATION_INDEX.md` – contains broken links to `cmake/Features/*.cmake`
- `cmake/CMakeLists.txt` – actual monolithic file (reference implementation)

## Current State

The document claims:
- `cmake/Features/` and `cmake/Targets/` directories exist
- Modular `.cmake` files (`LLM.cmake`, `GPU.cmake`, `gRPC.cmake`) exist
- Architecture is "Production Ready" and already refactored

None of these claims are true. The `cmake/` directory contains only
`CMakeLists.txt`, `CompilerOptions.cmake`, `Dependencies.cmake`,
`Versions.cmake`, and a few others.

## Expected State

Either:
1. Update the document to reflect the **actual** current cmake structure and
   mark the modular architecture as **planned** (not implemented), OR
2. Implement the modular cmake architecture described in the doc and remove
   the discrepancy

## Acceptance Criteria

- [ ] Document accurately reflects what currently exists in `cmake/`
- [ ] Any "planned" features are clearly marked as future work with a target quarter
- [ ] All broken links to non-existent `.cmake` files are removed or updated
- [ ] German and English documentation versions are kept in sync
""",
    },
    {
        "title": "[docs] Clarify plugin system documentation: templates vs. production",
        "labels": ["type:documentation", "priority:high", "status:open"],
        "body": """\
## Summary

`plugins/README.md` and related docs describe a **production-ready** runtime
plugin loading system with DLL/SO plugins. The actual `plugins/` directory
contains only example/template files, not a functional runtime plugin loader.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-002
- **Severity:** High
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §2.2

## Affected Files

- `plugins/README.md` – misleading production-ready claims
- `docs/architecture/` – references to plugin runtime system
- `plugins/cuda/` – labeled "example" but docs treat as production

## Current State

Documentation states the plugin system supports runtime-loadable
`.dll`/`.so` plugins for CUDA, Vulkan, DirectX, HIP, and Metal backends.
In reality, the `plugins/` directory contains only:
- `plugins/cuda/*.example` – template/example files
- `plugins/blob_storage/` – undocumented
- `plugins/exporters/jsonl_llm/` – undocumented
- `plugins/image_analysis/onnx_clip/` – undocumented
- `plugins/importers/postgres/` – undocumented
- `plugins/rpc/grpc/` – undocumented

## Expected State

- Mark current plugin system as "in development" or "template stage"
- Document all existing plugin subdirectories (`blob_storage`, `exporters`,
  `image_analysis`, `importers`, `rpc`)
- Create a development guide for plugin contributors that reflects reality
- Add roadmap entry for full runtime plugin loading if still planned

## Acceptance Criteria

- [ ] `plugins/README.md` accurately describes the current state
- [ ] All undocumented plugin subdirectories have at least a brief description
- [ ] Implementation status (template vs. production) is clearly indicated
- [ ] Plugin developer guide reflects actual build and testing workflow
""",
    },
    {
        "title": "[docs] Add comprehensive API documentation for WebSocket, MQTT, gRPC, MCP protocols",
        "labels": ["type:documentation", "priority:medium", "status:open"],
        "body": """\
## Summary

Multiple network protocols implemented in `src/server/` and `src/network/`
lack comprehensive documentation. WebSocket, MQTT, gRPC, and MCP APIs each
have implementation files but only minimal or no documentation.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-003
- **Severity:** Medium
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §6.1

## Affected Files

- `src/server/websocket_session.cpp` – WebSocket implementation, no API docs
- `src/server/mqtt_*.cpp` – MQTT implementation, no API docs
- `src/server/*grpc*.cpp` – gRPC implementation, partial docs
- `src/server/mcp_server.cpp` – MCP implementation, no API docs
- `docs/api/` – missing protocol-specific API reference files

## Current State

| Protocol | Source Files | Documentation | Status |
|----------|-------------|---------------|--------|
| HTTP/REST | src/api/http_server.cpp | ✅ Multiple docs | Good |
| WebSocket | src/server/websocket_*.cpp | ⚠️ Minimal | Gap |
| MQTT | src/server/mqtt_*.cpp | ⚠️ Minimal | Gap |
| gRPC | src/server/*grpc*.cpp | ⚠️ Partial | Gap |
| MCP | src/server/mcp_server.cpp | ⚠️ None | Gap |
| PostgreSQL Wire | src/server/postgres_*.cpp | ⚠️ Minimal | Gap |

## Expected State

Each protocol should have a dedicated API reference document covering:
- Connection setup and configuration
- Message format / request-response schema
- Authentication and authorization
- Error codes and handling
- Example client code

## Acceptance Criteria

- [ ] `docs/api/websocket_api.md` created with full reference
- [ ] `docs/api/mqtt_api.md` created with full reference
- [ ] `docs/api/grpc_api.md` completed with all RPC definitions
- [ ] `docs/api/mcp_api.md` created with full reference
- [ ] All documents include runnable code examples
- [ ] `docs/openapi.yaml` updated to reflect any REST endpoints for these protocols
""",
    },
    {
        "title": "[docs] Create examples index and quickstart guide for numbered examples (01-22)",
        "labels": ["type:documentation", "priority:medium", "status:open"],
        "body": """\
## Summary

The `examples/` directory contains 22+ numbered examples (`01_hello_world/`
through `22_aql_diagram_tool/`) that are NOT indexed in the main documentation.
Each has its own README but there is no overview document to help users
discover them.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-004
- **Severity:** Medium
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §3.1

## Affected Files

- `examples/` – 22+ numbered example directories without index
- `docs/EXAMPLES_INDEX.md` – exists but does not cover numbered examples
- `docs/EXAMPLES_INDEX_NEW.md` – newer version, unclear if complete
- `docs/00_DOCUMENTATION_INDEX.md` – no link to examples overview

## Current State

Examples documented:
- LLM Examples: 8 .cpp files – ✅ Documented
- Voice Assistant: ✅ Full guide
- Numbered (01-22): 22 directories – ⚠️ NOT indexed in main docs

## Expected State

1. A unified `examples/README.md` with a table of all 22+ examples, their
   purpose, and a one-line quickstart command
2. Updated `docs/00_DOCUMENTATION_INDEX.md` to link to the examples overview
3. Categorized index: Basic, Storage, LLM, Query, Sharding, Observability

## Acceptance Criteria

- [ ] `examples/README.md` covers all 22 numbered examples in a table
- [ ] Each row includes: name, one-line description, prerequisites, run command
- [ ] Main documentation index links to the examples overview
- [ ] Examples are grouped into logical categories
- [ ] All example READMEs are verified to be accurate
""",
    },
    {
        "title": "[docs] Create tools documentation hub for Python scripts and .NET admin tools",
        "labels": ["type:documentation", "priority:medium", "status:open"],
        "body": """\
## Summary

The `tools/` directory contains 30+ items (Python scripts, .NET applications,
C++ utilities) with minimal or no documentation. Only `Themis.IngestionTool`
has comprehensive documentation. Most Python scripts and all .NET admin tools
are undiscoverable.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-005
- **Severity:** Medium
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §4.1

## Affected Files

- `tools/` – 30+ items with minimal documentation
- `docs/TOOLS_INDEX.md` – exists but incomplete
- `docs/tools/` – subdirectory with some tool docs

## Current State

| Category | Items | Documentation Status |
|----------|-------|---------------------|
| Admin Tools (.NET/C#) | 11 projects | ⚠️ tools/README.md only |
| Python Scripts | 12 files | ⚠️ Minimal inline docs |
| C++ Utilities | 3 files | ⚠️ No documentation |
| Ingestion Tool | 1 main tool | ✅ Comprehensive docs |

Tools currently without documentation:
- `namespace_analyzer.py`
- `wordpress_category_extractor.py`
- `shard_bench.py`, `shard_loader.py`
- `fault_injector.py`
- `compare_hyperscaler.py`
- `aggregate_shard_results.py`
- All .NET admin tools (Themis.AdminTools.Shared, Themis.DocumentManager, etc.)

## Expected State

- `docs/tools/README.md` (or `docs/TOOLS_INDEX.md`) with table of all tools
- Each tool entry: name, purpose, usage example, prerequisites
- `.NET` admin tools: brief description of each project and its CLI interface

## Acceptance Criteria

- [ ] `docs/TOOLS_INDEX.md` lists every tool in `tools/` with description and usage
- [ ] Each Python script has at least a `--help` output documented
- [ ] Each .NET admin tool project has a README with purpose and CLI usage
- [ ] Main docs index links to the tools hub
""",
    },
    {
        "title": "[docs] Add implementation-status column to feature flags reference",
        "labels": ["type:documentation", "priority:medium", "status:open"],
        "body": """\
## Summary

`docs/architecture/FEATURE_FLAGS_REFERENCE.md` lists all CMake feature flags
but does not clearly indicate which flags correspond to fully implemented
features vs. experimental or planned ones. This causes confusion about what
is actually available.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-006
- **Severity:** Medium
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §1.2

## Affected Files

- `docs/architecture/FEATURE_FLAGS_REFERENCE.md` – missing status column
- `CMakeLists.txt` – source of truth for default values

## Current State

The feature flag table shows: flag name, description, default value.
It does NOT show: implementation status, stability tier, since-version.

Several flags have incorrect or missing default values compared to
`CMakeLists.txt`:
- `THEMIS_ENABLE_HTTP2` – flag exists, implementation status unknown
- `THEMIS_ENABLE_HTTP3` – marked experimental but not in docs
- `THEMIS_ENABLE_WEBSOCKET` – flag exists, implementation status unknown
- `THEMIS_ENABLE_MQTT` – flag exists, implementation status unknown
- `THEMIS_ENABLE_MCP` – flag exists, implementation status unknown

## Expected State

Add an **Implementation Status** column to the feature flag table:
- ✅ Stable – fully implemented and tested
- ⚠️ Experimental – functional but not production-ready
- 🚧 Planned – flag exists but feature not yet implemented
- ❌ Deprecated – no longer recommended

Also sync default values with `CMakeLists.txt`.

## Acceptance Criteria

- [ ] Feature flag table includes `Implementation Status` column
- [ ] All default values match `CMakeLists.txt`
- [ ] Experimental and planned flags are clearly marked
- [ ] Document links to the relevant module documentation for each flag
""",
    },
    {
        "title": "[docs] Create unified English documentation index for docs/en/",
        "labels": ["type:documentation", "priority:medium", "status:open"],
        "body": """\
## Summary

`docs/de/` has a comprehensive bilingual index (`00_DOCUMENTATION_INDEX.md`)
covering all German documentation. `docs/en/` has no equivalent unified index.
English documentation is less complete than German in several areas, making
the project harder to navigate for international contributors.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-007
- **Severity:** Medium
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §5.1, §5.2

## Affected Files

- `docs/en/` – no unified index file
- `docs/de/00_DOCUMENTATION_INDEX.md` – reference model to follow
- `docs/00_DOCUMENTATION_INDEX.md` – bilingual but German-centric

## Current State

| Document Category | DE | EN | Coverage Gap |
|-------------------|----|----|--------------|
| Core Documentation | ✅ | ✅ | Low |
| Architecture | ✅ | ✅ | Low |
| LLM Features | ✅ | ⚠️ | Medium |
| API Reference | ✅ | ⚠️ | Medium |
| Guides | ✅ | ⚠️ | Medium |
| Build Guides | ✅ | ⚠️ | Medium |

`docs/en/` has no `00_DOCUMENTATION_INDEX.md` or `DOCUMENTATION_INDEX.md`.

## Expected State

1. Create `docs/en/00_DOCUMENTATION_INDEX.md` modelled after the German index
2. Identify all German-only documents that should be translated to English
3. Prioritize translation for: architecture, API reference, build guides,
   getting started, and key guides

## Acceptance Criteria

- [ ] `docs/en/00_DOCUMENTATION_INDEX.md` created and links all English docs
- [ ] All documents in `docs/en/` are reachable from the index
- [ ] Priority list of German → English translation candidates is defined
- [ ] Main `docs/00_DOCUMENTATION_INDEX.md` updated to link the EN index
""",
    },
    {
        "title": "[docs] Add automated documentation link validation to CI pipeline",
        "labels": ["type:documentation", "priority:low", "status:open"],
        "body": """\
## Summary

The repository has a `docs/link_check_results.txt` (last updated Jan 2026)
showing dead links. There is no CI job that automatically detects broken
internal or external links in documentation on every PR. This allows link
rot to accumulate silently.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12
- **Finding ID:** DOCS-008
- **Severity:** Low
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §13
- **Evidence:** `docs/link_check_results.txt` shows 10+ dead links per file

## Affected Files

- `.github/workflows/` – no link-check workflow exists
- `docs/link_check_results.txt` – manual check, not automated
- Multiple docs files with dead external links

## Current State

Link checking is manual and last run in January 2026. Known dead links include:
- `https://makr-code.github.io/ThemisDB/` (GitHub Pages not set up)
- Multiple Medium/DZone/Microsoft Learn articles
- Google Research PDFs
- arXiv papers

## Expected State

A GitHub Actions workflow (e.g., using `lychee` or `markdown-link-check`)
that:
1. Runs on every PR that modifies `docs/**/*.md`
2. Reports dead links as PR annotations
3. Fails only on **internal** broken links (external may be flaky)
4. Weekly scheduled run for external link freshness

## Acceptance Criteria

- [ ] `.github/workflows/docs-link-check.yml` created
- [ ] Workflow triggers on PRs touching `docs/**`
- [ ] Internal broken links cause CI failure
- [ ] External broken links create a warning comment on PR
- [ ] `docs/link_check_results.txt` replaced by CI-generated report
- [ ] Documentation updated to describe the new link validation process
""",
    },
    {
        "title": "[docs] Document all undocumented src/ subdirectories",
        "labels": ["type:documentation", "priority:medium", "status:open"],
        "body": """\
## Summary

The `src/` directory contains 35 subdirectories of which only ~8 were
documented at the time of the January 2026 gap analysis. While
`docs/architecture/SOURCE_DIRECTORY_GUIDE.md` was created to address this,
several module-level ROADMAP and README files are still missing from source
directories.

## Audit Reference

- **Audit Version:** v1.4.1 / Gap Analysis 2026-01-12  
- **Finding ID:** DOCS-009
- **Severity:** Medium
- **Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` §2.1

## Affected Source Directories (verify current state)

Run `find src -maxdepth 1 -type d` to confirm current gaps. Based on the
gap analysis, these previously lacked any documentation:

`analytics`, `api`, `auth`, `base`, `cache`, `cdc`, `content`, `exporters`,
`geo`, `governance`, `importers`, `network`, `observability`, `performance`,
`plugins`, `scheduler`, `search`, `temporal`, `timeseries`, `transaction`,
`updates`, `utils`, `voice`

## Expected State

Each `src/<module>/` directory should have:
1. `src/<module>/README.md` – purpose, key classes, integration points
2. `src/<module>/ROADMAP.md` – current status, planned features, phases

Alternatively, `docs/architecture/SOURCE_DIRECTORY_GUIDE.md` should fully
cover all 35 directories with up-to-date information.

## Acceptance Criteria

- [ ] Audit current state: list src/ dirs still missing in-source README
- [ ] Each missing directory gets at minimum a `README.md` (purpose + key files)
- [ ] `docs/architecture/SOURCE_DIRECTORY_GUIDE.md` is verified as accurate
- [ ] `utils/` module (previously SRC-ONLY) gets documentation
""",
    },
]


# ---------------------------------------------------------------------------
# GitHub API helpers
# ---------------------------------------------------------------------------

def _headers() -> dict:
    return {
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }


def create_issue(title: str, body: str, labels: list[str]) -> dict | None:
    """Create a GitHub issue. Returns the created issue dict or None on failure."""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues"
    payload = json.dumps({"title": title, "body": body, "labels": labels}).encode()
    req = Request(url, data=payload, headers=_headers(), method="POST")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read())
    except HTTPError as exc:
        print(f"  ❌  HTTP {exc.code}: {exc.read().decode()}", file=sys.stderr)
        return None


def issue_exists(title: str) -> bool:
    """Return True if an open issue with this exact title already exists."""
    encoded = title.replace(" ", "+")
    url = (
        f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues"
        f"?state=open&per_page=100"
    )
    req = Request(url, headers=_headers())
    try:
        with urlopen(req) as resp:
            issues = json.loads(resp.read())
            return any(i["title"] == title for i in issues)
    except HTTPError:
        return False


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    if not GITHUB_TOKEN and not DRY_RUN:
        print("❌  GITHUB_TOKEN not set. Export it or set DRY_RUN=1.", file=sys.stderr)
        return 1

    print(f"Repository : {GITHUB_REPOSITORY}")
    print(f"Dry-run    : {DRY_RUN}")
    print(f"Issues     : {len(DOCS_AUDIT_ISSUES)}")
    print()

    created = 0
    skipped = 0
    failed = 0

    for issue in DOCS_AUDIT_ISSUES:
        title = issue["title"]
        labels = issue["labels"]
        body = issue["body"]

        print(f"→ {title}")

        if DRY_RUN:
            print(f"  [DRY-RUN] Would create with labels: {labels}")
            created += 1
            continue

        if issue_exists(title):
            print("  ⚠️  Already exists – skipping")
            skipped += 1
            continue

        result = create_issue(title, body, labels)
        if result:
            print(f"  ✅  Created: #{result['number']} – {result['html_url']}")
            created += 1
        else:
            print("  ❌  Failed to create issue")
            failed += 1

        # Respect GitHub secondary rate limit
        time.sleep(1)

    print()
    print("=" * 60)
    print(f"Created : {created}")
    print(f"Skipped : {skipped}")
    print(f"Failed  : {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
