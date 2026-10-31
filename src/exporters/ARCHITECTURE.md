---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "architecture-governance"
Canonical_References: "include/exporters/*.h; src/exporters/*.cpp; tests/exporters/CMakeLists.txt"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Architecture - Exporters Module

Exact model backend not verifiable from final document text alone.

<!-- Status: reviewed against source 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The exporters module serializes caller-supplied entities into format-specific artifacts and composes stream, incremental, and join workflows with optional export safety controls. It does not own source query planning or collection reads.

## Main Execution Planes

1. Format pipeline plane
- JSONL, Parquet, Arrow, and HuggingFace format generation
- template-driven output adaptation for LLM-oriented datasets

2. Export orchestration plane
- callers provide entity batches; cursor-based streaming pages records to output
- incremental export compares supplied entities with a sequence watermark
- join export builds an in-memory right-side hash table subject to its configured budget

3. Safety and governance plane
- policy-gated export authorization paths
- predicate filtering, PII redaction, and encryption controls

4. Operations and integration plane
- metrics and diagnostics surfaces
- registry-based format resolution and hub upload workflows

## Core Contracts

| Contract | Behavior |
|---|---|
| format contract | deterministic format-specific export semantics |
| orchestration contract | cursor/page-based streaming with writer buffering; watermark filtering of a supplied entity vector; budget-limited in-memory join state |
| safety contract | explicit policy/filter/PII/encryption gating |
| operations contract | observable metrics and registry/integration behavior |

## Failure Semantics

- invalid export configs or unsupported format paths fail with explicit errors.
- policy denials fail closed before export output activity.
- streaming processes cursor pages through a buffered writer; `max_buffer_bytes` is a flush trigger, not a cap on total process memory.
- incremental watermark state is persisted by its path; input is still supplied as a vector and is not a cursor/CDC scan.
- join memory is bounded by the configured right-side budget; this is not a general bound on all exporter inputs.

## Sourcecode Verification (Module: exporters/architecture)

- Verified files:
  - src/exporters/jsonl_llm_exporter.cpp
  - src/exporters/streaming_exporter.cpp
  - src/exporters/incremental_exporter.cpp
  - src/exporters/join_exporter.cpp
  - src/exporters/export_format_registry.cpp
  - src/exporters/huggingface_hub_client.cpp
  - src/exporters/export_encryption.cpp
  - src/exporters/pii_detector.cpp
- Verified architecture claims:
  - explicit format/orchestration/safety/operations planes
  - bounded failure behavior in policy and export execution paths
  - module-local ownership of export orchestration and integrations
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/exporters/exporter_interface.h`, `include/exporters/aql_predicate_filter.h`, `include/exporters/exporter_errors.h` | Export API handler resolves exporter implementations and applies AQL predicate filters during export runs (`include/server/export_api_handler.h`, `src/server/export_api_handler.cpp`) |
| `llm` | `include/exporters/jsonl_llm_exporter.h` | Training data iterator uses the JSONL LLM exporter for streaming fine-tuning dataset emission (`include/llm/training_data_iterator.h`) |
