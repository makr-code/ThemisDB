# Architecture - Prompt Engineering Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The prompt_engineering module composes template lifecycle operations, context-injection and validation, prompt version control, optimization/evaluation loops, and feedback/metrics tracking into a bounded prompt-quality subsystem.

## Main Execution Planes

1. Template and versioning plane
- template create/get/inject/validate behavior
- prompt commit/history and revision tracking behavior

2. Quality optimization plane
- optimization loop orchestration and scoring behavior
- prompt output evaluation and regression support behavior

3. Feedback and metrics plane
- feedback capture and aggregate stats behavior
- prompt metrics recording and performance tracking surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| template contract | deterministic create/get/inject/validate semantics |
| versioning contract | explicit commit/history semantics for prompt revisions |
| optimization contract | bounded optimization/evaluation loop behavior |
| observability contract | explicit feedback and metrics recording behavior |

## Failure Semantics

- invalid templates fail explicitly in validation paths.
- missing template IDs return explicit misses.
- version/optimization failures remain observable and non-silent.
- metrics/feedback recording failures are surfaced as explicit outcomes.

## Sourcecode Verification (Module: prompt_engineering/architecture)

- Verified files:
  - src/prompt_engineering/prompt_manager.cpp
  - src/prompt_engineering/prompt_version_control.cpp
  - src/prompt_engineering/prompt_optimizer.cpp
  - src/prompt_engineering/prompt_evaluator.cpp
  - src/prompt_engineering/feedback_collector.cpp
  - src/prompt_engineering/prompt_engineering_metrics.cpp
- Verified architecture claims:
  - template/versioning + optimization + feedback/metrics plane split
  - explicit failure boundaries for invalid templates, misses, and loop faults
  - module-local ownership of prompt engineering behavior surfaces

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` | Logging, audit, and observability helpers |
| observability (utils) | `include/utils/tracing.h` | Telemetry for prompt optimization loop iterations |
| storage | `include/storage/` | Persists prompt templates and version history |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/prompt_api_handler.h` | Server exposes prompt template CRUD and optimization APIs |
| rag | `include/prompt_engineering/` (rag_prompt_builder.h) | RAG module uses prompt builder to construct retrieval-augmented prompt contexts |
| llm | `include/prompt_engineering/` | LLM module injects validated templates into inference context |

## Integration Points

### Critical Integration: RAG Prompt Builder
**Files:** `src/prompt_engineering/prompt_manager.cpp` ↔ `rag/rag_prompt_builder.h`
**Contract:** RAG calls `prompt_manager.inject(context)` to fill template placeholders with retrieved evidence; template ownership remains with prompt_engineering.
**Thread Safety:** Template reads are concurrent-safe; version commits are serialised under the versioning lock.

### Critical Integration: Server Prompt API
**Files:** `server/prompt_api_handler.h` ↔ `include/prompt_engineering/`
**Contract:** Server delegates all prompt CRUD and optimization trigger calls; prompt_engineering is the sole owner of template state.
**Thread Safety:** All public API methods are thread-safe under internal locking.