# Architecture - Toolbox Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The toolbox module composes ingestion-oriented toolbox orchestration, content bridging, registry/composite dispatch, and shared text helper behavior into a bounded subsystem.

## Main Execution Planes

1. Orchestration and bridge plane
- ingestion toolbox, builder, and content bridge behavior

2. Registry and routing plane
- registry, composite routing, and streaming behavior

3. Text helper plane
- chunking, normalization, quality, language, and fingerprint behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| orchestration contract | deterministic toolbox build and extraction lifecycle behavior |
| bridge contract | explicit content-to-toolbox result semantics |
| registry contract | bounded initialization and access behavior |
| helper contract | deterministic text helper outputs |

## Failure Semantics

- builder and registry misuse faults are explicit.
- bridge soft-fail behavior remains diagnosable and non-silent.
- streaming and composite routing failures surface deterministic outcomes.
- helper utility edge cases remain bounded by explicit return behavior.

## Sourcecode Verification (Module: toolbox/architecture)

- Verified files:
  - src/toolbox/ingestion_toolbox.cpp
  - src/toolbox/toolbox_builder.cpp
  - src/toolbox/content_toolbox_bridge.cpp
  - src/toolbox/toolbox_registry.cpp
  - src/toolbox/toolbox_composite.cpp
  - src/toolbox/toolbox_streaming.cpp
  - src/toolbox/text_chunker.cpp
  - src/toolbox/text_normalizer.cpp
  - src/toolbox/text_quality_scorer.cpp
  - src/toolbox/language_detector.cpp
  - src/toolbox/content_fingerprinter.cpp
- Verified architecture claims:
  - orchestration/bridge + registry/routing + helper plane split
  - explicit failure boundaries for builder, registry, bridge, and helper faults
  - module-local ownership of toolbox behavior