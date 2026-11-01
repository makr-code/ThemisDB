---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Tests for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

The test targets and CTest labels below come from
`tests/user_storage_encrypted/CMakeLists.txt`. The root `tests/CMakeLists.txt`
auto-discovers module directories that contain a `CMakeLists.txt`. Building
requires the repository test configuration and GTest.

## Registered focused targets

| Source file | CTest tier | Additional labels | Scope |
|---|---|---|---|
| `test_user_storage_encrypted_contract_hardening_focused.cpp` | unit | none | 8 contract/error-value, descriptor/request default, and fail-closed-predicate checks |
| `test_backend_io_timeout_focused.cpp` | unit | `io_safety` | PipeGuard and timed-I/O tests; target is registered on UNIX |
| `test_backend_command_injection_focused.cpp` | unit | `command_injection`, `security` | Path, character, key-format, and flag validation checks |
| `test_phase2_error_codes_focused.cpp` | unit | `error_handling`, `diagnostics`, `phase2` | Error-code mapping and diagnostic event tests |
| `test_user_storage_encrypted_e2e_vault_integration_focused.cpp` | integration | `e2e`, `vault` | 8 scenario stubs; each calls `GTEST_SKIP()` before exercising its scenario, so this is not executable Vault validation |
| `test_user_storage_encrypted_stress_focused.cpp` | stress | `concurrency`, `error_injection` | Concurrent lifecycle and failure-path test cases |
| `test_user_storage_encrypted_highcardinality_stress.cpp` | stress | `wave_d`, `not_release_critical` | Three stub-backed concurrent write, rotation, and round-trip tests |

`test_user_storage_encrypted_phase1_hardening.cpp` is present in this
directory, but is not registered by the current module `CMakeLists.txt`.
Presence of that source file is not evidence that it is built or run.

## Running registered targets

Configure/build the repository with tests enabled, then run all registered
module tests:

```bash
ctest --preset linux-release -L user_storage_encrypted --output-on-failure
```

Select additional labels with CTest's label regular-expression filter, for
example:

```bash
ctest --preset linux-release -L io_safety --output-on-failure
ctest --preset linux-release -L 'command_injection|security' --output-on-failure
ctest --preset linux-release -L 'integration|e2e|vault' --output-on-failure
ctest --preset linux-release -L 'stress|wave_d' --output-on-failure
```

The local CMake registration specifies 120-second timeouts for unit targets
and 300-second timeouts for integration/stress targets. Individual targets
are conditionally registered when their source files exist; the E2E target is
also UNIX-only.

## External dependencies and evidence limits

- The backend requires host gocryptfs/FUSE for real encrypted mount behavior.
- The E2E source comments mention Vault, `VAULT_ADDR`, and `VAULT_TOKEN`, but
  the test code does not read those settings or connect to Vault. Each case
  skips before the scenario body; treat the comments as planning notes, not
  runnable integration configuration.
- High-cardinality tests use an in-process stub and do not validate actual
  gocryptfs encryption or disk I/O.
- CMake registration establishes build/test wiring, not a successful build or
  passing test run. No test execution result is claimed by this document.

## Source references

- `tests/user_storage_encrypted/CMakeLists.txt`
- `tests/CMakeLists.txt` (module directory discovery)
- `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`
- `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`
- `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`
- `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`
- `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`
- `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`
- `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`
