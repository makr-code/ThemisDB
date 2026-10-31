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

## Contract guarantee audit

| Declared policy | Implementation and test evidence | Status |
|---|---|---|
| Config file / schema / key / value size limits | No corresponding boundary checks found in the file loaders or encrypted store. Existing hardening tests assert constants or schema `maxLength`, not global limits. | Not enforced |
| Resolver path length limit | `validatePath()` rejects traversal, embedded NULs, and unsupported absolute paths; no length check found. | Not enforced |
| Top-level key count, nesting depth, validation-error cap | The validator recursively evaluates schemas and appends errors; no checks against these constants found. Hardening tests for key count/depth do not assert rejection. | Not enforced |
| Watcher intervals / operation timeout | Watcher uses platform event APIs. Constants do not configure its backend; tests mostly check constant values, and the deletion test checks continued running rather than callback delivery. | Not enforced as stated |
| Failure classes | `ConfigFailureClass` and `isFailClosedClass()` are standalone declarations; `ConfigException` types do not carry the enum or share a classifier. | Policy only |
| Schema behavior | Source handles `$ref`, `type`, `enum`, `const`, `allOf`, `anyOf`, `oneOf`, `not`, `required`, `properties`, `additionalProperties`, array constraints, string constraints/formats, and numeric bounds. External `$ref` is rejected. `enum` and `const` return before sibling validation; formats use regex checks, including a simplified IPv6 check. | Partial subset; not Draft 7 conformance |
| Encrypted-store crypto | AES-256-GCM is used; decrypt validates 12-byte IV and 16-byte tag and verifies the authentication tag. Snapshot decoding does not enforce an entry-size cap. | Verified, with size limitation |
| Key rotation | `rotateKey()` prepares new encrypted entries while holding a unique lock, then replaces key/store; prior key bytes are overwritten with `std::fill`. Reads/writes wait during rotation. | Atomic data replacement; blocking; zeroization strength not established |
| `tryGet()` | Missing entry returns `std::nullopt`; decryption exceptions now propagate. Focused tests cover absent and corrupted-tag cases. | Resolved |

## Open verification items

1. Implement or formally remove the unenforced size, nesting, top-level-key,
   validation-error, and watcher timing policies. Their constants must not be
   treated as security limits in the meantime.
2. Expand tests for actual watcher callback delivery and each platform backend;
   current tests do not establish end-to-end reload/apply behavior.
3. Run the release-gate benchmark target and retain results before treating its
   labels as performance evidence.
4. Regenerate and review config Doxygen XML; this page records the result when
   that run is completed.

## Evidence reviewed

- `src/config/*.cpp`
- `include/config/*.h`
- `tests/config/test_config_*.cpp`
- `tests/CMakeLists.txt`
- `benchmarks/config/*.cpp`
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md)

The table above is a source review, not a proof of runtime behavior for
unexecuted tests. Follow-up implementation work should add boundary assertions
and record reproducible test/build evidence.
