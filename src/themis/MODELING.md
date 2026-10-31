---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Runtime Models — Themis Core

## Purpose

This document describes the finite, deterministic state models that developers
must preserve when changing `src/themis`. The module has no statistical
training, parameter fitting, or probabilistic inference pipeline; “modeling”
here means the runtime decision and lifecycle state machines represented by
the public contracts.

## Edition and feature decisions

The build selects an `EditionType` and compile-time feature constants in
[`edition.h`](../../include/themis/edition.h). For gated features, the runtime
decision composes that build-time result with `RuntimeLicenseGate` state:

| Compile-time feature | Runtime license | Decision |
|---|---|---|
| disabled | any state | denied |
| enabled | valid active license | allowed |
| enabled | allowed grace state | allowed under the documented grace policy |
| enabled | invalid, expired, or uninitialized gate | denied |

The gate's named feature list and denial reasons are declared in
[`runtime_license_gate.h`](../../include/themis/runtime_license_gate.h).
Do not silently treat an unknown feature as gated or permitted across the
different interfaces: `EditionManager` documents unknown names as allowed,
while `RuntimeLicenseGate` only restricts its named enterprise features.
Call sites should use the authoritative check for their contract.

## Module verification and dependency order

The loader path combines module identity, integrity/signature evidence, policy
checks, and dependencies before activation. These are separate decisions:

- the SHA-256 verifier compares bytes with an expected manifest entry;
- the signature verifier checks a platform signature mechanism;
- the dependency resolver determines whether required modules and constraints
  can be satisfied; and
- the loader applies platform-specific activation behavior.

A positive result from one stage is not proof that the other stages passed.
Missing manifests, hash mismatches, invalid signatures, unresolved
dependencies, cycles, and platform load errors must remain distinct failures.
See [`module_dependency_resolver.cpp`](./module_dependency_resolver.cpp),
[`module_loader.cpp`](./module_loader.cpp), and the verifier headers linked
from [API.md](./API.md).

## Wire session lifecycle

The wire server owns connection/session admission, protocol handling, and
session completion or failure. A stalled or malformed peer is a runtime
failure case, not evidence that the whole host should silently accept or discard
messages. Validate this lifecycle with
[`test_themis_wire_protocol_server.cpp`](../../tests/themis/test_themis_wire_protocol_server.cpp)
and the high-cardinality stress tests. Exact timeout and message limits are
configuration/implementation contracts; consult the active wire header and
server configuration rather than inventing defaults here.

## Invariants for changes

- Feature decisions remain deterministic for fixed build and license state.
- Verification results do not replace authorization or dependency checks.
- Failures at a lifecycle boundary must propagate as explicit errors.
- Changes to state transitions require focused contract tests and integration
  coverage; performance changes also require the mapped benchmark cases.

See [research](./RESEARCH.md) for the evidence strategy and
[implementation](./IMPLEMENTATION.md) for the code ownership map.
