# Distributed Tensor Module Source Audit

**Last Updated:** 2026-10-31
**Source level:** Level 1 — module implementation
**SOT domain:** module-behavior
**Canonical references:** `src/distributed_tensor/CMakeLists.txt`, `cmake/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `benchmarks/CMakeLists.txt`, `benchmarks/distributed_tensor/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`
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
| Library/API standard | `src/distributed_tensor/CMakeLists.txt` declares `themis_distributed_tensor` and privately requests `cxx_std_17`; project CMake defaults to C++20. `tensor_storage_strategy.h` uses C++20 `std::span`, so C++17 compatibility of the installed public API is not established and consumer propagation is unresolved. |
| Integrity implementation | The target compiles root-level `integrity_verification.cc`, where `simple_sha256_hash()` is FNV-1a demonstration code. The separate nested SHA-256 implementation is not in this target's source list; no cryptographic SHA-256/Merkle security guarantee applies to the default target. |
| Training coordinator | `TensorTrainingCoordinator` invokes registered worker callbacks synchronously in-process; it does not implement network transport or distributed worker coordination. |
| Build graph | `cmake/CMakeLists.txt` conditionally adds the module target. |
| Public API | Nine headers are listed in the module's `DISTRIBUTED_TENSOR_PUBLIC_HEADERS`. |
| Local tests | `tests/distributed_tensor/CMakeLists.txt` registers three focused test executables from the three local test source files. |
| EPIC 3 tests | `tests/CMakeLists.txt` adds `tests/epic3_distributed_tensor/`; its CMake registers the phase/contract test sources and an integrity-verification benchmark target. |
| Integration test | `tests/integration/CMakeLists.txt` includes `test_distributed_tensor_soak.cpp` when the source exists and assigns it a 120-second timeout. |
| Benchmarks | `benchmarks/CMakeLists.txt` adds both benchmark directories. `benchmarks/distributed_tensor/` selects the local dedicated-gates source; `benchmarks/epic3_distributed_tensor/` defines four standard benchmark sources plus partial-refit and summary-first targets. |

## Audit findings

- The earlier audit's seven-file source count and seven-header count are
  incomplete for the current target source list.
- The EPIC 3 directories are present and configured separately from the
  `distributed_tensor/` directories. They are current source registrations;
  historical pass-status claims still require original execution artifacts.
- Current CMake registration confirms targets and source paths only. No
  configure, build, CTest, benchmark, or runtime validation was performed for
  this documentation task.
- No test/benchmark execution, measured performance result, or
  production-readiness conclusion is established by this source audit.

## Open verification gaps

Test pass status, representative-hardware performance, production integration,
and release enablement remain unverified here. Historical EPIC 3 evidence
references should be checked against their original CI artifacts before being
used as current evidence.
