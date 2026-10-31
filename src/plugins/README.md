---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Plugins Module

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · IMPLEMENTATION.md · API.md · RUNBOOK.md · TRUST.md -->

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

## Developer Documentation

| Document | Use it for |
|---|---|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Runtime boundaries, contracts, and failure domains |
| [RESEARCH.md](./RESEARCH.md) | Evaluation method and runtime trade-offs |
| [MODELING.md](./MODELING.md) | Lifecycle and capability state model |
| [EMBEDDING.md](./EMBEDDING.md) | Embedding interface boundary and evaluation ownership |
| [IMPLEMENTATION.md](./IMPLEMENTATION.md) | Runtime load, reload, and cleanup invariants |
| [API.md](./API.md) | Public plugin interfaces and return/error behavior |
| [MODULE_GUIDE.md](./MODULE_GUIDE.md) | Plugin author integration and test checklist |
| [RUNBOOK.md](./RUNBOOK.md) | Incident diagnosis, recovery, and escalation |
| [OPERATIONS.md](./OPERATIONS.md) | Deployment controls and monitoring |
| [SECURITY.md](./SECURITY.md) · [TRUST.md](./TRUST.md) | Security controls and trust boundaries |
| [ROADMAP.md](./ROADMAP.md) · [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) | Planned and future work |

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
- Verified behavior surfaces:
  - lifecycle, security, hot-plug, health, and integration paths
- `include/plugins/huggingface_ingestion_plugin.h` declares an ingestion
  interface, but no matching implementation file is present in the current
  `src/plugins` source set.
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md