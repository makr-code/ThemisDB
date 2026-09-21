# Architecture — ai_working Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The `ai_working` module is a documentation and planning artifact container. It holds no runtime C++ code. Its architecture is purely organizational: a governed directory structure that allows AI agents, maintainers, and CI governance tooling to locate, validate, and archive working materials produced during active development cycles.

## Context and Purpose

ThemisDB uses wave-driven development (Wave A → B → C → D) coordinated by AI agents. Each wave produces planning documents, execution reports, gap analysis packets, and completion summaries. The `ai_working` module provides the canonical storage location for these artifacts under repository governance.

## Components

| Component | Path | Responsibility |
|---|---|---|
| Root working directory | `ai_working/` | Wave planning, execution reports, agent collaboration artifacts |
| Module governance docs | `src/ai_working/` | README, ARCHITECTURE, ROADMAP, CHANGELOG, AUDIT, SECURITY, and enhancement docs |
| Test placeholder | `tests/ai_working/` | Reserved for future test coverage of working-context utilities |
| Benchmark placeholder | `benchmarks/ai_working/` | Reserved for future benchmark coverage |

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

All artifacts that are no longer active are moved to `docs/ARCHIVED/` rather than deleted, to preserve planning history.

## Interfaces

- No public C++ API.
- Governance tooling (e.g., `scripts/check_module_direct_doxygen.py`) treats this module as a documentation-only module.
- CI doc-metadata gates validate the presence of required governance files in `src/ai_working/`.

## Failure Paths

- Missing required governance documents → CI doc-metadata gate fails with `LOW` compliance score.
- Stale placeholders that do not reflect module purpose → documentation drift finding.

## Non-Goals

- Runtime execution: this module provides no production code paths.
- API surface: no public headers or exported symbols.
- Performance-critical paths: there are no latency or throughput requirements.
