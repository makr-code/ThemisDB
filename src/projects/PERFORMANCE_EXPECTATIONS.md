---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-11-20"
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
| PRJP-1 | Measure production `ProjectVersioning` snapshot creation at 100/1,000 documents, metadata retrieval, and 1,000-document restore; numeric release threshold pending a representative baseline | BM_ProjectVersioning_CreateSnapshot/100, BM_ProjectVersioning_CreateSnapshot/1000, BM_ProjectVersioning_GetSnapshot, BM_ProjectVersioning_RestoreSnapshot |
| PRJP-2 | Measure production snapshot diff at 100/1,000 documents and three-way merge at 1,000 documents with overlapping changes/conflicts; numeric release threshold pending a representative baseline | BM_ProjectDiff_Snapshots/100, BM_ProjectDiff_Snapshots/1000, BM_ProjectMerge_ThreeWay |
| PRJP-3 | Measure production web-application template instantiation, collaboration lock/unlock cycles, and persisted change notifications; numeric release threshold pending a representative baseline | BM_ProjectTemplate_Instantiate, BM_CollaborationManager_LockCycle, BM_CollaborationManager_ChangeNotification |

## Release-Gate Status

| Gate ID | Expectation | Measurement |
|---|---|---|
| PRJG-1 | Regression threshold vs a project-specific baseline | Not established |
| PRJG-2 | Project hot-path p99 threshold | Not established |
| PRJG-3 | Complete project benchmark manifest | Not established |

## Validation

- The project workflow cases now measure the production APIs listed above, but
  no numeric project-runtime target or release gate is verified. Treat the
  measurements as coverage evidence, not achieved release criteria.
- Derive numeric thresholds from repeatable release-profile measurements on
  declared hardware before enabling performance gates.

## Sourcecode Verification (Module: projects/performance)

- Verified benchmark sources:
  - benchmarks/projects/bench_projects_release_gates.cpp
  - benchmarks/projects/bench_projects_workflows.cpp
  - benchmarks/content/bench_content_versioning.cpp
  - benchmarks/query/bench_query_lazy_eval.cpp
- Result:
  - enum representation/dispatch and project snapshot, diff, merge, template,
    and collaboration workflows are directly benchmarked under `projects`;
    content/query cases remain cross-module context only.