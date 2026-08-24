# Dokifi Level-Based Orchestration — Practical Example (2026-06-25)

## Scenario: Update Root CHANGELOG.md with new feature

User issues:
```bash
/dokifi L3
```

## System Response (Automated L0→L3 Verification)

### Step 1: Check L2 Status (Informative Report)
```
[INFO] L2 (Developer Aggregates)...
  ℹ️  ai_working/gap_scanner_results.json: last updated 2026-06-25 09:15 (0.2 days ago)
  ℹ️  ai_working/module_doc_summary.md: last updated 2026-06-25 11:20 (fresh)
  ⚠️  ai_working/changelog_snapshot.md: last updated 2026-06-20 08:45 (5.2 days old — consider refresh)
  ✅  ROADMAP.md (L3-peer): up-to-date (0 days)
  ✅  FUTURE_ENHANCEMENTS.md (L3-peer): up-to-date (0.2 days)

STATUS: ℹ️ L2 AVAILABLE (age recommendations shown, but proceeding with L3 update)
```

Note: Even though `changelog_snapshot.md` is older, L3 update is NOT blocked. User can proceed or refresh L2 as desired.

### Step 2: Proceed with L3 Update

Since L2 is fresh, system proceeds to:

```
[UPDATE] CHANGELOG.md (L3)
  Source: ai_working/changelog_snapshot.md (L2 aggregate)
  Add: New feature "Graph Query Vectorization" with date 2026-06-25
  Link: src/graph/ROADMAP.md (L1 source), tests/graph/FUTURE_ENHANCEMENTS.md
  Validate: Naming (UPPER_SNAKE ✅), Structure (version/date/features ✅), Duktus (neutral, evidence-based ✅)

[UPDATE] README.md (L3)
  Source: ai_working/feature_summary.md (L2)
  Sync: Feature list with new "Graph Query Vectorization" entry
  Link: CHANGELOG.md date, ROADMAP.md phase

[VALIDATE] Peer L3 Coherence
  - SECURITY.md: ✅ No new security implications (audit passed)
  - ROADMAP.md: ✅ "Graph Query Vectorization" phase listed
  - FUTURE_ENHANCEMENTS.md: ✅ No conflicts with planned features

STATUS: ✅ L3 UPDATE COMPLETE
```

### Step 3: Conformance Report

```
CONFORMANCE REPORT (L3):
  ✅ Naming:        CHANGELOG.md, README.md conform to UPPER_SNAKE
  ✅ Structure:     Version headers, date markers, feature lists intact
  ✅ Duktus:        Technical, precise, evidence-backed claims
  ✅ SOT Mapping:   L2 aggregates → L3 summary coherent, linked
  ✅ L3-Peer Check: No conflicts between CHANGELOG, README, ROADMAP, SECURITY
  ✅ Release:       No draft/unstable markers in root docs

DEVIATIONS: None detected
```

### Step 4: Follow-Up Actions

```
NEXT STEP (Optional):
  - Publish updated CHANGELOG.md to release branch
  - Trigger CI to regenerate docs/ (L4 publication)
  - Suggested milestone: RELEASE-2026-Q3-FEATURE-BATCH

ISSUE (if needed):
  - Title: "Sync L3 CHANGELOG.md to published docs/ (L4)"
  - Milestone: RELEASE-2026-Q3-FEATURE-BATCH
  - Link: CHANGELOG.md commit hash
```

---

## Scenario 2: L2 Update with L1 Staleness Check

User issues:
```bash
/dokifi L2
```

### System Response

### System Response

```
[INFO] L1 (Module Docs)...
  ⚠️  src/graph/README.md: 80 days old (exceeds 7-day advisory threshold)
  ⚠️  src/graph/ROADMAP.md: 80 days old
  ⚠️  src/index/README.md: 75 days old
  ⚠️  ... (127 L1 files over 7-day advisory threshold)

L1 STATUS: ⚠️ Many L1 docs are old (127/139 files > 7 days)

RECOMMENDATION: Consider running /dokifi L1 first to refresh module documentation
             (but proceeding with L2 update as requested)
```

Note: Age warnings are informational. User can proceed with L2 or pause to update L1 first.

---

## Scenario 3: L1 Update with L0 Freshness Check

User issues:
```bash
/dokifi L1
```

### System Response

```
[INFO] L0 (Source Truth)...
  ℹ️  ai_working/gap_scanner_results.json: last updated 2026-06-25 09:15 (0.2 days old) ✅
  ℹ️  header_updates.log: last updated 2026-06-25 09:15 (0.1 days old) ✅
  ℹ️  src/graph/README.md headers: synced from gap_audit_pipeline_v3.py at 2026-06-25 09:15

STATUS: ℹ️ L0 AVAILABLE (fresh gap_scanner output available)
         Proceeding with L1 update
```

### Proceed with L1 Update

```
[UPDATE] L1 Module Docs
  Source: L0 gap_scanner_results.json + module headers
  
  Update src/graph/:
    - README.md: Add new sections from gap_scanner findings
    - ROADMAP.md: Mark complete phases, update target dates
    - ARCHITECTURE.md: Sync with source headers
  
  Update src/index/:
    - README.md, ROADMAP.md, ARCHITECTURE.md (similar)
  
  ... (127 module updates)

STATUS: ✅ L1 UPDATE COMPLETE (127 modules synchronized)
```

---

## Control Flow Diagram (Informative Age Reporting, No Hard Blocks)

```
/dokifi L0
  └─> Execute: gap_scanner_v3.py → gap_audit_pipeline_v3.py
      └─> Output: ai_working/gap_scanner_results.json, header_updates.log

/dokifi L1
  ├─> REPORT: L0 status (ℹ️ age, ⚠️ if old, but no block)
  │   └─> if missing: ⚠️ "Consider running /dokifi L0 first"
  └─> PROCEED: Update src/*/README.md, ROADMAP.md, ARCHITECTURE.md
      └─> Source: L0 gap_scanner output + module headers

/dokifi L2
  ├─> REPORT: L1 status (ℹ️ age, ⚠️ if old, but no block)
  │   └─> if stale/sparse: ⚠️ "Many L1 docs are old; consider running /dokifi L1 first"
  └─> PROCEED: Execute module_doc_generator.py
      └─> OUTPUT: ai_working/ snapshots, release narratives

/dokifi L3
  ├─> REPORT: L2 status (ℹ️ age, ⚠️ if old, but no block)
  │   └─> if stale: ⚠️ "L2 snapshots are old; consider running /dokifi L2 first"
  └─> PROCEED: Update CHANGELOG.md, README.md, SECURITY.md
      └─> Source: ai_working/ L2 aggregates
      └─> Validate: Peer L3 coherence (ROADMAP, FUTURE_ENHANCEMENTS)
```

---

## Key Features of Level-Based Model

✅ **Smart Ordering:** L0→L1→L2→L3 sequence is recommended, but flexible
✅ **Informative Warnings:** Age and freshness shown for context, not blocking
✅ **Self-Contained L0:** Gap scanning independent; can run anytime
✅ **Transparent Status:** Each level reports upstream age and availability
✅ **User Control:** User decides whether to refresh upstream docs or proceed
✅ **Audit Trail:** Each level logs source, timestamp, validation results
✅ **Backward-Compatible:** Old path-based commands still work (auto-infer level)
