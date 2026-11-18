---
Author: "ThemisDB Team"
Created: "2026-11-18"
Last Updated: "2026-11-18"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "src/ai_working/README.md; src/ai_working/ARCHITECTURE.md; src/ai_working/ROADMAP.md; scripts/ai-working-compact.py; scripts/archive_ai_working.py; .github/workflows/maintenance-ai-working.yml"
---

> Exact model backend not verifiable from final document text alone.

# ai_working — Developer LLM Wiki

This page is secondary coding context. The module has no C++ runtime, public
headers, build target, or benchmark. Its only executable surfaces are repository
artifact-maintenance scripts and their GitHub Actions workflow. The module
[README](../../src/ai_working/README.md),
[architecture](../../src/ai_working/ARCHITECTURE.md),
[roadmap](../../src/ai_working/ROADMAP.md), and
[audit](../../src/ai_working/AUDIT.md) are the detailed documentation sources.

## Artifact authority and lifecycle

- Root `ai_working/` files are working artifacts and historical evidence unless
  active governance explicitly promotes them. Generated compact pages are
  non-normative; repository source-of-truth documents remain authoritative.
- `scripts/ai-working-compact.py` groups eligible top-level files by
  filename-derived topic. It inlines at most 15 files up to 12 KiB each and
  extracts headings for other inputs. Source filenames, modification dates,
  and sizes are recorded; content-hash lineage is not.
- Default archive/cleanup thresholds are 90 and 365 days. `--dry-run` previews
  compaction, archive, and cleanup operations without writing.
- Unreadable compaction input fails with a source path rather than silently
  omitting content. Archive collisions use a unique suffix instead of
  overwriting existing entries.
- `scripts/archive_ai_working.py --archive-stale` selects all unprotected
  top-level Markdown files under `src/ai_working/`; it does not determine
  semantic staleness. Its `--check-freshness` check is structural: required
  file presence, one placeholder pattern, and minimum size. Missing,
  unreadable, or placeholder docs fail; short docs warn unless `--strict` is
  set. It does not check links, dates, or factual currency.

## Operations and failure boundaries

- `.github/workflows/maintenance-ai-working.yml` runs compaction, archival,
  and the non-strict document check. Each non-fast-forward push loop is bounded
  to three total attempts. Only the compaction job treats protected-branch
  push refusal as a warning/skip; this is not successful publication.
  Workflow logs and reports are retained for 14 days.
- [Workflow run #89](https://github.com/makr-code/ThemisDB/actions/runs/44165966484)
  on `develop` completed successfully on 2026-11-17. Compaction, archive, and
  freshness steps passed with no commit to create; wiki sync was skipped. This
  is not evidence of semantic freshness, secret scanning, or wiki publication.
- The wiki health file generated on 2026-11-17 reports 8,103 sources and a
  delta of 0 added, 0 removed, and 3 changed. It is not module runtime or
  release-status evidence.
- An incremental sync validated locally on 2026-11-18 with zero findings but
  reported 3 added, 12 removed, and more than 1,100 changed sources across the repository.
  Its broad generated inventory refresh was not retained; the checked-in source
  manifest and status remain the 2026-11-17 snapshot.
- Invalid capability/trust/configuration runtime states, runtime fallback,
  and recovery do not apply because this module has no application runtime.

## Tests and performance evidence

- Run the focused Python tests with
  `python -m unittest discover -s tests/ai_working -p 'test_*.py'`. Four tests
  cover whitespace counting, non-normative output labeling, unreadable-source
  failure, and archive collision preservation. They are not registered with
  CTest and do not cover the full lifecycle or interruption behavior.
- No benchmark source, target, measured baseline, or performance gate exists.
  Proposed corpus sizes, latency/regression thresholds, and discovery metrics
  in the module roadmap are future acceptance targets, not current results.

For code or status decisions, follow the source priority in
[documentation governance](../../DOCUMENTATION_GOVERNANCE.md); this wiki page
does not redefine module or release status.
