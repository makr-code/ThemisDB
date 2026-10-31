<!-- Status: candidate work | source checked: 2026-10-31 -->
<!-- AI_Used: true; AI_Models: unknown; AI_Contributor: GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Contributors -->
<!-- Exact model backend not verifiable from final document text alone. -->

# Config Module — Future Enhancements

## Scope

Candidate engineering work for `src/config`. Items remain proposals until
implemented, tested, and recorded in the module changelog. Root release status
is owned by [`../../ROADMAP.md`](../../ROADMAP.md).

## Design Constraints

- Keep path resolution, validation, notification, metrics, and encrypted-store
  responsibilities separate.
- Keep watcher callbacks as notifications unless a transactional reload API is
  explicitly designed and tested.
- Treat serialized encrypted-store snapshots as key material because they
  contain the current key.
- Keep behavior claims traceable to code and focused test evidence.

## Required Interfaces

No new public interface is currently specified. Any interface proposal must
identify caller need, error semantics, ownership/lifetime, thread-safety, and
compatibility impact before implementation.

## Implementation Notes

1. **Contract conformance audit (TODO):** map the constants and guarantees in
   `include/config/config_contract.h` to enforcing code and tests. Do not treat
   declared file-size, nesting, error-count, or timing bounds as enforced until
   verified.
2. **Schema support contract (TODO):** publish the exact implemented keyword
   set and test unsupported/invalid schema forms.
3. **Encrypted-store API errors (TODO):** decide whether `tryGet()` should
   distinguish missing keys from decryption/decoding failures; preserve current
   behavior in compatibility notes until changed.
4. **Watcher integration (TODO):** add consumer-level evidence that notification
   results in explicit reload, validation, and apply steps where required.

## Test Strategy

Use the target/CTest pairs in [`MODULE_GUIDE.md`](MODULE_GUIDE.md). Add focused
tests for each confirmed contract gap and retain platform-specific watcher
evidence. Test registration is not evidence that a test passed.

## Performance Targets

No measured target or baseline was verified. Existing benchmark cases and
targets are listed in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
Collect results on a defined release profile before describing a threshold as
achieved or release-blocking.

## Security / Reliability

Validate claimed input bounds against implementation. Preserve authenticated
decryption errors in APIs where callers require distinction; protect serialized
store snapshots. Consumer-level reload and recovery behavior remains the
owning service's responsibility.
