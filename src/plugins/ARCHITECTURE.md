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

# Architecture - Plugins Module

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The plugins module composes plugin lifecycle orchestration, manifest/security validation, hot-plug monitoring, health/metrics reporting, and remote repository integration into a bounded extension subsystem for ThemisDB.

## Main Execution Planes

1. Lifecycle and registry plane
- plugin load/unload/reload orchestration
- plugin registration and capability/edition gating

2. Security and validation plane
- manifest parsing and signature verification
- capability validation and runtime compatibility checks

3. Monitoring and integration plane
- plugin metrics, health monitoring, and hot-plug behavior
- OCI/rpc and WASM-related integration surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic plugin state transitions and rollback behavior |
| security contract | explicit manifest/signature/capability enforcement |
| monitoring contract | bounded health and metrics reporting behavior |
| integration contract | deterministic repository/rpc/runtime integration behavior |

## Failure Semantics

- invalid manifest/signature/capability input fails with explicit outcomes.
- reload failures preserve deterministic fallback behavior.
- unavailable integrations are surfaced as explicit errors.

## Sourcecode Verification (Module: plugins/architecture)

- Verified files:
  - src/plugins/plugin_manager.cpp
  - src/plugins/plugin_registry.cpp
  - src/plugins/plugin_hot_plug_monitor.cpp
  - src/plugins/plugin_health_monitor.cpp
  - src/plugins/signed_plugin_repository.cpp
  - src/plugins/oci_registry_client.cpp
- Verified architecture claims:
  - explicit lifecycle/security/monitoring/integration planes
  - deterministic failure boundaries across plugin workflows
  - module-local ownership of plugin orchestration behavior

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` | Logging, audit, and thread helpers for plugin lifecycle management |
| base | `include/base/module_loader.h` | Base module provides sandbox and wasm isolation primitives consumed by plugins |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/plugins/plugin_manager.h` | Server loads and unloads plugins via plugin manager |
| transaction | `include/plugins/` | Transaction module loads transaction extension plugins |
| llm | `include/plugins/` | LLM module loads inference backend plugins (e.g., llama_cpp, onnx_clip) |

## Integration Points

### Critical Integration: Server Plugin Lifecycle
**Files:** `src/plugins/plugin_manager.cpp` ↔ `server/`
**Contract:** Host integrations use `PluginManager::loadPlugin()` / `unloadPlugin()` for manifest-backed extension lifecycle; that path validates the manifest and binary policy and applies edition/license restrictions before activation. Reported capabilities are snapshotted after initialization; negotiation and escalation checks are separate APIs. Explicit-path and OCI loading have distinct manifest-policy semantics; see [TRUST.md](./TRUST.md).
**Thread Safety:** Load/unload operations are serialised under an internal lifecycle mutex; hot-plug monitor may trigger reload asynchronously but acquires the same lock.

### Critical Integration: LLM Inference Backend Plugins
**Files:** `src/plugins/plugin_registry.cpp` ↔ `llm/`
**Contract:** LLM module queries plugin registry for available inference backends by capability tag; registered plugin capabilities must remain stable after registration.
**Thread Safety:** Registry reads are concurrent-safe; registrations are performed at startup under a write lock.