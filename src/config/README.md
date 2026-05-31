# ThemisDB Config Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The config module provides configuration resolution, validation, observability, and secure config-storage support surfaces for ThemisDB runtime and operations.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| config_path_resolver.cpp | config path mapping, resolution, and fallback behavior |
| config_schema_validator.cpp | schema-based validation and parsing guard surfaces |
| config_metrics_exporter.cpp | config metrics export and collection interfaces |
| config_file_watcher.cpp | config change watching and hot-reload trigger surfaces |
| config_encrypted_store.cpp | encrypted config storage and key-rotation behavior |
| config_audit_log.cpp | config access audit logging surfaces |

## Scope

In scope:
- config path resolution and fallback control behavior
- schema validation and config-format safety checks
- config metrics/audit observability and watcher integration
- encrypted storage handling for sensitive configuration values

Out of scope:
- business-domain logic outside config lifecycle ownership
- external secret-management platforms beyond module integration points
- non-config runtime execution semantics in other subsystems

## Runtime Behavior and Limits

- resolution behavior depends on configured mapping and runtime environment.
- watcher/exporter paths depend on enabled runtime integrations.
- validation and encrypted-store paths return structured failure behavior.

## Sourcecode Verification (Module: config/readme)

- Verified files:
  - src/config/config_path_resolver.cpp
  - src/config/config_schema_validator.cpp
  - src/config/config_metrics_exporter.cpp
  - src/config/config_file_watcher.cpp
  - src/config/config_encrypted_store.cpp
  - src/config/config_audit_log.cpp
- Verified behavior surfaces:
  - resolution and schema-validation runtime paths
  - metrics/audit and watcher observability behavior
  - encrypted config storage and rotation integration surfaces
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md