<!-- Status: source review | code/CMake checked: 2026-10-31; tests/builds not run -->
<!-- AI_Used: true; AI_Models: unknown; AI_Contributor: GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Contributors -->
<!-- Exact model backend not verifiable from final document text alone. -->

# Config Module Roadmap

## Current Status

The source tree contains implementations for path resolution, schema parsing
and validation, file watching, metrics export, audit entries, and encrypted
in-memory storage. Focused test source files cover those areas. This inventory
does not establish production readiness, full contract compliance, benchmark
results, or a release gate pass. Project release posture remains governed by
the root [`../../ROADMAP.md`](../../ROADMAP.md).

## In Progress

- [~] Audit the policy constants in `include/config/config_contract.h`; the
  current enforcement findings are recorded in `MODULE_GAPS.md`. Implementing
  the confirmed missing bounds remains open.
- [ ] Specify and test the exact schema keyword subset; resolve the mismatch
  between the "Draft 7 subset" contract wording and actual implementation.
- [ ] Verify file-watcher behavior and downstream reload/apply handling on each
  supported platform; watcher notification alone is not a reload guarantee.
- [x] Make `tryGet()` return `std::nullopt` only for absence and propagate
  decryption failures; document that key rotation blocks concurrent access
  while the atomic re-encryption holds the exclusive store lock.
- [ ] Collect benchmark runs and an authoritative baseline before marking any
  thresholds measured or release-blocking.

## Planned Features

No separate feature commitment was verified. The follow-up candidates above
remain proposals until implemented, tested, and recorded in the changelog.

## Implementation Phases

The previous "completed Phase 1–6" statements described contract and benchmark
work as fully delivered without current run evidence. Completion status is
therefore unverified and is not carried forward as a fact.

1. **Design / API contract:** TODO—reconcile `config_contract.h` with code and
   public API comments.
2. **Core implementation:** source files exist; contract-wide behavior is
   unverified.
3. **Error handling and edge cases:** selected behavior has focused tests;
   completeness is unverified.
4. **Tests:** focused test sources and root CMake registrations exist; run the
   encrypted-store tests for the updated absence/error semantics.
5. **Performance and hardening:** benchmark sources exist; measured baselines
   and pass/fail gates are unverified.
6. **Documentation and acceptance:** this documentation set was reconciled
   against source; human review and acceptance remain pending.

## Production Readiness Checklist

- [ ] Contract limits and failure semantics verified against implementation.
- [ ] Supported schema subset specified and tested.
- [ ] Consumer reload/apply behavior verified where required.
- [ ] Benchmark results and baselines retained for any release threshold.
- [ ] Human review and release status confirmed by the owning governance path.

## Known Issues and Limitations

- Contract policy constants are not all runtime-enforced; see `MODULE_GAPS.md`.
- Validator behavior is a subset; full Draft 7 support is not established.
- Several declared input and watcher bounds remain unenforced.
- Watcher cache invalidation does not establish consumer reload/apply success.
- Benchmark labels and targets do not establish measured threshold compliance.

## Breaking Changes

No statement about future compatibility was verified in this source review.
Apply the root versioning policy and document migration impact for any public
API change; do not infer a no-breaking-change commitment from this roadmap.

## Source references

- `src/config/*.cpp`
- `include/config/*.h`
- `tests/config/*.cpp`
- `tests/CMakeLists.txt`
- `benchmarks/config/*.cpp`
- [`MODULE_GUIDE.md`](MODULE_GUIDE.md)
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
