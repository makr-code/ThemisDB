# Benchmark module regrouping plan

Scope: move the next obvious root benchmark targets into module-local folders and register them via benchmarks/<module>/CMakeLists.txt.

Files:
- move: bench_aql_functions.cpp -> benchmarks/aql/
- move: bench_query.cpp -> benchmarks/query/
- move: bench_transaction_throughput.cpp, bench_branch_manager.cpp -> benchmarks/transaction/
- move: bench_core_performance.cpp -> benchmarks/core/
- move: bench_ai_plugin_generator.cpp -> benchmarks/ai/
- move: bench_diff_engine.cpp -> benchmarks/analytics/
- move: bench_delegate_evaluator.cpp -> benchmarks/rag/
- update: benchmarks/CMakeLists.txt
- update: benchmarks/README.md, benchmarks/INDEX.md

Acceptance:
- audit_benchmark_registration.py still reports 0 unregistered benchmarks
- moved targets still build-register through module CMakeLists
- docs mention module-local layout for these modules
