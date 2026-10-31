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

1. **Contract conformance audit (partially completed):** see
   `MODULE_GAPS.md` for verified enforcement and specific missing coverage.
   Several declared input caps and watcher timing values are not enforced;
   implementing those limits remains separate work.
2. **Schema support contract (TODO):** publish the exact implemented keyword
   set and test unsupported/invalid schema forms.
3. **Encrypted-store API errors (resolved):** `tryGet()` returns
   `std::nullopt` only for absence and propagates decryption failures.
   `rotateKey()` atomically re-encrypts under an exclusive lock; concurrent
   reads/writes wait for the operation. Focused tests cover these semantics.
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

Do not treat unenforced constants as input limits. Preserve authenticated
decryption errors; protect serialized store snapshots. Consumer-level reload
and recovery behavior remains the owning service's responsibility.
