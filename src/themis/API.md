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

# API Contracts — Themis Core

## Purpose and audience

This page identifies the public C++ contracts relevant to `src/themis` and
their important edge cases. The headers are authoritative for signatures and
behavior; this summary is not a substitute for their Doxygen comments.

## Build and edition contracts

- [`build_info.h`](../../include/themis/build_info.h): query build/module
  configuration, format a summary, and export or verify a build manifest.
  Manifest export creates or overwrites the destination; verification returns
  `false` when compared metadata differs. The header warns that its STL-based
  DLL API requires compatible MSVC toolset and CRT settings.
- [`edition.h`](../../include/themis/edition.h): compile-time edition identity,
  feature flags, resource ceilings, and gated feature names. Unknown edition
  strings map to `EditionType::UNKNOWN`; unknown feature names in
  `IsFeatureEnabled` return `false`.
- [`edition_manager.h`](../../include/themis/edition_manager.h):
  process-wide thread-safe feature/resource checks and edition information.
  Its documented `isFeatureAvailable` behavior treats names outside the
  enterprise gate list as available, so callers must not use arbitrary
  untrusted names as an authorization mechanism.

## License contracts

- [`license_info.h`](../../include/themis/license_info.h): embedded license
  lookup and validity helpers, `LicenseClient` activation/validation/refresh,
  and `LicenseInfo` grace-period calculation. `LicenseClientConfig` includes a
  timeout, offline behavior, and grace period; callers must configure these for
  their deployment. The documented signature helper returns true when no
  signature is present, so it must not be treated as proof of signer identity.
- [`runtime_license_gate.h`](../../include/themis/runtime_license_gate.h):
  singleton initialized from activation/validation state; feature checks can
  return a boolean or structured `GateResult`. Initialize it after license
  validation and before relying on gated feature checks. Its named gate set is
  explicit; features outside that set are not restricted by this gate.

## Module integrity contracts

- [`module_hash_verifier.h`](../../include/themis/module_hash_verifier.h):
  computes a SHA-256 file digest and compares a module against a loaded
  manifest. Static helpers are thread-safe; manifest methods are documented as
  not thread-safe. File and parse errors return failure/empty results rather
  than a successful verification.
- [`module_signature_verifier.h`](../../include/themis/module_signature_verifier.h):
  dispatches to Windows Authenticode or Linux GPG verification. On Linux, an
  empty signature path triggers documented suffix detection. A successful
  signature result depends on the configured platform trust store/keyring.
- Loader and wire contracts are declared in
  [`include/themis/base/module_loader.h`](../../include/themis/base/module_loader.h)
  and [`include/themis/network/wire_protocol_server.hpp`](../../include/themis/network/wire_protocol_server.hpp);
  consult those headers for exact parameters, ownership, and error semantics.

## Compatibility and caller guidance

Treat explicit error results as part of the contract. Do not replace a failed
license, hash, signature, dependency, or protocol check with a caller-side
default permit. Before changing an exported declaration, search its callers,
update API comments and dependent module documentation, and add tests for the
new success and failure behavior.

See [implementation invariants](./IMPLEMENTATION.md) and the
[security trust model](./TRUST.md).
