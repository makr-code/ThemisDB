# Architecture - Themis Core Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The themis core module composes build/runtime identity behavior, license and edition gating, secure module load/verification, dependency resolution, and wire protocol runtime behavior into a bounded foundation subsystem.

## Main Execution Planes

1. Runtime identity and gating plane
- build info, edition manager, and license verification behavior

2. Secure module lifecycle plane
- loader, hash/signature verifier, security policy, and dependency resolver behavior

3. Wire runtime plane
- wire protocol server session and dispatch behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| runtime identity contract | deterministic build/configuration metadata behavior |
| gating contract | explicit license/edition feature decision behavior |
| module lifecycle contract | bounded secure load/verify/dependency behavior |
| wire contract | explicit wire server startup/session error behavior |

## Failure Semantics

- license and edition failures return explicit deny outcomes.
- module hash/signature/dependency failures remain deterministic and diagnosable.
- platform loader failures are explicit per target platform.
- wire server faults remain observable through runtime diagnostics.

## Sourcecode Verification (Module: themis/architecture)

- Verified files:
  - src/themis/build_info.cpp
  - src/themis/edition_manager.cpp
  - src/themis/license_info.cpp
  - src/themis/module_loader.cpp
  - src/themis/module_hash_verifier.cpp
  - src/themis/module_signature_verifier.cpp
  - src/themis/module_dependency_resolver.cpp
  - src/themis/wire_protocol_server.cpp
- Verified architecture claims:
  - identity/gating + secure lifecycle + wire runtime plane split
  - explicit failure boundaries for license, loader, and wire faults
  - module-local ownership of themis core behavior