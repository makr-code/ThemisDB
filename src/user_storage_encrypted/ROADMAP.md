---
Author: ThemisDB Contributors
Created: 2026-08-08
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# User Storage Encrypted Module Roadmap

Exact model backend not verifiable from final document text alone.

Status below is limited to repository source evidence. Source presence or CMake
registration is not evidence that a test passed, a benchmark met a threshold,
or a feature is production-ready. Related documentation sync: issue #7392.

## Current implementation

The repository contains a gocryptfs backend, KDF interfaces and implementation,
a separate rotation scheduler/store, and a multi-level plugin for JSON
user/group records. The operational boundaries and known discrepancies are in
[API.md](API.md), [ARCHITECTURE.md](ARCHITECTURE.md), and
[AUDIT.md](AUDIT.md).

## Follow-up work

| Status | Work | Evidence / qualification |
|---|---|---|
| [!] | Reconcile frozen contract guarantees with operational behavior | Different namespaces/error types; atomicity/cleanup claims are not demonstrated on all code paths. Owner decision required. |
| [~] | Improve mount/unmount recovery and result propagation | `unmountAll()` ignores per-level errors; some rotation unmount errors are ignored. |
| [~] | Make rotation execution/reporting reliable | Rotation config is parsed but scheduler wiring is absent; callback exceptions are swallowed and schedule state advances after an attempt. |
| [ ] | Implement executable end-to-end encrypted lifecycle integration tests | Current E2E cases unconditionally skip. |
| [ ] | Capture representative-hardware performance baselines and enforce gates | Current lifecycle benchmarks simulate operations; source labels are not fail/pass thresholds. |
| [ ] | Define deployment/security requirements supported by enforcement | Existing source does not enforce production-mode variables, audit policy, all permissions/resource controls, or provider retry policy. |

Target dates and completion status for these follow-ups require maintainer
confirmation. Earlier Q3/Q4 2026 and Q1 2027 status snapshots are not carried
forward as verified delivery evidence.

## Test and benchmark evidence map

- Focused test CMake registration: `tests/user_storage_encrypted/CMakeLists.txt`
- Test limitations and skips: `tests/user_storage_encrypted/README.md`
- Benchmark target registration: `benchmarks/user_storage_encrypted/CMakeLists.txt`
- Benchmark measurement limits: `benchmarks/user_storage_encrypted/README.md`
- No build, test, or benchmark execution results are claimed in this roadmap.

## References

- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/CMakeLists.txt`
- [Future enhancements](FUTURE_ENHANCEMENTS.md)
- [Performance evidence status](PERFORMANCE_EXPECTATIONS.md)
