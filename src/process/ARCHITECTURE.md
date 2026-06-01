# Architecture - Process Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The process module composes process-model lifecycle management, process format import/export, process graph retrieval, and linking/compliance support into a bounded process-modeling subsystem for ThemisDB.

## Main Execution Planes

1. Model lifecycle and format plane
- process model CRUD and versioned storage behavior
- BPMN/EPK/VCC/ARIS/CMMN/FIM import-export paths

2. Retrieval and descriptor plane
- process graph and agentic retrieval context assembly
- descriptor and prompt generation behavior

3. Linking and compliance plane
- process-object/process-process linking behavior
- DMN/OCEL and object-centric tracing support

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic process model load/save/import/export semantics |
| retrieval contract | bounded process context retrieval and assembly behavior |
| linking contract | explicit link registration and lookup behavior |
| compliance contract | deterministic DMN/evaluation/export support surfaces |

## Failure Semantics

- invalid process input or malformed models fail with explicit outcomes.
- retrieval path failures are surfaced explicitly.
- linking and evaluation errors remain observable and non-silent.

## Sourcecode Verification (Module: process/architecture)

- Verified files:
  - src/process/process_model_manager.cpp
  - src/process/bpmn_serializer.cpp
  - src/process/epk_serializer.cpp
  - src/process/process_graph_rag.cpp
  - src/process/process_agentic_rag.cpp
  - src/process/process_linker.cpp
  - src/process/dmn_evaluator.cpp
  - src/process/ocel_exporter.cpp
- Verified architecture claims:
  - explicit lifecycle/retrieval/linking/compliance planes
  - deterministic failure boundaries across process workflows
  - module-local ownership of process modeling behavior