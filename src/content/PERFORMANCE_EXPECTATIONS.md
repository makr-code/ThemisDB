**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-11-19
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [benchmark registration](../../benchmarks/content/CMakeLists.txt), [processor-path benchmark](../../benchmarks/content/bench_content_processor_paths.cpp), [text-extraction benchmark source](../../benchmarks/content/bench_text_extraction.cpp), [content benchmark notes](../../benchmarks/content/README.md)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/content

## Scope

- Module: src/content
- This file defines measurable content module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/content/bench_content_processor_paths.cpp` (registered as
    `bench_content_processor_paths`; its Office/OCR/archive paths use synthetic work,
    not the production format processors)
  - `benchmarks/content/bench_text_extraction.cpp` (contains mock extraction workloads;
    not registered in `benchmarks/content/CMakeLists.txt`)

## Release Gates

The following are acceptance gates, not claims that current code or benchmark runs pass.
The content roadmap remains release-blocked until each gate has current evidence.

| Gate | Measurable acceptance target | Evidence |
|---|---|---|
| CNT-C1 | 100% of required versioning and validation invariants pass focused regressions, including invalid/degraded cases; no missing outcome may be inferred as success. | Versioning, validator, streaming, security, and pipeline test results on the candidate SHA |
| CNT-S1 | 0 silently accepted invalid processor-chain fields; retries have an enforced finite attempt ceiling and finite total-delay ceiling. | Negative/overflow/wrong-type/unknown-category configuration tests plus injected transient-failure attempt counts |
| CNT-S2 | 100% of enabled safety controls either execute with a positive result flag or reject startup/configuration as unavailable; no unexecuted malware/PII/abuse/schema check is reported as passed. | Validator/security tests and capability-disabled configuration tests |
| CNT-P1 | For every selected production-path benchmark, p99 regression is at most 10% versus a reviewed baseline on the same workload and representative hardware. | Registered production benchmark, raw results, hardware/build/workload manifest, baseline and comparison |
| CNT-P2 | 100% of release-mapped benchmark cases appear in the run manifest; mock/synthetic work does not count as a production-path case. | Release benchmark manifest and source-to-case mapping |
| CNT-F1 | Test, security, and benchmark evidence is from the exact release-candidate commit and was executed within 14 days of sign-off. | CI run/artifact URLs, candidate SHA, timestamps, and human sign-off record |

These are proposed module gates. The 10% performance regression ceiling follows the
existing CTG-1 expectation; the 14-day evidence window is a freshness requirement for
this hardening target. Owners must approve the workload, baseline, hardware, and selected
format matrix before measurement. A missing baseline or artifact is `PENDING`, never a
pass.

## Current evidence and limitations

- The current CMake file registers `bench_content_processor_paths`,
  `bench_content_versioning`, and `bench_content_release_gates`; it does not register
  `bench_text_extraction.cpp`.
- The processor-path target performs deterministic synthetic work, not calls to
  `OfficeProcessor`, `OCRProcessor`, or `ArchiveProcessor`.
- `bench_text_extraction.cpp` contains mock extraction workloads and is unregistered.
- No benchmark result, representative-hardware baseline, or complete release manifest is
  established by the checked-in source. Current targets are therefore pending and do
  not support production extraction p99 or speedup claims.
- The scheduled content workflow's Wave-D soak job builds the whole test target before
  running soak; on 2026-11-15 that build failed in unrelated
  `tests/test_config_file_watcher.cpp` (`writeFile` undeclared), and the content test job
  was skipped. This is not content-test pass evidence; see
  [workflow run #185](https://github.com/makr-code/ThemisDB/actions/runs/43681511059).
