# query Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: query
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 1088
- Actionable Findings (Critical + High): 512
- Affected Files: 50

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 154 |
| High | 358 |
| Medium | 572 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 286 |
| container | 175 |
| reliability | 166 |
| concurrency | 87 |
| performance | 74 |
| security | 60 |
| determinism | 49 |
| observability | 48 |
| memory | 31 |
| exception_safety | 28 |
| llm_ai_safety | 24 |
| legacy_duplication | 17 |
| type_conversion | 15 |
| platform | 9 |
| audit_logging | 7 |
| input_validation | 7 |
| raii | 6 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/query/query_engine.cpp | 195 | 29 | 45 | 121 | 0 |
| src/query/aql_translator.cpp | 92 | 72 | 8 | 12 | 0 |
| src/query/functions/fulltext_functions.cpp | 65 | 2 | 24 | 39 | 0 |
| src/query/sparql_parser.cpp | 45 | 3 | 20 | 22 | 0 |
| src/query/cypher_parser.cpp | 39 | 0 | 12 | 27 | 0 |
| src/query/functions/process_mining_functions.cpp | 38 | 0 | 19 | 19 | 0 |
| src/query/tensor_contraction_engine.cpp | 36 | 1 | 14 | 21 | 0 |
| src/query/adaptive_join.cpp | 35 | 1 | 16 | 18 | 0 |
| src/query/let_evaluator.cpp | 34 | 0 | 15 | 19 | 0 |
| src/query/gremlin_parser.cpp | 33 | 1 | 1 | 31 | 0 |
| src/query/functions/lora_functions.cpp | 32 | 2 | 12 | 17 | 1 |
| src/query/query_plan_visualizer.cpp | 29 | 2 | 13 | 14 | 0 |
| src/query/optimizer_cost_model.cpp | 27 | 12 | 14 | 1 | 0 |
| src/query/aql_runner.cpp | 26 | 0 | 2 | 24 | 0 |
| src/query/query_federation.cpp | 26 | 0 | 6 | 20 | 0 |
| src/query/query_optimizer.cpp | 25 | 0 | 14 | 9 | 2 |
| src/query/functions/ethics_functions.cpp | 24 | 3 | 16 | 5 | 0 |
| src/query/aql_parser.cpp | 22 | 1 | 9 | 12 | 0 |
| src/query/parallel_executor.cpp | 22 | 4 | 4 | 14 | 0 |
| src/query/functions/tensor_functions.cpp | 21 | 1 | 12 | 8 | 0 |
| src/query/cte_subquery.cpp | 20 | 1 | 13 | 6 | 0 |
| src/query/cross_cluster_federation.cpp | 15 | 0 | 4 | 11 | 0 |
| src/query/sql_parser.cpp | 15 | 0 | 2 | 13 | 0 |
| src/query/result_type_annotation.cpp | 14 | 1 | 3 | 10 | 0 |
| src/query/window_evaluator.cpp | 14 | 0 | 5 | 9 | 0 |
| src/query/materialized_view.cpp | 12 | 1 | 4 | 7 | 0 |
| src/query/materialized_cte.cpp | 11 | 0 | 5 | 6 | 0 |
| src/query/vectorized_execution.cpp | 11 | 0 | 7 | 4 | 0 |
| src/query/query_rewrite_rule.cpp | 10 | 1 | 1 | 8 | 0 |
| src/query/continuous_query_engine.cpp | 9 | 0 | 3 | 6 | 0 |
| src/query/query_cache.cpp | 9 | 3 | 3 | 3 | 0 |
| src/query/functions/udf_registry.cpp | 8 | 0 | 3 | 5 | 0 |
| src/query/plan_cache.cpp | 7 | 1 | 2 | 4 | 0 |
| src/query/tensor_aware_query_optimizer.cpp | 7 | 4 | 1 | 2 | 0 |
| src/query/adaptive_optimizer.cpp | 6 | 0 | 2 | 4 | 0 |
| src/query/aql_parser_json.cpp | 6 | 0 | 2 | 4 | 0 |
| src/query/workload_cache_strategy.cpp | 6 | 0 | 5 | 1 | 0 |
| src/query/approximate_aggregator.cpp | 5 | 1 | 1 | 2 | 1 |
| src/query/cte_cache.cpp | 5 | 0 | 1 | 4 | 0 |
| src/query/query_cache_manager.cpp | 5 | 5 | 0 | 0 | 0 |
| src/query/query_compiler.cpp | 5 | 0 | 4 | 1 | 0 |
| src/query/semantic_cache.cpp | 4 | 1 | 2 | 1 | 0 |
| src/query/aql_safety_validator.cpp | 3 | 0 | 1 | 2 | 0 |
| src/query/continuous_query_planner.cpp | 3 | 0 | 0 | 3 | 0 |
| src/query/query_canceller.cpp | 3 | 1 | 0 | 2 | 0 |
| src/query/statistical_aggregator.cpp | 3 | 0 | 2 | 1 | 0 |
| src/query/cq_watermark.cpp | 2 | 0 | 2 | 0 | 0 |
| src/query/result_stream.cpp | 2 | 0 | 2 | 0 | 0 |
| src/query/functions/function_registry.cpp | 1 | 0 | 1 | 0 | 0 |
| src/query/query_profiler.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/query/query_engine.cpp
Total findings: 195

- Line 603: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 826: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 936: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 998: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1067: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1131: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 1268: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 2390: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 2739: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto checkFieldAccess = [](const std::shared_ptr<query::Expression>& e) -> std::pair<std::string, st
- Line 2743: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto varExpr = std::static_pointer_cast<query::VariableExpr>(fa->object);
- Line 2792: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: initial_context.cte_cache = parent_context->cte_cache;
- Line 2984: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bin = std::static_pointer_cast<query::BinaryOpExpr>(filter->condition);
- Line 2989: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->left);
- Line 2990: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->right);
- Line 2993: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lvar = std::static_pointer_cast<query::VariableExpr>(lfa->object)->name;
- Line 2994: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rvar = std::static_pointer_cast<query::VariableExpr>(rfa->object)->name;
- Line 3237: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t offset = (limit->offset <= 0) ? 0 : static_cast<size_t>(limit->offset);
- Line 3238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t count  = (limit->count  <= 0) ? 0 : static_cast<size_t>(limit->count);
- Line 3694: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg3.wait();
- Line 3996: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto xLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[0]);
- Line 3997: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto yLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[1]);
- Line 4115: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4157: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4188: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4354: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 4509: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: vgr.entity = cached->second;
- Line 4558: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg2.wait();
- Line 4582: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: r.entity = cached->second;
- Line 4650: severity=CRITICAL; category=no_timeout
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4507 feat(query): v2.0.0 â€“ edg... (2026-04-11) | #4364 docs(query): rewrit
- Line 591: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(errors_mutex);
- Line 812: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 923: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(errors_mutex);
- Line 977: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < q.disjuncts.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 985: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> eg(error_mutex);
- Line 1054: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1060: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1060: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1123: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1254: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1260: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 2171: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: sorted_fields.reserve(obj->fields.size());
- Line 2172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [k, e] : obj->fields) {
- Line 2206: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (d==0.0) return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Division by zero");
  Confidence: band=very_high; score=0.9
- Line 2211: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (d==0.0) return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Modulo by zero");
  Confidence: band=very_high; score=0.9
- Line 2287: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (pos_a == a.size() && pos_b == b.size()) {
  Confidence: band=very_high; score=0.9
- Line 2299: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (pos_a == a.size() && pos_b == b.size()) {
  Confidence: band=very_high; score=0.9
- Line 2540: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sel_a == sel_b) {
  Confidence: band=very_high; score=0.9
- Line 2623: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!candSet.empty() && candSet.find(k) == candSet.end()) continue; // filter
- Line 2623: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!candSet.empty() && candSet.find(k) == candSet.end()) continue; // filter
- Line 2704: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, val] : obj->fields) {
- Line 3015: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result_or_err = evaluateExpression(return_node->expression, ctx);
- Line 3111: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result_or_err = evaluateExpression(return_node->expression, ctx);
- Line 3403: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result_or_err = evaluateExpression(return_node->expression, ctx);
- Line 3566: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto slashPos = vertexPk.find('/');
- Line 3658: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto slashPos = vertexPk.find('/');
- Line 4061: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (eqPrefilter && !eqPrefilter->empty()) {
- Line 4063: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (eqPrefilter->size() < stats.entry_count * 0.05) return VGPlan::VectorThenSpatial;
- Line 4142: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: std::string tmp = st.column; size_t pos=0; while(true){ size_t n = tmp.find('+', pos); if(n==std::st
- Line 4146: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: for (auto &c : cols) { auto it = equalityMap.find(c); if (it==equalityMap.end()) { all=false; break;
- Line 4176: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto itRange = rangeMap.find(column);
- Line 4195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: span.setAttribute("index_prefilter_size", static_cast<int64_t>(indexPrefilter->size()));
- Line 4196: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (indexPrefilter->empty()) { span.setAttribute("result_count", static_cast<int64_t>(0)); span.setS
- Line 4262: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i=0;i<std::min(tmp.size(),k);++i) {
- Line 4298: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ci.prefilterSize = indexPrefilter ? indexPrefilter->size() : 0; ci.k = q.k; ci.vectorDim = q.query_v
- Line 4539: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = entityCache.find(pk);
- Line 4539: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = entityCache.find(pk);
- Line 4540: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = entityCache.find(pk);
  Confidence: band=very_high; score=0.9
- Line 4699: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sa == sb) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 115: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 185: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 225: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: phraseKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fuzzyKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fulltextKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialKeys.emplace_back(res.primary_key);
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(st.message);
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto scoreMap = std::make_shared<std::unordered_map<std::string, double>>();
  Confidence: band=medium; score=0.66
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fulltextKeys.emplace_back(res.pk);
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto filteredScores = std::make_shared<std::unordered_map<std::string, double>>();
  Confidence: band=medium; score=0.66
- Line 766: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 795: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
- Line 815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 817: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(result.error().context());
  Confidence: band=high; score=0.74
- Line 985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(result.error().message());
  Confidence: band=high; score=0.74
- Line 1040: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeOrEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}"
- Line 1057: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1059: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeOrEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
- Line 1120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1122: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1237: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesSequential: Deserialisierung fehlgeschlagen für PK={}",
- Line 1257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1259: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1334: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stod(v.get<std::string>()); } catch (...) { return 0.0; }
- Line 1358: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return nullptr; }
- Line 1640: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coords = g["coordinates"];
  Confidence: band=high; score=0.74
- Line 1845: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: std::string u = trim(wkt); std::string up=u; std::transform(up.begin(), up.end(), up.begin(), ::toup
- Line 1983: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (t=="LineString"||t=="MultiPoint") { nlohmann::json nc=nlohmann::json::array(); for (const a
- Line 1983: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: else if (t=="Polygon"||t=="MultiLineString") { nlohmann::json nr=nlohmann::json::array(); for (const auto& ring : g["coordinates"]) { nlohmann::json r=nlohmann::json::array(); for (const auto& pt : ring) r.push_back(strip2D(pt)); nr.push_back(r);} result["coordinates"]=nr; }
  Confidence: band=high; score=0.71
- Line 2163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.push_back(*elemRes);
- Line 2172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_fields.emplace_back(k, e);
  Confidence: band=high; score=0.74
- Line 2292: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2304: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matchesPredicates(e)) out.emplace_back(std::move(entry.pk));
  Confidence: band=high; score=0.74
- Line 2357: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matchesPredicates(e)) local.emplace_back(std::move(entry.pk));
  Confidence: band=high; score=0.74
- Line 2383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 2507: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}
- Line 2561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lists.emplace_back(std::move(keys));
  Confidence: band=high; score=0.74
- Line 2581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lists.emplace_back(std::move(keys));
  Confidence: band=high; score=0.74
- Line 2608: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candSet;
  Confidence: band=medium; score=0.66
- Line 2624: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.emplace_back(std::move(k));
  Confidence: band=high; score=0.74
- Line 2647: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 2649: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesRangeAware_: Deserialisierung fehlgeschlagen für PK={}"
- Line 2807: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::shared_ptr<query::FilterNode>>> single_var_filters;
  Confidence: band=high; score=0.74
- Line 2815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: single_var_filters[*vars.begin()].push_back(filter);
  Confidence: band=high; score=0.74
- Line 2834: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 2864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bucket_it->second.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2904: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bucket_it->second.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2907: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 3044: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_probe_docs.emplace_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 3046: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 3075: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_bindings.emplace_back(var, val);
  Confidence: band=high; score=0.74
- Line 3116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(*result_or_err));
  Confidence: band=high; score=0.74
- Line 3188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_scan_docs.emplace_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 3190: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3271: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> groups;
  Confidence: band=medium; score=0.66
- Line 3307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_it->second.push_back(doc);
  Confidence: band=high; score=0.74
- Line 3309: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_docs.push_back(&doc);
  Confidence: band=high; score=0.74
- Line 3425: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryEngine::executeRecursivePathQuery(const RecursivePathQuery& q) const {
  Confidence: band=high; score=0.74
- Line 3447: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> frontier{q.start_node};
  Confidence: band=medium; score=0.66
- Line 3449: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> next;
  Confidence: band=medium; score=0.66
- Line 3518: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: GraphIndexManager::Status st;
- Line 3572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertexKeys.emplace_back(table + ":" + vertexPk);
  Confidence: band=high; score=0.74
- Line 3592: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3622: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: GraphIndexManager::Status st;
- Line 3664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertexKeys.emplace_back(table + ":" + vertexPk);
  Confidence: band=high; score=0.74
- Line 3687: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { continue; }
- Line 3688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (evaluateCondition(sc.spatial_filter, ctx)) buf.push_back(vertexPk);
  Confidence: band=high; score=0.74
- Line 3730: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allPaths.emplace_back(std::move(pathResult.path));
  Confidence: band=high; score=0.74
- Line 3831: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4032: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4072: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryEngine::executeVectorGeoQuery(const VectorGeoQuery& q) const {
  Confidence: band=high; score=0.74
- Line 4091: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RangeAcc> rangeMap;
  Confidence: band=medium; score=0.66
- Line 4093: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> equalityMap;
  Confidence: band=medium; score=0.66
- Line 4163: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4220: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: nlohmann::json doc; try { std::string s(blobs[i]->begin(), blobs[i]->end()); doc = nlohmann::json::p
- Line 4227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4242: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return true; }
- Line 4252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tmp.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4270: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 4271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4294: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { continue; }
- Line 4347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4380: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> entityCache;
  Confidence: band=medium; score=0.66
- Line 4408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(r.primary_key);
  Confidence: band=high; score=0.74
- Line 4408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(r.primary_key);
  Confidence: band=high; score=0.74
- Line 4411: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* skip */ }
- Line 4442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(pk);
  Confidence: band=high; score=0.74
- Line 4446: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4460: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> tmpCache;
  Confidence: band=medium; score=0.66
- Line 4473: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (spatialOK && extraOK) { spatialCandidates.push_back(pk); tmpCache.try_emplace(pk, std::move(entity)); }
  Confidence: band=high; score=0.74
- Line 4475: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* skip */ }
- Line 4509: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(vgr));
  Confidence: band=high; score=0.74
- Line 4552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4582: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4596: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryEngine::executeContentGeoQuery(const ContentGeoQuery& q) const {
  Confidence: band=high; score=0.74
- Line 4644: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string,double> bm25; bm25.reserve(ftResults.size());
  Confidence: band=medium; score=0.66
- Line 4649: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: for(size_t bi=0; bi<buckets.size(); ++bi){ tg.run([&,bi](){ size_t start=bi*CHUNK; size_t end=std::m
- Line 4656: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string,nlohmann::json> cache;
  Confidence: band=medium; score=0.66
- Line 4666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for(size_t i=0;i<indexResults.size();++i){ if(!blobs[i].has_value()) continue; try { auto entity = BaseEntity::deserialize(indexResults[i].primary_key, *blobs[i]); nlohmann::json doc = nlohmann::json::parse(entity.toJson()); spatialCandidates.emplace_back(indexResults[i].primary_key); cache.emplace(indexResults[i].primary_key, std::move(doc));} catch (...) {} }
  Confidence: band=high; score=0.71
- Line 4667: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: for(size_t i=0;i<indexResults.size();++i){ if(!blobs[i].has_value()) continue; try { auto entity = B
- Line 4678: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tokenSet(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 4684: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { if (doc[q.text_field].is_string()) text = doc[q.text_field].get<std::string>(); else continue;
- Line 4685: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto docTokens = SecondaryIndexManager::tokenize(text); std::unordered_set<std::string> docSet(docTokens.begin(), docTokens.end());
  Confidence: band=medium; score=0.66
- Line 4689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 4689: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (q.boost_by_distance && q.center_point){ if (doc.contains(q.geom_field)){ nlohmann::json geom; if
- Line 4805: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cte_results.push_back(entity.toJson());
  Confidence: band=high; score=0.74
- Line 4848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cte_results.push_back(result.entity);
  Confidence: band=high; score=0.74
- Line 4882: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.orderedPredicates.push_back(pred);
  Confidence: band=high; score=0.74

### src/query/aql_translator.cpp
Total findings: 92

- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto varName = std::static_pointer_cast<VariableExpr>(sortExpr)->name;
- Line 108: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(sim->arguments[1]);
- Line 130: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto kLit = std::static_pointer_cast<LiteralExpr>(sim->arguments[2]);
- Line 138: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 175: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (prox->arguments[0]->getType() != ASTNodeType::FieldAccess) {
- Line 178: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 182: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: std::string geomField = extractColumnName(prox->arguments[0]);
- Line 183: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 183: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 242: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 299: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* fn  = static_cast<FunctionCallExpr*>(bin->left.get());
- Line 300: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* rhs = static_cast<LiteralExpr*>(bin->right.get());
- Line 313: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto* fa0 = static_cast<FieldAccessExpr*>(fn->arguments[0].get());
- Line 314: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto* fa1 = static_cast<FieldAccessExpr*>(fn->arguments[1].get());
- Line 319: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string var0 = static_cast<VariableExpr*>(fa0->object.get())->name;
- Line 320: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string var1 = static_cast<VariableExpr*>(fa1->object.get())->name;
- Line 407: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 445: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (args[0]->getType() != ASTNodeType::FieldAccess) {
- Line 448: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: if (args[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 452: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: std::string geomField = extractColumnName(args[0]);
- Line 453: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);
- Line 503: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 512: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 534: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 559: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto kLit = std::static_pointer_cast<LiteralExpr>(func->arguments[2]);
- Line 589: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 625: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (func->arguments[0]->getType() != ASTNodeType::FieldAccess) {
- Line 628: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 632: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Fix loop condition or increase array size
  Context: std::string geomField = extractColumnName(func->arguments[0]);
- Line 633: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Fix loop condition or increase array size
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 633: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 683: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 714: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 815: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto funcCall = std::static_pointer_cast<FunctionCallExpr>(filter->condition);
- Line 835: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 847: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 879: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto phraseLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 891: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 923: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 935: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 953: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[3]);
- Line 1014: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 1016: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
- Line 1097: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: findFulltext = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1121: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: findSpatial = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1173: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto binOp = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
- Line 1192: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[1]);
- Line 1203: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[2]);
- Line 1276: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(spatialFunc->arguments[2]);
- Line 1278: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
- Line 1443: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto literal = std::static_pointer_cast<LiteralExpr>(binOp->right);
- Line 1568: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, limit->offset));
- Line 1569: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, limit->count));
- Line 1618: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
- Line 1619: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
- Line 1627: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
- Line 1628: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
- Line 1638: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1639: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1647: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto eqExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, binOp->left, binOp->right);
- Line 1653: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gte, binOp->left, binOp->right);
- Line 1659: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lte, binOp->left, binOp->right);
- Line 1665: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1757: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1758: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1804: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 1817: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 449: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
- Line 455: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon,
- Line 629: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
- Line 635: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon,
- Line 761: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // If existing disjuncts = [A, B] and new disjuncts = [C, D], the result is
- Line 822: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("FULLTEXT() requires 2-3 arguments: FULLTEXT(column, query [, limit]
- Line 866: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("PHRASE() requires 2-3 arguments: PHRASE(column, phrase [, limit])")
- Line 910: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return TranslationResult::Error("FUZZY() requires 2-4 arguments: FUZZY(column, query [, maxDistance]
- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cte_executions.push_back(std::move(exec));
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extraPreds.push_back(cond);
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extraPreds.push_back(cond);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 607: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extraPreds.push_back(cond);
  Confidence: band=high; score=0.74
- Line 646: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
  Confidence: band=high; score=0.74
- Line 1038: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numericOnly.push_back(c);
  Confidence: band=high; score=0.74
- Line 1299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numericOnly.push_back(c);
  Confidence: band=high; score=0.74

### src/query/functions/fulltext_functions.cpp
Total findings: 65

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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3636 fix(query): build system au... (2026-03-12) | #3377 [WIP] Add highlight
- Line 209: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& t : tokenize(queryArg.get<std::string>()))
- Line 336: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 336: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 396: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 456: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 531: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 591: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 591: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 665: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 665: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 718: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 718: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 750: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 750: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 780: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 780: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 808: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 815: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string primary = mf.execute(args, ctx).get<std::string>();
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(current);
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ngrams.push_back(s.substr(i, n));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ngrams.push_back(s.substr(i, n));
- Line 103: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: while (result.length() < 4) result += '0';
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
- Line 142: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (next == 'H') { result += 'X'; i++; }
- Line 143: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (next == 'I' || next == 'E' || next == 'Y') result += 'S';
- Line 144: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else result += 'K';
- Line 147: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (next == 'G') { result += 'J'; i++; }
- Line 148: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else result += 'T';
- Line 150: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'F': result += 'F'; break;
- Line 153: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (next == 'I' || next == 'E' || next == 'Y') result += 'J';
- Line 154: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else result += 'K';
- Line 157: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i == 0 || !isVowel(upper[i - 1])) result += 'H';
- Line 159: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'J': result += 'J'; break;
- Line 198: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> queryTermSet(const json& queryArg) {
  Confidence: band=medium; score=0.66
- Line 199: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> terms;
  Confidence: band=medium; score=0.66
- Line 336: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back({{"_note", "FULLTEXT: no SecondaryIndexManager in context"},
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_error", st.message}, {"_collection", collection},
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
- Line 396: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 398: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
- Line 456: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
- Line 531: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 665: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 678: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> count1, count2;
  Confidence: band=medium; score=0.66
- Line 718: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 750: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=high; score=0.74
- Line 808: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74

### src/query/sparql_parser.cpp
Total findings: 45

- Line 871: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(tp.subject.value);
- Line 885: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(tp.predicate.value);
- Line 899: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = var_bindings.find(tp.object.value);
- Line 10: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // SPARQL compatibility layer – SELECT query parsing and AQL transpilation.
  Confidence: band=high; score=0.8
- Line 648: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->op   = "||";
- Line 649: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left = std::move(*left);
- Line 650: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = std::move(*right);
- Line 664: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->op   = "&&";
- Line 665: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left = std::move(*left);
- Line 666: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = std::move(*right);
- Line 689: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->op   = op;
- Line 690: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left = std::move(*left);
- Line 691: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = std::move(*right);
- Line 703: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->op      = "!";
- Line 704: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->operand = std::move(*operand);
- Line 722: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->name = current().value;
- Line 728: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->value = std::string(current().value);
- Line 734: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: try { node->value = std::stoll(current().value); }
- Line 743: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: try { node->value = std::stod(current().value); }
- Line 752: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->value = true;
- Line 758: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->value = false;
- Line 764: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->value = std::string(current().value);
- Line 870: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = var_bindings.find(tp.subject.value);
- Line 55: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')       out += "\\\"";
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')       out += "\\\"";
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') out += "\\r";
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') out += "\\t";
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::VAR,
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::URI, uri, start});
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::LTE, "<=", start});
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({SPARQLTokenType::LT, "<", start});
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readString(c, start));
- Line 261: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  val += '\n'; break;
- Line 262: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  val += '\r'; break;
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  val += '\t'; break;
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: default:   val += '\\'; val += esc; break;
- Line 405: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.variables.push_back(current().value);
- Line 779: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& var_bindings,
  Confidence: band=high; score=0.74
- Line 800: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& var_bindings) {
  Confidence: band=high; score=0.74
- Line 856: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> var_bindings;
  Confidence: band=high; score=0.74
- Line 872: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: constraints.push_back(t_var + ".subject == " + it->second);
  Confidence: band=high; score=0.74
- Line 873: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: constraints.push_back(t_var + ".subject == " + it->second);
- Line 878: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: constraints.push_back(

### src/query/cypher_parser.cpp
Total findings: 39

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5329 perf(query): PERF-06 â€” re... (2026-05-27) | #4400 [WIP] Add GNN-based
- Line 332: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 342: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 453: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 558: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 584: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
- Line 603: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
- Line 679: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 680: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "Expected NULL after IS [NOT]",
- Line 770: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 826: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CypherParseError{
- Line 1098: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: edge_var + "._type IN [" + type_list + "]");
- Line 158: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  s += '\n'; break;
- Line 159: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  s += '\t'; break;
- Line 160: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  s += '\r'; break;
- Line 301: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '.';
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '.';
- Line 361: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: CypherASTNode parseQuery() {
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.match_patterns.push_back(parsePathPattern());
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ast.match_patterns.push_back(parsePathPattern());
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.labels.push_back(tokens[cursor++].value);
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rel.types.push_back(tokens[cursor++].value);
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rel.types.push_back(expectIdent("as relationship type"));
- Line 758: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: list_literal += ", ";
- Line 857: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > start) expr_text += " ";
  Confidence: band=high; score=0.74
- Line 858: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > start) expr_text += " ";
- Line 866: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ast.return_items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > start) expr_text += " ";
  Confidence: band=high; score=0.74
- Line 880: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > start) expr_text += " ";
- Line 886: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ast.order_by.push_back(std::move(spec));
  Confidence: band=high; score=0.74
- Line 938: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"' || c == '\\') out += '\\';
  Confidence: band=high; score=0.74
- Line 939: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"' || c == '\\') out += '\\';
- Line 997: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!filter.empty()) filter += " AND ";
- Line 1054: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_clauses.push_back(std::move(node_filter));
  Confidence: band=high; score=0.74
- Line 1093: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) type_list += ", ";
  Confidence: band=high; score=0.74
- Line 1094: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) type_list += ", ";
- Line 1095: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: type_list += "\"" + rel.types[i] + "\"";
- Line 1096: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_clauses.push_back(
  Confidence: band=high; score=0.74
- Line 1097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filter_clauses.push_back(

### src/query/functions/process_mining_functions.cpp
Total findings: 38

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3636 fix(query): build system au... (2026-03-12) | #1100 [WIP] Fix missing a
- Line 76: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(name + ": function not implemented");
- Line 137: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (!ev.activity.empty() && act_to_id.find(ev.activity) == act_to_id.end()) {
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmFindSimilarFunction::execute(
- Line 278: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["results"] = json::array();
- Line 283: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmCompareIdealFunction::execute(
- Line 291: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["deviations"]     = json::array();
- Line 295: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmHasPatternFunction::execute(
- Line 305: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmExtractLogFunction::execute(
- Line 311: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmExtractTraceFunction::execute(
- Line 315: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["trace"] = json::array();
- Line 329: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmDiscoverProcessFunction::execute(
- Line 375: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmVariantsFunction::execute(
- Line 437: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmListAdminModelsFunction::execute(
- Line 450: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmConformanceFunction::execute(
- Line 499: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmDeviationsFunction::execute(
- Line 528: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmBottlenecksFunction::execute(
- Line 562: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmPredictEndFunction::execute(
- Line 588: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json PmExportBpmnFunction::execute(
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> act_to_id;
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.id_to_activity.push_back(ev.activity);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.id_to_activity.push_back(ev.activity);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: log.id_to_activity.push_back(ev.activity);
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(nj));
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proc.nodes.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proc.edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(vj));
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devs.push_back(d);
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b);
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 593: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 598: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 598: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 604: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
- Line 604: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");

### src/query/tensor_contraction_engine.cpp
Total findings: 36

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
- Line 119: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("TT-core merge: core dimension product overflows size_t");
- Line 174: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("TT-core kron: core dimension product overflows size_t");
- Line 397: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (va == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.mode_sizes.push_back(1);  // placeholder, removed below
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(contracted));
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(contracted));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(train.cores[k]);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(train.cores[k]);
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(std::move(ng));
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.cores.push_back(train.cores[k]);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto k : free_b) result_shape.push_back(shb[k]);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto k : free_b) ridx.push_back(idx_b[k]);
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scalar.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74

### src/query/adaptive_join.cpp
Total findings: 35

- Line 489: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(spec.left_key);
- Line 75: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Scan both sides.  Add sort cost for unsorted inputs.
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // --- AC-2: Merge Join — both inputs sorted on join key -------------------
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: spdlog::debug("AdaptiveJoin: MERGE_JOIN selected (both inputs sorted)");
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: spdlog::debug("AdaptiveJoin: SHUFFLE_JOIN selected (distributed, large inputs)");
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(build_key);
- Line 285: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto key_it = probe_row.find(probe_key);
- Line 289: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket_it = hash_table.find(key_it->second);
- Line 413: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto rk_it = right_row.find(spec.right_key);
  Confidence: band=very_high; score=0.9
- Line 441: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.right_key);
- Line 450: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lk_it = left_row.find(spec.left_key);
- Line 453: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket = index.find(lk_it->second);
- Line 488: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.left_key);
- Line 497: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.right_key);
- Line 497: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.right_key);
- Line 522: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket = ht.find(lk_it->second);
- Line 522: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bucket = ht.find(lk_it->second);
- Line 273: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<const RowValue*>> hash_table;
  Confidence: band=medium; score=0.66
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : right.rows) right_ptrs.push_back(&r);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, right_row));
  Confidence: band=high; score=0.74
- Line 439: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<const RowValue*>> index;
  Confidence: band=medium; score=0.66
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 491: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_parts[p].push_back(&row);
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: right_parts[p].push_back(&row);
- Line 513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[it->second].push_back(row);
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(mergeRows(*left_row, *right_row));
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rows.push_back(mergeRows(*left_row, *right_row));

### src/query/let_evaluator.cpp
Total findings: 34

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2853 [geo] Complete GeoJSON spec... (2026-03-12) | #2851 [geo] Implement ST_
- Line 87: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: path-based field access (supports array indices)
  Confidence: band=high; score=0.8
- Line 125: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: binary op with string operator
  Confidence: band=high; score=0.8
- Line 147: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: unary op with string operator
  Confidence: band=high; score=0.8
- Line 176: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: function call shim with functionName + arguments
  Confidence: band=high; score=0.8
- Line 185: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, valExpr] : objConstr->fields) {
- Line 229: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compat: numeric string treated as array index
  Confidence: band=high; score=0.8
- Line 326: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (rightNum == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (rightNum == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // These remain here for backward compatibility with custom EWKB parsing
  Confidence: band=high; score=0.8
- Line 621: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::runtime_error("ST_Within: g1 must be a GeoJSON Point or [x,y] array");
- Line 645: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (cross == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 699: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return (px == g2j["coordinates"][0].get<double>()
  Confidence: band=very_high; score=0.9
- Line 700: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: && py == g2j["coordinates"][1].get<double>());
  Confidence: band=very_high; score=0.9
- Line 1450: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
- Line 34: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
- Line 239: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 266: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evaluatedArgs.push_back(evaluateExpression(arg, currentDoc));
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 708: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1079: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords.push_back({x, y, z});
- Line 1081: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords.push_back({x, y});
- Line 1119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({x, y, z});
- Line 1121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({x, y});
- Line 1291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newCoords.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: newCoords.push_back(strip2D(pt));
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newRing.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newRing.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1302: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: newRing.push_back(strip2D(pt));
- Line 1486: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/gremlin_parser.cpp
Total findings: 33

- Line 298: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = opMap.find(opName);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5329 perf(query): PERF-06 â€” re... (2026-05-27) | #4400 [WIP] Add GNN-based
- Line 90: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': buf += '\n'; break;
- Line 91: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': buf += '\t'; break;
- Line 92: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': buf += '\r'; break;
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::FLOAT_LIT, num, start});
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::INT_LIT, num, start});
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::BOOL_TRUE, ident, start});
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::BOOL_FALSE, ident, start});
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({GremlinTokenType::IDENT, ident, start});
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vals.push_back(parseValue());
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.values.push_back(parseValue());
- Line 378: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.strings.push_back(peek().value);
- Line 612: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') out += "\\\"";
  Confidence: band=high; score=0.74
- Line 613: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') out += "\\\"";
- Line 614: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 645: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) list += ", ";
- Line 654: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) list += ", ";
- Line 729: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) labels.push_back(l);
  Confidence: band=high; score=0.74
- Line 735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
  Confidence: band=high; score=0.74
- Line 736: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
- Line 738: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(vVar + "." + key + " == " + valueToAQL(step.values[0]));
- Line 742: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(vVar + "." + key + " != null");
- Line 748: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filters.push_back(vVar + "." + step.strings[0] + " == null");
- Line 761: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
  Confidence: band=high; score=0.74
- Line 766: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) edgeLabels.push_back(l);
  Confidence: band=high; score=0.74
- Line 769: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& p : step.strings) valueProps.push_back(p);
  Confidence: band=high; score=0.74
- Line 773: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& p : step.strings) valueMapProps.push_back(p);
  Confidence: band=high; score=0.74
- Line 791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& a : step.strings) selectAliases.push_back(a);
  Confidence: band=high; score=0.74

### src/query/functions/lora_functions.cpp
Total findings: 32

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        std::string start_model = args[0].get<std::string>();', '        std::string end_model = args[1].get<std::string>();', '        int max_depth = args.size() > 2 ? args[2].get<int>() : 5;', '        (void)max_depth;', '']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        // Parse arguments', '        std::string adapter_id = args[0].get<std::string>();', '        int depth = args.size() > 1 ? args[1].get<int>() : 10;', '', '        // Get orchestrator']
  Confidence: band=very_high; score=0.93
- Line 150: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraTrainFunction::execute(
- Line 159: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: json dataset_json = args[2];
- Line 237: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraQueryFunction::execute(
- Line 312: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraSimilarFunction::execute(
- Line 447: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraPathFunction::execute(
- Line 524: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraStatsFunction::execute(
- Line 615: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraRecommendFunction::execute(
- Line 688: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: error["adapter_id"] = nullptr;
- Line 722: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraLineageFunction::execute(
- Line 790: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraProvenanceFunction::execute(
- Line 836: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraAuditLogFunction::execute(
- Line 935: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json LoraVerifyChainFunction::execute(
- Line 19: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 20: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.samples.push_back(s);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "LORA_QUERY('llama-2-7b', 'themis_help_lora', question.text, {max_tokens: 500})"
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> ws;
  Confidence: band=medium; score=0.66
- Line 400: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 489: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 750: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lineage.push_back(version);
  Confidence: band=high; score=0.74
- Line 756: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 802: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 851: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.toJSON());
- Line 855: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 898: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 899: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(s.toJSON());
- Line 902: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 824: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: "LORA_AUDIT_LOG('legal-lora-v2', 100)"
  Confidence: band=medium; score=0.6

### src/query/query_plan_visualizer.cpp
Total findings: 29

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: filter_node->estimated_rows = static_cast<size_t>(i) < plan.details.size()
- Line 96: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: filter_node->estimated_cost = 50.0 + 10.0 * static_cast<double>(i);
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: filter_node->type = PlanNodeType::Filter;
- Line 91: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: filter_node->description = pred.column + " == " + pred.value;
- Line 92: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: filter_node->selectivity = std::min(1.0, std::max(0.0, selectivity));
- Line 93: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: filter_node->estimated_rows = static_cast<size_t>(i) < plan.details.size()
- Line 96: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: filter_node->estimated_cost = 50.0 + 10.0 * static_cast<double>(i);
- Line 97: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: filter_node->attributes.push_back(pred.column);
- Line 107: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->type = PlanNodeType::IndexScan;
- Line 108: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->description = "IndexScan on " + query.table;
- Line 110: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->index_name = plan.orderedPredicates.front().column + "_idx";
- Line 112: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->type = PlanNodeType::SeqScan;
- Line 113: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->description = "SeqScan on " + query.table;
- Line 115: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->estimated_rows = plan.details.empty() ? 0
- Line 117: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: scan_node->estimated_cost = 200.0;
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_node->attributes.push_back(pred.column);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filter_node->attributes.push_back(pred.column);
- Line 141: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += std::string(4, ' ') + "<max depth exceeded>\n";
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: children_arr.push_back(toJSONImpl(*child, analyze, depth + 1));
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: children_arr.push_back(toJSONImpl(*child, analyze, depth + 1));
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  result += "\\\""; break;
- Line 271: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': result += "\\\\"; break;
- Line 272: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': result += "\\n";  break;
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': result += "\\r";  break;
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': result += "\\t";  break;
- Line 322: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: edges_out += "  n" + std::to_string(my_id)
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: edges_out += "  n" + std::to_string(my_id)

### src/query/optimizer_cost_model.cpp
Total findings: 27

- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.99
- Line 138: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.99
- Line 157: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.outputRows = static_cast<size_t>(inputRows * combinedSelectivity);
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // CPU cost: evaluate predicates for each input row
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.cpuCost = calculateCpuCost(inputRows, predicateCost);
  Confidence: band=very_high; score=0.99
- Line 235: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Sort both inputs
  Confidence: band=very_high; score=0.99
- Line 241: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Merge phase: single scan of both sorted inputs
  Confidence: band=very_high; score=0.99
- Line 258: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.99
- Line 265: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.99
- Line 270: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Cost: hash each input row and update aggregate
  Confidence: band=very_high; score=0.99
- Line 271: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double hashCost = calculateCpuCost(inputRows, constants_.cpuCostPerHash);
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double aggregateCost = static_cast<double>(inputRows * numAggregates) *
  Confidence: band=very_high; score=0.99
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.outputRows = static_cast<size_t>(inputRows * combinedSelectivity);
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // CPU cost: evaluate predicates for each input row
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.cpuCost = calculateCpuCost(inputRows, predicateCost);
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sort both inputs
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Sort both inputs
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Merge phase: single scan of both sorted inputs
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Merge phase: single scan of both sorted inputs
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t inputRows,
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cost.inputRows = inputRows;
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Cost: hash each input row and update aggregate
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double hashCost = calculateCpuCost(inputRows, constants_.cpuCostPerHash);
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double aggregateCost = static_cast<double>(inputRows * numAggregates) *
  Confidence: band=very_high; score=0.9
- Line 135: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, ColumnStatistics>& columnStats) const {
  Confidence: band=high; score=0.74

### src/query/aql_runner.cpp
Total findings: 26

- Line 265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: item["data"] = r.vertex_data;
- Line 829: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── SQL dialect compatibility layer ──────────────────────────────────────────
  Confidence: band=high; score=0.8
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(e.getPrimaryKey(), std::move(geom));
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(entityToResultRow(e));
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qr.rows.push_back(entityToResultRow(e));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.attributes.push_back("start: " + tv.startVertex);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("start: " + tv.startVertex);
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("depth: " + std::to_string(tv.minDepth) +
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("direction: " + dir_name);
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("algorithm: " + algo_name);
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.attributes.push_back("end: " + tv.endVertex);
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"pk", r.pk}, {"distance", r.vector_distance}, {"entity", r.entity}});
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"pk", r.pk}, {"distance", r.vector_distance}, {"entity", r.entity}});
- Line 572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& p : *res) arr.push_back(p);
  Confidence: band=high; score=0.74
- Line 624: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"vertex", r.vertex_pk}, {"depth", r.depth},
  Confidence: band=high; score=0.74
- Line 625: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"vertex", r.vertex_pk}, {"depth", r.depth},
- Line 648: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{sj.outer_var, p.key_a}, {sj.inner_var, p.key_b}, {"distance_m", p.distance_m}});
- Line 659: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& e : *res) arr.push_back(entityToResultRow(e));
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(stmtResult));
  Confidence: band=high; score=0.74

### src/query/query_federation.cpp
Total findings: 26

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4364 docs(query): rewrite ROADMA... (2026-03-21) | #4156 [WIP] Implement rea
- Line 26: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::shared_ptr<themis::sharding::ShardRouter> requireShardRouter(
- Line 269: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json QueryFederation::execute(const std::string& query) {    total_queries_++;
- Line 617: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = hash_table.find(key);
- Line 963: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Shard-key range [{}, {}] → {} shard(s)",
- Line 1013: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("QueryFederation: range-lookup [{},{}] → {} shard(s)",
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(value);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_results.push_back(std::move(rr));
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rr.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 537: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 866: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1024: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(s.shard_id);
  Confidence: band=high; score=0.74
- Line 1067: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_results.emplace_back(result);
  Confidence: band=high; score=0.74
- Line 1112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 1117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(row);
  Confidence: band=high; score=0.74
- Line 1142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paginated.push_back(result[i]);
  Confidence: band=high; score=0.74
- Line 1143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paginated.push_back(result[i]);

### src/query/query_optimizer.cpp
Total findings: 25

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4269 feat(timeseries): TSStore s... (2026-03-15) | #4166 feat(query): Wire S
- Line 119: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = table_stats_ptr->column_stats.find(p.column);
- Line 120: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it != table_stats_ptr->column_stats.end() &&
- Line 121: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: table_stats_ptr->row_count > 0) {
- Line 124: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<double>(table_stats_ptr->row_count));
- Line 159: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? table_stats_ptr->row_count
- Line 159: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? table_stats_ptr->row_count
- Line 163: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? static_cast<size_t>(table_stats_ptr->avg_row_size_bytes > 0.0
- Line 163: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? static_cast<size_t>(table_stats_ptr->avg_row_size_bytes > 0.0
- Line 164: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? table_stats_ptr->avg_row_size_bytes
- Line 164: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? table_stats_ptr->avg_row_size_bytes
- Line 579: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(plan.recommended_parallelism, size_t(8)); ++i) {
- Line 765: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = table_stats_ptr->column_stats.find(pred.column);
- Line 766: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it != table_stats_ptr->column_stats.end()) {
- Line 94: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryOptimizer::Plan QueryOptimizer::chooseOrderForAndQuery(const ConjunctiveQuery& q, size_t maxProbePerPred) const {
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.details.push_back(Estimation{p, cnt, capped});
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.details.push_back(Estimation{p, cnt, capped});
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto i : idx) plan.orderedPredicates.push_back(plan.details[i].pred);
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_infos.push_back(info);
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pruned_shards.push_back(info.shard_id);
  Confidence: band=high; score=0.74
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pruned_shards.push_back(info.shard_id);
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.preferred_cpu_affinity.push_back(static_cast<int>(i));
  Confidence: band=high; score=0.74
- Line 580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.preferred_cpu_affinity.push_back(static_cast<int>(i));
- Line 389: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: vectorSearchCost = std::log(static_cast<double>(universe) + 1.0) * dimScale; // ANN approximation
  Confidence: band=medium; score=0.6
- Line 418: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double ftPhase = C_fulltext_base * std::log(static_cast<double>(hits) + 5.0);
  Confidence: band=medium; score=0.6

### src/query/functions/ethics_functions.cpp
Total findings: 24

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    [[maybe_unused]] const std::string& philosophy = args[0];', '    [[maybe_unused]] const json& types = args.size() > 1 ? args[1] : json::array();', '    [[maybe_unused]] int limit = args.size() > 2 ? args[2].get<int>() : 20;', '', '    // F-028: throw so the AQL runtime surfaces a real error instead of']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    [[maybe_unused]] const std::string& query_text = args[0];', '    [[maybe_unused]] double threshold = args.size() > 1 ? args[1].get<double>() : 0.65;', '    [[maybe_unused]] int limit = args.size() > 2 ? args[2].get<int>() : 10;', '', '    // F-028: throw instead of silent empty array.']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    // Requires the ethics_ai plugin to create the ethics_arguments_graph.', '    [[maybe_unused]] const std::string& start_id = args[0];', '    [[maybe_unused]] int max_depth = args.size() > 1 ? args[1].get<int>() : 5;', '', '    // F-028: throw instead of silent empty array.']
  Confidence: band=very_high; score=0.9
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3574 fix: clear all remaining st... (2026-03-12) | #946 [FEATURE] Ethics AI
- Line 43: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsMakeDecisionFunction::execute(
- Line 51: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["decision_id"] = "decision_" + std::to_string(std::time(nullptr));
- Line 66: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsInitializeDebateFunction::execute(
- Line 71: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["debate_id"] = "debate_" + std::to_string(std::time(nullptr));
- Line 85: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsEvaluateFunction::execute(
- Line 112: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsEvaluateDimensionFunction::execute(
- Line 121: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json full_eval = EthicsEvaluateFunction().execute(eval_args, ctx);
- Line 155: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 180: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 200: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 209: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsLoadProfileFunction::execute(
- Line 227: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsListSchoolsFunction::execute(
- Line 254: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsBuildContextFunction::execute(
- Line 277: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsStatsFunction::execute(
- Line 296: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json EthicsMetricsFunction::execute(
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "kant"}, {"available", true}});
- Line 236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "utilitarianism"}, {"available", true}});
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "virtue_ethics"}, {"available", true}});
- Line 238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "contractualism"}, {"available", true}});
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schools.push_back({{"name", "rationalism"}, {"available", true}});

### src/query/aql_parser.cpp
Total findings: 22

- Line 860: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: // std::cerr << "parseComparison current token: " << (int)current().type << " value='" << current().value << "'\n";
  Confidence: band=very_high; score=0.92
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4140 feat(security): AQLInjectio... (2026-03-12) | #3481 [WIP] Synchronize A
- Line 426: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // One or more FOR clauses (first is also stored in for_node for backward compat)
  Confidence: band=high; score=0.8
- Line 491: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: also accept SHORTEST_PATH after RETURN
  Confidence: band=high; score=0.8
- Line 631: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Still return a ForNode for compatibility (collection = "graph")
  Confidence: band=high; score=0.8
- Line 735: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!withNode->ctes.empty()) {
- Line 759: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: withNode->ctes.push_back(std::move(cte));
- Line 779: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->groups.emplace_back(var, expr);
- Line 810: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->aggregations.push_back(std::move(ag));
- Line 167: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': value += '\n'; break;
- Line 168: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': value += '\t'; break;
- Line 169: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': value += '\r'; break;
- Line 170: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': value += '"'; break;
- Line 171: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': value += '\''; break;
- Line 172: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': value += '\\'; break;
- Line 340: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: auto query = parseQuery(false); // false = not a subquery
  Confidence: band=high; score=0.74
- Line 418: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::shared_ptr<Query> parseQuery(bool isSubquery = false) {
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query->for_nodes.push_back(query->for_node);
- Line 1387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::string(1, ch));
- Line 1553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args.push_back(std::stoll(t));
- Line 1554: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/parallel_executor.cpp
Total findings: 22

- Line 212: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 278: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 313: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 372: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: tg.wait();
- Line 198: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 348: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: arena.execute([&]() {
  Confidence: band=very_high; score=0.9
- Line 70: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!key.empty()) key += '|';
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!key.empty()) key += '|';
- Line 75: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ':';
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({l, *it->second});
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({l, *it->second});
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({l, *it->second});
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto k = rows[i].getFieldAsString(key_field);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: morsel_partitions[m][slot].push_back(rows[i]);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: morsel_partitions[m][slot].push_back(rows[i]);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: morsel_partitions[m][slot].push_back(rows[i]);
- Line 297: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: arena.execute([&]() {
  Confidence: band=high; score=0.74

### src/query/functions/tensor_functions.cpp
Total findings: 21

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['                "Marginalize a tensor over one mode: PROJECT(t, mode). "', '                "Sums over all indices along mode, returning a train of order (d-1). "', '                "Operates entirely in the compressed domain (O(d*n*r^2)). "', '                "Ref: tensor marginalization, paper §AQL operators.",', '            .arguments = {']
  Confidence: band=very_high; score=0.9
- Line 15: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: *   1) inline objects {data:[...], shape:[...], eps?:...}
- Line 61: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto next = path.find('.', pos);
- Line 139: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->is_object() &&
- Line 140: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->contains("data") && ptr->contains("shape")) {
- Line 177: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [train, stats] = dec.decompose(data, shape, cfg);
- Line 192: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: "Arguments: two objects {data:[...], shape:[...], eps:float}. "
- Line 212: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 247: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 280: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 328: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 376: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 487: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json execute(const std::vector<json>& args,
- Line 77: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : shape_arr) shape.push_back(s.get<std::size_t>());
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : args[2]) modes_a.push_back(v.get<std::size_t>());
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& v : args[3]) modes_b.push_back(v.get<std::size_t>());
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : args[3]) modes_b.push_back(v.get<std::size_t>());
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : args[0]) data.push_back(v.get<float>());
- Line 553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());

### src/query/cte_subquery.cpp
Total findings: 20

- Line 712: severity=CRITICAL; category=data_race
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
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, val] : obj->fields) {
- Line 390: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<nlohmann::json> SubqueryEvaluator::evaluateSubquery(
- Line 410: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (isCorrelatedSubquery(subquery.subquery, outerVarNames)) {
- Line 413: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return evaluateArraySubquery(subquery.subquery, queryEngine, outerRow);
- Line 418: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return evaluateScalarSubquery(subquery.subquery, queryEngine, outerRow);
- Line 421: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<nlohmann::json> SubqueryEvaluator::evaluateScalarSubquery(
- Line 517: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<nlohmann::json> SubqueryEvaluator::evaluateArraySubquery(
- Line 599: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> SubqueryEvaluator::evaluateInSubquery(
- Line 692: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<bool> SubqueryEvaluator::evaluateExistsSubquery(
- Line 367: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& outerVarNames
  Confidence: band=medium; score=0.66
- Line 405: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> outerVarNames;
  Confidence: band=medium; score=0.66
- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entityToJSON(entity));
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entityToJSON(entity));
  Confidence: band=high; score=0.74
- Line 585: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entityToJSON(entity));
  Confidence: band=high; score=0.74

### src/query/cross_cluster_federation.cpp
Total findings: 15

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3350 [query] Cross-cluster feder... (2026-03-12)
- Line 162: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lat_it = latency_cache_.find(id);
- Line 231: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(selected.begin(), selected.end(), ep.cluster_id) !=
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.selected_clusters.push_back(est.cluster_id);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.selected_clusters.push_back(est.cluster_id);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.selected_clusters.push_back(est.cluster_id);
- Line 231: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cluster_list.push_back(c);
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: endpoint.cluster_id, status_code);
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74

### src/query/sql_parser.cpp
Total findings: 15

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3427 feat(query): Per-query reso... (2026-03-12) | #3352 feat(query): SPARQL
- Line 10: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // SQL dialect compatibility layer – SELECT/INSERT/UPDATE/DELETE passthrough.
  Confidence: band=high; score=0.8
- Line 54: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  out += "\\\"";
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  out += "\\\"";
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') out += "\\r";
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') out += "\\t";
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readString(c, start));
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readNumber(start));
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(readIdent(start));
- Line 285: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n': val += '\n'; break;
- Line 286: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r': val += '\r'; break;
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't': val += '\t'; break;
- Line 543: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stmt.columns.push_back(current().value);

### src/query/result_type_annotation.cpp
Total findings: 14

- Line 182: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = row.begin(); it != row.end(); ++it) {
- Line 86: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (std::isfinite(d) && d == std::floor(d) &&
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (seen_fields.find(fname) == seen_fields.end()) {
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (row.find(fname) == row.end()) {
  Confidence: band=very_high; score=0.9
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(f.toJson());
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(f.toJson());
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(f.toJson());
- Line 175: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ResultFieldType> field_types;
  Confidence: band=medium; score=0.66
- Line 176: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>                  nullable_fields;
  Confidence: band=medium; score=0.66
- Line 177: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>                  seen_fields;
  Confidence: band=medium; score=0.66
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_order.push_back(fname);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_order.push_back(fname);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.fields.push_back(std::move(ann));
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.fields.push_back(std::move(ann));
  Confidence: band=high; score=0.74

### src/query/window_evaluator.cpp
Total findings: 14

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
- Line 30: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (expr) partBy.push_back(expr->toJSON());
  Confidence: band=high; score=0.74
- Line 31: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (expr) partBy.push_back(expr->toJSON());
- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordBy.push_back(spec.toJSON());
  Confidence: band=high; score=0.74
- Line 37: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ordBy.push_back(spec.toJSON());
- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(indices));
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(currentRank);
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(currentDenseRank);
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(defaultVal);
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(defaultVal);
  Confidence: band=high; score=0.74

### src/query/materialized_view.cpp
Total findings: 12

- Line 143: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new MaterializedView(def, config));
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 372: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(filter_field);
- Line 512: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != views_.end()) ? it->second : nullptr;
- Line 99: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!s.empty()) s += ',';
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!s.empty()) s += ',';
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!s.empty()) s += ',';
- Line 191: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: MaterializedView::isStale()
  Context: bool MaterializedView::isStale() const {
  Confidence: band=medium; score=0.56
- Line 374: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(row);
  Confidence: band=high; score=0.74
- Line 498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_index_[table].push_back(name);
  Confidence: band=high; score=0.74
- Line 650: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_refresh.push_back(view);
  Confidence: band=high; score=0.74

### src/query/materialized_cte.cpp
Total findings: 11

- Line 182: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = vrow.values.find(agg.output_name);
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: MaterializedCTEResult MaterializedCTEView::query(
- Line 228: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto vqr = view_->query({}, limit, offset);
- Line 329: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: MaterializedCTEResult MaterializedCTERegistry::query(
- Line 340: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return it->second->query(limit, offset);
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vd.aggregations.push_back(spec);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vd.base_filters.push_back(vf);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back({std::move(obj)});
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back({std::move(obj)});
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back({std::move(obj)});
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recs.push_back(toChangeRecord(c));
  Confidence: band=high; score=0.74

### src/query/vectorized_execution.cpp
Total findings: 11

- Line 150: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<std::vector<nlohmann::json>> VectorizedExecutionEngine::execute(
- Line 177: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch out   = analytics_engine.execute(batch, pipeline);
- Line 216: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 225: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 234: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 243: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(rows, plan);
- Line 393: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: obj[name] = nullptr;
- Line 259: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> col_index;
  Confidence: band=medium; score=0.66
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(key);
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(key);
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back({sk.field, sk.ascending});
  Confidence: band=high; score=0.74

### src/query/query_rewrite_rule.cpp
Total findings: 10

- Line 94: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator v may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto v = eq.find("value");
- Line 341: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: else if (t == "div" && rv != 0.0) result = lv / rv;
  Confidence: band=very_high; score=0.9
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filters.push_back(std::move(child));
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scan["children"].push_back(std::move(f));
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: in_expr["values"].push_back(std::move(v));
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> seen;
  Confidence: band=medium; score=0.66
- Line 423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (rule) rules_.push_back(std::move(rule));
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.applied_rule_names.push_back(rule->name());
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.applied_rule_names.push_back(rule->name());
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.applied_rule_names.push_back(rule->name());

### src/query/continuous_query_engine.cpp
Total findings: 9

- Line 73: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return cancelled_.load(std::memory_order_acquire);
- Line 126: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(loop_mutex_);
- Line 254: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> inj_lock(inject_mutex_);
- Line 149: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ContinuousQueryEngineImpl::registerQuery(ContinuousQuerySpec spec) {
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> ContinuousQueryEngineImpl::dropQuery(const std::string& name) {
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: it->second.subscribers.push_back(std::move(queue));
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: inject_queue_.push_back({collection, tuple, event_ts});

### src/query/query_cache.cpp
Total findings: 9

- Line 264: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 325: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 493: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(fingerprint);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 288: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(cache_mutex_);
- Line 579: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = dependency_index_.find(dep);
- Line 287: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: QueryCache::clear()
  Context: Result<void> QueryCache::clear() {
  Confidence: band=medium; score=0.56
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(fingerprint);
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependency_index_[dep].push_back(fingerprint);
  Confidence: band=high; score=0.74

### src/query/functions/udf_registry.cpp
Total findings: 8

- Line 172: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json UdfFunction::execute(
- Line 264: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": division by zero");
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": modulo by zero");
  Confidence: band=very_high; score=0.9
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args_json.push_back({
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args_json.push_back({
- Line 229: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: callArgs.push_back(evalExpr(a, args, context, depth + 1));
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: callArgs.push_back(evalExpr(a, args, context, depth + 1));
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/query/plan_cache.cpp
Total findings: 7

- Line 213: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = cache_.begin(); it != cache_.end(); ++it) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4229 feat(query): Query Plan Cac... (2026-03-15) | #3226 [graph] Register pa
- Line 186: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cache_.find(fp);
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_index_[tbl].push_back(fp);
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_map<std::string, Entry>::iterator> to_remove;
  Confidence: band=medium; score=0.66
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(it);
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Entry>::iterator it)
  Confidence: band=medium; score=0.66

### src/query/tensor_aware_query_optimizer.cpp
Total findings: 7

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
- Line 225: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (upper_desc.find(fn) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 208: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/adaptive_optimizer.cpp
Total findings: 6

- Line 509: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pthread_t thread = pthread_self();
- Line 510: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) == 0;
- Line 400: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.indexes_to_use.push_back(idx.index_name);
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.indexes_to_use.push_back(idx.index_name);
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: placement.cpu_affinity.push_back(i);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: placement.cpu_affinity.push_back(i);
  Confidence: band=high; score=0.74

### src/query/aql_parser_json.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3427 feat(query): Per-query reso... (2026-03-12) | #3352 feat(query): SPARQL
- Line 45: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: {"object", object->toJSON()},
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args_json.push_back(arg->toJSON());
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: args_json.push_back(arg->toJSON());
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elems_json.push_back(elem->toJSON());
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elems_json.push_back(elem->toJSON());

### src/query/workload_cache_strategy.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 444: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(limit, query_frequencies.size()); ++i) {
- Line 444: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hot_queries.push_back(query_frequencies[i].first);
  Confidence: band=high; score=0.74

### src/query/approximate_aggregator.cpp
Total findings: 5

- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: registers_[static_cast<size_t>(i)] = o->registers_[static_cast<size_t>(i)];
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(c);
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids_.push_back(c);
  Confidence: band=high; score=0.74
- Line 105: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(static_cast<double>(num_registers_) /
  Confidence: band=medium; score=0.6

### src/query/cte_cache.cpp
Total findings: 5

- Line 208: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: total_estimate += (sizeof(nlohmann::json) + 64) * data.size();
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 239: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(nlohmann::json::parse(serialized));
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();

### src/query/query_cache_manager.cpp
Total findings: 5

- Line 133: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_result = basic_cache_->get(query, params);
- Line 135: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = cache_result->result;
- Line 140: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_entry = adaptive_cache_->get(fingerprint, "");
- Line 142: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = cache_entry->result;
- Line 528: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto put_result = basic_cache_->put(query, params, result, dependencies, ttl);

### src/query/query_compiler.cpp
Total findings: 5

- Line 21: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: *     execute() delegates to the ExecuteFn supplied at compile() time —
- Line 161: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<QueryResult> execute(
- Line 385: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<QueryResult> QueryCompiler::execute(
- Line 389: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(compiled, params);
- Line 344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/query/semantic_cache.cpp
Total findings: 4

- Line 455: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(queryStr);
- Line 216: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 548: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(tokens.begin(), tokens.end(), kw) != tokens.end()) {
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(current);
  Confidence: band=high; score=0.74

### src/query/aql_safety_validator.cpp
Total findings: 3

- Line 125: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: entire = nullptr;
  Context: "enforce_read_only context. This pattern can delete entire "
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::toupper(c)));
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "enforce_read_only context. This pattern can delete entire "

### src/query/continuous_query_planner.cpp
Total findings: 3

- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({t.payload, /*is_retract=*/true});
  Confidence: band=high; score=0.74
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({t.payload, /*is_retract=*/true});
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({t.payload, false});
  Confidence: band=high; score=0.74

### src/query/query_canceller.cpp
Total findings: 3

- Line 38: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto token = it->second.lock();
- Line 23: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryCanceller::registerQuery(const std::string& request_id) {
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void QueryCanceller::unregisterQuery(const std::string& request_id) {
  Confidence: band=high; score=0.74

### src/query/statistical_aggregator.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    if (lowerIndex == upperIndex) {', '        return Ok(nlohmann::json(values[lowerIndex]));', '    }', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Linear interpolation', '    double weight = rank - lowerIndex;', '    double result = values[lowerIndex] * (1.0 - weight) + values[upperIndex] * weight;', '', '    return Ok(nlohmann::json(result));']
  Confidence: band=high; score=0.81
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(val.get<double>());
  Confidence: band=high; score=0.74

### src/query/cq_watermark.cpp
Total findings: 2

- Line 21: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: int64_t current_max = max_seen_us_.load(std::memory_order_relaxed);
- Line 30: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const int64_t wm = watermark_us_.load(std::memory_order_acquire);

### src/query/result_stream.cpp
Total findings: 2

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/query/functions/function_registry.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4220 feat(aql): wire detectInten... (2026-03-14) | #2758 [analytics] Advance

### src/query/query_profiler.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12) | #3328 [WIP] Add SLO/SLA c

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
