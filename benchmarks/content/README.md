**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `benchmarks/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [benchmark registration](CMakeLists.txt), [benchmark sources](.), [benchmark build preset](../../CMakePresets.json), [content module implementation](../../src/content/)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content benchmarks

## Registered targets

Benchmarks are gated by `THEMIS_BUILD_BENCHMARKS`. The current
[`CMakeLists.txt`](CMakeLists.txt) registers these targets:

| Target | Source | Evidence scope |
|---|---|---|
| `bench_content_processor_paths` | `bench_content_processor_paths.cpp` | Deterministic synthetic work for Office-like text, image-byte and archive-entry paths; it does not invoke the production format processors. |
| `bench_content_versioning` | `bench_content_versioning.cpp` | `VersionManager` version creation, diff, retrieval, storage-overhead and concurrency workloads. |
| `bench_content_release_gates` | `bench_content_release_gates.cpp` | Small `content_api_contract.h` value/enum microbenchmarks; not end-to-end ingestion gates. |

`bench_text_extraction.cpp` is present in this directory but is not listed as a target in
the current CMake registration. Treat it as an unregistered source, not an available
benchmark. Whether it should be registered is a follow-up decision; its current
implementation also describes mock extraction.

## Configure, build, and run

The `community-debug` preset enables benchmarks. For example:

```bash
cmake --preset community-debug -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset community-debug --target bench_content_processor_paths
```

Run the resulting executable from the configured build tree. With the standard Linux
directory layout, for example:

```bash
./build-community-debug/bin/bench_content_processor_paths
```

The other registered executables are `bench_content_versioning` and
`bench_content_release_gates`. Record the build configuration, hardware, and benchmark
arguments with any reported result; these sources alone are not release-performance
evidence or a deployment SLO.
