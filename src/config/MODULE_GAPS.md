---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Module Gaps

## Scope and evidence status

This is a small, source-review gap list, not a fresh automated scanner result.
The former counts (895 gaps from a `C:\Projects\...` scan) are stale and are
not evidence of current findings. No current whole-repository gap scan was run.

## Open documentation and verification items

1. **Contract-to-implementation bounds — TODO.** `config_contract.h` declares
   limits for input sizes, nesting, error collection and watcher timing.
   Verify each against code paths and focused tests before claiming enforcement.
2. **Schema keyword coverage — TODO.** Document the exact supported keyword
   set and test unsupported keywords explicitly. The implementation and current
   tests do not establish full Draft 7 conformance.
3. **Encrypted-store error semantics — verified caveat.** `get()` throws on
   absence/decryption failure; `tryGet()` catches decryption/decoding errors
   and returns `std::nullopt`, conflating those cases.
4. **Watcher end-to-end reload — TODO.** Current watcher delivery and resolver
   cache invalidation do not prove that a consuming service reloads and applies
   validated settings.
5. **Benchmark evidence — TODO.** Benchmark source labels name desired
   thresholds, but no run result or maintained baseline was inspected for this
   documentation update. See [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
6. **Release-gate benchmark API mismatch — TODO.** The current
   `bench_config_release_gates.cpp` calls `ConfigSchemaValidator::validate()`
   with a JSON value, while `config_schema_validator.h` declares the file-path
   overload and `validateFromString()`; no matching JSON-value overload was
   found in the reviewed implementation. Target buildability is unverified;
   reconcile the benchmark call and API before relying on this target.
7. **Contract/runtime mismatch — TODO.** The contract describes key rotation as
   non-blocking to readers/writers, but `ConfigEncryptedStore::rotateKey()`
   takes an exclusive `shared_mutex` lock while re-encrypting entries. The
   contract also declares a maximum validation-error count that was not
   confirmed in the inspected validator path.

## Evidence reviewed

- `src/config/*.cpp`
- `include/config/*.h`
- `tests/config/test_config_*.cpp`
- `tests/CMakeLists.txt`
- `benchmarks/config/*.cpp`
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md)

**TODO:** Replace these review notes with specific issue IDs and reproducible
test evidence when the open verification work is completed.
