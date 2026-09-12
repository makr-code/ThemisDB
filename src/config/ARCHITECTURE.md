# Architecture - Config Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The config module composes path resolution, schema validation, metrics/audit observability, file-watch triggers, and encrypted config storage into a unified configuration runtime layer.

## Main Execution Planes

1. Resolution and mapping plane
- resolve config paths with mapped fallback semantics
- maintain deterministic resolution and compatibility behavior

2. Validation plane
- schema-driven config validation and parsing guard paths
- structured error signaling for invalid config payloads

3. Observability and operations plane
- metrics export and access-audit surfaces
- file watcher integration for config change signaling

4. Secure storage plane
- encrypted config key/value storage for sensitive settings
- key-rotation-aware secure persistence behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| resolution interfaces | deterministic path mapping and fallback behavior |
| validation interfaces | explicit schema and parse validation outcomes |
| observability interfaces | stable metrics and audit reporting surfaces |
| secure-store interfaces | bounded encrypted storage and rotation semantics |

## Failure Semantics

- invalid or unresolved config inputs fail with structured errors.
- schema/parse failures remain explicit and non-silent.
- watcher and exporter degradation paths remain bounded and diagnosable.

## Sourcecode Verification (Module: config/architecture)

- Verified files:
  - src/config/config_path_resolver.cpp
  - src/config/config_schema_validator.cpp
  - src/config/config_metrics_exporter.cpp
  - src/config/config_file_watcher.cpp
  - src/config/config_encrypted_store.cpp
  - src/config/config_audit_log.cpp
- Verified architecture claims:
  - explicit resolution, validation, observability, and secure-store planes
  - bounded failure behavior for config lifecycle paths
  - dedicated module-layer composition for config runtime concerns
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/config/config_path_resolver.h`, `include/config/config_metrics_exporter.h` | HTTP server, auth middleware, MCP server, and monitoring handler resolve paths and export config metrics (`src/server/http_server.cpp`, `src/server/auth_middleware.cpp`, `src/server/mcp_server.cpp`, `src/server/monitoring_api_handler.cpp`) |
| `content` | `include/config/config_path_resolver.h`, `include/config/config_schema_validator.h` | Async ingestion worker, MIME detector, and OCR processor resolve runtime config paths and validate schemas (`src/content/async_ingestion_worker.cpp`, `src/content/mime_detector.cpp`, `src/content/ocr_processor.cpp`) |
| `index` | `include/config/config_path_resolver.h` | `VectorIndex` resolves index data path from config at construction (`src/index/vector_index.cpp`) |
| `llm_wiki` | `include/config/config_path_resolver.h`, `include/config/config_schema_validator.h` | Process policy manager and Wikipedia LLM plugin resolve paths and validate config schemas (`src/llm_wiki/process_policy_manager.cpp`, `src/llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp`) |
| `utils` | `include/config/config_path_resolver.h` | PII detector resolves pattern-list config paths at runtime (`src/utils/pii_detector.cpp`) |
| `main` | `include/config/config_path_resolver.h`, `include/config/config_metrics_exporter.h` | Server entry point initializes config resolution and starts the metrics exporter (`src/main_server.cpp`) |
