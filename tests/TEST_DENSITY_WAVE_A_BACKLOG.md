# Wave A Test Density Backlog (`tests/`)

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
**Status:** active

## Purpose

This file is the canonical Wave A execution backlog for test-density closure.
It translates the Wave A scope from `TEST_DENSITY_WAVE_PLAN.md` into module-by-module traceability anchors and immediate closure work.

## Canonical command anchors

- Configure release-critical test graph: `cmake --preset community-release -DTHEMIS_BUILD_TESTS=ON`
- Build release-critical aggregate: `cmake --build build-community-release --target themis_release_critical_tests --parallel "$(nproc)"`
- Run release-critical suites: `ctest --test-dir build-community-release --label-regex "release_critical" --output-on-failure --parallel 1 --timeout 120`
- Build Wave A `server <-> llm` gate: `cmake --build build-community-release --target themis_wave_a_server_llm_tests --parallel "$(nproc)"`
- Run Wave A `server <-> llm` gate: `ctest --test-dir build-community-release --label-regex "wave_a_flow_server_llm" --output-on-failure --parallel 1 --timeout 120`
- Build Wave A `server -> query -> storage -> transaction` gate: `cmake --build build-community-release --target themis_wave_a_server_query_storage_transaction_tests --parallel "$(nproc)"`
- Run Wave A `server -> query -> storage -> transaction` gate: `ctest --test-dir build-community-release --label-regex "wave_a_flow_server_query_storage_transaction" --output-on-failure --parallel 1 --timeout 120`
- Build Wave A `sharding <-> transaction` gate: `cmake --build build-community-release --target themis_wave_a_sharding_transaction_tests --parallel "$(nproc)"`
- Run Wave A `sharding <-> transaction` gate: `ctest --test-dir build-community-release --label-regex "wave_a_flow_sharding_transaction" --output-on-failure --parallel 1 --timeout 120`
- Configure benchmark smoke graph: `cmake --preset community-debug`
- Build Wave A `server <-> llm` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_server_llm_benchmarks --parallel "$(nproc)"`
- Run Wave A `server <-> llm` benchmark gate: `ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_server_llm" --output-on-failure --parallel 1 --timeout 180`
- Build Wave A `server -> query -> storage -> transaction` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_server_query_storage_transaction_benchmarks --parallel "$(nproc)"`
- Run Wave A `server -> query -> storage -> transaction` benchmark gate: `ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_server_query_storage_transaction" --output-on-failure --parallel 1 --timeout 180`
- Build Wave A `sharding <-> transaction` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_sharding_transaction_benchmarks --parallel "$(nproc)"`
- Run Wave A `sharding <-> transaction` benchmark gate: `ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_sharding_transaction" --output-on-failure --parallel 1 --timeout 180`
- Build Wave A `search -> index -> tensor -> graph -> llm` benchmark gate: `cmake --build build-community-debug --target themis_wave_a_search_index_tensor_graph_llm_benchmarks --parallel "$(nproc)"`
- Run Wave A `search -> index -> tensor -> graph -> llm` benchmark gate: `ctest --test-dir build-community-debug --label-regex "wave_a_benchmark_search_index_tensor_graph_llm" --output-on-failure --parallel 1 --timeout 180`
- Build Wave A `server -> query -> storage -> transaction` long-run lane: `cmake --build build-community-release --target themis_wave_a_server_query_storage_transaction_longrun_tests --parallel "$(nproc)"`
- Run Wave A `server -> query -> storage -> transaction` long-run lane: `ctest --test-dir build-community-release --label-regex "wave_a_longrun_server_query_storage_transaction" --output-on-failure --parallel 1 --timeout 7200`
- Build Wave A `sharding <-> transaction` long-run lane: `cmake --build build-community-release --target themis_wave_a_sharding_transaction_longrun_tests --parallel "$(nproc)"`
- Run Wave A `sharding <-> transaction` long-run lane: `THEMIS_SOAK_DURATION_MS=120000 ctest --test-dir build-community-release --label-regex "wave_a_longrun_sharding_transaction" --output-on-failure --parallel 1 --timeout 7200`
- Build Wave A `server <-> llm` long-run lane: `cmake --build build-community-release --target themis_wave_a_server_llm_longrun_tests --parallel "$(nproc)"`
- Run Wave A `server <-> llm` long-run lane: `ctest --test-dir build-community-release --label-regex "wave_a_longrun_server_llm" --output-on-failure --parallel 1 --timeout 7200`
- Build Wave A `search -> index -> tensor -> graph -> llm` long-run lane: `cmake --build build-community-release --target themis_wave_a_search_index_tensor_graph_llm_longrun_tests --parallel "$(nproc)"`
- Run Wave A `search -> index -> tensor -> graph -> llm` long-run lane: `ctest --test-dir build-community-release --label-regex "wave_a_longrun_search_index_tensor_graph_llm" --output-on-failure --parallel 1 --timeout 7200`
- CI benchmark lane: `.github/workflows/build-benchmarks.yml` job `wave-a-flow-benchmark-gates`
- CI long-run lane: `.github/workflows/build-benchmarks.yml` job `wave-a-longrun-evidence`
- Pipeline inventory: `ctest --test-dir build-community-release --label-regex "pipeline_integration" --output-on-failure`
- Benchmark build baseline: `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

## Dependency-complete CI validation anchor

- Release-critical gate workflow: `.github/workflows/gate-pr-core.yml`
- Source anchor:
  - configure `community-release` with tests enabled
  - build `themis_release_critical_tests`
  - run `ctest --label-regex "release_critical"`
- Current local limitation:
  - this sandbox still lacks required `rocksdb` and `fmt` system dependencies for end-to-end community configure

## Primary Wave A modules

| Module | Direct owner suite anchor | Direct benchmark / perf anchor | Flow attachment | Wave A gap before this batch | Immediate closure action |
|---|---|---|---|---|---|
| `query` | `tests/query/test_query_engine.cpp`, `tests/query/test_continuous_query_engine.cpp`, `tests/query/test_continuous_query_e2e.cpp` | `benchmarks/query/bench_query.cpp`, `benchmarks/query/bench_phase4_performance.cpp` | `server -> query -> storage -> transaction` | direct suites existed, but no explicit module-owned `release_critical` anchor | Use `QueryEngineFocusedTests`, `ContinuousQueryEngineTests`, and `ContinuousQueryE2ETests` as Wave A release anchors |
| `index` | `tests/index/test_distributed_vector_index.cpp`, `tests/index/test_wave5_index_hardening.cpp` | `benchmarks/index/bench_index_rebuild.cpp` | `search -> index -> tensor -> graph -> llm` | direct suites existed, but no explicit module-owned `release_critical` anchor | Use distributed-vector + Wave 5 hardening as Wave A release anchors |
| `rag` | `tests/rag/test_wave7_bm25_positional_fts.cpp`, `tests/rag/test_wave7_rag_costmodel_guardrail.cpp` | `benchmarks/rag/bench_fts_phase_b.cpp`, `benchmarks/rag/bench_rag_hybrid_retriever.cpp` | `search -> index -> tensor -> graph -> llm` | flow evidence existed but remained fragmented across RAG and search | Keep the two existing Wave B `release_critical` suites as the current strongest Wave A closure anchors |
| `transaction` | `tests/transaction/test_transaction_distributed_phase2.cpp`, `tests/transaction/test_transaction_fault_injection_phase3.cpp`, `tests/transaction/test_transaction_wave_a_closure.cpp` | `benchmarks/transaction/bench_transaction_throughput.cpp`, `benchmarks/transaction/bench_transaction_phase4.cpp` | `server -> query -> storage -> transaction`, `sharding <-> transaction` | release-critical evidence existed but needs canonical wave-driver linkage | Keep Phase 2/3 and Wave A closure suites as canonical release anchors |
| `llm_wiki` | `tests/llm_wiki/test_llm_wiki_core_focused.cpp`, `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp` | `benchmarks/llm_wiki/bench_wikipedia_throughput.cpp` | `server <-> llm` via wiki-backed orchestration | minimal direct suites and no explicit module-owned `release_critical` anchor | Use core + llm-integration focused tests as the first module-owned Wave A release anchors |
| `search` | `tests/search/test_search_integration_phase4.cpp`, `tests/search/test_layered_retrieval_integration_phase4.cpp` | `benchmarks/search/bench_search_release_gates.cpp`, `benchmarks/search/bench_layered_retrieval_phase5.cpp` | `search -> index -> tensor -> graph -> llm` | broad evidence existed, but no explicit module-owned `release_critical` anchor | Use integration + layered retrieval phase 4 suites as Wave A release anchors |

## Secondary Wave A hub modules

| Module | Strongest release anchor | Benchmark / perf anchor | Flow attachment | Remaining gap |
|---|---|---|---|---|
| `server` | `tests/server/test_server_gateway_resilience_focused.cpp`, `tests/server/test_wave7_server_llm_hardening.cpp`, `tests/server/test_server_openapi_drift_focused.cpp` | `benchmarks/server/bench_server_http3_gates.cpp`, `benchmarks/server/bench_server_hotpaths.cpp` | `server -> query -> storage -> transaction`, `server <-> llm` | many anchors exist, but one canonical ingress-to-response path is still scattered |
| `sharding` | `tests/sharding/test_sharding_wave1_critical_closure.cpp`, `tests/sharding/test_sharding_contract_hardening_focused.cpp`, `tests/sharding/test_sharding_multishard_exact.cpp` | `benchmarks/sharding/bench_sharding_release_gates.cpp`, `benchmarks/sharding/WAVE_A_BASELINE_METRICS.json` | `sharding <-> transaction` | multiple recovery paths exist, but 2PC/WAL ownership still spans several suites |
| `llm` | `tests/llm/test_llm_phase1_hardening.cpp`, `tests/llm/test_llm_memory_safety_hardening.cpp`, `tests/llm/test_llm_wiki_phase_b_integration.cpp` | `benchmarks/llm/bench_llm_inference_performance.cpp`, `benchmarks/llm/bench_llm_hotpaths.cpp` | `search -> index -> tensor -> graph -> llm`, `server <-> llm` | protocol-to-inference round-trip still lacks one consolidated gate |
| `storage` | `tests/storage/test_storage_contract_hardening_focused.cpp`, `tests/storage/test_storage_phase3_error_handling_focused.cpp`, `tests/storage/test_mvcc_chain_pruner.cpp` | `benchmarks/storage/bench_storage_release_gates.cpp`, `benchmarks/storage/bench_mvcc.cpp` | `server -> query -> storage -> transaction` | direct suites existed, but no explicit module-owned `release_critical` anchor before this batch |

## Canonical Wave A flow anchors

| Flow | Focused / unit anchor | Integration / pipeline anchor | Chaos / soak anchor | Benchmark / perf anchor | Remaining gap |
|---|---|---|---|---|---|
| `server -> query -> storage -> transaction` | `tests/server/test_server_gateway_resilience_focused.cpp`, `tests/query/test_query_engine.cpp`, `tests/storage/test_storage_contract_hardening_focused.cpp`, `tests/transaction/test_transaction_wave_a_closure.cpp` | `tests/integration/pipeline/query_execution_pipeline_test.cpp`, `tests/integration/end_to_end/storage_pipeline_e2e_test.cpp`, `tests/integration/pipeline/w5a_e2e_critical_journeys_test.cpp` | `tests/integration/pipeline/w9c_chaos_fault_tolerance_test.cpp`, `tests/integration/pipeline/w6b_stress_soak_stability_test.cpp`, `tests/integration/pipeline/w7c_endurance_stability_test.cpp` | `benchmarks/query/bench_phase4_performance.cpp`, `benchmarks/storage/bench_storage_release_gates.cpp`, `benchmarks/transaction/bench_transaction_phase4.cpp` | focused + pipeline + e2e + chaos anchors now feed the Wave A aggregate gate; benchmark and long-run evidence now each have explicit aggregate build/ctest entrypoints, while dependency-complete execution is pending |
| `sharding <-> transaction` | `tests/sharding/test_sharding_wave1_critical_closure.cpp`, `tests/transaction/test_transaction_distributed_phase2.cpp` | `tests/integration/pipeline/transaction_replication_pipeline_test.cpp` | `tests/sharding/test_converged_chaos.cpp`, `tests/integration/pipeline/w9c_chaos_fault_tolerance_test.cpp`, `tests/integration/test_replication_soak_60min.cpp`, `tests/integration/test_sharding_distributed_write_soak.cpp` | `benchmarks/sharding/bench_sharding_release_gates.cpp`, `benchmarks/transaction/bench_transaction_throughput.cpp` | focused + pipeline + chaos release-critical sign-off wiring is explicit, and both benchmark and long-run soak evidence now have dedicated Wave A aggregate entrypoints |
| `server <-> llm` | `tests/server/test_wave7_server_llm_hardening.cpp`, `tests/llm/test_streaming_handler.cpp`, `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp` | `tests/llm/test_llm_multi_model_integration.cpp`, `tests/integration/pipeline/rag_ai_pipeline_test.cpp` | `tests/integration/voice/test_voice_endurance_stress.cpp` | `benchmarks/server/bench_server_http3_gates.cpp`, `benchmarks/llm/bench_llm_inference_performance.cpp` | server/llm benchmark and long-run evidence now each have explicit aggregate entrypoints; stronger true soak evidence remains desirable beyond the current voice endurance anchor |
| `search -> index -> tensor -> graph -> llm` | `tests/search/test_search_integration_phase4.cpp`, `tests/index/test_distributed_vector_index.cpp`, `tests/tensor/test_tensor_stress_suite_focused.cpp`, `tests/graph/test_graph_distributed.cpp`, `tests/llm/test_llm_phase5_hardening.cpp` | `tests/search/test_search_distributed_merge_stress.cpp` | `tests/tensor/test_tensor_stress_suite_focused.cpp`, `tests/graph/test_graph_distributed.cpp` (`DistributedGraphSharedMutexFocusedTests`) | `benchmarks/search/bench_layered_retrieval_phase5.cpp`, `benchmarks/rag/bench_fts_phase_b.cpp`, `benchmarks/llm/bench_llm_inference_performance.cpp` | benchmark and long-run stress evidence now have explicit aggregate entrypoints; end-to-end soak remains thinner than the other Wave A flows |

## First implementation batch status

- `query`: direct Wave A `release_critical` anchor added
- `index`: direct Wave A `release_critical` anchors added
- `search`: direct Wave A `release_critical` anchors added
- `storage`: direct Wave A `release_critical` anchors added
- `llm_wiki`: direct Wave A `release_critical` anchors added

## Next Wave A execution steps

1. Validate the new `release_critical` registrations in a test-enabled community configure.
2. Validate the new `themis_wave_a_server_llm_tests`, `themis_wave_a_server_query_storage_transaction_tests`, and `themis_wave_a_sharding_transaction_tests` targets in a dependency-complete environment.
3. Execute the new CI benchmark lane in `.github/workflows/build-benchmarks.yml` for `themis_wave_a_server_llm_benchmarks`, `themis_wave_a_server_query_storage_transaction_benchmarks`, `themis_wave_a_sharding_transaction_benchmarks`, and `themis_wave_a_search_index_tensor_graph_llm_benchmarks`.
4. Execute the new long-run lane for `themis_wave_a_server_query_storage_transaction_longrun_tests` and `themis_wave_a_sharding_transaction_longrun_tests`.
5. Continue Wave B boundary/API/policy densification after dependency-complete execution of the four benchmark lanes and four long-run Wave A lanes.
6. Reclassify the remaining indirect tensor/graph flow evidence as direct owner coverage or explicit accepted-indirect evidence.
