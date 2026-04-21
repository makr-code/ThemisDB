# Docs-Audit Issue Backlog

**Version:** 1.0  
**Created:** 2026-02-23  
**Source:** `docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md` (2026-01-12)  
**Audit:** v1.4.1 findings  
**Status:** Backlog — Issues to be created via `.github/scripts/create-docs-audit-issues.py`

---

## How to Create These Issues

Run the batch-creation script with a valid GitHub token:

```bash
# Dry-run (preview without creating)
DRY_RUN=1 \
  GITHUB_TOKEN=<token> \
  GITHUB_REPOSITORY=makr-code/ThemisDB \
  python .github/scripts/create-docs-audit-issues.py

# Actually create the issues
DRY_RUN=0 \
  GITHUB_TOKEN=<token> \
  GITHUB_REPOSITORY=makr-code/ThemisDB \
  python .github/scripts/create-docs-audit-issues.py
```

Or trigger the manual workflow in GitHub Actions:
**Actions → "Create Docs-Audit Issues" → Run workflow**

---

## Issue Backlog

| ID | Title | Priority | Labels | Status |
|----|-------|----------|--------|--------|
| DOCS-001 | Fix stale CMake modular architecture documentation | High | `type:documentation`, `priority:high` | ⬜ Open |
| DOCS-002 | Clarify plugin system documentation: templates vs. production | High | `type:documentation`, `priority:high` | ⬜ Open |
| DOCS-003 | Add comprehensive API documentation for WebSocket, MQTT, gRPC, MCP | Medium | `type:documentation`, `priority:medium` | ⬜ Open |
| DOCS-004 | Create examples index and quickstart guide for numbered examples (01-22) | Medium | `type:documentation`, `priority:medium` | ⬜ Open |
| DOCS-005 | Create tools documentation hub for Python scripts and .NET admin tools | Medium | `type:documentation`, `priority:medium` | ⬜ Open |
| DOCS-006 | Add implementation-status column to feature flags reference | Medium | `type:documentation`, `priority:medium` | ⬜ Open |
| DOCS-007 | Create unified English documentation index for `docs/en/` | Medium | `type:documentation`, `priority:medium` | ⬜ Open |
| DOCS-008 | Add automated documentation link validation to CI pipeline | Low | `type:documentation`, `priority:low` | ⬜ Open |
| DOCS-009 | Document all undocumented `src/` subdirectories | Medium | `type:documentation`, `priority:medium` | ⬜ Open |

---

## Issue Details

### DOCS-001 – Fix stale CMake modular architecture documentation

**Affected file:** `docs/architecture/CMAKE_MODULAR_ARCHITECTURE.md`

**Problem:** Document describes `cmake/Features/`, `cmake/Targets/`,
`LLM.cmake`, `GPU.cmake`, `gRPC.cmake` — none of which exist. The document
also claims the modular architecture is "Production Ready".

**Fix options:**
1. Update doc to reflect the **actual** current `cmake/` structure
2. Implement the modular cmake architecture described and close the gap

**Acceptance criteria:**
- [ ] Document reflects what actually exists in `cmake/`
- [ ] Planned features are clearly marked as future work with a target quarter
- [ ] Broken links to non-existent `.cmake` files are removed
- [ ] DE/EN versions kept in sync

---

### DOCS-002 – Clarify plugin system documentation: templates vs. production

**Affected files:** `plugins/README.md`, `docs/architecture/`

**Problem:** Documentation describes a production-ready runtime plugin loading
system. In reality, `plugins/` contains only template/example files.
5 plugin subdirectories (`blob_storage`, `exporters`, `image_analysis`,
`importers`, `rpc`) are completely undocumented.

**Fix:**
- Mark current system as "in development / template stage"
- Document all existing plugin subdirectories
- Create contributor guide reflecting actual workflow

**Acceptance criteria:**
- [ ] `plugins/README.md` accurately describes current state
- [ ] All undocumented plugin directories have at least a brief description
- [ ] Implementation status (template vs. production) is clearly indicated

---

### DOCS-003 – Add comprehensive API documentation for WebSocket, MQTT, gRPC, MCP

**Affected files:** `docs/api/` (missing files for each protocol)

**Problem:** WebSocket, MQTT, gRPC, and MCP APIs have implementation files
but only minimal or no documentation. Impact: integration partners and
external developers cannot use these APIs without reverse-engineering the code.

**Deliverables:**
- `docs/api/websocket_api.md`
- `docs/api/mqtt_api.md`
- `docs/api/grpc_api.md` (complete)
- `docs/api/mcp_api.md`

**Acceptance criteria:**
- [ ] Each protocol doc covers: connection setup, message format, auth, errors, examples
- [ ] All documents include runnable code examples
- [ ] `docs/openapi.yaml` updated for REST endpoints

---

### DOCS-004 – Create examples index and quickstart guide (examples 01-22)

**Affected files:** `examples/README.md`, `docs/00_DOCUMENTATION_INDEX.md`

**Problem:** 22 numbered examples are not indexed anywhere in the main docs.
Users cannot discover them.

**Deliverables:**
- Updated `examples/README.md` with table of all 22+ examples
- Categories: Basic, Storage, LLM, Query, Sharding, Observability
- Link from main docs index

**Acceptance criteria:**
- [ ] All 22 numbered examples listed with description and run command
- [ ] Main docs index links to examples overview

---

### DOCS-005 – Create tools documentation hub

**Affected files:** `docs/TOOLS_INDEX.md`, `tools/README.md`

**Problem:** 30+ tools with minimal or no documentation. Only `Themis.IngestionTool`
is documented. Key undocumented tools: `namespace_analyzer.py`,
`shard_bench.py`, `fault_injector.py`, all 11 .NET admin tool projects.

**Deliverables:**
- `docs/TOOLS_INDEX.md` with complete table of all tools
- `--help` output documented for each Python script
- README for each .NET admin tool project

---

### DOCS-006 – Add implementation-status column to feature flags reference

**Affected file:** `docs/architecture/FEATURE_FLAGS_REFERENCE.md`

**Problem:** Feature flags table does not indicate which flags correspond to
implemented features vs. experimental or planned ones. Default values in doc
sometimes differ from `CMakeLists.txt`.

**Fix:** Add `Implementation Status` column (Stable / Experimental / Planned / Deprecated).
Sync all default values with `CMakeLists.txt`.

---

### DOCS-007 – Create unified English documentation index for `docs/en/`

**Affected files:** `docs/en/` (no index exists)

**Problem:** `docs/de/` has a comprehensive index. `docs/en/` has none.
English documentation coverage lags German in LLM Features, API Reference,
Guides, and Build Guides.

**Deliverables:**
- `docs/en/00_DOCUMENTATION_INDEX.md` modelled after the German index
- Priority list of DE → EN translation candidates

---

### DOCS-008 – Add automated documentation link validation to CI

**Affected files:** `.github/workflows/` (new workflow needed)

**Problem:** Link checking is manual (last run Jan 2026). `docs/link_check_results.txt`
shows 10+ dead links per file. No CI prevents new dead links being added.

**Deliverables:**
- `.github/workflows/docs-link-check.yml` using `lychee` or `markdown-link-check`
- Triggers on PRs touching `docs/**`
- Internal broken links → CI failure; external → PR warning

---

### DOCS-009 – Document all undocumented `src/` subdirectories

**Affected directories:** Various `src/<module>/` directories

**Problem:** Despite `docs/architecture/SOURCE_DIRECTORY_GUIDE.md` being created,
several module-level README files are still missing from within the source tree.
`src/utils/` in particular has no in-tree documentation.

**Fix:** Add `README.md` to each `src/<module>/` that still lacks one.
Verify `SOURCE_DIRECTORY_GUIDE.md` is accurate and up-to-date.

---

## Tracking

Update this table when issues are created in GitHub:

| ID | GitHub Issue # | URL | Created |
|----|----------------|-----|---------|
| DOCS-001 | — | — | — |
| DOCS-002 | — | — | — |
| DOCS-003 | — | — | — |
| DOCS-004 | — | — | — |
| DOCS-005 | — | — | — |
| DOCS-006 | — | — | — |
| DOCS-007 | — | — | — |
| DOCS-008 | — | — | — |
| DOCS-009 | — | — | — |

---

*Last updated: 2026-02-23*
