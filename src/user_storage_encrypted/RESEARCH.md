---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Research Notes for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

These notes distinguish source structure from behavior that still requires
integration or operational evidence. They are not a security certification or
production-readiness assessment.

## Implementation boundaries

- `GocryptfsBackend` is the module's host-command/FUSE integration boundary.
  Availability and mount operations depend on the host environment. Review
  `gocryptfs_backend.cpp` and its header for the actual checks and command
  behavior; do not infer guarantees such as complete traversal prevention,
  safe subprocess handling, or stale-mount recovery from the API contract
  comments alone.
- `KeyDerivationService` has Argon2id support when CMake finds libargon2 and a
  PBKDF2-HMAC-SHA256 fallback otherwise. The multi-level plugin constructs its
  backend without injecting a KDF service. Existing-data compatibility across
  build modes is not established by the reviewed sources.
- `KeyRotationScheduler` is a separate component. Its callback and persistence
  behavior should not be confused with configured rotation being automatically
  wired into `MultiLevelEncryptedStorage`; that wiring is not present in the
  reviewed implementation.
- `MultiLevelEncryptedStorage` parses the multi-level JSON configuration and
  coordinates backend instances. Health checks and metrics have limited
  coverage; see `AUDIT.md` and `OPERATIONS.md`.

## Source-backed questions

1. **Contract/runtime parity:** The frozen contract in
   `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
   differs from the operational API in namespace/error types. Contract
   guarantees include partial-mount cleanup, general key/salt zeroing,
   transactional rotation, per-container serialization, and callback
   restrictions. Current source does not establish those guarantees across
   implementation paths. Resolve the discrepancy before treating normative
   contract text as verified runtime behavior; see [API comparison](API.md).
2. **Failure propagation:** `unmountAll()` does not preserve individual
   unmount errors, and some rotation unmount errors are ignored. Define and
   test the desired recovery semantics.
3. **Rotation semantics:** `isRotationDue()` and `getNextRotationTime()` are
   timestamp helpers. `triggerRotation()` invokes a callback, not
   `MultiLevelEncryptedStorage::rotateKey()`; manual calls execute on the
   caller's thread. The scheduler swallows callback exceptions and
   advances/persists its timestamp after an attempt. Decide what success,
   retry, and observable failure should mean.
4. **KDF deployment:** Specify how deployments select and persist derivation
   parameters, and how they handle data created with a different build-time
   KDF. Current source evidence does not establish cross-mode compatibility.
5. **Evidence:** E2E cases currently skip unconditionally; the hardening test
   source is not registered in the scoped CMake file. Lifecycle benchmark
   cases include simulations, while dedicated gates use stubs. Real lifecycle
   and performance evidence remains follow-up work.

## Research direction

Prioritize explicit API/implementation parity, failure reporting, KDF
compatibility policy, and executable integration coverage before making
stronger availability, atomicity, rotation, or performance claims. See
[module roadmap](ROADMAP.md), [audit notes](AUDIT.md), and
[performance evidence](PERFORMANCE_EXPECTATIONS.md).
