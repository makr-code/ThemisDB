# ThemisDB Themis Core Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

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