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

# Implementation Notes — Themis Core

## Scope and ownership

The implementation under `src/themis` provides runtime foundation behavior.
Keep domain-specific query, storage, vector, and model execution in their
owning modules. Public contracts are in `include/themis/` and its relevant
subdirectories; do not infer a public API from an internal `.cpp` filename.

| Source | Responsibility |
|---|---|
| `build_info.cpp` | Build and reproducibility metadata |
| `edition_manager.cpp` | Edition feature and resource decisions |
| `license_info.cpp` | Embedded license access, activation/validation, and runtime gate support |
| `module_hash_verifier.cpp` | SHA-256 computation and manifest comparison |
| `module_signature_verifier.cpp` | Platform signature verification |
| `module_security.cpp` | Module security policy and verification coordination |
| `module_dependency_resolver.cpp` | Dependency constraints and load ordering |
| `module_loader.cpp` | Shared module lifecycle and load orchestration |
| `module_loader_linux.cpp`, `module_loader_win32.cpp` | Platform-specific loading |
| `wire_protocol_server.cpp` | Themis wire protocol server and session behavior |

## Implementation invariants

1. Keep edition selection (compile-time) distinct from license validity
   (runtime); gated-feature checks combine both where the API specifies that
   behavior.
2. Keep content hash, signature, dependency, security-policy, and platform-load
   results distinct. A success in one stage cannot mask failure in another.
3. Propagate malformed input, missing dependencies, invalid trust evidence,
   and platform errors explicitly. Do not add a silent permit or insecure
   fallback.
4. Keep platform-specific system calls in their platform implementation files.
   Shared lifecycle code should not assume Linux or Windows loader semantics.
5. Do not use this module to add vector generation or domain-model execution;
   use the responsible subsystem's public contract.

## Failure and validation expectations

Changes to license or edition decisions need contract tests for both allowed
and denied states, including expiry/grace or uninitialized state where
applicable. Verifier and resolver changes need tests for missing entries,
invalid evidence, and invalid dependency graphs. Loader changes require
platform-aware coverage. Wire changes require malformed-frame, disconnect,
session cleanup, and concurrent-client coverage.

Start with
[`test_themis_contract_hardening_focused.cpp`](../../tests/themis/test_themis_contract_hardening_focused.cpp),
[`test_themis_integration.cpp`](../../tests/themis/test_themis_integration.cpp),
and [`test_themis_wire_protocol_server.cpp`](../../tests/themis/test_themis_wire_protocol_server.cpp);
use the current test registration in
[`tests/themis/CMakeLists.txt`](../../tests/themis/CMakeLists.txt) as the
build-system authority. Run matching benchmark cases for hot-path changes.
The dated [`BUILD_TEST_EVIDENCE.md`](./BUILD_TEST_EVIDENCE.md) is an inventory,
not proof that a target was built in the current checkout.

See [API.md](./API.md), [research evaluation](./RESEARCH.md), and
[operations](./OPERATIONS.md) before changing contracts or recovery behavior.
