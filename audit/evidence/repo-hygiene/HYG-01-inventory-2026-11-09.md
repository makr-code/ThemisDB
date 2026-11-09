---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# HYG-01 Repository Artifact Inventory

## Snapshot and reproduction

This evidence refreshes the 2026-10-31 audit examples against the checkout at
commit `dc551ed9fe746262737c9c5776921947fc30ed9e`. The before/after machine
inventory is [`HYG-01-inventory-2026-11-09.json`](HYG-01-inventory-2026-11-09.json).
It records Git blob sizes, generated-output classifications, responsible owner,
proposed disposition, source-tree references, and the measured Git-directory
footprint. The after tree is measured from the staged index before the JSON
inventory path is excluded from the after-tree counts and bytes even when the
JSON is already tracked; the final tree totals below add that file once.

Reproduce after staging the intended changes:

```sh
python3 tools/ci/repo_artifact_policy.py inventory \
  --before-ref dc551ed9fe746262737c9c5776921947fc30ed9e \
  --after-ref INDEX \
  --output audit/evidence/repo-hygiene/HYG-01-inventory-2026-11-09.json
```

Git blob bytes measure the tracked content, not the compressed clone. The
repository's Git directory is reported separately because this change does not
rewrite history or prune historical blobs.

## Tracked-content evidence

| Measure | Before | After |
|---|---:|---:|
| Tracked entries | 65,632 | 23,966 in the inventoried index; final tree adds the JSON inventory itself (23,967). |
| Tracked Git-blob bytes | 2,306,110,638 | 360,617,925 in the inventoried index; final tree adds the 159,546-byte JSON inventory (360,777,471 total). |
| Tracked files strictly over 5 MiB | 24 | 3 intentional published-document files |
| Generated Doxygen non-Markdown outputs | 41,478 files | 0 |
| Archived raw scan JSON/XML outputs | 180 files | 0 |
| Git directory bytes | 218,036,635 before removal | See `git_directory_bytes_at_capture` in the JSON inventory (after staging removals, before the evidence commit); historical objects are retained. |

The retained over-limit files are versioned published documentation: one
ThemisDB Kompendium PDF and the v1.4.0 compendium HTML/PDF pair. They are
classified as documentation deliverables, not fixtures, and are not newly
introduced by this change.

## Historical audit examples

| Path | Verification at baseline | Owner / classification | Disposition |
|---|---|---|---|
| `docs/ARCHIVED/ai-working-history/scan_tests.json` | Present; 84,863,612 Git-blob bytes | AI working-history maintainers; generated scan output | Removed from the current tree. Fresh GS3 output is uploaded as a 30-day CI artifact; Git history remains intact. |
| `archive/moved_files.txt` | Present; 30,396,533 Git-blob bytes | Repository hygiene maintainers; generated move report | Removed; no active consumer reference was found. |
| `bench_access_coordinator_gates.exe.json` | Present; 3,968 Git-blob bytes | Benchmark maintainers; generated benchmark result | Removed; fresh benchmark runs are published through existing 30-day CI artifacts. |
| `bench_access_coordinator_gates.json` | Present; 3,969 Git-blob bytes | Benchmark maintainers; generated benchmark result | Removed; fresh benchmark runs are published through existing 30-day CI artifacts. |

The inventory JSON contains the exact baseline sizes, classifications, owners,
and proposed dispositions for all 24 files over the threshold, plus per-file
records for the removed archived scan reports, root benchmark outputs, and
other confirmed transient outputs.

## Removal and retention decisions

- Removed all 41,478 generated non-Markdown files from the two
  `module_doxygen_artifacts` output roots. The two derived Temporal Markdown
  summaries remain. The Doxygen workflow already generates under
  `/tmp/module-doxygen` and publishes `module-doxygen-xml-<run>` for 21 days.
- Removed 180 raw JSON/XML reports from the historical AI working archive and
  the confirmed oversized/transient move list, scan outputs, build database,
  compiler outputs, tool installer/binary, symbol dump, and build log. Curated
  historical Markdown and other records remain; no Git history was rewritten.
- Removed the two root benchmark JSON outputs. Existing referenced benchmark
  evidence under `benchmarks/benchmark_results/` remains to preserve consumer
  links; new benchmark runs use existing workflow artifacts with 30-day
  retention.
- `maintenance-issues.yml` retains current GS3 scan outputs for 30 days.
  `maintenance-soll-ist-gap-issues.yml` retains issue reports for 14 days.
  Published benchmark workflow outputs use 30 days. These are workflow-specific
  periods, not a promise of permanent storage.
- `.gitignore` now excludes the removed generated output locations and new raw
  benchmark results. The exact-path large-fixture allowlist is empty because
  no oversized test fixture is currently approved.

## PR gate

`gate-pr-artifact-size.yml` checks every PR branch for newly added Git blobs.
Files strictly larger than 5 MiB (5,242,880 bytes) fail; exactly 5 MiB passes.
Exceptions require a reviewed exact repository-relative path and rationale in
`tools/ci/large_file_allowlist.json`; wildcard and directory-wide entries are
rejected. The focused tests cover rejected, allowlisted, boundary, and invalid
allowlist cases.

## Canonical references

- [`audit/ROADMAP.md`](../../ROADMAP.md) tracks HYG-01 and links this evidence.
- [`.github/WORKFLOW_GUIDELINES.md`](../../../.github/WORKFLOW_GUIDELINES.md)
  documents the gate and retention contract.
- [`.github/WORKFLOW_REGISTRY.md`](../../../.github/WORKFLOW_REGISTRY.md)
  registers the PR gate.
- The workflow YAML and `tools/ci/repo_artifact_policy.py` remain authoritative
  for actual gate behavior and artifact retention.
