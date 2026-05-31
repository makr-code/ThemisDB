# Security - Themis Core Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the themis core module focuses on deterministic license/edition decisions, secure module verification and loading, integrity-preserving dependency resolution, and explicit wire-runtime fault signaling.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| invalid/forged license acceptance | explicit license verification behavior |
| tampered module loading | hash/signature verification and security policy checks |
| dependency graph manipulation | deterministic resolver outcomes and load-order checks |
| opaque wire runtime degradation | explicit wire server error and session signaling |

## Implemented Security Controls

- license/edition gate operations expose explicit allow/deny results.
- module hash/signature verification paths are explicit and diagnosable.
- loader security policy outcomes remain deterministic.
- wire runtime failures remain observable and non-silent.

## Security Follow-ups

- broaden fault-injection coverage for signature and dependency edge cases.
- deepen stress coverage for concurrent wire session admission pressure.
- tighten diagnostics taxonomy across license/load/verify incidents.

## Sourcecode Verification (Module: themis/security)

- Verified files:
  - src/themis/license_info.cpp
  - src/themis/edition_manager.cpp
  - src/themis/module_hash_verifier.cpp
  - src/themis/module_signature_verifier.cpp
  - src/themis/module_security.cpp
  - src/themis/module_loader.cpp
  - src/themis/wire_protocol_server.cpp
- Verified controls:
  - explicit gating and verification signaling
  - deterministic secure module loading outcomes
  - observable wire-runtime fault behavior