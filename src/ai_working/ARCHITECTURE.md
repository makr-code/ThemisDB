# Architecture — ai_working Module

<!-- Status: current | validated: 2026-11-18 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Overview

The `ai_working` module is a documentation and planning artifact container. It holds no runtime C++ code. Its architecture is purely organizational: a governed directory structure that allows AI agents, maintainers, and CI governance tooling to locate, validate, and archive working materials produced during active development cycles.

## Context and Purpose

ThemisDB uses wave-driven development (Wave A → B → C → D) coordinated by AI agents. Each wave can produce planning documents, execution reports, gap analysis packets, and completion summaries. `ai_working/` is a repository area for these working artifacts, not an authority boundary: artifacts remain historical evidence unless promoted by active governance.

## Components

| Component | Path | Responsibility |
|---|---|---|
| Root working directory | `ai_working/` | Wave planning, execution reports, agent collaboration artifacts |
| Module governance docs | `src/ai_working/` | README, ARCHITECTURE, ROADMAP, CHANGELOG, AUDIT, SECURITY, and enhancement docs |
| Test scaffold | `tests/ai_working/` | Standard-library Python tests; the CMake file has no C++ test target |
| Benchmark docs | `benchmarks/ai_working/` | README only; no benchmark source, target, or root registration |

## Artifact Lifecycle

The root working-artifact and module-document flows are separate:

```text
ai_working/ root files
  ├─ compaction ─> non-normative ai_working/compact/ pages
  └─ mtime cutoff ─> ai_working/ARCHIVE_PRE_* ─> age-based cleanup

src/ai_working/*.md
  ├─ required/protected docs ─> remain active; structural check only
  └─ other top-level Markdown ─> dated docs/ARCHIVED/ai-working-history/
```

The root `ai_working/` directory is compacted, rotated, and cleaned by `scripts/ai-working-compact.py`. With default options, files older than 90 days are moved into a month-labeled archive and empty root files plus archive entries older than 365 days are removed. A dry run previews actions without writing. Generated compact pages list source names, modification dates, and sizes but have no content-hash lineage; they are non-normative. Files exceeding the 15-file or 12-KiB inline limits are represented by extracted headings. An unreadable source now fails compaction rather than producing a seemingly complete page.

Separately, `scripts/archive_ai_working.py --archive-stale` moves every unprotected top-level Markdown file in `src/ai_working/` into a dated `docs/ARCHIVED/ai-working-history/` directory. It does not determine whether a file is old or superseded. On collision it selects an unused suffixed name; inspect `--dry-run` output before moving files.

## Interfaces

- No public C++ API.
- `scripts/archive_ai_working.py --check-freshness` checks nine required documents for existence, one placeholder pattern, and 200 non-whitespace characters. Missing, unreadable, and placeholder files fail; short files warn by default and fail with `--strict`. It does not compare dates, links, or factual content.
- `scripts/ai-working-compact.py` provides compaction, age-based archive/cleanup, and dry-run modes. Read errors fail with a source path; generated pages disclaim normative authority; archive collision handling chooses an unused suffix.
- `.github/workflows/maintenance-ai-working.yml` runs root compaction, module archival, and the non-strict document check on scheduled/push/manual paths. Each push loop allows up to three attempts total for non-fast-forward conflicts. The compaction job treats protected-branch push refusal as a warning/skip; the separate wiki-sync job does not. Workflow logs/reports are retained as artifacts for 14 days.
- There is no `src/ai_working/CMakeLists.txt`, no public header, and no registered C++ module target.

## Failure Paths

- Missing, unreadable, or placeholder required documents → the freshness command fails; fewer than 200 non-whitespace characters warn unless strict mode is selected.
- An unreadable root working artifact → compaction fails instead of silently omitting that artifact's content. The operation may have already written earlier topic outputs when a later read fails; source files are not changed by compaction, but generated output can be partial locally. CI stops before its commit step on command failure.
- A document can pass the structural freshness check while containing outdated claims; compare it with source, scripts, workflow configuration, and current Actions evidence during review.
- There is no module runtime fallback or recovery logic. The only retry is the workflow's bounded push-conflict loop; protected-branch push skips do not count as successful publication.

## Recent Operational Evidence

The [maintenance run #89](https://github.com/makr-code/ThemisDB/actions/runs/44165966484) on `develop` completed successfully on 2026-11-17. Its compaction/archive/freshness job passed, made no commit, and the developer-wiki sync job was skipped because the push did not meet that job's trigger condition. This run proves those workflow steps passed at that revision; it is not evidence that the wiki was regenerated, that semantic freshness was checked, or that generated content was pushed.

## Non-Goals

- Runtime execution: this module provides no production code paths.
- API surface: no public headers or exported symbols.
- Performance-critical paths: there are no latency or throughput requirements.
