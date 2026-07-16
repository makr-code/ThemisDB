# Architecture - Exporters Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The exporters module composes format-specific exporters, stream/delta/join orchestration, and export safety controls into a bounded runtime contract for dataset and document export operations.

## Main Execution Planes

1. Format pipeline plane
- JSONL, Parquet, Arrow, and HuggingFace format generation
- template-driven output adaptation for LLM-oriented datasets

2. Export orchestration plane
- batch, streaming, incremental, and join export workflows
- checkpoint/watermark and bounded-memory execution behavior

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
| orchestration contract | bounded streaming/incremental/join execution behavior |
| safety contract | explicit policy/filter/PII/encryption gating |
| operations contract | observable metrics and registry/integration behavior |

## Failure Semantics

- invalid export configs or unsupported format paths fail with explicit errors.
- policy denials fail closed before export output activity.
- bounded-memory and watermark/checkpoint paths expose deterministic failure states.

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