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

# Developer Guide — Themis Core Module

## Start here

`src/themis` is a runtime foundation module, not a feature-domain module. Its
primary code paths cover build identity, edition and license gating, secure
module lifecycle, and wire protocol service. Read
[`ARCHITECTURE.md`](./ARCHITECTURE.md) for boundaries, then select the closest
ownership path:

| Change | Start with |
|---|---|
| Build metadata or manifest | `build_info.cpp` and `include/themis/build_info.h` |
| Edition/resource decision | `edition_manager.cpp`, `include/themis/edition.h`, and `include/themis/edition_manager.h` |
| License state or gating | `license_info.cpp`, `include/themis/license_info.h`, and `include/themis/runtime_license_gate.h` |
| Module integrity or loading | verifier/loader source files and `include/themis/module_*_verifier.h` |
| Dependency order | `module_dependency_resolver.cpp` and its resolver contract |
| Wire protocol/session | `wire_protocol_server.cpp` and `include/themis/network/wire_protocol_server.hpp` |

Some similarly named loader interfaces live under `include/themis/base/`.
Confirm which implementation and API a caller uses before editing; do not
assume all plugin or module loaders route through this module's loader source.

## Change workflow

1. State the affected contract and its success, failure, and boundary cases.
2. Read the corresponding public header and implementation before changing
   behavior. Keep the public/internal ownership boundary explicit.
3. Add or update focused tests in `tests/themis/`; add integration or stress
   coverage when the change crosses module or concurrency boundaries.
4. For performance-sensitive code, run the mapped cases in
   [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) against the
   configured baseline; do not invent threshold values.
5. Update API-facing docs and operational procedures if caller-visible or
   operator-visible behavior changes.

## Edges to preserve

- Unknown and uninitialized gate states have API-specific semantics; use the
  exact gate named in the caller's contract.
- A hash is not a signature, and neither replaces dependency or feature
  authorization checks.
- Keep platform loading behavior in the Linux/Windows-specific source files.
- Treat wire input as untrusted and preserve explicit malformed-input and
  disconnect handling.
- This module does not own embedding/model inference; see
  [`EMBEDDING.md`](./EMBEDDING.md) for that boundary.

For tests and measured evaluation, see [`RESEARCH.md`](./RESEARCH.md) and
[`BUILD_TEST_EVIDENCE.md`](./BUILD_TEST_EVIDENCE.md). For incident response,
see [`RUNBOOK.md`](./RUNBOOK.md).
