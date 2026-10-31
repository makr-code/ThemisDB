# Evaluation Module

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

<!-- Last Updated: 2026-10-31 · Source level: level1 · SOT domain: module-behavior -->
<!-- Issue: #7344 · Milestone: DOC-WEEKLY-2026-44 -->

## Purpose and scope

The evaluation module provides C++ contracts and code for computing supplied
retrieval/evidence metrics, recording benchmark values, validating hardware
profiles and approximation boundaries, running ablation calculations,
selecting query execution paths, and deriving artifact lifecycle states.
Callers supply the data and perform retrieval, persistence, rebuild work and
downstream observability.

The query planner returns a decision; it is not a query executor. The
`BenchmarkMatrix` stores values; it does not run benchmarks. The public-path
`include/evaluation/evaluation.h` currently does not include component
declarations. The component headers are in `src/evaluation/include/`.

## Source and build overview

`src/evaluation/CMakeLists.txt` defines `themis_evaluation` from the benchmark
matrix, hardware profile, retrieval metrics, ablation and artifact lifecycle
sources. Query planning and approximation rules are separate conditional
libraries; artifact lifecycle also has a separately declared conditional
target. The CMake file's presence does not establish that a target is included
in a particular configured build.

See [ARCHITECTURE.md](./ARCHITECTURE.md) for the full source/target map and
[IMPLEMENTATION.md](./IMPLEMENTATION.md) for target and test registration
details.

## Quick start for developers

1. Read the relevant declaration in `src/evaluation/include/` and its matching
   implementation in `src/evaluation/src/`.
2. Check `src/evaluation/CMakeLists.txt` to identify which library owns the
   implementation and whether its target is conditional.
3. Check `tests/evaluation/CMakeLists.txt` or
   `tests/epic2_evaluation/CMakeLists.txt` for the actual test registration and
   dependencies before selecting a target.
4. Treat source presence and CMake registration as distinct from build, test or
   benchmark execution evidence.

## Build and run

The module is a library, not a standalone executable or service. After
configuring a build, check that the desired target is present and then build it,
for example:

```bash
cmake --build <build-dir> --target themis_evaluation
```

The query-planner, approximation and lifecycle targets have separate
availability conditions; their target names and prerequisites are listed in
`src/evaluation/CMakeLists.txt`. No module-owned runtime command or service
configuration is defined here.

No build, test or benchmark was run for this documentation-only update.

## Current evidence and limitations

The local module CMake files contain component-library and test/benchmark
registrations, some guarded by dependency or option checks. The module evidence
documents historical validation attempts; they are not current execution
results. The source inventory locates the storage assessor under `distributed_tensor`,
but does not establish that it is the exact root-roadmap item 2.7. The root
roadmap does not claim aggregate EPIC 2 acceptance without execution evidence.
The separate EPIC 2 storage benchmark measures synthetic shard placement, not
the assessor; see [ROADMAP.md](./ROADMAP.md).

## Related documentation

- [API contracts](./API.md)
- [Architecture](./ARCHITECTURE.md)
- [Implementation guide](./IMPLEMENTATION.md)
- [Developer guide](./MODULE_GUIDE.md)
- [Research and evaluation assumptions](./RESEARCH.md)
- [Modeling](./MODELING.md)
- [Embedding evaluation](./EMBEDDING.md)
- [Operations](./OPERATIONS.md) and [runbook](./RUNBOOK.md)
- [Security](./SECURITY.md) and [trust model](./TRUST.md)
- [Roadmap](./ROADMAP.md) and [evidence record](./MODULE_EVIDENCE.md)
