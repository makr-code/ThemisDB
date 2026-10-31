# Architecture — ai_working Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Overview

The `ai_working` module is a documentation and planning artifact container. It holds no runtime C++ code. Its architecture is purely organizational: a governed directory structure that allows AI agents, maintainers, and CI governance tooling to locate, validate, and archive working materials produced during active development cycles.

## Context and Purpose

ThemisDB uses wave-driven development (Wave A → B → C → D) coordinated by AI agents. Each wave produces planning documents, execution reports, gap analysis packets, and completion summaries. The `ai_working` module provides the canonical storage location for these artifacts under repository governance.

## Components

| Component | Path | Responsibility |
|---|---|---|
| Root working directory | `ai_working/` | Wave planning, execution reports, agent collaboration artifacts |
| Module governance docs | `src/ai_working/` | README, ARCHITECTURE, ROADMAP, CHANGELOG, AUDIT, SECURITY, and enhancement docs |
| Test scaffold | `tests/ai_working/` | README and a `CMakeLists.txt` containing only the minimum CMake version; no test cases or root registration |
| Benchmark docs | `benchmarks/ai_working/` | README only; no benchmark source, target, or root registration |

## Artifact Lifecycle

```
Created (wave active)
    |
    v
Used (agent references artifact during execution)
    |
    v
Superseded (wave completes or document is no longer current)
    |
    v
Archived to docs/ARCHIVED/ai-working-history/
```

There are two distinct artifact flows. The root `ai_working/` directory is compacted, rotated, and cleaned by `scripts/ai-working-compact.py`; `scripts/archive_ai_working.py --archive-stale` separately moves every top-level Markdown file in `src/ai_working/` that is not on its protected-file list into a dated `docs/ARCHIVED/ai-working-history/` directory. The latter does not determine whether a file is actually stale, so review candidates before running it outside the scheduled workflow.

```text
ai_working/ ── ai-working-compact.py ──> ai_working/compact/ and archive rotation
src/ai_working/*.md ── archive_ai_working.py ──> docs/ARCHIVED/ai-working-history/
src/ai_working/*.md ── --check-freshness ──> presence/content report
```

## Interfaces

- No public C++ API.
- `scripts/archive_ai_working.py --check-freshness` checks the nine required governance files for existence, placeholder text, and a minimum 200 non-whitespace characters. It does not compare document timestamps or validate factual freshness.
- `.github/workflows/maintenance-ai-working.yml` runs archive and freshness steps after root artifact compaction; the workflow also performs a developer-wiki sync on its scheduled/manual paths.
- There is no `src/ai_working/CMakeLists.txt`, no public header, and no registered C++ module target.

## Failure Paths

- Missing required governance documents or placeholder content → the maintenance workflow's freshness step fails.
- A document can pass the freshness check while containing outdated claims; compare it with source, scripts, and workflow configuration during review.

## Non-Goals

- Runtime execution: this module provides no production code paths.
- API surface: no public headers or exported symbols.
- Performance-critical paths: there are no latency or throughput requirements.
