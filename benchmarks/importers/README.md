---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/importers/CMakeLists.txt; benchmarks/importers/bench_importers_release_gates.cpp; benchmarks/importers/bench_importer_throughput.cpp; src/importers/PERFORMANCE_EXPECTATIONS.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# benchmarks/importers

When `THEMIS_BUILD_BENCHMARKS` is enabled and Google Benchmark is available,
the local CMake file declares `bench_importer_throughput` and
`bench_importers_release_gates`.

The release-gate source contains six in-memory/mock cases and target labels;
the labels are not themselves computed p99s, threshold assertions, or proof of a
passing run. The throughput source uses generated payloads/scans and does not
call production importer connectors for all its cases. See
[`../../src/importers/PERFORMANCE_EXPECTATIONS.md`](../../src/importers/PERFORMANCE_EXPECTATIONS.md).

On 2026-10-31 both sources were compiled and run standalone with GCC 13.3.0 and
Google Benchmark 1.8.3. The library reported a DEBUG build; this exploratory run
is not release-profile, connector-runtime, or p99 threshold evidence.
