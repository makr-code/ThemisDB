---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/projects

## Scope

- Module: src/projects
- This file defines measurable projects module performance expectations for release gating.

## Existing Benchmark Coverage

- `benchmarks/projects/bench_projects_release_gates.cpp` exercises `ProjError`
  enum casts, switch dispatch, range checks, and batch casts. It does not call
  project lifecycle, snapshot, diff, template, collaboration, or audit APIs.
- `benchmarks/content/bench_content_versioning.cpp` measures the content
  `VersionManager`, not `projects::ProjectVersioning` or `ProjectDiff`.
- `benchmarks/query/bench_query_lazy_eval.cpp` measures query lazy evaluation,
  not project operations.
- The content/query cases are therefore cross-module context only and are not
  evidence of projects-module latency or throughput.

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| PRJP-1 | Measure snapshot creation, retrieval, and restore on `ProjectVersioning` | Not currently measured |
| PRJP-2 | Measure `ProjectDiff`/`ProjectMerge` workloads with representative document counts and conflicts | Not currently measured |
| PRJP-3 | Measure template instantiation and collaboration change/lock paths | Not currently measured |

## Release-Gate Status

| Gate ID | Expectation | Measurement |
|---|---|---|
| PRJG-1 | Regression threshold vs a project-specific baseline | Not established |
| PRJG-2 | Project hot-path p99 threshold | Not established |
| PRJG-3 | Complete project benchmark manifest | Not established |

## Validation

- No project-runtime performance target or release gate is verified by the
  current benchmark source. Treat the targets above as work to define, not
  achieved release criteria.
- Add module-native benchmark cases before setting numeric limits; derive
  thresholds from repeatable release-profile measurements on declared hardware.

## Sourcecode Verification (Module: projects/performance)

- Verified benchmark sources:
  - benchmarks/projects/bench_projects_release_gates.cpp
  - benchmarks/content/bench_content_versioning.cpp
  - benchmarks/query/bench_query_lazy_eval.cpp
- Result:
  - only enum representation/dispatch is directly benchmarked under `projects`;
    the other workloads belong to separate modules and do not validate project
    runtime performance.