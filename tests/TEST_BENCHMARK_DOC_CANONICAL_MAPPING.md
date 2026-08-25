# Test and Benchmark Documentation Mapping

Status: Active maintainer reference
Last update: 2026-08-21
Purpose: Fast mapping from historical reports to current canonical sources.

## Canonical Sources (Current)

- Test rules: [TESTING_STANDARDS.md](TESTING_STANDARDS.md)
- Benchmark rules: [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md)
- Benchmark measurement protocol: [../benchmarks/MEASUREMENT_HYGIENE.md](../benchmarks/MEASUREMENT_HYGIENE.md)
- CTest run/inventory truth: [../CTEST.md](../CTEST.md)
- Area entry points: [README.md](README.md), [../benchmarks/README.md](../benchmarks/README.md)

## Alt -> Canonical (Top 10)

| Historical file | Scope | Canonical replacement | Current handling |
|---|---|---|---|
| [GOOGLE_TEST_WIRKSAMKEIT_ZUSAMMENFASSUNG.md](GOOGLE_TEST_WIRKSAMKEIT_ZUSAMMENFASSUNG.md) | Test effectiveness summary | [TESTING_STANDARDS.md](TESTING_STANDARDS.md), [../CTEST.md](../CTEST.md) | Keep as historical context |
| [TEST_VERIFICATION_SUMMARY.md](TEST_VERIFICATION_SUMMARY.md) | Verification snapshot | [TESTING_STANDARDS.md](TESTING_STANDARDS.md), [../CTEST.md](../CTEST.md) | Keep as historical context |
| [TEST_ENHANCEMENT_SUMMARY.md](TEST_ENHANCEMENT_SUMMARY.md) | Test/benchmark enhancement snapshot | [TESTING_STANDARDS.md](TESTING_STANDARDS.md), [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md) | Keep as historical context |
| [TEST_COVERAGE_REPORT.md](TEST_COVERAGE_REPORT.md) | Coverage analysis snapshot | [TESTING_STANDARDS.md](TESTING_STANDARDS.md), [../CTEST.md](../CTEST.md) | Keep as historical context |
| [../benchmarks/docs/BENCHMARKS_EXECUTIVE_SUMMARY.md](../benchmarks/docs/BENCHMARKS_EXECUTIVE_SUMMARY.md) | Executive benchmark status | [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md), [../benchmarks/README.md](../benchmarks/README.md) | Keep as historical context |
| [../benchmarks/docs/BENCHMARK_IMPLEMENTATION_REPORT.md](../benchmarks/docs/BENCHMARK_IMPLEMENTATION_REPORT.md) | Implementation/report snapshot | [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md), [../benchmarks/MEASUREMENT_HYGIENE.md](../benchmarks/MEASUREMENT_HYGIENE.md) | Keep as historical context |
| [../benchmarks/docs/SCIENTIFIC_BENCHMARKS_COMPLETION_SUMMARY.md](../benchmarks/docs/SCIENTIFIC_BENCHMARKS_COMPLETION_SUMMARY.md) | Scientific benchmark rollout snapshot | [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md), [../benchmarks/MEASUREMENT_HYGIENE.md](../benchmarks/MEASUREMENT_HYGIENE.md) | Keep as historical context |
| [../benchmarks/docs/ROCKSDB_BENCHMARK_BEST_PRACTICES.md](../benchmarks/docs/ROCKSDB_BENCHMARK_BEST_PRACTICES.md) | Domain best practices (legacy) | [../benchmarks/MEASUREMENT_HYGIENE.md](../benchmarks/MEASUREMENT_HYGIENE.md), [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md) | Archived (redirect stub + archive copy) |
| [../benchmarks/docs/STANDARD_BENCHMARKS_README.md](../benchmarks/docs/STANDARD_BENCHMARKS_README.md) | Legacy standards narrative | [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md) | Archived (redirect stub + archive copy) |
| [../benchmarks/docs/BENCHMARK_STATUS.md](../benchmarks/docs/BENCHMARK_STATUS.md) | Point-in-time status page | [../benchmarks/README.md](../benchmarks/README.md), [../CTEST.md](../CTEST.md) | Archived (redirect stub + archive copy) |
| [../benchmarks/docs/DOCKER_BENCHMARKS_STATUS_REPORT.md](../benchmarks/docs/DOCKER_BENCHMARKS_STATUS_REPORT.md) | Docker status snapshot | [../benchmarks/README.md](../benchmarks/README.md), [../benchmarks/BENCHMARK_STANDARDS.md](../benchmarks/BENCHMARK_STANDARDS.md) | Archived (redirect stub + archive copy) |

## Archive Guidance

- Keep files with unique historical implementation context.
- Archive files that duplicate canonical rules without unique evidence.
- Before archiving, add a one-line pointer to canonical replacements.
