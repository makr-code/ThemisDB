---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI ChatGPT"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

# Evaluation module capabilities

AI provenance: Exact model backend not verifiable from final document text alone.

## Scope and reader

This is a source-backed capability summary for developers evaluating the
`src/evaluation` C++ component. It describes contracts in the checked-in headers
and implementations; it does not claim that a target was built, a benchmark
passed, or an end-to-end retrieval path is deployed.

## Implemented code surfaces

- Retrieval metrics compute retrieval, evidence, provenance, compression, LLM
  answer, distributed-efficiency and tensor-graph summary values from caller
  inputs. They do not retrieve data or invoke an LLM judge.
- `BenchmarkMatrix` stores results by scenario and dimension, and provides
  lookup, invalidation, slicing and comparison helpers. It is not a benchmark
  runner or a measurement store with persistence.
- Hardware-profile contracts define profile, resource, tier and transition
  data, with validation and lookup functions.
- `AblationRunner` computes metrics over caller-supplied query fixtures and
  configured variants.
- `QueryPlanner` returns a typed path decision from eligibility, freshness and
  configuration inputs; it does not execute the selected retrieval path.
- Approximation rules evaluate a requested boundary and planned path against a
  policy.
- Artifact lifecycle APIs compute states from supplied metadata and policy and
  return metadata for transitions; they do not run rebuild jobs.

The `themis_evaluation` static library lists `benchmark_matrix.cc`,
`hardware_profile.cc`, `retrieval_metrics.cc`, `ablation_framework.cc`, and
`artifact_lifecycle.cc`. Query planning and approximation rules are separate
conditional CMake targets. There is no `storage_strategy` implementation in the
checked-in `src/evaluation` source/header inventory reviewed for this page.

## Value, target scenarios and limitations

These APIs provide typed, testable calculation and decision contracts for
callers comparing retrieval metrics, evaluating configured ablations, or
selecting a policy-based query path. Inputs, labels, hardware signals and
benchmark values remain caller-supplied; a returned metric or planner decision
is not proof of representative retrieval quality or production readiness. The
benchmark file under `benchmarks/evaluation/` uses stubs and is not a
performance result for these implementations.

## References

- [Module overview](./README.md)
- [Architecture and build boundaries](./ARCHITECTURE.md)
- [API contracts](./API.md)
- [Implementation map](./IMPLEMENTATION.md)
- `src/evaluation/CMakeLists.txt`
- `src/evaluation/include/`
- `src/evaluation/src/`
- `tests/evaluation/CMakeLists.txt`
- `benchmarks/evaluation/CMakeLists.txt`
