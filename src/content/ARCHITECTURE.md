# Architecture - Content Module

<!-- Status: current | validated: 2026-08-15 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md -->

## Overview

The content module composes ingestion, extraction, validation, enrichment, and storage-integration surfaces into a unified content-processing runtime. It provides deterministic routing from content type detection through processor execution and post-processing stages.

## Main Execution Planes

1. Ingestion and routing plane
- mime/content classification and processor selection
- sync and async ingestion orchestration paths

2. Validation and security plane
- schema/format validation and policy enforcement
- archive/content safety checks and bounded rejection behavior

3. Extraction and enrichment plane
- format-specific extraction (text, image, PDF, Office, audio/video)
- OCR, language, LLM, and embedding integration paths

4. Deduplication and operations plane
- hash/perceptual deduplication checks
- metrics, logging, versioning, and plugin extension surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| ingestion interfaces | deterministic content intake and processor routing |
| validation/security interfaces | explicit pre-processing gates and failure semantics |
| extraction/enrichment interfaces | format-aware extraction and optional enhancement paths |
| operations interfaces | stable metrics/audit/plugin extension behavior |

## Failure Semantics

- unsupported or invalid inputs fail with structured content errors.
- dependency-degraded processors return explicit non-silent failure states.
- policy/security violations fail closed before downstream processing.

## Sourcecode Verification (Module: content/architecture)

- Verified files:
  - src/content/content_manager.cpp
  - src/content/content_validator.cpp
  - src/content/content_policy.cpp
  - src/content/content_security.cpp
  - src/content/mime_detector.cpp
  - src/content/pdf_processor.cpp
  - src/content/office_processor.cpp
  - src/content/ocr_processor.cpp
  - src/content/embedding_pipeline.cpp
  - src/content/deduplication_checker.cpp
- Verified architecture claims:
  - explicit ingestion/routing, validation/security, and enrichment planes
  - bounded structured failures for invalid/degraded processing paths
  - module-local orchestration for multi-format content processing