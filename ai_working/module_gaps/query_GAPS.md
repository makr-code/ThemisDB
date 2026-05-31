# query Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: query
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 1576
- Actionable Findings (Critical + High): 764
- Affected Files: 50

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 207 |
| High | 557 |
| Medium | 812 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 440 |
| performance_patterns | 315 |
| reliability | 282 |
| concurrency | 105 |
| security | 78 |
| performance | 74 |
| memory | 68 |
| determinism | 49 |
| observability | 48 |
| exception_safety | 28 |
| llm_ai_safety | 24 |
| legacy_duplication | 19 |
| type_conversion | 15 |
| platform | 9 |
| audit_logging | 7 |
| input_validation | 7 |
| raii | 6 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/query/query_engine.cpp | 261 | 31 | 78 | 152 | 0 |
| src/query/aql_translator.cpp | 128 | 72 | 21 | 35 | 0 |
| src/query/let_evaluator.cpp | 69 | 4 | 44 | 21 | 0 |
| src/query/functions/fulltext_functions.cpp | 68 | 2 | 23 | 43 | 0 |
| src/query/tensor_contraction_engine.cpp | 67 | 1 | 24 | 42 | 0 |
| src/query/adaptive_join.cpp | 58 | 9 | 22 | 27 | 0 |
| src/query/gremlin_parser.cpp | 57 | 2 | 12 | 43 | 0 |
| src/query/query_federation.cpp | 56 | 4 | 19 | 33 | 0 |
| src/query/functions/process_mining_functions.cpp | 53 | 0 | 24 | 29 | 0 |
| src/query/aql_parser.cpp | 50 | 1 | 28 | 21 | 0 |
| src/query/cypher_parser.cpp | 49 | 0 | 12 | 37 | 0 |
| src/query/functions/tensor_functions.cpp | 48 | 1 | 38 | 9 | 0 |
| src/query/aql_runner.cpp | 39 | 0 | 5 | 34 | 0 |
| src/query/functions/lora_functions.cpp | 37 | 2 | 14 | 20 | 1 |
| src/query/cross_cluster_federation.cpp | 32 | 2 | 10 | 20 | 0 |
| src/query/sparql_parser.cpp | 32 | 4 | 4 | 24 | 0 |
| src/query/functions/ethics_functions.cpp | 28 | 3 | 20 | 5 | 0 |
| src/query/optimizer_cost_model.cpp | 27 | 12 | 14 | 1 | 0 |
| src/query/window_evaluator.cpp | 27 | 0 | 5 | 22 | 0 |
| src/query/cte_subquery.cpp | 26 | 1 | 14 | 11 | 0 |
| src/query/materialized_view.cpp | 26 | 3 | 11 | 12 | 0 |
| src/query/parallel_executor.cpp | 25 | 4 | 4 | 17 | 0 |
| src/query/query_optimizer.cpp | 25 | 0 | 13 | 10 | 2 |
| src/query/functions/udf_registry.cpp | 24 | 1 | 17 | 6 | 0 |
| src/query/materialized_cte.cpp | 20 | 1 | 8 | 11 | 0 |
| src/query/continuous_query_engine.cpp | 19 | 2 | 9 | 8 | 0 |
| src/query/query_rewrite_rule.cpp | 19 | 3 | 3 | 13 | 0 |
| src/query/sql_parser.cpp | 19 | 0 | 2 | 17 | 0 |
| src/query/plan_cache.cpp | 17 | 5 | 6 | 6 | 0 |
| src/query/query_cache.cpp | 17 | 8 | 4 | 5 | 0 |
| src/query/query_plan_visualizer.cpp | 16 | 2 | 0 | 14 | 0 |
| src/query/result_type_annotation.cpp | 16 | 1 | 3 | 12 | 0 |
| src/query/query_cache_manager.cpp | 15 | 14 | 1 | 0 | 0 |
| src/query/vectorized_execution.cpp | 15 | 0 | 7 | 8 | 0 |
| src/query/approximate_aggregator.cpp | 11 | 1 | 4 | 5 | 1 |
| src/query/adaptive_optimizer.cpp | 10 | 1 | 4 | 5 | 0 |
| src/query/query_compiler.cpp | 8 | 2 | 5 | 1 | 0 |
| src/query/tensor_aware_query_optimizer.cpp | 8 | 4 | 2 | 2 | 0 |
| src/query/workload_cache_strategy.cpp | 8 | 0 | 6 | 2 | 0 |
| src/query/cte_cache.cpp | 7 | 0 | 2 | 5 | 0 |
| src/query/semantic_cache.cpp | 7 | 2 | 2 | 3 | 0 |
| src/query/aql_parser_json.cpp | 6 | 0 | 2 | 4 | 0 |
| src/query/aql_safety_validator.cpp | 4 | 0 | 1 | 3 | 0 |
| src/query/continuous_query_planner.cpp | 4 | 0 | 0 | 4 | 0 |
| src/query/query_canceller.cpp | 4 | 2 | 0 | 2 | 0 |
| src/query/result_stream.cpp | 4 | 0 | 3 | 1 | 0 |
| src/query/statistical_aggregator.cpp | 4 | 0 | 2 | 2 | 0 |
| src/query/synopsis_store.cpp | 3 | 0 | 2 | 1 | 0 |
| src/query/cq_watermark.cpp | 2 | 0 | 2 | 0 | 0 |
| src/query/query_profiler.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/query/query_engine.cpp
Total findings: 261

- Line 602: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 825: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 935: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 997: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1066: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1130: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1267: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1316: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cached = cte_cache->get(name);
- Line 2389: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 2738: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto checkFieldAccess = [](const std::shared_ptr<query::Expression>& e) -> std::pair<std::string, st
- Line 2742: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto varExpr = std::static_pointer_cast<query::VariableExpr>(fa->object);
- Line 2791: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: initial_context.cte_cache = parent_context->cte_cache;
- Line 2983: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bin = std::static_pointer_cast<query::BinaryOpExpr>(filter->condition);
- Line 2988: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->left);
- Line 2989: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->right);
- Line 2992: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lvar = std::static_pointer_cast<query::VariableExpr>(lfa->object)->name;
- Line 2993: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rvar = std::static_pointer_cast<query::VariableExpr>(rfa->object)->name;
- Line 3236: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t offset = (limit->offset <= 0) ? 0 : static_cast<size_t>(limit->offset);
- Line 3237: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t count  = (limit->count  <= 0) ? 0 : static_cast<size_t>(limit->count);
- Line 3693: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg3.wait();
- Line 3995: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto xLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[0]);
- Line 3996: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto yLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[1]);
- Line 4114: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4156: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4187: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4353: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 4508: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: vgr.entity = cached->second;
- Line 4557: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg2.wait();
- Line 4581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: r.entity = cached->second;
- Line 4648: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator entity may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for(size_t bi=0; bi<buckets.size(); ++bi){ tg.run([&,bi](){ size_t start=bi*CHUNK; size_t end=std::m
- Line 4649: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait(); for(auto &b : buckets){ results.insert(results.end(), std::make_move_iterator(b.begin()),
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t// Guard against negative int64_t values: cast to size_t only after clamping.', '\t\tsize_t offset = (limit->offset <= 0) ? 0 : static_cast<size_t>(limit->offset);', '\t\tsize_t count  = (limit->count  <= 0) ? 0 : static_cast<size_t>(limit->count);', '\t\tif (offset >= results.size()) {', '\t\t\tresults.clear();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("QueryEngine: index_manager cannot be null");
- Line 119: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view /*value*/) {
- Line 581: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < q.predicates.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 590: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(errors_mutex);
- Line 804: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
  Confidence: band=very_high; score=0.9
- Line 811: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 913: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < q.disjuncts.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 922: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(errors_mutex);
- Line 976: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < q.disjuncts.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 984: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> eg(error_mutex);
- Line 1047: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
  Confidence: band=very_high; score=0.9
- Line 1053: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1059: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1059: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1109: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
  Confidence: band=very_high; score=0.9
- Line 1116: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1122: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1246: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
  Confidence: band=very_high; score=0.9
- Line 1253: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1259: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1349: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (current->is_object()) {
- Line 1351: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it == current->end()) return nullptr;
- Line 1356: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (idx < current->size()) current = &((*current)[idx]); else return nullptr;
- Line 1385: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (v->is_array() || v->is_object()) return Ok(nlohmann::json(v->size()));
- Line 2035: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
- Line 2087: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
- Line 2151: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto base = qe_evalExpr(fa->object, ctx);
- Line 2153: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (base->is_null()) return Ok(nlohmann::json(nullptr));
- Line 2170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: sorted_fields.reserve(obj->fields.size());
- Line 2171: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [k, e] : obj->fields) {
- Line 2205: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (d==0.0) return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Division by zero");
  Confidence: band=very_high; score=0.9
- Line 2210: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (d==0.0) return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Modulo by zero");
  Confidence: band=very_high; score=0.9
- Line 2286: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (pos_a == a.size() && pos_b == b.size()) {
  Confidence: band=very_high; score=0.9
- Line 2298: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (pos_a == a.size() && pos_b == b.size()) {
  Confidence: band=very_high; score=0.9
- Line 2337: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
- Line 2539: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sel_a == sel_b) {
  Confidence: band=very_high; score=0.9
- Line 2622: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!candSet.empty() && candSet.find(k) == candSet.end()) continue; // filter
- Line 2622: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!candSet.empty() && candSet.find(k) == candSet.end()) continue; // filter
- Line 2673: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: collectVariables(fa->object, vars);
- Line 2703: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, val] : obj->fields) {
- Line 2738: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto checkFieldAccess = [](const std::shared_ptr<query::Expression>& e) -> std::pair<std::string, st
- Line 2741: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fa->object->getType() != query::ASTNodeType::Variable) return {"", ""};
- Line 2742: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto varExpr = std::static_pointer_cast<query::VariableExpr>(fa->object);
- Line 2874: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(build_prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 2990: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (lfa->object->getType() != query::ASTNodeType::Variable) return false;
- Line 2991: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (rfa->object->getType() != query::ASTNodeType::Variable) return false;
- Line 2992: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto lvar = std::static_pointer_cast<query::VariableExpr>(lfa->object)->name;
- Line 2993: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto rvar = std::static_pointer_cast<query::VariableExpr>(rfa->object)->name;
- Line 3037: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(probe_prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 3163: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 3275: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 3565: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto slashPos = vertexPk.find('/');
- Line 3657: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto slashPos = vertexPk.find('/');
- Line 4060: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (eqPrefilter && !eqPrefilter->empty()) {
- Line 4062: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (eqPrefilter->size() < stats.entry_count * 0.05) return VGPlan::VectorThenSpatial;
- Line 4101: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto *var = dynamic_cast<query::VariableExpr*>(fa->object.get()); if (!var) continue;
- Line 4141: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: std::string tmp = st.column; size_t pos=0; while(true){ size_t n = tmp.find('+', pos); if(n==std::st
- Line 4145: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: for (auto &c : cols) { auto it = equalityMap.find(c); if (it==equalityMap.end()) { all=false; break;
- Line 4175: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto itRange = rangeMap.find(column);
- Line 4194: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: span.setAttribute("index_prefilter_size", static_cast<int64_t>(indexPrefilter->size()));
- Line 4195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (indexPrefilter->empty()) { span.setAttribute("result_count", static_cast<int64_t>(0)); span.setS
- Line 4211: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, vr] = vectorIdx_->searchKnn(q.query_vector, k, indexPrefilter ? &*indexPrefilter : nullptr
- Line 4234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
- Line 4261: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i=0;i<std::min(tmp.size(),k);++i) {
- Line 4297: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ci.prefilterSize = indexPrefilter ? indexPrefilter->size() : 0; ci.k = q.k; ci.vectorDim = q.query_v
- Line 4311: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, vr] = vectorIdx_->searchKnn(q.query_vector, overfetch, indexPrefilter ? &*indexPrefilter :
- Line 4311: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [st, vr] = vectorIdx_->searchKnn(q.query_vector, overfetch, indexPrefilter ? &*indexPrefilter :
- Line 4422: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
- Line 4460: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
- Line 4538: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = entityCache.find(pk);
- Line 4538: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = entityCache.find(pk);
- Line 4539: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = entityCache.find(pk);
  Confidence: band=very_high; score=0.9
- Line 4648: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for(size_t bi=0; bi<buckets.size(); ++bi){ tg.run([&,bi](){ size_t start=bi*CHUNK; size_t end=std::m
- Line 4698: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sa == sb) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 114: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 184: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 224: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: phraseKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fuzzyKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fulltextKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialKeys.emplace_back(res.primary_key);
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(st.message);
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back(st.message);
- Line 654: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto scoreMap = std::make_shared<std::unordered_map<std::string, double>>();
  Confidence: band=medium; score=0.66
- Line 659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fulltextKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 702: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto filteredScores = std::make_shared<std::unordered_map<std::string, double>>();
  Confidence: band=medium; score=0.66
- Line 765: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 794: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
- Line 814: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 816: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 818: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_deserialize_pks.push_back(pk);
- Line 922: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(result.error().context());
  Confidence: band=high; score=0.74
- Line 923: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back(result.error().context());
- Line 984: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(result.error().message());
  Confidence: band=high; score=0.74
- Line 985: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back(result.error().message());
- Line 1039: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeOrEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}"
- Line 1056: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1058: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1060: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_deserialize_pks.push_back(pk);
- Line 1100: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeOrEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
- Line 1119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1121: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_deserialize_pks.push_back(pk);
- Line 1236: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesSequential: Deserialisierung fehlgeschlagen für PK={}",
- Line 1256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_deserialize_pks.push_back(pk);
- Line 1333: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stod(v.get<std::string>()); } catch (...) { return 0.0; }
- Line 1357: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return nullptr; }
- Line 1639: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coords = g["coordinates"];
  Confidence: band=high; score=0.74
- Line 1844: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: std::string u = trim(wkt); std::string up=u; std::transform(up.begin(), up.end(), up.begin(), ::toup
- Line 1982: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (t=="LineString"||t=="MultiPoint") { nlohmann::json nc=nlohmann::json::array(); for (const a
- Line 1982: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: else if (t=="Polygon"||t=="MultiLineString") { nlohmann::json nr=nlohmann::json::array(); for (const auto& ring : g["coordinates"]) { nlohmann::json r=nlohmann::json::array(); for (const auto& pt : ring) r.push_back(strip2D(pt)); nr.push_back(r);} result["coordinates"]=nr; }
  Confidence: band=high; score=0.71
- Line 1983: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (t=="Polygon"||t=="MultiLineString") { nlohmann::json nr=nlohmann::json::array(); for (const
- Line 2162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.push_back(*elemRes);
- Line 2171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_fields.emplace_back(k, e);
  Confidence: band=high; score=0.74
- Line 2291: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2303: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matchesPredicates(e)) out.emplace_back(std::move(entry.pk));
  Confidence: band=high; score=0.74
- Line 2356: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2380: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matchesPredicates(e)) local.emplace_back(std::move(entry.pk));
  Confidence: band=high; score=0.74
- Line 2382: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 2506: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}
- Line 2560: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lists.emplace_back(std::move(keys));
  Confidence: band=high; score=0.74
- Line 2580: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lists.emplace_back(std::move(keys));
  Confidence: band=high; score=0.74
- Line 2607: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candSet;
  Confidence: band=medium; score=0.66
- Line 2623: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.emplace_back(std::move(k));
  Confidence: band=high; score=0.74
- Line 2646: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 2648: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesRangeAware_: Deserialisierung fehlgeschlagen für PK={}"
- Line 2806: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::shared_ptr<query::FilterNode>>> single_var_filters;
  Confidence: band=high; score=0.74
- Line 2814: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: single_var_filters[*vars.begin()].push_back(filter);
  Confidence: band=high; score=0.74
- Line 2815: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: single_var_filters[*vars.begin()].push_back(filter);
- Line 2819: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: multi_var_filters.push_back(filter);
- Line 2833: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 2863: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bucket_it->second.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2864: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bucket_it->second.push_back(doc);
- Line 2903: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bucket_it->second.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2904: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bucket_it->second.push_back(doc);
- Line 2906: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 3043: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_probe_docs.emplace_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 3045: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 3074: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_bindings.emplace_back(var, val);
  Confidence: band=high; score=0.74
- Line 3115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(*result_or_err));
  Confidence: band=high; score=0.74
- Line 3187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_scan_docs.emplace_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 3189: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3270: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> groups;
  Confidence: band=medium; score=0.66
- Line 3306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_it->second.push_back(doc);
  Confidence: band=high; score=0.74
- Line 3307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: group_it->second.push_back(doc);
- Line 3308: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sorted_group_keys.push_back(group_key);
- Line 3328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_docs.push_back(&doc);
  Confidence: band=high; score=0.74
- Line 3329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ordered_docs.push_back(&doc);
- Line 3424: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryEngine::executeRecursivePathQuery(const RecursivePathQuery& q) const {
  Confidence: band=high; score=0.74
- Line 3446: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> frontier{q.start_node};
  Confidence: band=medium; score=0.66
- Line 3448: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> next;
  Confidence: band=medium; score=0.66
- Line 3517: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: GraphIndexManager::Status st;
- Line 3571: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertexKeys.emplace_back(table + ":" + vertexPk);
  Confidence: band=high; score=0.74
- Line 3591: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3621: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: GraphIndexManager::Status st;
- Line 3663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertexKeys.emplace_back(table + ":" + vertexPk);
  Confidence: band=high; score=0.74
- Line 3686: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { continue; }
- Line 3687: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (evaluateCondition(sc.spatial_filter, ctx)) buf.push_back(vertexPk);
  Confidence: band=high; score=0.74
- Line 3688: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (evaluateCondition(sc.spatial_filter, ctx)) buf.push_back(vertexPk);
- Line 3729: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allPaths.emplace_back(std::move(pathResult.path));
  Confidence: band=high; score=0.74
- Line 3830: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4031: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4071: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryEngine::executeVectorGeoQuery(const VectorGeoQuery& q) const {
  Confidence: band=high; score=0.74
- Line 4090: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RangeAcc> rangeMap;
  Confidence: band=medium; score=0.66
- Line 4092: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> equalityMap;
  Confidence: band=medium; score=0.66
- Line 4115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while(it1!=current.end() && it2!=keys.end()) { if(*it1<*it2) ++it1; else if(*it2<*it1) ++it2; else {
- Line 4142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::string tmp = st.column; size_t pos=0; while(true){ size_t n = tmp.find('+', pos); if(n==std::st
- Line 4146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto &c : cols) { auto it = equalityMap.find(c); if (it==equalityMap.end()) { all=false; break;
- Line 4157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while(it1!=current.end() && it2!=keys.end()) { if(*it1<*it2) ++it1; else if(*it2<*it1) ++it2; else {
- Line 4162: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sortedRangeColumns.push_back(kv.first);
- Line 4188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while(it1!=current.end() && it2!=keys.end()) { if(*it1<*it2) ++it1; else if(*it2<*it1) ++it2; else {
- Line 4219: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: nlohmann::json doc; try { std::string s(blobs[i]->begin(), blobs[i]->end()); doc = nlohmann::json::p
- Line 4226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4241: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return true; }
- Line 4251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tmp.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 4270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4293: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4343: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { continue; }
- Line 4346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4379: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> entityCache;
  Confidence: band=medium; score=0.66
- Line 4407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(r.primary_key);
  Confidence: band=high; score=0.74
- Line 4407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(r.primary_key);
  Confidence: band=high; score=0.74
- Line 4408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: spatialCandidates.push_back(r.primary_key);
- Line 4410: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* skip */ }
- Line 4441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(pk);
  Confidence: band=high; score=0.74
- Line 4442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: spatialCandidates.push_back(pk);
- Line 4445: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4459: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> tmpCache;
  Confidence: band=medium; score=0.66
- Line 4472: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (spatialOK && extraOK) { spatialCandidates.push_back(pk); tmpCache.try_emplace(pk, std::move(entity)); }
  Confidence: band=high; score=0.74
- Line 4473: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (spatialOK && extraOK) { spatialCandidates.push_back(pk); tmpCache.try_emplace(pk, std::move(enti
- Line 4474: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* skip */ }
- Line 4508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(vgr));
  Confidence: band=high; score=0.74
- Line 4551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4595: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryEngine::executeContentGeoQuery(const ContentGeoQuery& q) const {
  Confidence: band=high; score=0.74
- Line 4643: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string,double> bm25; bm25.reserve(ftResults.size());
  Confidence: band=medium; score=0.66
- Line 4648: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: for(size_t bi=0; bi<buckets.size(); ++bi){ tg.run([&,bi](){ size_t start=bi*CHUNK; size_t end=std::m
- Line 4655: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string,nlohmann::json> cache;
  Confidence: band=medium; score=0.66
- Line 4665: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for(size_t i=0;i<indexResults.size();++i){ if(!blobs[i].has_value()) continue; try { auto entity = BaseEntity::deserialize(indexResults[i].primary_key, *blobs[i]); nlohmann::json doc = nlohmann::json::parse(entity.toJson()); spatialCandidates.emplace_back(indexResults[i].primary_key); cache.emplace(indexResults[i].primary_key, std::move(doc));} catch (...) {} }
  Confidence: band=high; score=0.71
- Line 4666: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: for(size_t i=0;i<indexResults.size();++i){ if(!blobs[i].has_value()) continue; try { auto entity = B
- Line 4677: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tokenSet(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 4683: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { if (doc[q.text_field].is_string()) text = doc[q.text_field].get<std::string>(); else continue;
- Line 4684: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto docTokens = SecondaryIndexManager::tokenize(text); std::unordered_set<std::string> docSet(docTokens.begin(), docTokens.end());
  Confidence: band=medium; score=0.66
- Line 4688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4688: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (q.boost_by_distance && q.center_point){ if (doc.contains(q.geom_field)){ nlohmann::json geom; if
- Line 4788: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cte_results.push_back(entity.toJson());
- Line 4804: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cte_results.push_back(entity.toJson());
  Confidence: band=high; score=0.74
- Line 4805: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cte_results.push_back(entity.toJson());
- Line 4831: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cte_results.push_back(result.entity);
- Line 4847: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cte_results.push_back(result.entity);
  Confidence: band=high; score=0.74
- Line 4848: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cte_results.push_back(result.entity);
- Line 4881: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.orderedPredicates.push_back(pred);
  Confidence: band=high; score=0.74
- Line 4882: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.orderedPredicates.push_back(pred);
- Line 4886: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.details.push_back(est);

### src/query/aql_translator.cpp
Total findings: 128

- Line 96: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto varName = std::static_pointer_cast<VariableExpr>(sortExpr)->name;
- Line 110: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(sim->arguments[1]);
- Line 132: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto kLit = std::static_pointer_cast<LiteralExpr>(sim->arguments[2]);
- Line 140: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 177: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (prox->arguments[0]->getType() != ASTNodeType::FieldAccess) {
- Line 180: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 184: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: std::string geomField = extractColumnName(prox->arguments[0]);
- Line 185: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 185: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 244: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 301: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* fn  = static_cast<FunctionCallExpr*>(bin->left.get());
- Line 302: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* rhs = static_cast<LiteralExpr*>(bin->right.get());
- Line 315: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto* fa0 = static_cast<FieldAccessExpr*>(fn->arguments[0].get());
- Line 316: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto* fa1 = static_cast<FieldAccessExpr*>(fn->arguments[1].get());
- Line 321: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string var0 = static_cast<VariableExpr*>(fa0->object.get())->name;
- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string var1 = static_cast<VariableExpr*>(fa1->object.get())->name;
- Line 409: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 447: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (args[0]->getType() != ASTNodeType::FieldAccess) {
- Line 450: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: if (args[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 454: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: std::string geomField = extractColumnName(args[0]);
- Line 455: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);
- Line 505: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 514: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 536: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 561: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 583: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto kLit = std::static_pointer_cast<LiteralExpr>(func->arguments[2]);
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 627: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (func->arguments[0]->getType() != ASTNodeType::FieldAccess) {
- Line 630: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 634: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: std::string geomField = extractColumnName(func->arguments[0]);
- Line 635: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 635: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 685: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 694: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 716: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 817: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto funcCall = std::static_pointer_cast<FunctionCallExpr>(filter->condition);
- Line 837: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 849: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 881: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto phraseLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 893: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 925: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 937: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 955: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[3]);
- Line 1016: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 1018: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
- Line 1099: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: findFulltext = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1123: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: findSpatial = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto binOp = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
- Line 1194: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[1]);
- Line 1205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[2]);
- Line 1278: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(spatialFunc->arguments[2]);
- Line 1280: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
- Line 1445: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto literal = std::static_pointer_cast<LiteralExpr>(binOp->right);
- Line 1570: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, limit->offset));
- Line 1571: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, limit->count));
- Line 1620: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
- Line 1621: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
- Line 1629: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
- Line 1630: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
- Line 1640: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1641: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1649: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto eqExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, binOp->left, binOp->right);
- Line 1655: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gte, binOp->left, binOp->right);
- Line 1661: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lte, binOp->left, binOp->right);
- Line 1667: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1673: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1759: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1760: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1806: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 1819: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 318: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fa0->object && fa0->object->getType() == ASTNodeType::Variable &&
- Line 319: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: fa1->object && fa1->object->getType() == ASTNodeType::Variable)
- Line 321: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string var0 = static_cast<VariableExpr*>(fa0->object.get())->name;
- Line 322: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string var1 = static_cast<VariableExpr*>(fa1->object.get())->name;
- Line 451: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
- Line 457: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon,
- Line 543: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: treat legacy FunctionCall nodes for SIMILARITY/PROXIMITY
  Confidence: band=high; score=0.8
- Line 631: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
- Line 637: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon,
- Line 763: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // If existing disjuncts = [A, B] and new disjuncts = [C, D], the result is
- Line 824: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("FULLTEXT() requires 2-3 arguments: FULLTEXT(column, query [, limit]
- Line 868: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PHRASE() requires 2-3 arguments: PHRASE(column, phrase [, limit])")
- Line 912: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("FUZZY() requires 2-4 arguments: FUZZY(column, query [, maxDistance]
- Line 1099: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: findFulltext = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1123: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: findSpatial = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1123: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: findSpatial = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1147: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: collectNonFulltext = [&](const std::shared_ptr<Expression>& e, std::vector<std::shared_ptr<Expressio
- Line 1520: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fieldAccess->object->getType() == ASTNodeType::FieldAccess) {
- Line 1521: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: result = extractColumnName(fieldAccess->object) + ".";
- Line 1897: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: count += countCTEReferencesInExpr(any->arrayExpr, cte_name);
- Line 1903: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: count += countCTEReferencesInExpr(all->arrayExpr, cte_name);
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cte_executions.push_back(std::move(exec));
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cte_executions.push_back(std::move(exec));
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVec.push_back(static_cast<float>(std::get<double>(lit->value)));
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extraPreds.push_back(cond);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: extraPreds.push_back(cond);
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: point.push_back(static_cast<float>(std::get<double>(lit->value)));
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVec.push_back(static_cast<float>(std::get<double>(lit->value)));
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extraPreds.push_back(cond);
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: extraPreds.push_back(cond);
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
- Line 471: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: point.push_back(static_cast<float>(std::get<double>(lit->value)));
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 570: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVec.push_back(static_cast<float>(std::get<double>(lit->value)));
- Line 609: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extraPreds.push_back(cond);
  Confidence: band=high; score=0.74
- Line 610: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: extraPreds.push_back(cond);
- Line 648: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 649: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
- Line 651: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: point.push_back(static_cast<float>(std::get<double>(lit->value)));
- Line 794: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(combined));
- Line 1040: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numericOnly.push_back(c);
  Confidence: band=high; score=0.74
- Line 1041: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: numericOnly.push_back(c);
- Line 1043: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: numericOnly.push_back(' ');
- Line 1155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preds.push_back(e); // Non-FULLTEXT, non-spatial function
- Line 1171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preds.push_back(e);
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numericOnly.push_back(c);
  Confidence: band=high; score=0.74
- Line 1302: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: numericOnly.push_back(c);
- Line 1304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: numericOnly.push_back(' ');

### src/query/let_evaluator.cpp
Total findings: 69

- Line 1071: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lpos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lpos = token.find_first_not_of(" \t\n\r");
- Line 1072: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rpos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rpos = token.find_last_not_of(" \t\n\r");
- Line 1111: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lpos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lpos = token.find_first_not_of(" \t\n\r");
- Line 1112: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rpos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rpos = token.find_last_not_of(" \t\n\r");
- Line 84: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: JSON literal wrapper from legacy tests
  Confidence: band=high; score=0.8
- Line 89: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: path-based field access (supports array indices)
  Confidence: band=high; score=0.8
- Line 127: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: binary op with string operator
  Confidence: band=high; score=0.8
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown legacy binary operator: " + op);
- Line 149: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: unary op with string operator
  Confidence: band=high; score=0.8
- Line 158: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown legacy unary operator: " + sunary->op);
- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Undefined variable: " + var->name);
- Line 178: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: function call shim with functionName + arguments
  Confidence: band=high; score=0.8
- Line 187: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, valExpr] : objConstr->fields) {
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown expression type in LET evaluator");
- Line 224: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto baseValue = evaluateExpression(fieldAccess->object, currentDoc);
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (baseValue.is_object() && baseValue.contains(fieldAccess->field)) {
- Line 231: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: numeric string treated as array index
  Confidence: band=high; score=0.8
- Line 306: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown binary operator");
- Line 328: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (rightNum == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 329: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Division by zero");
- Line 334: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (rightNum == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Modulo by zero");
- Line 340: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown arithmetic operator: " + op);
- Line 386: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown logical operator: " + op);
- Line 403: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown unary operator");
- Line 452: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy fallback for ST_* functions
  Confidence: band=high; score=0.8
- Line 453: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // These remain here for backward compatibility with custom EWKB parsing
  Confidence: band=high; score=0.8
- Line 558: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_Intersects expects 2 arguments: ST_Intersects(geom1, geom2)");
- Line 623: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::runtime_error("ST_Within: g1 must be a GeoJSON Point or [x,y] array");
- Line 647: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (cross == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 701: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return (px == g2j["coordinates"][0].get<double>()
  Confidence: band=very_high; score=0.9
- Line 702: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: && py == g2j["coordinates"][1].get<double>());
  Confidence: band=very_high; score=0.9
- Line 705: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_Within: g2 must be a GeoJSON Polygon, bbox array, or Point");
- Line 754: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_Contains expects 2 arguments: ST_Contains(geom1, geom2)");
- Line 882: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_Z expects 1 argument");
- Line 1033: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Invalid WKT POINT syntax");
- Line 1041: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Invalid POINT coordinates");
- Line 1059: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Invalid WKT LINESTRING syntax");
- Line 1078: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Invalid LINESTRING point");
- Line 1100: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Invalid WKT POLYGON syntax");
- Line 1118: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Invalid POLYGON point");
- Line 1139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_GeomFromText: Unsupported WKT type (only POINT, LINESTRING, POLYGON sup
- Line 1230: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_3DDistance expects 2 arguments: ST_3DDistance(geom1, geom2)");
- Line 1375: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ST_Buffer expects 2 arguments: ST_Buffer(geom, distance)");
- Line 1395: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: nlohmann::json poly; poly["type"] = "Polygon"; poly["coordinates"] = nlohmann::json::array({ring});
- Line 1401: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!rings.is_array() || rings.empty()) throw std::runtime_error("ST_Buffer: invalid Polygon");
- Line 1413: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
- Line 1452: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
- Line 36: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
- Line 241: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 268: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 426: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evaluatedArgs.push_back(evaluateExpression(arg, currentDoc));
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: evaluatedArgs.push_back(evaluateExpression(arg, currentDoc));
- Line 608: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 710: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1081: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords.push_back({x, y, z});
- Line 1083: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords.push_back({x, y});
- Line 1121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({x, y, z});
- Line 1123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({x, y});
- Line 1293: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newCoords.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1294: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: newCoords.push_back(strip2D(pt));
- Line 1303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newRing.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newRing.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: newRing.push_back(strip2D(pt));
- Line 1307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: newRings.push_back(newRing);
- Line 1488: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/functions/fulltext_functions.cpp
Total findings: 68

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        std::string s1 = args[0].get<std::string>();', '        std::string s2 = args[1].get<std::string>();', '        int n = args.size() > 2 ? args[2].get<int>() : 2;', '', '        if (s1.empty() || s2.empty()) return 0.0;']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['', '        std::string word = args[0].get<std::string>();', '        int maxLen = args.size() > 1 ? args[1].get<int>() : 6;', '', '        return metaphone(word, maxLen);']
  Confidence: band=very_high; score=0.93
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    for (size_t hi = 0; hi < positions.size(); ++hi) {', '        while (positions[hi] - positions[lo] >= windowSize) ++lo;', '        size_t count = hi - lo + 1;', '        if (count > bestCount) {', '            bestCount = count;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 211: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& t : tokenize(queryArg.get<std::string>()))
- Line 338: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 338: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 398: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 458: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 458: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 533: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 533: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 593: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 593: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 667: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 667: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 720: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 720: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 752: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 752: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 782: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 782: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string primary = mf.execute(args, ctx).get<std::string>();
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(current);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(current);
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(current);
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ngrams.push_back(s);
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ngrams.push_back(s.substr(i, n));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ngrams.push_back(s.substr(i, n));
- Line 105: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: while (result.length() < 4) result += '0';
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
- Line 144: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (next == 'H') { result += 'X'; i++; }
- Line 145: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (next == 'I' || next == 'E' || next == 'Y') result += 'S';
- Line 146: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else result += 'K';
- Line 149: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (next == 'G') { result += 'J'; i++; }
- Line 150: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else result += 'T';
- Line 152: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'F': result += 'F'; break;
- Line 155: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (next == 'I' || next == 'E' || next == 'Y') result += 'J';
- Line 156: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else result += 'K';
- Line 159: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i == 0 || !isVowel(upper[i - 1])) result += 'H';
- Line 161: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'J': result += 'J'; break;
- Line 200: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> queryTermSet(const json& queryArg) {
  Confidence: band=medium; score=0.66
- Line 201: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> terms;
  Confidence: band=medium; score=0.66
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (terms.count(lower.substr(i, end - i))) positions.push_back(i);
- Line 338: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back({{"_note", "FULLTEXT: no SecondaryIndexManager in context"},
- Line 364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_error", st.message}, {"_collection", collection},
- Line 368: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
  Confidence: band=high; score=0.74
- Line 369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
- Line 398: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 400: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
- Line 458: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 460: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
- Line 533: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 680: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> count1, count2;
  Confidence: band=medium; score=0.66
- Line 720: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 752: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 782: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 810: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74

### src/query/tensor_contraction_engine.cpp
Total findings: 67

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['            throw std::overflow_error("TT-core kron: core dimension product overflows size_t");', '        }', '        cr.data.resize(rl * n * rr, 0.0f);', '', '        // Kronecker product of core matrices for each physical index i']
  Confidence: band=very_high; score=0.9
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("TensorContractionEngine::slice: dim out of range");
- Line 72: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("TensorContractionEngine::slice: idx out of range");
- Line 121: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("TT-core merge: core dimension product overflows size_t");
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorContractionEngine::hadamardProduct: incompatible mode_sizes");
- Line 176: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("TT-core kron: core dimension product overflows size_t");
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("TensorContractionEngine::project: mode out of range");
- Line 227: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorContractionEngine::project: order must be ≥ 2");
- Line 310: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 315: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 318: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 321: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 399: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (va == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 439: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: core.data    = {result_dense.empty() ? 0.0f : result_dense[0]};
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.mode_sizes.push_back(1);  // placeholder, removed below
  Confidence: band=high; score=0.74
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(1);  // placeholder, removed below
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(contracted));
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(contracted));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(std::move(contracted));
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(train.mode_sizes[k]);
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(train.cores[k]);
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(std::move(cr));
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(std::move(ng));
- Line 261: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(c1.n);
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(train.cores[k]);
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(train.cores[k]);
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(train.mode_sizes[k]);
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(train.cores[k]);
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(train.cores[k]);
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(train.mode_sizes[k]);
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(std::move(ng));
- Line 285: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(prev.n);
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(train.cores[k]);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cores.push_back(train.cores[k]);
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mode_sizes.push_back(train.mode_sizes[k]);
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!contracted_a[k]) free_a.push_back(k);
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!contracted_b[k]) free_b.push_back(k);
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto k : free_a) result_shape.push_back(sha[k]);
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto k : free_b) result_shape.push_back(shb[k]);
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto k : free_b) result_shape.push_back(shb[k]);
- Line 419: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto k : free_a) ridx.push_back(idx_a[k]);
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto k : free_b) ridx.push_back(idx_b[k]);
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto k : free_b) ridx.push_back(idx_b[k]);
- Line 439: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scalar.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74

### src/query/adaptive_join.cpp
Total findings: 58

- Line 279: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(build_key);
- Line 292: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator bucket_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto bucket_it = hash_table.find(key_it->second);
- Line 411: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lk_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lk_it = left_row.find(spec.left_key);
- Line 415: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rk_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rk_it = right_row.find(spec.right_key);
- Line 444: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(spec.right_key);
- Line 456: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator bucket may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto bucket = index.find(lk_it->second);
- Line 491: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(spec.left_key);
- Line 500: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(spec.right_key);
- Line 525: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator bucket may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto bucket = ht.find(lk_it->second);
- Line 77: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Scan both sides.  Add sort cost for unsorted inputs.
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // --- AC-2: Merge Join — both inputs sorted on join key -------------------
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: spdlog::debug("AdaptiveJoin: MERGE_JOIN selected (both inputs sorted)");
  Confidence: band=very_high; score=0.9
- Line 176: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: spdlog::debug("AdaptiveJoin: SHUFFLE_JOIN selected (distributed, large inputs)");
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("AdaptiveJoinExecutor: unhandled JoinAlgorithm");
- Line 278: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(build_key);
- Line 287: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto key_it = probe_row.find(probe_key);
- Line 291: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket_it = hash_table.find(key_it->second);
- Line 352: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const std::string* value = findKeyValue(left_ptrs[li], spec.left_key);
- Line 357: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const std::string* value = findKeyValue(right_ptrs[ri], spec.right_key);
- Line 369: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const std::string* value = findKeyValue(left_ptrs[li_end], spec.left_key);
- Line 381: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const RowValue* left_row_ptr = left_ptrs[a];
- Line 382: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const RowValue* right_row_ptr = right_ptrs[b];
- Line 415: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto rk_it = right_row.find(spec.right_key);
  Confidence: band=very_high; score=0.9
- Line 443: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.right_key);
- Line 452: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lk_it = left_row.find(spec.left_key);
- Line 455: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket = index.find(lk_it->second);
- Line 490: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.left_key);
- Line 499: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.right_key);
- Line 499: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.right_key);
- Line 524: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket = ht.find(lk_it->second);
- Line 524: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket = ht.find(lk_it->second);
- Line 275: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<const RowValue*>> hash_table;
  Confidence: band=medium; score=0.66
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash_table[it->second].push_back(&row);
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(left_row, right_row));
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& r : left.rows)  left_ptrs.push_back(&r);
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : right.rows) right_ptrs.push_back(&r);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& r : right.rows) right_ptrs.push_back(&r);
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(left_row, right_row));
- Line 441: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<const RowValue*>> index;
  Confidence: band=medium; score=0.66
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: index[it->second].push_back(&row);
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(left_row, *right_row));
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_parts[p].push_back(&row);
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: left_parts[p].push_back(&row);
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: right_parts[p].push_back(&row);
- Line 515: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[it->second].push_back(row);
  Confidence: band=high; score=0.74
- Line 516: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ht[it->second].push_back(row);
- Line 532: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 533: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(*left_row, *right_row));

### src/query/gremlin_parser.cpp
Total findings: 57

- Line 273: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = opMap.find(opName);
- Line 300: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = opMap.find(opName);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4400 [WIP] Add GNN-based node embeddings implementation (2026-03-24T20:33:54Z)
- Line 203: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected " + ctx + " at position "
- Line 259: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected predicate name after P.");
- Line 275: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown predicate: " + opName);
- Line 312: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected a predicate at position "
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown Gremlin step '" + name
- Line 514: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Gremlin query must start with 'g'");
- Line 519: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected '.' after 'g'");
- Line 524: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected 'V' or 'E' after 'g.'");
- Line 528: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected 'V' or 'E' at position "
- Line 566: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unexpected token at position "
- Line 570: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Empty Gremlin traversal");
- Line 92: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': buf += '\n'; break;
- Line 93: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': buf += '\t'; break;
- Line 94: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': buf += '\r'; break;
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::END_OF_FILE, "", pos});
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::DOT, ".", start});
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::LPAREN, "(", start});
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::RPAREN, ")", start});
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::FLOAT_LIT, num, start});
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::INT_LIT, num, start});
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::BOOL_TRUE, ident, start});
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::BOOL_FALSE, ident, start});
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::IDENT, ident, start});
- Line 283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vals.push_back(parseValue());
- Line 330: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.values.push_back(parseValue());
- Line 380: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 562: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.steps.push_back(parseStep(stepName, stepPos));
- Line 614: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') out += "\\\"";
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') out += "\\\"";
- Line 616: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 647: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) list += ", ";
- Line 656: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) list += ", ";
- Line 731: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) labels.push_back(l);
  Confidence: band=high; score=0.74
- Line 732: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& l : step.strings) labels.push_back(l);
- Line 737: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
  Confidence: band=high; score=0.74
- Line 738: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
- Line 740: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(vVar + "." + key + " == " + valueToAQL(step.values[0]));
- Line 744: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(vVar + "." + key + " != null");
- Line 750: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(vVar + "." + step.strings[0] + " == null");
- Line 759: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
- Line 763: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
- Line 768: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
  Confidence: band=high; score=0.74
- Line 769: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
- Line 771: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& p : step.strings) valueProps.push_back(p);
  Confidence: band=high; score=0.74
- Line 772: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : step.strings) valueProps.push_back(p);
- Line 775: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& p : step.strings) valueMapProps.push_back(p);
  Confidence: band=high; score=0.74
- Line 776: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : step.strings) valueMapProps.push_back(p);
- Line 793: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& a : step.strings) selectAliases.push_back(a);
  Confidence: band=high; score=0.74
- Line 794: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& a : step.strings) selectAliases.push_back(a);

### src/query/query_federation.cpp
Total findings: 56

- Line 950: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string shard = sharding_manager_->GetShardForKey(
- Line 960: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto shards = sharding_manager_->GetShardsForKeyRange(
- Line 976: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string shard = sharding_manager_->GetShardForKey(
- Line 984: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto shards = sharding_manager_->GetShardsForKeyRange(
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4156 [WIP] Implement real shard determination logic for QueryFederation (2026-03-13T06:21:48Z
- Line 28: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::shared_ptr<themis::sharding::ShardRouter> requireShardRouter(
- Line 31: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("QueryFederation: shard_router cannot be null");
- Line 79: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 89: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 183: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error(
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error(
- Line 271: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json QueryFederation::execute(const std::string& query) {    total_queries_++;
- Line 447: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 451: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 455: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 493: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 504: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 619: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = hash_table.find(key);
- Line 770: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.point_lookup_key = m[1].str();
- Line 788: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.key_range = std::make_pair(m[1].str(), m[2].str());
- Line 965: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Shard-key range [{}, {}] → {} shard(s)",
- Line 1015: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("QueryFederation: range-lookup [{},{}] → {} shard(s)",
- Line 1049: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const QueryMetadata& metadata
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(value);
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(value);
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_results.push_back(std::move(rr));
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rag_results.push_back(std::move(rr));
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rr.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rr.documents.push_back(std::move(doc));
- Line 259: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rag_results.push_back(std::move(rr));
- Line 533: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 539: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash_table[key].push_back(row);
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(merged));
- Line 602: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 609: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash_table[key].push_back(row);
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 636: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(merged));
- Line 725: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(value);
- Line 868: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(s.shard_id);
  Confidence: band=high; score=0.74
- Line 1027: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(s.shard_id);
- Line 1062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(value);
- Line 1069: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_results.emplace_back(result);
  Confidence: band=high; score=0.74
- Line 1114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 1115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ordered_rows.push_back(row);
- Line 1119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(row);
  Confidence: band=high; score=0.74
- Line 1120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(row);
- Line 1144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paginated.push_back(result[i]);
  Confidence: band=high; score=0.74
- Line 1145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paginated.push_back(result[i]);

### src/query/functions/process_mining_functions.cpp
Total findings: 53

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #1100 [WIP] Fix missing and stub implementations from deep-dive audit (2026-03-11T17:52:41Z)
- Line 43: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(predict_end_fn_mutex_);
- Line 48: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(admin_model_load_fn_mutex_);
- Line 78: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(name + ": function not implemented");
- Line 139: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (!ev.activity.empty() && act_to_id.find(ev.activity) == act_to_id.end()) {
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmFindSimilarFunction::execute(
- Line 280: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["results"] = json::array();
- Line 285: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmCompareIdealFunction::execute(
- Line 293: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["deviations"]     = json::array();
- Line 297: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmHasPatternFunction::execute(
- Line 307: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmExtractLogFunction::execute(
- Line 313: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmExtractTraceFunction::execute(
- Line 317: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["trace"] = json::array();
- Line 331: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmDiscoverProcessFunction::execute(
- Line 348: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["nodes"]           = json::array();
- Line 349: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["edges"]           = json::array();
- Line 377: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmVariantsFunction::execute(
- Line 439: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmListAdminModelsFunction::execute(
- Line 452: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmConformanceFunction::execute(
- Line 501: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmDeviationsFunction::execute(
- Line 530: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmBottlenecksFunction::execute(
- Line 564: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmPredictEndFunction::execute(
- Line 568: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["predicted_end"] = nullptr;
- Line 590: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmExportBpmnFunction::execute(
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 120: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> act_to_id;
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.id_to_activity.push_back(ev.activity);
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.id_to_activity.push_back(ev.activity);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: log.id_to_activity.push_back(ev.activity);
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: trace.events.push_back(std::move(ev));
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(nj));
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(nj));
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(ej));
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proc.nodes.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: proc.nodes.push_back(std::move(n));
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proc.edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: proc.edges.push_back(std::move(e));
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(vj));
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(vj));
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devs.push_back(d);
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: devs.push_back(d);
- Line 517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(d);
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b);
  Confidence: band=high; score=0.74
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b);
- Line 595: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 595: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 600: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 600: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 606: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 606: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");

### src/query/aql_parser.cpp
Total findings: 50

- Line 862: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: // std::cerr << "parseComparison current token: " << (int)current().type << " value='" << current().value << "'\n";
  Confidence: band=very_high; score=0.92
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3481 [WIP] Synchronize AQL documentation with parser code (2026-03-12T07:20:02Z)
- Line 415: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(msg);
- Line 428: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // One or more FOR clauses (first is also stored in for_node for backward compat)
  Confidence: band=high; score=0.8
- Line 430: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected FOR");
- Line 493: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: also accept SHORTEST_PATH after RETURN
  Confidence: band=high; score=0.8
- Line 508: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected variable name after FOR");
- Line 519: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected edge variable name after ','");
- Line 526: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected path variable name after second ','");
- Line 596: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected start vertex string literal in traversal");
- Line 607: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected edge type string literal after TYPE");
- Line 615: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected graph name string literal after GRAPH");
- Line 633: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Still return a ForNode for compatibility (collection = "graph")
  Confidence: band=high; score=0.8
- Line 640: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected collection name or traversal after IN");
- Line 646: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected variable name after LET");
- Line 696: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected integer after LIMIT");
- Line 745: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected CTE name after WITH");
- Line 775: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected variable name after COLLECT");
- Line 794: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected aggregation variable name after AGGREGATE");
- Line 801: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected aggregation function name (COUNT, SUM, AVG, MIN, MAX)");
- Line 822: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 958: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected field name after '.'");
- Line 1002: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected object key (identifier or string)");
- Line 1076: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected variable name after ANY");
- Line 1098: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Expected variable name after ALL");
- Line 1170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unexpected token: " + current().value);
- Line 1203: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(result.error().message());
- Line 1229: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown operator: " + op_str);
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);
- Line 169: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': value += '\n'; break;
- Line 170: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': value += '\t'; break;
- Line 171: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': value += '\r'; break;
- Line 172: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': value += '"'; break;
- Line 173: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': value += '\''; break;
- Line 174: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': value += '\\'; break;
- Line 342: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: auto query = parseQuery(false); // false = not a subquery
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::shared_ptr<Query> parseQuery(bool isSubquery = false) {
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query->for_nodes.push_back(query->for_node);
- Line 442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query->for_nodes.push_back(std::move(f));
- Line 448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query->let_nodes.push_back(std::move(let));
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query->filters.push_back(parseFilterClause());
- Line 761: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: withNode->ctes.push_back(std::move(cte));
- Line 1022: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elems.push_back(parseExpression());
- Line 1151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args.push_back(parseExpression());
- Line 1329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: block.statements.push_back(std::move(*stmtResult));
- Line 1389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::string(1, ch));
- Line 1407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(upper));
- Line 1555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args.push_back(std::stoll(t));
- Line 1556: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/cypher_parser.cpp
Total findings: 49

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4400 [WIP] Add GNN-based node embeddings implementation (2026-03-24T20:33:54Z)
- Line 334: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 344: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 455: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 560: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 586: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
- Line 605: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
- Line 681: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 682: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "Expected NULL after IS [NOT]",
- Line 772: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 828: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 1100: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: edge_var + "._type IN [" + type_list + "]");
- Line 160: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  s += '\n'; break;
- Line 161: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  s += '\t'; break;
- Line 162: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  s += '\r'; break;
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(tok));
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(tok));
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(tok));
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(tok));
- Line 303: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '.';
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '.';
- Line 363: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: CypherASTNode parseQuery() {
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.match_patterns.push_back(parsePathPattern());
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.match_patterns.push_back(parsePathPattern());
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.segments.push_back(std::move(seg));
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.labels.push_back(tokens[cursor++].value);
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rel.types.push_back(tokens[cursor++].value);
- Line 567: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rel.types.push_back(expectIdent("as relationship type"));
- Line 760: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: list_literal += ", ";
- Line 859: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > start) expr_text += " ";
  Confidence: band=high; score=0.74
- Line 860: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > start) expr_text += " ";
- Line 868: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ast.return_items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 869: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.return_items.push_back(std::move(item));
- Line 881: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > start) expr_text += " ";
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > start) expr_text += " ";
- Line 888: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ast.order_by.push_back(std::move(spec));
  Confidence: band=high; score=0.74
- Line 889: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.order_by.push_back(std::move(spec));
- Line 940: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"' || c == '\\') out += '\\';
  Confidence: band=high; score=0.74
- Line 941: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"' || c == '\\') out += '\\';
- Line 999: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!filter.empty()) filter += " AND ";
- Line 1025: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_vars.push_back(v);
- Line 1056: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_clauses.push_back(std::move(node_filter));
  Confidence: band=high; score=0.74
- Line 1057: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filter_clauses.push_back(std::move(node_filter));
- Line 1095: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) type_list += ", ";
  Confidence: band=high; score=0.74
- Line 1096: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) type_list += ", ";
- Line 1097: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: type_list += "\"" + rel.types[i] + "\"";
- Line 1098: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_clauses.push_back(
  Confidence: band=high; score=0.74
- Line 1099: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filter_clauses.push_back(
- Line 1106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filter_clauses.push_back(std::move(dest_filter));

### src/query/functions/tensor_functions.cpp
Total findings: 48

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['                "Marginalize a tensor over one mode: PROJECT(t, mode). "', '                "Sums over all indices along mode, returning a train of order (d-1). "', '                "Operates entirely in the compressed domain (O(d*n*r^2)). "', '                "Ref: tensor marginalization, paper §AQL operators.",', '            .arguments = {']
  Confidence: band=very_high; score=0.9
- Line 17: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: *   1) inline objects {data:[...], shape:[...], eps?:...}
- Line 63: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto next = path.find('.', pos);
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (current->is_object()) {
- Line 72: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it == current->end()) return nullptr;
- Line 77: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (idx >= current->size()) return nullptr;
- Line 96: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 103: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 109: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Tensor field path cannot be empty");
- Line 114: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Tensor field path cannot be empty");
- Line 129: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (resolved && resolved->is_object() &&
- Line 141: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->is_object() &&
- Line 142: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->contains("data") && ptr->contains("shape")) {
- Line 150: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (resolved->is_object() &&
- Line 179: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [train, stats] = dec.decompose(data, shape, cfg);
- Line 194: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: "Arguments: two objects {data:[...], shape:[...], eps:float}. "
- Line 214: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 217: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TENSOR_SIMILARITY: requires 2 arguments");
- Line 235: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: "Argument: {data:[...], shape:[...]}. Returns float ≥ 0.",
- Line 249: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 252: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TENSOR_NORM: requires 1 argument");
- Line 282: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 285: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TENSOR_SLICE: requires 3 arguments (tensor, dim, idx)");
- Line 291: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (dimI < 0) throw std::invalid_argument("TENSOR_SLICE: dim must be >= 0");
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (idxI < 0) throw std::invalid_argument("TENSOR_SLICE: idx must be >= 0");
- Line 330: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 333: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TENSOR_COMPRESS: requires at least 1 argument");
- Line 340: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (mrI < 0) throw std::invalid_argument("TENSOR_COMPRESS: max_rank must be >= 0");
- Line 366: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: "Argument: {data:[...], shape:[...]}. "
- Line 378: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 381: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TENSOR_INFO: requires 1 argument");
- Line 432: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 489: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 492: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 498: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (modeI < 0) throw std::invalid_argument("TENSOR_PROJECT: mode must be >= 0");
- Line 550: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 555: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& v : args[0]) data.push_back(v.get<float>());
- Line 562: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (mrI < 0) throw std::invalid_argument("TENSOR_DECOMPOSE: max_rank must be >= 0");
- Line 574: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [train, stats] = dec.decompose(data, shape, cfg);
- Line 48: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : arr) out.push_back(v.get<float>());
- Line 79: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : shape_arr) shape.push_back(s.get<std::size_t>());
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : args[2]) modes_a.push_back(v.get<std::size_t>());
- Line 439: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& v : args[3]) modes_b.push_back(v.get<std::size_t>());
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : args[3]) modes_b.push_back(v.get<std::size_t>());
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : args[0]) data.push_back(v.get<float>());
- Line 555: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());
  Confidence: band=high; score=0.74
- Line 556: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());

### src/query/aql_runner.cpp
Total findings: 39

- Line 267: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: item["data"] = r.vertex_data;
- Line 806: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (limits.max_rows > 0 && rows_ptr->size() > limits.max_rows) {
- Line 809: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: "result row count " + std::to_string(rows_ptr->size()) +
- Line 816: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string serialised = rows_ptr->dump();
- Line 831: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── SQL dialect compatibility layer ──────────────────────────────────────────
  Confidence: band=high; score=0.8
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(e.getPrimaryKey(), std::move(geom));
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(row));
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(entityToResultRow(e));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(entityToResultRow(e));
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : paths) arr.push_back(p);
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(item));
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qr.rows.push_back(entityToResultRow(e));
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qr.rows.push_back(entityToResultRow(e));
- Line 358: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& row : jit_result->rows) arr.push_back(row);
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.attributes.push_back("start: " + tv.startVertex);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("start: " + tv.startVertex);
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("depth: " + std::to_string(tv.minDepth) +
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("direction: " + dir_name);
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("algorithm: " + algo_name);
- Line 414: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("end: " + tv.endVertex);
- Line 560: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"pk", r.pk}, {"distance", r.vector_distance}, {"entity", r.entity}});
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"pk", r.pk}, {"distance", r.vector_distance}, {"entity", r.entity}});
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 575: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(row));
- Line 586: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& e : *res) arr.push_back(entityToResultRow(e));
- Line 602: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& p : *res) arr.push_back(p);
  Confidence: band=high; score=0.74
- Line 603: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : *res) arr.push_back(p);
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"vertex", r.vertex_pk}, {"depth", r.depth},
  Confidence: band=high; score=0.74
- Line 627: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"vertex", r.vertex_pk}, {"depth", r.depth},
- Line 650: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{sj.outer_var, p.key_a}, {sj.inner_var, p.key_b}, {"distance_m", p.distance_m}});
- Line 661: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& e : *res) arr.push_back(entityToResultRow(e));
- Line 664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(stmtResult));
  Confidence: band=high; score=0.74
- Line 665: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(stmtResult));

### src/query/functions/lora_functions.cpp
Total findings: 37

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        std::string start_model = args[0].get<std::string>();', '        std::string end_model = args[1].get<std::string>();', '        int max_depth = args.size() > 2 ? args[2].get<int>() : 5;', '        (void)max_depth;', '']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        // Parse arguments', '        std::string adapter_id = args[0].get<std::string>();', '        int depth = args.size() > 1 ? args[1].get<int>() : 10;', '', '        // Get orchestrator']
  Confidence: band=very_high; score=0.93
- Line 152: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraTrainFunction::execute(
- Line 161: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: json dataset_json = args[2];
- Line 239: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraQueryFunction::execute(
- Line 314: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraSimilarFunction::execute(
- Line 449: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraPathFunction::execute(
- Line 469: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: start_node["edge"] = nullptr;
- Line 526: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraStatsFunction::execute(
- Line 617: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraRecommendFunction::execute(
- Line 679: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: recommendation["adapter_id"] = nullptr;
- Line 690: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: error["adapter_id"] = nullptr;
- Line 724: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraLineageFunction::execute(
- Line 792: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraProvenanceFunction::execute(
- Line 838: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraAuditLogFunction::execute(
- Line 937: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraVerifyChainFunction::execute(
- Line 21: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 22: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.samples.push_back(s);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data.samples.push_back(s);
- Line 227: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "LORA_QUERY('llama-2-7b', 'themis_help_lora', question.text, {max_tokens: 500})"
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> ws;
  Confidence: band=medium; score=0.66
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 412: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 491: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 752: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lineage.push_back(version);
  Confidence: band=high; score=0.74
- Line 753: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lineage.push_back(version);
- Line 758: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 804: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 853: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.toJSON());
- Line 857: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 900: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 901: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(s.toJSON());
- Line 904: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 826: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: "LORA_AUDIT_LOG('legal-lora-v2', 100)"
  Confidence: band=medium; score=0.6

### src/query/cross_cluster_federation.cpp
Total findings: 32

- Line 125: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = clusters_.find(cluster_id);
- Line 356: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lat_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lat_it = latency_cache_.find(id);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3350 [query] Cross-cluster federated AQL with cost estimation (Phase 4, ... (2026-03-12T07:06
- Line 63: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: acc->buffer->append(ptr, total);
- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: acc->buffer->append(ptr, total);
- Line 112: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 138: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, ep] : clusters_) {
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lat_it = latency_cache_.find(id);
- Line 233: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 259: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(selected.begin(), selected.end(), ep.cluster_id) !=
- Line 297: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 320: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(ep);
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.selected_clusters.push_back(est.cluster_id);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.selected_clusters.push_back(est.cluster_id);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.selected_clusters.push_back(est.cluster_id);
- Line 233: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back(ep);
- Line 261: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active.push_back(ep);
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_results.push_back(std::move(result));
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_results.push_back(std::move(result));
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cluster_list.push_back(c);
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cluster_list.push_back(c);
- Line 427: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: endpoint.cluster_id, status_code);
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(item);
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(sr);

### src/query/sparql_parser.cpp
Total findings: 32

- Line 873: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(tp.subject.value);
- Line 887: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(tp.predicate.value);
- Line 901: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(tp.object.value);
- Line 952: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(stmt.variables[0]);
- Line 7: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * PR: #3352 feat(query): SPARQL compatibility layer for RDF/knowledge-graph que... (2026-03-12T07:06:48Z)
  Confidence: band=high; score=0.8
- Line 12: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // SPARQL compatibility layer – SELECT query parsing and AQL transpilation.
  Confidence: band=high; score=0.8
- Line 872: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = var_bindings.find(tp.subject.value);
- Line 903: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: constraints.push_back(t_var + ".object == " + it->second);
- Line 57: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')       out += "\\\"";
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')       out += "\\\"";
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') out += "\\r";
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') out += "\\t";
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::VAR,
- Line 162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::URI, uri, start});
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::LTE, "<=", start});
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::LT, "<", start});
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readString(c, start));
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  val += '\n'; break;
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  val += '\r'; break;
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  val += '\t'; break;
- Line 266: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: default:   val += '\\'; val += esc; break;
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.variables.push_back(current().value);
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.where_clauses.push_back(std::move(clause));
- Line 781: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& var_bindings,
  Confidence: band=high; score=0.74
- Line 802: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& var_bindings) {
  Confidence: band=high; score=0.74
- Line 858: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> var_bindings;
  Confidence: band=high; score=0.74
- Line 874: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: constraints.push_back(t_var + ".subject == " + it->second);
  Confidence: band=high; score=0.74
- Line 875: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: constraints.push_back(t_var + ".subject == " + it->second);
- Line 880: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: constraints.push_back(
- Line 960: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [k, _] : var_bindings) all_keys.push_back(k);

### src/query/functions/ethics_functions.cpp
Total findings: 28

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    [[maybe_unused]] const std::string& philosophy = args[0];', '    [[maybe_unused]] const json& types = args.size() > 1 ? args[1] : json::array();', '    [[maybe_unused]] int limit = args.size() > 2 ? args[2].get<int>() : 20;', '', '    // F-028: throw so the AQL runtime surfaces a real error instead of']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    [[maybe_unused]] const std::string& query_text = args[0];', '    [[maybe_unused]] double threshold = args.size() > 1 ? args[1].get<double>() : 0.65;', '    [[maybe_unused]] int limit = args.size() > 2 ? args[2].get<int>() : 10;', '', '    // F-028: throw instead of silent empty array.']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    // Requires the ethics_ai plugin to create the ethics_arguments_graph.', '    [[maybe_unused]] const std::string& start_id = args[0];', '    [[maybe_unused]] int max_depth = args.size() > 1 ? args[1].get<int>() : 5;', '', '    // F-028: throw instead of silent empty array.']
  Confidence: band=very_high; score=0.9
- Line 45: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsMakeDecisionFunction::execute(
- Line 53: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["decision_id"] = "decision_" + std::to_string(std::time(nullptr));
- Line 62: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["created_at"] = std::time(nullptr);
- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["argument_chain_ids"] = json::array();
- Line 68: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsInitializeDebateFunction::execute(
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["debate_id"] = "debate_" + std::to_string(std::time(nullptr));
- Line 87: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsEvaluateFunction::execute(
- Line 114: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsEvaluateDimensionFunction::execute(
- Line 123: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json full_eval = EthicsEvaluateFunction().execute(eval_args, ctx);
- Line 157: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 182: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 211: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsLoadProfileFunction::execute(
- Line 229: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsListSchoolsFunction::execute(
- Line 256: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsBuildContextFunction::execute(
- Line 268: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["best_practices"] = json::array();
- Line 269: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["recent_debates"] = json::array();
- Line 270: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: context["consensus_decisions"] = json::array();
- Line 279: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsStatsFunction::execute(
- Line 298: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsMetricsFunction::execute(
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "kant"}, {"available", true}});
- Line 238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "utilitarianism"}, {"available", true}});
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "virtue_ethics"}, {"available", true}});
- Line 240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "contractualism"}, {"available", true}});
- Line 241: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "rationalism"}, {"available", true}});

### src/query/optimizer_cost_model.cpp
Total findings: 27

- Line 135: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.99
- Line 140: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.outputRows = static_cast<size_t>(inputRows * combinedSelectivity);
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // CPU cost: evaluate predicates for each input row
  Confidence: band=very_high; score=0.99
- Line 164: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.cpuCost = calculateCpuCost(inputRows, predicateCost);
  Confidence: band=very_high; score=0.99
- Line 237: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Sort both inputs
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Merge phase: single scan of both sorted inputs
  Confidence: band=very_high; score=0.99
- Line 260: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.99
- Line 267: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Cost: hash each input row and update aggregate
  Confidence: band=very_high; score=0.99
- Line 273: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double hashCost = calculateCpuCost(inputRows, constants_.cpuCostPerHash);
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double aggregateCost = static_cast<double>(inputRows * numAggregates) *
  Confidence: band=very_high; score=0.99
- Line 135: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.9
- Line 140: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.outputRows = static_cast<size_t>(inputRows * combinedSelectivity);
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // CPU cost: evaluate predicates for each input row
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.cpuCost = calculateCpuCost(inputRows, predicateCost);
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sort both inputs
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Sort both inputs
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Merge phase: single scan of both sorted inputs
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Merge phase: single scan of both sorted inputs
  Confidence: band=very_high; score=0.9
- Line 260: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Cost: hash each input row and update aggregate
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double hashCost = calculateCpuCost(inputRows, constants_.cpuCostPerHash);
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double aggregateCost = static_cast<double>(inputRows * numAggregates) *
  Confidence: band=very_high; score=0.9
- Line 137: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, ColumnStatistics>& columnStats) const {
  Confidence: band=high; score=0.74

### src/query/window_evaluator.cpp
Total findings: 27

- Line 0: severity=HIGH; category=uncategorized
  Context: ['        for (size_t i = 0; i < sortedIndices.size(); ++i) {', '            size_t originalIdx = sortedIndices[i];', '            results[originalIdx] = partitionResults[i];', '        }', '    }']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Zugriff auf vorherige Row', '            size_t prevRowIdx = sortedIndices[static_cast<size_t>(lagIdx)];', '            const auto& prevRow = rows[prevRowIdx];', '', '            if (argument) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Zugriff auf nächste Row', '            size_t nextRowIdx = sortedIndices[static_cast<size_t>(leadIdx)];', '            const auto& nextRow = rows[nextRowIdx];', '', '            if (argument) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // FIRST_VALUE ist der Wert der ersten Row in der Partition', '    size_t firstRowIdx = sortedIndices[0];', '    const auto& firstRow = rows[firstRowIdx];', '', '    nlohmann::json firstVal = nullptr;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        }', '', '        const auto& lastRow = rows[lastRowIdx];', '', '        nlohmann::json lastVal = nullptr;']
  Confidence: band=high; score=0.81
- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (expr) partBy.push_back(expr->toJSON());
  Confidence: band=high; score=0.74
- Line 33: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (expr) partBy.push_back(expr->toJSON());
- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordBy.push_back(spec.toJSON());
  Confidence: band=high; score=0.74
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ordBy.push_back(spec.toJSON());
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: partitionMap[key].push_back(i);
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(indices));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(indices));
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(static_cast<int64_t>(i + 1));  // 1-based
- Line 324: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(currentRank);
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(currentRank);
- Line 358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(currentDenseRank);
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(currentDenseRank);
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(defaultVal);
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(defaultVal);
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(val);
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(nullptr);
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(defaultVal);
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(defaultVal);
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(val);
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(nullptr);
- Line 468: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(firstVal);
- Line 521: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(lastVal);

### src/query/cte_subquery.cpp
Total findings: 26

- Line 714: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: optimizedQuery->limit = std::make_shared<query::LimitNode>(0, 1);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 329: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return expressionReferencesVariables(f->object, vars);
- Line 355: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, val] : obj->fields) {
- Line 392: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<nlohmann::json> SubqueryEvaluator::evaluateSubquery(
- Line 412: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (isCorrelatedSubquery(subquery.subquery, outerVarNames)) {
- Line 415: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return evaluateArraySubquery(subquery.subquery, queryEngine, outerRow);
- Line 420: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return evaluateScalarSubquery(subquery.subquery, queryEngine, outerRow);
- Line 423: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<nlohmann::json> SubqueryEvaluator::evaluateScalarSubquery(
- Line 519: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<nlohmann::json> SubqueryEvaluator::evaluateArraySubquery(
- Line 601: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> SubqueryEvaluator::evaluateInSubquery(
- Line 694: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> SubqueryEvaluator::evaluateExistsSubquery(
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(newResults);
- Line 369: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& outerVarNames
  Confidence: band=medium; score=0.66
- Line 407: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> outerVarNames;
  Confidence: band=medium; score=0.66
- Line 490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entityToJSON(entity));
  Confidence: band=high; score=0.74
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(entityToJSON(entity));
- Line 580: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entityToJSON(entity));
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(entityToJSON(entity));
- Line 587: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(row));
- Line 671: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entityToJSON(entity));
  Confidence: band=high; score=0.74
- Line 672: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(entityToJSON(entity));

### src/query/materialized_view.cpp
Total findings: 26

- Line 145: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new MaterializedView(def, config));
- Line 375: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(filter_field);
- Line 519: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = views_.find(name);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 374: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(filter_field);
- Line 500: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& table : view->getDefinition().base_tables) {
  Confidence: band=very_high; score=0.9
- Line 514: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != views_.end()) ? it->second : nullptr;
- Line 514: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != views_.end()) ? it->second : nullptr;
- Line 523: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& table : it->second->getDefinition().base_tables) {
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [field, value] : entity.getAllFields()) {
  Confidence: band=very_high; score=0.9
- Line 615: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& view_name : tit->second) {
  Confidence: band=very_high; score=0.9
- Line 631: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, view] : views_) {
  Confidence: band=very_high; score=0.9
- Line 661: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result = view->refresh(true);
- Line 101: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!s.empty()) s += ',';
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!s.empty()) s += ',';
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!s.empty()) s += ',';
- Line 193: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: MaterializedView::isStale()
  Context: bool MaterializedView::isStale() const {
  Confidence: band=medium; score=0.56
- Line 327: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rows_.push_back(row);
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(row);
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(row);
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_index_[table].push_back(name);
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table_index_[table].push_back(name);
- Line 537: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [n, _] : views_) names.push_back(n);
- Line 652: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_refresh.push_back(view);
  Confidence: band=high; score=0.74
- Line 653: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_refresh.push_back(view);

### src/query/parallel_executor.cpp
Total findings: 25

- Line 214: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 280: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 315: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 374: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 200: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 350: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 72: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!key.empty()) key += '|';
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!key.empty()) key += '|';
- Line 77: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ':';
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (filter(e)) out.push_back(e);
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({l, *it->second});
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({l, *it->second});
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({l, *it->second});
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({l, *it->second});
- Line 200: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (filter(input[i])) local.push_back(input[i]);
- Line 273: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto k = rows[i].getFieldAsString(key_field);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: morsel_partitions[m][slot].push_back(rows[i]);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: morsel_partitions[m][slot].push_back(rows[i]);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: morsel_partitions[m][slot].push_back(rows[i]);
- Line 299: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74

### src/query/query_optimizer.cpp
Total findings: 25

- Line 121: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = table_stats_ptr->column_stats.find(p.column);
- Line 122: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it != table_stats_ptr->column_stats.end() &&
- Line 123: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: table_stats_ptr->row_count > 0) {
- Line 126: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<double>(table_stats_ptr->row_count));
- Line 161: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? table_stats_ptr->row_count
- Line 161: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? table_stats_ptr->row_count
- Line 165: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? static_cast<size_t>(table_stats_ptr->avg_row_size_bytes > 0.0
- Line 165: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? static_cast<size_t>(table_stats_ptr->avg_row_size_bytes > 0.0
- Line 166: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? table_stats_ptr->avg_row_size_bytes
- Line 166: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? table_stats_ptr->avg_row_size_bytes
- Line 581: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(plan.recommended_parallelism, size_t(8)); ++i) {
- Line 767: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = table_stats_ptr->column_stats.find(pred.column);
- Line 768: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it != table_stats_ptr->column_stats.end()) {
- Line 96: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryOptimizer::Plan QueryOptimizer::chooseOrderForAndQuery(const ConjunctiveQuery& q, size_t maxProbePerPred) const {
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.details.push_back(Estimation{p, cnt, capped});
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.details.push_back(Estimation{p, cnt, capped});
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto i : idx) plan.orderedPredicates.push_back(plan.details[i].pred);
- Line 542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_infos.push_back(info);
  Confidence: band=high; score=0.74
- Line 543: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_infos.push_back(info);
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pruned_shards.push_back(info.shard_id);
  Confidence: band=high; score=0.74
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pruned_shards.push_back(info.shard_id);
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.preferred_cpu_affinity.push_back(static_cast<int>(i));
  Confidence: band=high; score=0.74
- Line 582: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.preferred_cpu_affinity.push_back(static_cast<int>(i));
- Line 391: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: vectorSearchCost = std::log(static_cast<double>(universe) + 1.0) * dimScale; // ANN approximation
  Confidence: band=medium; score=0.6
- Line 420: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double ftPhase = C_fulltext_base * std::log(static_cast<double>(hits) + 5.0);
  Confidence: band=medium; score=0.6

### src/query/functions/udf_registry.cpp
Total findings: 24

- Line 386: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = udfs_.find(name);
- Line 60: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown argument type: " + s);
- Line 174: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json UdfFunction::execute(
- Line 188: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": expression depth limit exceeded");
- Line 192: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": body expression must be an object");
- Line 196: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": expression node must have a string 'type' field");
- Line 204: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": 'const' node requires 'value'");
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": 'arg' node requires integer 'index'");
- Line 216: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": argument index " +
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": 'call' node requires string 'function'");
- Line 266: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": division by zero");
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": division by zero");
- Line 271: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": modulo by zero");
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": modulo by zero");
- Line 321: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": unknown operator '" + op + "'");
- Line 327: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": 'if' node requires 'cond', 'then', 'else'");
- Line 337: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(def_.name + ": unknown expression type '" + type + "'");
- Line 349: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (char c : def.name) {
  Confidence: band=very_high; score=0.9
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args_json.push_back({
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args_json.push_back({
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: callArgs.push_back(evalExpr(a, args, context, depth + 1));
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: callArgs.push_back(evalExpr(a, args, context, depth + 1));
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.second);

### src/query/materialized_cte.cpp
Total findings: 20

- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: applied += static_cast<uint64_t>(view->applyChanges(changes));
- Line 184: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = vrow.values.find(agg.output_name);
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: obj[agg.output_name] = fieldValueToJson(it->second);
- Line 227: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: MaterializedCTEResult MaterializedCTEView::query(
- Line 230: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto vqr = view_->query({}, limit, offset);
- Line 300: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != views_.end()) ? it->second : nullptr;
- Line 300: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != views_.end()) ? it->second : nullptr;
- Line 331: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: MaterializedCTEResult MaterializedCTERegistry::query(
- Line 342: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return it->second->query(limit, offset);
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vd.aggregations.push_back(spec);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vd.aggregations.push_back(spec);
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vd.base_filters.push_back(vf);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vd.base_filters.push_back(vf);
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back({std::move(obj)});
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back({std::move(obj)});
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back({std::move(obj)});
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back({std::move(obj)});
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recs.push_back(toChangeRecord(c));
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recs.push_back(toChangeRecord(c));
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [n, _] : views_) names.push_back(n);

### src/query/continuous_query_engine.cpp
Total findings: 19

- Line 192: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = registry_.find(name);
- Line 209: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = registry_.find(name);
- Line 51: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: if (!cv_.wait_for(lock, timeout, [this] {
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return cancelled_.load(std::memory_order_acquire);
- Line 116: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [name, entry] : registry_) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& q : entry.subscribers) {
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(loop_mutex_);
- Line 198: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& q : it->second.subscribers) {
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, entry] : registry_) {
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& q : entry.subscribers) {
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& q : entry.subscribers) {
  Confidence: band=very_high; score=0.9
- Line 151: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ContinuousQueryEngineImpl::registerQuery(ContinuousQuerySpec spec) {
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> ContinuousQueryEngineImpl::dropQuery(const std::string& name) {
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: it->second.subscribers.push_back(std::move(queue));
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: it->second.subscribers.push_back(std::move(queue));
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: inject_queue_.push_back({collection, tuple, event_ts});

### src/query/query_rewrite_rule.cpp
Total findings: 19

- Line 95: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator f may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto f = eq.find("field");
- Line 96: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator v may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto v = eq.find("value");
- Line 160: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator children_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto children_it = node.find("children");
- Line 286: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: in_expr["values"] = nlohmann::json::array();
- Line 343: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: else if (t == "div" && rv != 0.0) result = lv / rv;
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ref["alias"] = it->second;
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filters.push_back(std::move(child));
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(std::move(child));
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: others.push_back(std::move(child));
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scan["children"].push_back(std::move(f));
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scan["children"].push_back(std::move(f));
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: in_expr["values"].push_back(std::move(v));
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: in_expr["values"].push_back(std::move(v));
- Line 364: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> seen;
  Confidence: band=medium; score=0.66
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (rule) rules_.push_back(std::move(rule));
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (rule) rules_.push_back(std::move(rule));
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.applied_rule_names.push_back(rule->name());
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.applied_rule_names.push_back(rule->name());
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.applied_rule_names.push_back(rule->name());

### src/query/sql_parser.cpp
Total findings: 19

- Line 7: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * PR: #2577 feat(query): SQL dialect compatibility layer â€“ SELECT/INSERT/UPDA... (2026-03-12T05:52:06Z)
  Confidence: band=high; score=0.8
- Line 12: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // SQL dialect compatibility layer – SELECT/INSERT/UPDATE/DELETE passthrough.
  Confidence: band=high; score=0.8
- Line 56: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  out += "\\\"";
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  out += "\\\"";
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') out += "\\r";
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') out += "\\t";
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readString(c, start));
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readNumber(start));
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readIdent(start));
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': val += '\n'; break;
- Line 288: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': val += '\r'; break;
- Line 289: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': val += '\t'; break;
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.order_by.push_back(spec);
- Line 545: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.columns.push_back(current().value);
- Line 566: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.values.push_back(std::move(val.value()));
- Line 612: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.assignments.push_back({col, std::move(val.value())});
- Line 737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elems.push_back(std::move(elem.value()));

### src/query/plan_cache.cpp
Total findings: 17

- Line 82: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: CachedPlan& cached = it->second.plan;
- Line 128: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator existing may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto existing = cache_.find(fp);
- Line 189: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fp);
- Line 215: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = cache_.begin(); it != cache_.end(); ++it) {
- Line 278: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator tidx may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto tidx = table_index_.find(tbl);
- Line 57: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& tbl : existing->second.plan.referenced_tables) {
- Line 165: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& tbl : tables) {
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cache_.find(fp);
- Line 221: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& it : to_remove) {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& tbl : it->second.plan.referenced_tables) {
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_index_[tbl].push_back(fp);
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table_index_[tbl].push_back(fp);
- Line 213: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_map<std::string, Entry>::iterator> to_remove;
  Confidence: band=medium; score=0.66
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(it);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_remove.push_back(it);
- Line 272: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Entry>::iterator it)
  Confidence: band=medium; score=0.66

### src/query/query_cache.cpp
Total findings: 17

- Line 119: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 223: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = dependency_index_.find(dependency);
- Line 239: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 266: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 327: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 459: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = cache_.begin(); it != cache_.end(); ++it) {
- Line 495: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 582: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = dependency_index_.find(dep);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 538: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& dep : entry.dependencies) {
  Confidence: band=very_high; score=0.9
- Line 567: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& dep : dependencies) {
  Confidence: band=very_high; score=0.9
- Line 581: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = dependency_index_.find(dep);
- Line 289: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: QueryCache::clear()
  Context: Result<void> QueryCache::clear() {
  Confidence: band=medium; score=0.56
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(fingerprint);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_remove.push_back(fingerprint);
- Line 567: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependency_index_[dep].push_back(fingerprint);
  Confidence: band=high; score=0.74
- Line 568: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependency_index_[dep].push_back(fingerprint);

### src/query/query_plan_visualizer.cpp
Total findings: 16

- Line 95: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: filter_node->estimated_rows = static_cast<size_t>(i) < plan.details.size()
- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: filter_node->estimated_cost = 50.0 + 10.0 * static_cast<double>(i);
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_node->attributes.push_back(pred.column);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filter_node->attributes.push_back(pred.column);
- Line 143: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += std::string(4, ' ') + "<max depth exceeded>\n";
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: children_arr.push_back(toJSONImpl(*child, analyze, depth + 1));
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: children_arr.push_back(toJSONImpl(*child, analyze, depth + 1));
- Line 271: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  result += "\\\""; break;
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': result += "\\\\"; break;
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': result += "\\n";  break;
- Line 275: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': result += "\\r";  break;
- Line 276: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': result += "\\t";  break;
- Line 324: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: edges_out += "  n" + std::to_string(my_id)
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: edges_out += "  n" + std::to_string(my_id)

### src/query/result_type_annotation.cpp
Total findings: 16

- Line 184: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = row.begin(); it != row.end(); ++it) {
- Line 88: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (std::isfinite(d) && d == std::floor(d) &&
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (seen_fields.find(fname) == seen_fields.end()) {
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (row.find(fname) == row.end()) {
  Confidence: band=very_high; score=0.9
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(f.toJson());
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(f.toJson());
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(f.toJson());
- Line 177: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ResultFieldType> field_types;
  Confidence: band=medium; score=0.66
- Line 178: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>                  nullable_fields;
  Confidence: band=medium; score=0.66
- Line 179: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>                  seen_fields;
  Confidence: band=medium; score=0.66
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_order.push_back(fname);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_order.push_back(fname);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: field_order.push_back(fname);
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.fields.push_back(std::move(ann));
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.fields.push_back(std::move(ann));
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schema.fields.push_back(std::move(ann));

### src/query/query_cache_manager.cpp
Total findings: 15

- Line 135: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_result = basic_cache_->get(query, params);
- Line 137: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = cache_result->result;
- Line 142: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_entry = adaptive_cache_->get(fingerprint, "");
- Line 144: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = cache_entry->result;
- Line 244: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto result = basic_cache_->invalidateByDependency(dependency);
- Line 273: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto result = basic_cache_->invalidate(query, params);
- Line 277: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto result = adaptive_cache_->invalidate(fingerprint);
- Line 337: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto put_result = basic_cache_->put(fingerprint, params, result, deps,
- Line 385: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: info["cache_info"] = basic_cache_->getDetailedInfo();
- Line 388: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: info["cache_info"] = adaptive_cache_->getDetailedInfo();
- Line 471: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_stats = basic_cache_->getStats();
- Line 475: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_stats = adaptive_cache_->getStats();
- Line 478: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_info = adaptive_cache_->getDetailedInfo();
- Line 530: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto put_result = basic_cache_->put(query, params, result, dependencies, ttl);
- Line 431: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
  Confidence: band=very_high; score=0.9

### src/query/vectorized_execution.cpp
Total findings: 15

- Line 152: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::vector<nlohmann::json>> VectorizedExecutionEngine::execute(
- Line 179: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch out   = analytics_engine.execute(batch, pipeline);
- Line 218: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 227: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 236: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 245: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 395: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: obj[name] = nullptr;
- Line 261: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> col_index;
  Confidence: band=medium; score=0.66
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(key);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(key);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col_names.push_back(key);
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back(std::move(col));
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preds.push_back(translatePredicate(vp));
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back({sk.field, sk.ascending});
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back({sk.field, sk.ascending});

### src/query/approximate_aggregator.cpp
Total findings: 11

- Line 85: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: registers_[static_cast<size_t>(i)] = o->registers_[static_cast<size_t>(i)];
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 80: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 182: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 266: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(c);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(c);
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(c);
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids_.push_back(c);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids_.push_back(c);
- Line 107: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(static_cast<double>(num_registers_) /
  Confidence: band=medium; score=0.6

### src/query/adaptive_optimizer.cpp
Total findings: 10

- Line 141: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = executions_.begin(); it != executions_.end();) {
- Line 109: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& exec : history) {
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("No plan alternatives provided");
- Line 511: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pthread_t thread = pthread_self();
- Line 512: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) == 0;
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.indexes_to_use.push_back(idx.index_name);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.indexes_to_use.push_back(idx.index_name);
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: placement.cpu_affinity.push_back(i);
  Confidence: band=high; score=0.74
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: placement.cpu_affinity.push_back(i);
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: placement.cpu_affinity.push_back(i);

### src/query/query_compiler.cpp
Total findings: 8

- Line 131: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = entries_.find(key);
- Line 135: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator oldest may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto oldest = entries_.begin();
- Line 23: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: *     execute() delegates to the ExecuteFn supplied at compile() time —
- Line 163: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<QueryResult> execute(
- Line 228: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: it->second.hot_fn     = nullptr;
- Line 387: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<QueryResult> QueryCompiler::execute(
- Line 391: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(compiled, params);
- Line 346: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/tensor_aware_query_optimizer.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        function_name == "TENSOR_CONTRACT") {', '        // Inner-product / transfer-matrix: O(d·n·r³)', '        return d * n * r * r * r;', '    }', '    if (function_name == "TENSOR_SLICE" ||']
  Confidence: band=very_high; score=0.93
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        function_name == "TENSOR_PROJECT") {', '        // Slice / marginalize one core: O(d·n·r²)', '        return d * n * r * r;', '    }', '    if (function_name == "TENSOR_COMPRESS" ||']
  Confidence: band=very_high; score=0.93
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        function_name == "TENSOR_DECOMPOSE") {', '        // TT-rounding / decomposition: O(d·r²·n)', '        return d * r * r * n * std::log2(n + 1.0);', '    }', '    if (function_name == "TENSOR_INFO") {']
  Confidence: band=very_high; score=0.93
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    }', '    // Unknown — use a generic linear estimate.', '    return d * n * r;', '}', '']
  Confidence: band=very_high; score=0.93
- Line 56: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(s_ast_visitor_fn_mutex);
- Line 227: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (upper_desc.find(fn) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 210: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/workload_cache_strategy.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 446: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < std::min(limit, query_frequencies.size()); ++i) {
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(limit, query_frequencies.size()); ++i) {
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hot_queries.push_back(query_frequencies[i].first);
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hot_queries.push_back(query_frequencies[i].first);

### src/query/cte_cache.cpp
Total findings: 7

- Line 198: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string serialized = data[i].dump();
- Line 210: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: total_estimate += (sizeof(nlohmann::json) + 64) * data.size();
- Line 157: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 241: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(nlohmann::json::parse(serialized));
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(nlohmann::json::parse(serialized));
- Line 288: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();

### src/query/semantic_cache.cpp
Total findings: 7

- Line 199: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(std::string(query));
- Line 457: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(queryStr);
- Line 299: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& query : toRemove) {
  Confidence: band=very_high; score=0.9
- Line 550: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(tokens.begin(), tokens.end(), kw) != tokens.end()) {
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(current);
  Confidence: band=high; score=0.74
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(current);
- Line 509: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(current);

### src/query/aql_parser_json.cpp
Total findings: 6

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3351 [WIP] Improve multi-statement transaction AQL support (2026-03-12T07:06:46Z)
- Line 47: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: {"object", object->toJSON()},
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args_json.push_back(arg->toJSON());
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args_json.push_back(arg->toJSON());
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elems_json.push_back(elem->toJSON());
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elems_json.push_back(elem->toJSON());

### src/query/aql_safety_validator.cpp
Total findings: 4

- Line 127: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: entire = nullptr;
  Context: "enforce_read_only context. This pattern can delete entire "
- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::toupper(c)));
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(std::toupper(c)));
- Line 127: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "enforce_read_only context. This pattern can delete entire "

### src/query/continuous_query_planner.cpp
Total findings: 4

- Line 45: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({t.payload, /*is_retract=*/true});
  Confidence: band=high; score=0.74
- Line 46: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({t.payload, /*is_retract=*/true});
- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({t.payload, false});
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({t.payload, false});

### src/query/query_canceller.cpp
Total findings: 4

- Line 36: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tokens_.find(request_id);
- Line 40: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto token = it->second.lock();
- Line 25: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryCanceller::registerQuery(const std::string& request_id) {
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void QueryCanceller::unregisterQuery(const std::string& request_id) {
  Confidence: band=high; score=0.74

### src/query/result_stream.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 102: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: T item = buffer_[buffer_pos_];
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.items.push_back(*result);

### src/query/statistical_aggregator.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    if (lowerIndex == upperIndex) {', '        return Ok(nlohmann::json(values[lowerIndex]));', '    }', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Linear interpolation', '    double weight = rank - lowerIndex;', '    double result = values[lowerIndex] * (1.0 - weight) + values[upperIndex] * weight;', '', '    return Ok(nlohmann::json(result));']
  Confidence: band=high; score=0.81
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(val.get<double>());
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(val.get<double>());

### src/query/synopsis_store.cpp
Total findings: 3

- Line 48: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 53: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: expired.push_back(std::move(tuples_.front()));

### src/query/cq_watermark.cpp
Total findings: 2

- Line 23: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: int64_t current_max = max_seen_us_.load(std::memory_order_relaxed);
- Line 32: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const int64_t wm = watermark_us_.load(std::memory_order_acquire);

### src/query/query_profiler.cpp
Total findings: 1

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3328 [WIP] Add SLO/SLA compliance reporting with burn-rate alerts (2026-03-12T06:59:43Z)

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
