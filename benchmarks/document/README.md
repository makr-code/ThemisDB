# Document Module Benchmarks

This directory contains CMake-registered benchmarks for the document module: in-memory store, diff/merge, and round-trip interface suites, plus a separate synthetic serialization-gates suite.

- [Benchmark coverage and case inventory](DOCUMENT_BENCHMARK_COVERAGE.md)
- [Declared release gate behavior and limitations](RELEASE_GATES.md)
- [Module performance expectations](../../src/document/PERFORMANCE_EXPECTATIONS.md)

The suite is not a mirror of `src/document/`; three executables exercise in-memory document implementations, while `bench_document_serialization_gates.cpp` uses stubs and is not production-path evidence. See `CMakeLists.txt` for conditional target registration.
