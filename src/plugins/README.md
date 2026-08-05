# ThemisDB Plugins Module

<!-- Status: current | validated: 2026-08-05 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The plugins module provides dynamic plugin loading, registration, lifecycle management, security validation/signing, hot-plug behavior, health monitoring, and plugin metrics/integration infrastructure for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| plugin_manager.cpp | core plugin lifecycle orchestration |
| plugin_registry.cpp | plugin registration and manifest/security validation |
| plugin_system_edition.cpp | edition-based plugin capability gating |
| plugin_metrics.cpp | per-plugin metrics and telemetry surfaces |
| plugin_health_monitor.cpp | plugin liveness and recovery monitoring |
| plugin_hot_plug_monitor.cpp | file-system hot-plug detection and reload behavior |
| signed_plugin_repository.cpp | signed plugin repository and key verification |
| oci_registry_client.cpp | OCI-based remote plugin retrieval |
| rpc_service_registry.cpp | RPC service registration for plugins |
| wasm_plugin_loader.cpp | WASM plugin loading pathway |
| huggingface_ingestion_plugin.cpp | first-party reference ingestion plugin |

## Scope

In scope:
- plugin lifecycle and loading/unloading/reload behavior
- plugin security/manifest/signature and capability handling
- plugin health/metrics/hot-plug and registry integration behavior

Out of scope:
- business logic inside individual external plugins
- external plugin marketplace ownership beyond integration interfaces
- non-plugin operational domains

## Runtime Behavior and Limits

- behavior depends on plugin manifests, enabled edition/features, and runtime configuration.
- unsupported plugin runtime paths degrade deterministically with explicit outcomes.
- validation/security checks are enforced before activation boundaries.

## Sourcecode Verification (Module: plugins/readme)

- Verified files:
  - src/plugins/plugin_manager.cpp
  - src/plugins/plugin_registry.cpp
  - src/plugins/plugin_system_edition.cpp
  - src/plugins/plugin_metrics.cpp
  - src/plugins/plugin_health_monitor.cpp
  - src/plugins/plugin_hot_plug_monitor.cpp
  - src/plugins/signed_plugin_repository.cpp
  - src/plugins/oci_registry_client.cpp
  - src/plugins/rpc_service_registry.cpp
  - src/plugins/wasm_plugin_loader.cpp
  - src/plugins/huggingface_ingestion_plugin.cpp
- Verified behavior surfaces:
  - lifecycle, security, hot-plug, health, and integration paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md