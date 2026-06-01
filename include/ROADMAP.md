> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md -->

# ThemisDB Public Headers (`include/`) — Documentation Roadmap

**Directory:** `include/`
**Implementation counterpart:** [`../src/`](../src/)

---

## Overview

This document tracks the documentation coverage rollout across all `include/<module>/` subdirectories. Each module requires a `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, and `FUTURE_ENHANCEMENTS.md` to fully document its public header surface.

---

## Priority A — Core Protocol Modules ✅

All Priority A modules have complete header documentation (README + ARCHITECTURE + ROADMAP + FUTURE_ENHANCEMENTS).

| Module | README | ARCHITECTURE | ROADMAP | FUTURE_ENHANCEMENTS |
|---|---|---|---|---|
| `api` | ✅ | ✅ | ✅ | ✅ |
| `auth` | ✅ | — | — | — |
| `cache` | ✅ | — | — | — |
| `core` | ✅ | — | — | — |
| `metadata` | ✅ | — | — | — |
| `network` | ✅ | — | — | — |
| `observability` | ✅ | — | — | — |
| `performance` | ✅ | — | — | — |
| `query` | ✅ | — | — | — |
| `replication` | ✅ | — | — | — |
| `search` | ✅ | — | — | — |
| `security` | ✅ | — | — | — |
| `server` | ✅ | — | — | — |
| `sharding` | ✅ | — | — | — |
| `storage` | ✅ | — | — | — |
| `transaction` | ✅ | — | — | — |

---

## Priority B — Extended Modules ✅

Priority B modules have complete README documentation; ARCHITECTURE/ROADMAP/FUTURE_ENHANCEMENTS were the subject of the initial include-documentation pass.

| Module | README | ARCHITECTURE | ROADMAP | FUTURE_ENHANCEMENTS |
|---|---|---|---|---|
| `ai` | ✅ | ✅ | ✅ | ✅ |

---

## Priority C — Domain Modules ✅

All Priority C modules now have complete header documentation sets (ARCHITECTURE + ROADMAP + FUTURE_ENHANCEMENTS).

| Module | README | ARCHITECTURE | ROADMAP | FUTURE_ENHANCEMENTS |
|---|---|---|---|---|
| `analytics` | ✅ | ✅ | ✅ | ✅ |
| `geo` | ✅ | ✅ | ✅ | ✅ |
| `llm` | ✅ | ✅ | ✅ | ✅ |
| `plugins` | ✅ | ✅ | ✅ | ✅ |
| `rag` | ✅ | ✅ | ✅ | ✅ |
| `temporal` | ✅ | ✅ | ✅ | ✅ |
| `timeseries` | ✅ | ✅ | ✅ | ✅ |
| `training` | ✅ | ✅ | ✅ | ✅ |
| `voice` | ✅ | ✅ | ✅ | ✅ |

Priority C documentation completed: 2026-06-01.

---

## Remaining Work 📋

### Priority D — Supplementary / Specialized Modules

The following modules have a `README.md` but no extended documentation set:

- `auth`, `cache`, `core`, `metadata`, `network`, `observability`, `performance`, `query`, `replication`, `search`, `security`, `server`, `sharding`, `storage`, `transaction`

These are tracked for a future documentation pass. Priority is lower because the source-level docs in `src/<module>/` already provide comprehensive coverage that consumers can reference.

---

## Conventions

- Every `include/<module>/ARCHITECTURE.md` cross-links to `../../src/<module>/ARCHITECTURE.md` as the canonical implementation architecture document.
- Every `include/<module>/ROADMAP.md` cross-links to `../../src/<module>/ROADMAP.md`.
- Every `include/<module>/FUTURE_ENHANCEMENTS.md` cross-links to `../../src/<module>/FUTURE_ENHANCEMENTS.md`.
- All headers must have `#pragma once` guard and carry no implementation code.

---

## References

- Module source docs: [`../src/`](../src/)
- ThemisDB root README: [`../README.md`](../README.md)
