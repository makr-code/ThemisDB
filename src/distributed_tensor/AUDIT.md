# Distributed Tensor Module Source Audit

**Last Updated:** 2026-11-23
**Source level:** Level 1 — module implementation
**SOT domain:** module-behavior
**Canonical references:** `src/distributed_tensor/CMakeLists.txt`, `include/sharding/shard_artifact_fragment_store.h`, `src/sharding/shard_artifact_fragment_store.cpp`, `src/main_server.cpp`, `cmake/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `tests/sharding/CMakeLists.txt`, `tests/sharding/test_shard_transfer_store.cpp`, `benchmarks/CMakeLists.txt`, `benchmarks/distributed_tensor/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

## Source inventory checked

| Area | Source evidence |
|---|---|
| Library/API standard | `src/distributed_tensor/CMakeLists.txt` declares the public `cxx_std_20` compile feature. `tensor_storage_strategy.h` exposes `std::span`, so the installed public API requires C++20; CMake consumers linking the target inherit that requirement. |
| Integrity implementation | The target compiles root-level `integrity_verification.cc`, which now uses OpenSSL EVP SHA-256 conditionally and fails closed when OpenSSL Crypto is unavailable. Only SHA-256 is accepted; versioned receipts bind the artifact ID, manifest content hash/version, and declared shard digests, but not raw shard bytes or producer identity. `ArtifactManifest::compute_manifest_hash()` remains non-cryptographic. The separate nested implementation is still outside this target. The change has not been built or tested. |
| Training coordinator | `TensorTrainingCoordinator` invokes registered worker callbacks synchronously in-process; it does not implement network transport or distributed worker coordination. |
| Build graph | `cmake/CMakeLists.txt` conditionally adds the module target. |
| Public API | Ten headers are listed in the module's `DISTRIBUTED_TENSOR_PUBLIC_HEADERS`. |
| Local tests | `tests/distributed_tensor/CMakeLists.txt` registers three focused test executables from the three local test source files. |
| EPIC 3 tests | `tests/CMakeLists.txt` adds `tests/epic3_distributed_tensor/`; its CMake registers the phase/contract test sources and the separate private integrity test only when OpenSSL Crypto is found. |
| Integration and storage tests | No distributed-tensor soak is registered under `tests/integration`; the former in-process stub-only source was removed. `ShardTransferStoreTests` includes RocksDB-backed fragment-store regressions for principal/shard isolation, immutable replay/conflict, digest/size bounds, and restart persistence. The Linux/gRPC-conditional `ShardTransferGrpcTests` includes an unrun source case composing the local publisher, database reopen, mTLS, and manifest-bound retrieval with a fixture summary. It does not exercise production startup or an automatic producer. Neither target has been built or run in this session. |
| Benchmarks | `benchmarks/CMakeLists.txt` adds both benchmark directories. `benchmarks/distributed_tensor/` selects the local dedicated-gates source; `benchmarks/epic3_distributed_tensor/` defines the workload benchmarks, with its private integrity benchmark only when OpenSSL Crypto is found. |

## Audit findings

- The earlier audit's seven-file source count and seven-header count are
  incomplete for the current target source list.
- The EPIC 3 directories are present and configured separately from the
  `distributed_tensor/` directories. They are current source registrations;
  historical pass-status claims still require original execution artifacts.
- Current CMake registration confirms targets and source paths only. A
  diagnostic configure was attempted for this follow-up but stopped at
  `cmake/Dependencies.cmake:727` because the required `fmt` library was
  unavailable; no build tree or module targets were generated.
- A standalone syntax-only compile of the public storage-strategy header passed
  as C++20 and was rejected as C++17 at `std::span`, matching the declared
  language requirement.
- No distributed-tensor test or benchmark execution, measured performance
  result, or production-readiness conclusion is established by this source
  audit.

## Open verification gaps

Test pass status, representative-hardware performance, production integration,
and release enablement remain unverified here. Historical EPIC 3 evidence
references should be checked against their original CI artifacts before being
used as current evidence.
