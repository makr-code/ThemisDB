# Test Density Closure Wave Plan (`tests/`)

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
**Status:** active

## Purpose

This file is the canonical execution plan for closing the source-validated test-density gaps documented in `TEST_DENSITY_MATRIX.md`.
It converts the current-state matrix into a Wave A -> B -> C -> D closure program aligned with the root governance wave model on `develop`.

## Preconditions

- Use `TEST_DENSITY_MATRIX.md` as the current ownership and flow baseline.
- Count only source-validated evidence from `tests/**`, `benchmarks/**`, `tests/CMakeLists.txt`, `tests/**/CMakeLists.txt`, and active release/test workflow paths.
- Do not treat historical summary markdown files as closure evidence.

## Evidence Requirements Per Module / Flow

Each closure claim should resolve to the following reproducible evidence classes where applicable:

1. direct owner suite under `tests/<module>/`
2. direct benchmark or perf owner under `benchmarks/<module>/`
3. cross-module or integration path under `tests/integration/**` or curated root suites
4. explicit CTest registration and labels
5. explicit gate or operator command using current presets
6. residual risk classification if evidence remains intentionally incomplete

## Wave A — Release-Critical Core Flows

### Scope

Primary modules:
- `query`
- `index`
- `rag`
- `transaction`
- `llm_wiki`
- `search`

Secondary hub modules:
- `server`
- `sharding`
- `llm`
- `storage`

Canonical flows:
- `server -> query -> storage -> transaction`
- `sharding <-> transaction`
- `search -> index -> tensor -> graph -> llm`
- `server <-> llm`

### Required closure work

- Assign one canonical evidence path per critical flow from focused tests through benchmark/perf evidence.
- Consolidate the `release_critical` mapping for the truly release-blocking suites that anchor those flows.
- Remove unclassified `indirect only` claims for the Wave A modules by pointing each module to an owned focused suite and one flow suite.
- Record the exact reproducible command or label entry point for each module/flow pair.

### Canonical command anchors

- Full build/test baseline: `cmake --preset linux-release && cmake --build --preset linux-release && ctest --preset linux-release`
- Release-critical regression gate: `ctest --preset linux-release -L release_critical`
- Pipeline inventory anchor: `ctest --preset linux-release -L pipeline_integration`
- Benchmark baseline: `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

### Exit criteria

- Every critical flow has one documented start suite, flow suite, recovery/chaos suite, stress/soak suite, benchmark/perf suite, and gate owner.
- Every Wave A module has direct suite ownership plus a documented flow attachment.
- `release_critical` coverage for Wave A modules is explicit rather than implied.

## Wave B — Boundary / API / Policy Densification

### Scope

- `api`
- `network`
- `rpc_grpc`
- `auth`
- `governance`
- `plugins`
- `importers`
- `exporters`
- `content`
- `scheduler`
- `observability`

### Required closure work

- Bind each boundary module to at least one production-near flow or gate.
- Consolidate API, RPC, auth, governance, and plugin contract suites into explicit regression paths.
- Publish reproducible commands and labels for each boundary block.
- Classify remaining indirect evidence as either sufficient-by-design or backlog.

### Exit criteria

- Each boundary module has direct suite ownership and at least one documented cross-module flow.
- No boundary module is considered covered only because an adjacent module incidentally exercises it.
- Commands and labels needed for regression reruns are documented for every boundary block.

## Wave C — Thin Ownership Gap Closure

### Scope

Mandatory gap modules:
- `distributed_tensor`
- `execution`
- `llama_cpp`
- `stable_diffusion`
- `evaluation`
- `retrieval`

### Required closure work

- Add or designate dedicated module-owner test suites for `distributed_tensor` and `execution`.
- Add module-owner test paths for `llama_cpp` and `stable_diffusion`.
- Add module-owner benchmark/perf evidence for `evaluation` and `retrieval`.
- Where a dedicated owner suite is not justified, define an explicit accepted-indirect-evidence rule with source validation.

### Exit criteria

- No prioritized code-bearing module remains without a test owner.
- No known gap module remains without either direct benchmark ownership or an explicit accepted exception.
- The immediate ownership gaps listed in `TEST_DENSITY_MATRIX.md` are closed or formally classified.

## Wave D — Hardening, Operability, and Sign-off

### Scope

- All prior waves as one repo-wide closure audit

### Required closure work

- Back-link soak, stress, chaos, recovery, and representative-hardware evidence to the owning modules and flows.
- Remove historical coverage summaries from operational closure decisions and keep them as context only.
- Synchronize matrix, wave plan, roadmap tracking, and active gate references.
- Publish explicit residual risks for modules or flows that remain intentionally deferred.

### Exit criteria

- For all prioritized modules the record shows owner suite, flow coverage, perf evidence, gate path, and residual risk state.
- Test density closure can be defended from source, registration, labels, and commands without relying on historical summaries.
- The repo has one canonical closure narrative for test density instead of scattered local claims.

## Recommended Execution Order

1. Complete Wave A first.
2. Then complete Wave B.
3. Then close the thin ownership gaps in Wave C.
4. Finish with Wave D hardening and sign-off.

## Immediate Next Block

Start with the Wave A primary modules:
- `query`
- `index`
- `rag`
- `transaction`
- `llm_wiki`
- `search`

Then fold in the remaining Wave A hub modules:
- `server`
- `sharding`
- `llm`
- `storage`
