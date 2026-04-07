<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Plugins Module — Architecture Guide

## Overview

The plugins module provides the full lifecycle for ThemisDB extensions: discovery, dependency resolution, capability negotiation, hot-plug loading/unloading, cryptographic signature verification, OCI registry distribution, WASM host sandboxing, health monitoring, and self-healing. The central interface is `IThemisPlugin` in `plugin_interface.h`.

## Design Principles

- **Signed distribution** — all production plugins must be signed with Ed25519 keys (`signed_plugin_repository.h`).
- **Hot-reload without restart** — `plugin_hot_plug_monitor.h` watches filesystem events; plugins reload without service interruption.
- **Capability negotiation** — `PluginCapabilityNegotiator` matches plugin-provided capabilities against host requirements before load.
- **Dependency graph** — `PluginDependencyResolver` resolves load-order via topological sort.
- **Sandboxed WASM** — `wasm_host_api.h` provides a WASM runtime with restricted host-call surface.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `huggingface_ingestion_plugin.h` | `HuggingFaceIngestionPlugin` | Ingestion plugin for Hugging Face datasets |
| `image_analysis_interface.h` | `IImageAnalysisPlugin` | Abstract interface for image analysis plugins |
| `image_analysis_manager.h` | `ImageAnalysisManager` | Manages active image analysis plugin instances |
| `oci_registry_client.h` | `OciRegistryClient` | OCI registry push/pull for plugin distribution |
| `plugin_api.h` | Plugin C ABI macros | `THEMIS_PLUGIN_EXPORT`, version macros |
| `plugin_dependency_resolver.h` | `PluginDependencyResolver` | Topological dependency resolution |
| `plugin_health_monitor.h` | `PluginHealthMonitor` | Liveness/readiness probes per plugin |
| `plugin_hot_plug_monitor.h` | `PluginHotPlugMonitor` | inotify/FSEvents-based hot-reload trigger |
| `plugin_interface.h` | `IThemisPlugin` | Core plugin interface; all plugins implement this |
| `plugin_manager.h` | `PluginManager` | Central registry: load, unload, list, query |
| `plugin_metrics.h` | `PluginMetrics` | Per-plugin Prometheus metrics (calls, errors, latency) |
| `plugin_registry.h` | `PluginRegistry` | Thread-safe plugin catalogue |
| `rpc_plugin_interface.h` | `IRpcPlugin` | RPC-over-plugin transport interface |
| `self_healing_plugin.h` | `SelfHealingPlugin` | Automatic crash recovery and restart policy |
| `signed_plugin_repository.h` | `SignedPluginRepository` | Ed25519 signature verification for plugin artifacts |
| `wasm_host_api.h` | `WasmHostApi` | WASM runtime sandbox with restricted host-call surface |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `observability` | `PluginMetrics` | Per-plugin Prometheus metrics |
| `network` | `IRpcPlugin` | RPC transport for remote plugins |
| `scheduler` | `PluginManager` | Plugin-triggered scheduled tasks |
| OCI registries | `OciRegistryClient` | Docker Hub / GHCR distribution |
| WASM runtime | `WasmHostApi` | Wasmtime or WAMR sandbox |

## Implementation

Implementation in `../../src/plugins/`.
