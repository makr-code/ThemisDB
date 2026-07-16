# Dokifi Scope Limiting Examples (2026-06-25)

## Problem: Large Project Task Narrowing

**Context:** ThemisDB has 127+ module documentation files across src/, include/, tests/, benchmarks/. Standard `/dokifi L1` would update all of them. For targeted work (e.g., only updating graph module after refactoring), scope-limiting reduces task scope.

---

## Example 1: Update Only src/graph/ L1 Docs

### Command
```bash
/dokifi L1 src/graph
```

### Execution Flow

```
[SCOPE] Level 1 (Module Docs) — SCOPED to src/graph/
[INFO] L0 status (informative)...
  ℹ️  ai_working/gap_scanner_results.json: last run 2026-06-25 09:15 (0.2 days ago)

[FILTER] L1 file discovery...
  Full scope would include: 127+ module docs across src/, include/, tests/
  Applied scope filter: src/graph/ only
  Matching files found:
    ✅ src/graph/README.md
    ✅ src/graph/ROADMAP.md
    ✅ src/graph/ARCHITECTURE.md
    ✅ src/graph/FUTURE_ENHANCEMENTS.md

[UPDATE] Scoped L1 documents
  src/graph/README.md: Updated from L0 gap_scanner
  src/graph/ROADMAP.md: Synced with module headers
  src/graph/ARCHITECTURE.md: Refreshed from source
  src/graph/FUTURE_ENHANCEMENTS.md: Updated with new findings

STATUS: ✅ L1 UPDATE COMPLETE (scoped to src/graph/ only)

[NEXT] Remaining L1 modules untouched. Run /dokifi L1 to update full scope or /dokifi L1 src/index for next module.
```

**Result:** Only 4 files updated instead of 127+, task focused on graph module changes.

---

## Example 2: Update Only ai_working/changelog Snapshots (L2)

### Command
```bash
/dokifi L2 ai_working/changelog_*.md
```

### Execution Flow

```
[SCOPE] Level 2 (Developer Aggregates) — SCOPED to ai_working/changelog_*.md
[INFO] L1 status (informative)...
  ⚠️  src/graph/README.md: 3 days old
  ⚠️  src/index/README.md: 5 days old
  ✅ Other L1 modules: recent (< 7 days)

[FILTER] L2 file discovery...
  Full scope would include: all ai_working/*.md (40+ files)
  Applied scope filter: ai_working/changelog_*.md pattern
  Matching files found:
    ✅ ai_working/changelog_snapshot_2026-06-25.md
    ✅ ai_working/changelog_summary.md
    ✅ ai_working/changelog_release_notes.md

[AGGREGATE] Scoped L2 updates
  Execute: module_doc_generator.py (changelog mode only)
  Source: src/graph/README.md, src/index/README.md (L1 changelog sections)
  ai_working/changelog_snapshot_2026-06-25.md: Aggregated graph + index changes
  ai_working/changelog_summary.md: Consolidated release narrative
  ai_working/changelog_release_notes.md: Formatted for publication

STATUS: ✅ L2 UPDATE COMPLETE (changelog snapshots only)

[NEXT] Other L2 snapshots (feature_*.md, security_*.md) untouched. Run /dokifi L2 for full scope or /dokifi L2 ai_working/feature_*.md for feature updates.
```

**Result:** Focused on changelog aggregation; other developer docs unmodified.

---

## Example 3: Update Only CHANGELOG.md (L3)

### Command
```bash
/dokifi L3 CHANGELOG.md
```

### Execution Flow

```
[SCOPE] Level 3 (Root Docs) — SCOPED to CHANGELOG.md
[INFO] L2 status (informative)...
  ℹ️  ai_working/changelog_snapshot_2026-06-25.md: last updated 2026-06-25 11:20 (0.2 days ago)
  ℹ️  ai_working/changelog_summary.md: last updated 2026-06-24 14:30 (1.5 days ago)

[FILTER] L3 file discovery...
  Full scope would include: CHANGELOG.md, README.md, SECURITY.md, ROADMAP.md (4 root docs)
  Applied scope filter: CHANGELOG.md only
  Matching files found:
    ✅ CHANGELOG.md

[VALIDATE] Peer L3 Coherence (within scope context)
  ℹ️  README.md: Exists (not in scope, no updates)
  ℹ️  SECURITY.md: Exists (not in scope, no updates)
  ✅ ROADMAP.md: Coherent with CHANGELOG.md changes (cross-link valid)

[UPDATE] Scoped L3 document
  CHANGELOG.md: Updated from L2 changelog_snapshot + release_notes
  Added: New feature entries from graph module refactoring
  Updated: Version markers, date stamps
  Links: Cross-references to ROADMAP.md maintained

STATUS: ✅ L3 UPDATE COMPLETE (CHANGELOG.md only)

[NEXT] Other L3 root docs (README.md, SECURITY.md) untouched. Run /dokifi L3 for full scope or /dokifi L3 README.md SECURITY.md for publication sync.
```

**Result:** CHANGELOG.md updated independently; other root docs remain stable.

---

## Example 4: Update Multiple Scoped Modules at Once

### Command
```bash
/dokifi L1 src/graph src/index src/query
```

### Execution Flow

```
[SCOPE] Level 1 (Module Docs) — SCOPED to 3 modules: src/graph, src/index, src/query
[INFO] L0 status (informative)...
  ✅ ai_working/gap_scanner_results.json: fresh (0.2 days old)

[FILTER] L1 file discovery...
  Applied scope filter: src/graph/ + src/index/ + src/query/
  Matching files found:
    ✅ src/graph/{README.md, ROADMAP.md, ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md}
    ✅ src/index/{README.md, ROADMAP.md, ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md}
    ✅ src/query/{README.md, ROADMAP.md, ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md}

[UPDATE] Scoped L1 documents (12 files across 3 modules)
  src/graph: 4 files updated
  src/index: 4 files updated
  src/query: 4 files updated

STATUS: ✅ L1 UPDATE COMPLETE (3 modules, 12 files)

[NEXT] Other 124+ L1 modules untouched. Run /dokifi L1 for full scope or /dokifi L1 src/cache src/storage for other modules.
```

**Result:** Batched update of 3 related modules; other 124 modules unchanged.

---

## Scope Limiting Patterns

| Pattern | Use Case | Example |
|---------|----------|---------|
| **Single module** | Update one module after changes | `/dokifi L1 src/graph` |
| **Multiple modules** | Batch update related modules | `/dokifi L1 src/graph src/index src/query` |
| **File pattern** | Update snapshots matching pattern | `/dokifi L2 ai_working/changelog_*.md` |
| **Single file** | Update one root doc | `/dokifi L3 CHANGELOG.md` |
| **Multiple files** | Update multiple root docs | `/dokifi L3 CHANGELOG.md README.md SECURITY.md` |
| **Directory** | Scoped gap_scan or doc aggregation | `/dokifi L0 src/graph` |

---

## Benefits of Scope Limiting

✅ **Focused Tasks:** Update only affected modules/docs instead of full scope
✅ **Performance:** Smaller scopes run faster (4 files vs 127+)
✅ **Parallel Workflows:** Team can work on different modules without conflicts
✅ **Incremental Updates:** Handle refactoring batch-by-batch
✅ **Testing:** Validate changes on scoped docs before full rollout
✅ **Large Project Manageability:** Task sizes proportional to actual work

---

## Validation & Prerequisites Still Apply

**Important:** All level-gated checks and validations still apply within the scoped context.

Example: `/dokifi L2 ai_working/changelog_*.md` still checks L1 status (informatively) even though it only updates changelog snapshots. If L1 is very stale, warning will note that and suggest `/dokifi L1` first (but won't block).

Scope limiting filters **what gets updated**, not **what gets validated**.
