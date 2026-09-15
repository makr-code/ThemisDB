# Test Density Closure Wave Plan (`tests/`)

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
**Status:** active

## Purpose

This file is the canonical execution plan for closing the source-validated test-density gaps documented in `TEST_DENSITY_MATRIX.md`.
It converts the current-state matrix into a Wave A -> B -> C -> D closure program aligned with the root governance wave model on `develop`.

Companion execution backlog:
- `TEST_DENSITY_WAVE_A_BACKLOG.md`

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
- Promote unified-only supplemental Wave A tests into dedicated CTest-owned entries where this can be done without adding new mock-only code paths.
- Promote canonical Wave A perf anchors into benchmark CTest smoke entries and aggregate benchmark build targets.

### Canonical command anchors

- Full release-critical baseline: `cmake --preset community-release -DTHEMIS_BUILD_TESTS=ON && cmake --build build-community-release --target themis_release_critical_tests --parallel "$(nproc)"`
- Release-critical regression gate: `ctest --test-dir build-community-release --label-regex "release_critical" --output-on-failure --parallel 1 --timeout 120`
- Wave A `server <-> llm` gate: `cmake --build build-community-release --target themis_wave_a_server_llm_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_a_flow_server_llm" --output-on-failure --parallel 1 --timeout 120`
- Wave A `server -> query -> storage -> transaction` gate: `cmake --build build-community-release --target themis_wave_a_server_query_storage_transaction_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_a_flow_server_query_storage_transaction" --output-on-failure --parallel 1 --timeout 120`
- Wave A `sharding <-> transaction` gate: `cmake --build build-community-release --target themis_wave_a_sharding_transaction_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_a_flow_sharding_transaction" --output-on-failure --parallel 1 --timeout 120`
- Benchmark smoke baseline: `cmake --preset community-debug`
- Wave A `server <-> llm` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_server_llm_benchmarks --parallel "$(nproc)" && ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_server_llm" --output-on-failure --parallel 1 --timeout 180`
- Wave A `server -> query -> storage -> transaction` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_server_query_storage_transaction_benchmarks --parallel "$(nproc)" && ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_server_query_storage_transaction" --output-on-failure --parallel 1 --timeout 180`
- Wave A `sharding <-> transaction` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_sharding_transaction_benchmarks --parallel "$(nproc)" && ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_sharding_transaction" --output-on-failure --parallel 1 --timeout 180`
- Wave A `search -> index -> tensor -> graph -> llm` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_search_index_tensor_graph_llm_benchmarks --parallel "$(nproc)" && ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_search_index_tensor_graph_llm" --output-on-failure --parallel 1 --timeout 180`
- Wave A `server -> query -> storage -> transaction` long-run evidence: `cmake --build build-community-release --target themis_wave_a_server_query_storage_transaction_longrun_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_a_longrun_server_query_storage_transaction" --output-on-failure --parallel 1 --timeout 7200`
- Wave A `sharding <-> transaction` long-run evidence: `cmake --build build-community-release --target themis_wave_a_sharding_transaction_longrun_tests --parallel "$(nproc)" && THEMIS_SOAK_DURATION_MS=120000 ctest --test-dir build-community-release --label-regex "wave_a_longrun_sharding_transaction" --output-on-failure --parallel 1 --timeout 7200`
- Wave A `server <-> llm` long-run evidence: `cmake --build build-community-release --target themis_wave_a_server_llm_longrun_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_a_longrun_server_llm" --output-on-failure --parallel 1 --timeout 7200`
- Wave A `search -> index -> tensor -> graph -> llm` long-run evidence: `cmake --build build-community-release --target themis_wave_a_search_index_tensor_graph_llm_longrun_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_a_longrun_search_index_tensor_graph_llm" --output-on-failure --parallel 1 --timeout 7200`
- CI execution lane: `.github/workflows/build-benchmarks.yml` job `wave-a-flow-benchmark-gates`
- CI long-run lane: `.github/workflows/build-benchmarks.yml` job `wave-a-longrun-evidence`
- Pipeline inventory anchor: `ctest --test-dir build-community-release --label-regex "pipeline_integration" --output-on-failure`
- Benchmark baseline: `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

### Exit criteria

- Every critical flow has one documented start suite, flow suite, recovery/chaos suite, stress/soak suite, benchmark/perf suite, and gate owner.
- Every Wave A module has direct suite ownership plus a documented flow attachment.
- `release_critical` coverage for Wave A modules is explicit rather than implied.
- Slow soak / long-run evidence is attached through dedicated Wave A labels and aggregate targets, not mixed into the fast release-critical lanes.
- Canonical benchmark anchors are buildable through one flow-specific aggregate target and runnable through one flow-specific benchmark CTest label.

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

### Canonical execution blocks

- Wave B full boundary rerun: `cmake --build build-community-release --target themis_wave_b_boundary_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_b" --output-on-failure --parallel 1 --timeout 180`
- Wave B API / transport block: `cmake --build build-community-release --target themis_wave_b_api_transport_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_b_boundary_api_transport" --output-on-failure --parallel 1 --timeout 180`
- Wave B policy / control block: `cmake --build build-community-release --target themis_wave_b_policy_control_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_b_boundary_policy_control" --output-on-failure --parallel 1 --timeout 180`
- Wave B content / I/O block: `cmake --build build-community-release --target themis_wave_b_content_io_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_b_boundary_content_io" --output-on-failure --parallel 1 --timeout 180`
- Wave B runtime / ops block: `cmake --build build-community-release --target themis_wave_b_runtime_ops_tests --parallel "$(nproc)" && ctest --test-dir build-community-release --label-regex "wave_b_boundary_runtime_ops" --output-on-failure --parallel 1 --timeout 180`

### Current source-backed anchor policy

- `api`, `network`, and `rpc_grpc` use contract/integration suites as the first explicit API/transport rerun block.
- `auth`, `governance`, and `plugins` use policy, contract, and edition-boundary suites as the first explicit policy/control rerun block.
- `importers`, `exporters`, and `content` use contract and hardening suites as the first explicit content/I/O rerun block.
- `scheduler` and `observability` use contract plus runtime-integration suites as the first explicit runtime/ops rerun block.
- Known quarantined suites remain excluded until their API/assertion drift is repaired, notably `tests/governance/test_operational_audit_evidence.cpp`, `tests/governance/test_policy_versioning_and_approval.cpp`, `tests/importers/test_importers_phase2b_exception_safety_focused.cpp`, `tests/importers/test_importers_phase2_phase3_integration_focused.cpp`, and `tests/importers/test_importers_phase2a_data_race_focused.cpp`.

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
