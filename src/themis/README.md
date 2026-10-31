# ThemisDB Themis Core Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · RESEARCH.md · MODELING.md · EMBEDDING.md · IMPLEMENTATION.md · API.md · MODULE_GUIDE.md · RUNBOOK.md · OPERATIONS.md · TRUST.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The themis core module provides runtime foundation behavior for build metadata, edition/licensing gates, secure module loading and verification, dependency resolution, and wire protocol server operation.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| build_info.cpp | build metadata and configuration behavior |
| edition_manager.cpp | edition/feature-gate behavior |
| license_info.cpp | license verification behavior |
| module_loader.cpp | core module loading behavior |
| module_loader_linux.cpp | Linux-specific module loading behavior |
| module_loader_win32.cpp | Windows-specific module loading behavior |
| module_hash_verifier.cpp | module hash verification behavior |
| module_signature_verifier.cpp | module signature verification behavior |
| module_security.cpp | module security policy behavior |
| module_dependency_resolver.cpp | module dependency and load-order behavior |
| wire_protocol_server.cpp | wire server behavior |

## Scope

In scope:
- build/edition/license/runtime core framework behavior
- secure module loading and verification paths
- dependency resolution and wire protocol server behavior

Out of scope:
- domain-specific business logic in higher feature modules
- non-themis subsystem internals outside module boundaries

## Runtime Behavior and Limits

- license/edition checks expose explicit allow/deny outcomes.
- module loading requires deterministic verification and dependency ordering.
- platform-specific loading paths are explicit and diagnosable.
- wire protocol behavior remains bounded by server and session constraints.

## Developer Documentation

- [Architecture](./ARCHITECTURE.md) describes module boundaries and runtime flows.
- [Research](./RESEARCH.md), [modeling](./MODELING.md), and
  [embedding](./EMBEDDING.md) record the evidence and algorithm scope; this
  module does not implement vector embedding generation.
- [Implementation](./IMPLEMENTATION.md), [API](./API.md), and
  [module guide](./MODULE_GUIDE.md) support code changes and integration.
- [Runbook](./RUNBOOK.md), [operations](./OPERATIONS.md), and
  [trust](./TRUST.md) cover incident handling and security boundaries.
- The [core operator runbook](../../docs/operability/RUNBOOK_THEMIS_CORE.md)
  is the operational reference for alert names, signals, and recovery procedures.

## Sourcecode Verification (Module: themis/readme)

- Verified files:
  - src/themis/build_info.cpp
  - src/themis/edition_manager.cpp
  - src/themis/license_info.cpp
  - src/themis/module_loader.cpp
  - src/themis/module_loader_linux.cpp
  - src/themis/module_loader_win32.cpp
  - src/themis/module_hash_verifier.cpp
  - src/themis/module_signature_verifier.cpp
  - src/themis/module_security.cpp
  - src/themis/module_dependency_resolver.cpp
  - src/themis/wire_protocol_server.cpp
- Verified behavior surfaces:
  - build/license/edition + secure load/verify + dependency and wire-server paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md