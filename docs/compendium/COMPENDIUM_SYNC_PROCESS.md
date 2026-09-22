# Compendium Sync Process

> Status: active  
> Version: 1.0  
> Last Updated: 2026-09-22  
> Scope: `docs/compendium/docs/*.md` maintenance against ThemisDB v2.4.0-alpha SOT

This document describes the **human-in-the-loop operating model** for recurring
compendium refreshes. It is the authoritative reference for maintainers who
receive drift notifications from the `maintenance-compendium-sync` workflow.

---

## 1. Overview

The compendium (`docs/compendium/docs/*.md`) is a human-readable multi-chapter
reference book for ThemisDB. Because the project roadmap evolves continuously,
chapters can drift from the canonical source-of-truth (SOT) documents.

**This process keeps them aligned incrementally and safely**, with human
approval at every write step. No automated system rewrites or merges chapter
content without review.

### Control surfaces

| Surface | Role |
|---------|------|
| `CHAPTER_ROADMAP_MAPPING.yml` | Maps each chapter to its roadmap topics and SOT files |
| `scripts/compendium-drift-scan.py` | Detects stale chapters; emits JSON + Markdown reports |
| `.github/workflows/maintenance-compendium-sync.yml` | Schedules / triggers scans; opens tracker issues |
| GitHub Issue (label: `compendium`) | Tracks detected drift; routes tasks to assignees |
| Pull Request against `develop` | The safe unit of chapter update; requires human review |

---

## 2. Recurring Cycle

```
Weekly Monday 06:30 UTC
        │
        ▼
[maintenance-compendium-sync]
   Runs compendium-drift-scan.py
        │
        ├─ No drift found ──► Close / update tracker issue ✅
        │
        └─ Drift detected
               │
               ▼
        Upsert tracker issue
        Label: documentation, compendium, status/needs-attention
               │
               ▼
        Human maintainer reviews issue + artifact report
               │
               ├─ Assign chapter update tasks (one issue comment per chapter)
               │
               ▼
        Author opens draft PR against `develop`
        Updates one or more chapters per PR
               │
               ▼
        Human review + approval
               │
               ▼
        Merge to `develop` ✅
```

---

## 3. Chapter Update Procedure (step by step)

### 3.1 Download the drift report

1. Open the tracker issue labeled `compendium`.
2. Click the linked workflow run.
3. Download the `compendium-drift-report` artifact.
4. Open `compendium-drift-summary.md` for the human-readable overview.
5. Open `compendium-drift.json` for chapter-level detail (stale versions, missing
   v2.4.0 signals, active SOT topics).

### 3.2 Identify which chapters to update

Use `CHAPTER_ROADMAP_MAPPING.yml` to find:
- Which SOT files drive a chapter's content.
- Which roadmap topics the chapter must reflect.
- The chapter's priority (`high` / `medium` / `low`).

Start with `priority: high` chapters. Use the JSON report's `active_sot_topics`
list to know exactly which roadmap areas are missing from the chapter.

### 3.3 Open a draft PR

- Target branch: `develop`
- Scope: one chapter per PR (or tightly related chapter pairs)
- Title convention: `docs(compendium): update chapter_NN_<topic> to v2.4.0-alpha`
- Reference the tracker issue in the PR body (`Closes #NNN` or `Part of #NNN`)

### 3.4 Update the chapter

Use the SOT files listed in `CHAPTER_ROADMAP_MAPPING.yml` as your input sources:

- `ROADMAP.md` — v2.4.0-alpha status, Wave gate posture, source-verified claims
- `FUTURE_ENHANCEMENTS.md` — planned capability direction
- `src/<module>/ROADMAP.md` — module-level detail
- `docs/architecture/MODULE_ARCHITECTURE.md` — cross-module integration

**Rules:**
- Do not claim capabilities that are not source-verified in `ROADMAP.md`.
- Use the Wave A/B/C/D posture language from `ROADMAP.md` when describing
  implementation status.
- Update the version header in the chapter file to `2.4.0-alpha`.
- Keep the existing chapter structure; add or revise sections rather than
  rewriting from scratch.

### 3.5 Request review and merge

- Remove the `draft` flag once the update is ready.
- Assign at least one human reviewer.
- Merge only after approval.
- Close the per-chapter sub-task in the tracker issue after merge.

---

## 4. New Chapter Proposals

The drift scan also evaluates `gap_topics` entries in `CHAPTER_ROADMAP_MAPPING.yml`.
When a gap topic's keywords appear in its mapped SOT files, the scan emits a
**new chapter proposal** in the report.

### When a new chapter is proposed

1. **Confirm scope**: decide whether the topic warrants a standalone chapter or
   whether an existing chapter should be extended. Discuss in the tracker issue.
2. **Choose the filename**: use the `suggested_file` from the proposal as a
   starting point. Rename as needed to fit the compendium numbering scheme.
3. **Update `CHAPTER_ROADMAP_MAPPING.yml`**:
   - Move the entry from `gap_topics` into the `chapters` list with the confirmed
     `file` name.
   - Set `priority`, `roadmap_topics`, and `sot_paths` appropriately.
4. **Create the chapter file** under `docs/compendium/docs/`.
   - Follow the existing chapter structure (version header, sections, cross-links).
   - Use the SOT files from the mapping entry as primary sources.
   - Ensure the version header reads `2.4.0-alpha`.
5. **Register in `mkdocs-nav.yml`**: add the new file to the navigation. If the
   chapter number is already in use, assign the next available `a/b` suffix and
   document it in `docs/compendium/docs/INTEGRATION_MAPPING.md`.
6. **Open a draft PR against `develop`** and follow the standard review process.

### Adding a new gap topic

When a new roadmap area emerges that has no existing chapter:

1. Add an entry to the `gap_topics` section of `CHAPTER_ROADMAP_MAPPING.yml`.
2. Fill in: `topic`, `keywords`, `sot_paths`, `priority`, `rationale`,
   `suggested_file`.
3. The next drift scan will automatically propose the new chapter if the keywords
   are found in the SOT files.

---

## 5. Manual Trigger

Maintainers can run the drift scan on demand:

```
GitHub → Actions → "Maintenance: Compendium Sync" → Run workflow
```

Available inputs:
| Input | Default | Description |
|-------|---------|-------------|
| `priority` | `medium` | Minimum chapter priority to scan |
| `target_version` | `2.4.0-alpha` | Version string to check against |
| `verbose` | `false` | Print per-chapter detail to workflow log |
| `dry_run` | `false` | Scan only; skip issue creation |

---

## 6. Running the Drift Scan Locally

```bash
# Install dependency
pip install pyyaml

# Run from repository root
python3 scripts/compendium-drift-scan.py \
  --priority medium \
  --verbose

# View results
cat /tmp/compendium-drift-summary.md
cat /tmp/compendium-drift.json
```

---

## 7. Adding or Updating Chapter Mappings

When a new compendium chapter is created or an existing chapter changes scope:

1. Edit `docs/compendium/CHAPTER_ROADMAP_MAPPING.yml`.
2. Add or update the chapter entry with:
   - `file` — relative path under `docs/compendium/docs/`
   - `title` — human-readable title
   - `chapter_number` — optional display number (e.g. `"43"` or `"16a"`) for duplicate-prefix clarity
   - `target_version` — the version this entry targets
   - `roadmap_topics` — keywords from ROADMAP.md / FUTURE_ENHANCEMENTS.md
   - `sot_paths` — SOT files most relevant to this chapter
   - `priority` — `high` | `medium` | `low`
3. Run the drift scan locally to verify the new entry is correctly detected.

---

## 8. Governance Alignment

- This process targets `develop` only. Never target `community` or `military`
  branches with compendium draft PRs unless explicitly requested.
- Do not auto-merge compendium changes. Every merge requires human approval.
- The compendium is documentation only; changes here do not affect the build,
  tests, or production code.
- Compendium PRs are subject to `gate-pr-doc-metadata` and
  `gate-pr-primary-doc-structure` gates.

---

## 9. Related Files

| File | Purpose |
|------|---------|
| `docs/compendium/CHAPTER_ROADMAP_MAPPING.yml` | Chapter ↔ SOT mapping + gap topics |
| `scripts/compendium-drift-scan.py` | Drift detection and new-chapter proposal script |
| `.github/workflows/maintenance-compendium-sync.yml` | Automation workflow |
| `docs/compendium/docs/INTEGRATION_MAPPING.md` | Chapter structure / consolidation notes |
| `ROADMAP.md` | Root SOT for v2.4.0-alpha status |
| `FUTURE_ENHANCEMENTS.md` | Root SOT for enhancement direction |
| `docs/architecture/MODULE_ARCHITECTURE.md` | Cross-module architecture reference |
