---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot, MAI-Code-1.1-Flash"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Guide

## Who this module is for

This module is intended for developers and operators working on runtime integration, module lifecycle, dependency safety, sandboxing, and live reload in ThemisDB. It is not a user-facing domain implementation layer; it is a runtime control plane.

## Responsibilities

The base module owns the following responsibilities:

- module discovery, verification, metadata extraction, and lifecycle management,
- compatibility validation and required-symbol enforcement,
- dependency graph construction and ordering,
- sandboxed execution boundaries for plugin and WASM code,
- atomic hot reload and rollback semantics,
- remote registry intake and artifact integrity checks,
- optional rollout testing and promotion control.

## Key files

- `include/themis/base/module_loader.h` — module lifecycle and verification API
- `include/themis/base/module_sandbox.h` — sandboxing and ABI validation contracts
- `include/themis/base/hot_reload_manager.h` — reload and rollback lifecycle API
- `include/themis/base/plugin_dependency_graph.h` — graph model and ordering analysis
- `include/themis/base/remote_registry_client.h` — authenticated plugin registry intake
- `include/themis/base/wasm_plugin_sandbox.h` — WASM isolation and host-function allowlist
- `include/themis/base/base_error_taxonomy.h` — human-readable operational error taxonomy

## Working conventions

### 1. Keep trust checks in the base layer

The loader, security verifier, and ABI checker are where risk is evaluated. Do not spread trust checks into callers or feature modules. The trust boundary belongs here.

### 2. Make failure states explicit

The system prefers structured failures and typed error codes over ambiguous generic errors. When editing this area, preserve the error taxonomy and remediation hints.

### 3. Preserve rollback safety

Hot reload changes must keep the previous version recoverable until the replacement is confirmed good. Do not turn hot reload into an optimistic pointer swap.

### 4. Keep dependency graphs deterministic

Any dependency or ordering logic should be plain and visible. Silent dependency acceptance is a correctness bug in this module.

### 5. Keep execution bounded

Sandbox and WASM restrictions are part of the contract, not optional release decoration. Changes here should preserve fail-closed semantics and explicit resource limits.

## Development checklist

Before landing a base change, verify:

- the module still fails closed on signature or ABI issues,
- reload and rollback behavior remain atomic and reversible,
- dependency ordering logic still detects cycles correctly,
- sandbox constraints remain platform-aware and non-silent,
- any new public API has matching docs and contract notes,
- tests cover the path being changed.

## Testing and validation guidance

The module has strong coverage around:

- reload rollback regression cases,
- dependency conflict and ordering edge cases,
- sandbox degraded-state handling,
- WASM validation-only and allowlist cases,
- observability and tracing integration under reload stress.

When adding or changing behavior, target the closest tests for the affected surface rather than broad unrelated test sets.

## Summary

The base module should be easy to reason about: verify modules, constrain execution, order dependencies deterministically, and replace runtime artifacts only under explicit safe conditions. If a change makes this layer less explicit, it is likely in the wrong place.
