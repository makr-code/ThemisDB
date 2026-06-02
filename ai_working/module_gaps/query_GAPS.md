# query Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: query
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 428
- Actionable Findings (Critical + High): 86
- Affected Files: 50

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 13 |
| High | 73 |
| Medium | 338 |
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
| src/query/query_engine.cpp | 87 | 0 | 12 | 75 | 0 |
| src/query/functions/fulltext_functions.cpp | 32 | 0 | 10 | 22 | 0 |
| src/query/optimizer_cost_model.cpp | 27 | 12 | 14 | 1 | 0 |
| src/query/tensor_contraction_engine.cpp | 22 | 0 | 1 | 21 | 0 |
| src/query/adaptive_join.cpp | 20 | 0 | 5 | 15 | 0 |
| src/query/query_federation.cpp | 18 | 0 | 0 | 18 | 0 |
| src/query/let_evaluator.cpp | 17 | 0 | 11 | 6 | 0 |
| src/query/parallel_executor.cpp | 15 | 0 | 4 | 11 | 0 |
| src/query/aql_runner.cpp | 13 | 0 | 1 | 12 | 0 |
| src/query/functions/process_mining_functions.cpp | 13 | 0 | 1 | 12 | 0 |
| src/query/aql_translator.cpp | 12 | 0 | 0 | 12 | 0 |
| src/query/result_type_annotation.cpp | 12 | 0 | 3 | 9 | 0 |
| src/query/cross_cluster_federation.cpp | 10 | 0 | 1 | 9 | 0 |
| src/query/cypher_parser.cpp | 10 | 0 | 0 | 10 | 0 |
| src/query/functions/lora_functions.cpp | 10 | 0 | 0 | 9 | 1 |
| src/query/gremlin_parser.cpp | 8 | 0 | 0 | 8 | 0 |
| src/query/query_rewrite_rule.cpp | 8 | 0 | 1 | 7 | 0 |
| src/query/materialized_cte.cpp | 7 | 0 | 1 | 6 | 0 |
| src/query/query_optimizer.cpp | 7 | 0 | 0 | 5 | 2 |
| src/query/window_evaluator.cpp | 7 | 0 | 0 | 7 | 0 |
| src/query/aql_parser.cpp | 6 | 1 | 3 | 2 | 0 |
| src/query/cte_subquery.cpp | 6 | 0 | 0 | 6 | 0 |
| src/query/materialized_view.cpp | 6 | 0 | 0 | 6 | 0 |
| src/query/query_plan_visualizer.cpp | 6 | 0 | 0 | 6 | 0 |
| src/query/sparql_parser.cpp | 6 | 0 | 1 | 5 | 0 |
| src/query/continuous_query_engine.cpp | 5 | 0 | 0 | 5 | 0 |
| src/query/functions/udf_registry.cpp | 5 | 0 | 2 | 3 | 0 |
| src/query/plan_cache.cpp | 4 | 0 | 0 | 4 | 0 |
| src/query/vectorized_execution.cpp | 4 | 0 | 0 | 4 | 0 |
| src/query/adaptive_optimizer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/query/approximate_aggregator.cpp | 3 | 0 | 0 | 2 | 1 |
| src/query/query_cache.cpp | 3 | 0 | 0 | 3 | 0 |
| src/query/aql_parser_json.cpp | 2 | 0 | 0 | 2 | 0 |
| src/query/continuous_query_planner.cpp | 2 | 0 | 0 | 2 | 0 |
| src/query/functions/tensor_functions.cpp | 2 | 0 | 0 | 2 | 0 |
| src/query/query_canceller.cpp | 2 | 0 | 0 | 2 | 0 |
| src/query/sql_parser.cpp | 2 | 0 | 1 | 1 | 0 |
| src/query/aql_safety_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/query/cte_cache.cpp | 1 | 0 | 0 | 1 | 0 |
| src/query/semantic_cache.cpp | 1 | 0 | 0 | 1 | 0 |
| src/query/statistical_aggregator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/query/tensor_aware_query_optimizer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/query/workload_cache_strategy.cpp | 1 | 0 | 0 | 1 | 0 |
| src/query/cq_watermark.cpp | 0 | 0 | 0 | 0 | 0 |
| src/query/functions/ethics_functions.cpp | 0 | 0 | 0 | 0 | 0 |
| src/query/functions/function_registry.cpp | 0 | 0 | 0 | 0 | 0 |
| src/query/query_cache_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/query/query_compiler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/query/query_profiler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/query/result_stream.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/query/query_engine.cpp
Total findings: 87

- Line 812: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 977: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < q.disjuncts.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 1054: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1254: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < end; ++i) {
  Confidence: band=very_high; score=0.9
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
- Line 4540: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = entityCache.find(pk);
  Confidence: band=very_high; score=0.9
- Line 4699: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sa == sb) {
  Confidence: band=very_high; score=0.9
- Line 115: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
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
- Line 815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(result.error().context());
  Confidence: band=high; score=0.74
- Line 985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(result.error().message());
  Confidence: band=high; score=0.74
- Line 1057: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
- Line 1640: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coords = g["coordinates"];
  Confidence: band=high; score=0.74
- Line 1983: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: else if (t=="Polygon"||t=="MultiLineString") { nlohmann::json nr=nlohmann::json::array(); for (const auto& ring : g["coordinates"]) { nlohmann::json r=nlohmann::json::array(); for (const auto& pt : ring) r.push_back(strip2D(pt)); nr.push_back(r);} result["coordinates"]=nr; }
  Confidence: band=high; score=0.71
- Line 2172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_fields.emplace_back(k, e);
  Confidence: band=high; score=0.74
- Line 2355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matchesPredicates(e)) out.emplace_back(std::move(entry.pk));
  Confidence: band=high; score=0.74
- Line 2381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matchesPredicates(e)) local.emplace_back(std::move(entry.pk));
  Confidence: band=high; score=0.74
- Line 2505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
  Confidence: band=high; score=0.74
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
- Line 3044: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_probe_docs.emplace_back(std::move(doc));
  Confidence: band=high; score=0.74
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
- Line 3271: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> groups;
  Confidence: band=medium; score=0.66
- Line 3307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_it->second.push_back(doc);
  Confidence: band=high; score=0.74
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
- Line 3572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertexKeys.emplace_back(table + ":" + vertexPk);
  Confidence: band=high; score=0.74
- Line 3664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertexKeys.emplace_back(table + ":" + vertexPk);
  Confidence: band=high; score=0.74
- Line 3688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (evaluateCondition(sc.spatial_filter, ctx)) buf.push_back(vertexPk);
  Confidence: band=high; score=0.74
- Line 3730: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allPaths.emplace_back(std::move(pathResult.path));
  Confidence: band=high; score=0.74
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
- Line 4252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tmp.emplace_back(pk, d);
  Confidence: band=high; score=0.74
- Line 4271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::move(r));
  Confidence: band=high; score=0.74
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
- Line 4442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spatialCandidates.push_back(pk);
  Confidence: band=high; score=0.74
- Line 4460: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> tmpCache;
  Confidence: band=medium; score=0.66
- Line 4473: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (spatialOK && extraOK) { spatialCandidates.push_back(pk); tmpCache.try_emplace(pk, std::move(entity)); }
  Confidence: band=high; score=0.74
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
- Line 4656: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string,nlohmann::json> cache;
  Confidence: band=medium; score=0.66
- Line 4666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for(size_t i=0;i<indexResults.size();++i){ if(!blobs[i].has_value()) continue; try { auto entity = BaseEntity::deserialize(indexResults[i].primary_key, *blobs[i]); nlohmann::json doc = nlohmann::json::parse(entity.toJson()); spatialCandidates.emplace_back(indexResults[i].primary_key); cache.emplace(indexResults[i].primary_key, std::move(doc));} catch (...) {} }
  Confidence: band=high; score=0.71
- Line 4678: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tokenSet(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
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

### src/query/functions/fulltext_functions.cpp
Total findings: 32

- Line 336: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 591: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 665: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 718: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 750: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 780: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
  Confidence: band=very_high; score=0.9
- Line 808: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(current);
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ngrams.push_back(s.substr(i, n));
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: while (result.length() < 4) result += '0';
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
  Confidence: band=high; score=0.74
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
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({{"_key", r.pk}, {"_score", r.score}});
  Confidence: band=high; score=0.74
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
- Line 456: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto collection = args[0].get<std::string>();
  Confidence: band=high; score=0.74
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

### src/query/tensor_contraction_engine.cpp
Total findings: 22

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
Total findings: 20

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
- Line 413: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto rk_it = right_row.find(spec.right_key);
  Confidence: band=very_high; score=0.9
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

### src/query/query_federation.cpp
Total findings: 18

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

### src/query/let_evaluator.cpp
Total findings: 17

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
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(evaluateExpression(elemExpr, currentDoc));
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evaluatedArgs.push_back(evaluateExpression(arg, currentDoc));
  Confidence: band=high; score=0.74
- Line 1291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newCoords.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newRing.push_back(strip2D(pt));
  Confidence: band=high; score=0.74
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newRing.push_back(strip2D(pt));
  Confidence: band=high; score=0.74

### src/query/parallel_executor.cpp
Total findings: 15

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

### src/query/aql_runner.cpp
Total findings: 13

- Line 829: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── SQL dialect compatibility layer ──────────────────────────────────────────
  Confidence: band=high; score=0.8
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(e.getPrimaryKey(), std::move(geom));
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({
  Confidence: band=high; score=0.74
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
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qr.rows.push_back(entityToResultRow(e));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.attributes.push_back("start: " + tv.startVertex);
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"pk", r.pk}, {"distance", r.vector_distance}, {"entity", r.entity}});
  Confidence: band=high; score=0.74
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
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(stmtResult));
  Confidence: band=high; score=0.74

### src/query/functions/process_mining_functions.cpp
Total findings: 13

- Line 137: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (!ev.activity.empty() && act_to_id.find(ev.activity) == act_to_id.end()) {
  Confidence: band=very_high; score=0.9
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

### src/query/aql_translator.cpp
Total findings: 12

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

### src/query/result_type_annotation.cpp
Total findings: 12

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

### src/query/cross_cluster_federation.cpp
Total findings: 10

- Line 231: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.selected_clusters.push_back(est.cluster_id);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.selected_clusters.push_back(est.cluster_id);
  Confidence: band=high; score=0.74
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
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74

### src/query/cypher_parser.cpp
Total findings: 10

- Line 301: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '.';
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: CypherASTNode parseQuery() {
  Confidence: band=high; score=0.74
- Line 857: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > start) expr_text += " ";
  Confidence: band=high; score=0.74
- Line 866: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ast.return_items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > start) expr_text += " ";
  Confidence: band=high; score=0.74
- Line 886: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ast.order_by.push_back(std::move(spec));
  Confidence: band=high; score=0.74
- Line 938: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"' || c == '\\') out += '\\';
  Confidence: band=high; score=0.74
- Line 1054: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_clauses.push_back(std::move(node_filter));
  Confidence: band=high; score=0.74
- Line 1093: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) type_list += ", ";
  Confidence: band=high; score=0.74
- Line 1096: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_clauses.push_back(
  Confidence: band=high; score=0.74

### src/query/functions/lora_functions.cpp
Total findings: 10

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
- Line 750: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lineage.push_back(version);
  Confidence: band=high; score=0.74
- Line 850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 898: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 824: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: "LORA_AUDIT_LOG('legal-lora-v2', 100)"
  Confidence: band=medium; score=0.6

### src/query/gremlin_parser.cpp
Total findings: 8

- Line 612: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') out += "\\\"";
  Confidence: band=high; score=0.74
- Line 729: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& l : step.strings) labels.push_back(l);
  Confidence: band=high; score=0.74
- Line 735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
  Confidence: band=high; score=0.74
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

### src/query/query_rewrite_rule.cpp
Total findings: 8

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

### src/query/materialized_cte.cpp
Total findings: 7

- Line 182: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = vrow.values.find(agg.output_name);
  Confidence: band=very_high; score=0.9
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

### src/query/query_optimizer.cpp
Total findings: 7

- Line 94: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryOptimizer::Plan QueryOptimizer::chooseOrderForAndQuery(const ConjunctiveQuery& q, size_t maxProbePerPred) const {
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.details.push_back(Estimation{p, cnt, capped});
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_infos.push_back(info);
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pruned_shards.push_back(info.shard_id);
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.preferred_cpu_affinity.push_back(static_cast<int>(i));
  Confidence: band=high; score=0.74
- Line 389: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: vectorSearchCost = std::log(static_cast<double>(universe) + 1.0) * dimScale; // ANN approximation
  Confidence: band=medium; score=0.6
- Line 418: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double ftPhase = C_fulltext_base * std::log(static_cast<double>(hits) + 5.0);
  Confidence: band=medium; score=0.6

### src/query/window_evaluator.cpp
Total findings: 7

- Line 30: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (expr) partBy.push_back(expr->toJSON());
  Confidence: band=high; score=0.74
- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordBy.push_back(spec.toJSON());
  Confidence: band=high; score=0.74
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

### src/query/aql_parser.cpp
Total findings: 6

- Line 860: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: // std::cerr << "parseComparison current token: " << (int)current().type << " value='" << current().value << "'\n";
  Confidence: band=very_high; score=0.92
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
- Line 340: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: auto query = parseQuery(false); // false = not a subquery
  Confidence: band=high; score=0.74
- Line 418: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::shared_ptr<Query> parseQuery(bool isSubquery = false) {
  Confidence: band=high; score=0.74

### src/query/cte_subquery.cpp
Total findings: 6

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

### src/query/materialized_view.cpp
Total findings: 6

- Line 99: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!s.empty()) s += ',';
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!s.empty()) s += ',';
  Confidence: band=high; score=0.74
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

### src/query/query_plan_visualizer.cpp
Total findings: 6

- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_node->attributes.push_back(pred.column);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += std::string(4, ' ') + "<max depth exceeded>\n";
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: children_arr.push_back(toJSONImpl(*child, analyze, depth + 1));
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: edges_out += "  n" + std::to_string(my_id)
  Confidence: band=high; score=0.74

### src/query/sparql_parser.cpp
Total findings: 6

- Line 10: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // SPARQL compatibility layer – SELECT query parsing and AQL transpilation.
  Confidence: band=high; score=0.8
- Line 55: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')       out += "\\\"";
  Confidence: band=high; score=0.74
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

### src/query/continuous_query_engine.cpp
Total findings: 5

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

### src/query/functions/udf_registry.cpp
Total findings: 5

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
- Line 229: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: callArgs.push_back(evalExpr(a, args, context, depth + 1));
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/query/plan_cache.cpp
Total findings: 4

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

### src/query/vectorized_execution.cpp
Total findings: 4

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

### src/query/adaptive_optimizer.cpp
Total findings: 3

- Line 400: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.indexes_to_use.push_back(idx.index_name);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: placement.cpu_affinity.push_back(i);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: placement.cpu_affinity.push_back(i);
  Confidence: band=high; score=0.74

### src/query/approximate_aggregator.cpp
Total findings: 3

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

### src/query/query_cache.cpp
Total findings: 3

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

### src/query/aql_parser_json.cpp
Total findings: 2

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: args_json.push_back(arg->toJSON());
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elems_json.push_back(elem->toJSON());
  Confidence: band=high; score=0.74

### src/query/continuous_query_planner.cpp
Total findings: 2

- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({t.payload, /*is_retract=*/true});
  Confidence: band=high; score=0.74
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({t.payload, false});
  Confidence: band=high; score=0.74

### src/query/functions/tensor_functions.cpp
Total findings: 2

- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& v : args[3]) modes_b.push_back(v.get<std::size_t>());
  Confidence: band=high; score=0.74
- Line 553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());
  Confidence: band=high; score=0.74

### src/query/query_canceller.cpp
Total findings: 2

- Line 23: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: QueryCanceller::registerQuery(const std::string& request_id) {
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void QueryCanceller::unregisterQuery(const std::string& request_id) {
  Confidence: band=high; score=0.74

### src/query/sql_parser.cpp
Total findings: 2

- Line 10: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // SQL dialect compatibility layer – SELECT/INSERT/UPDATE/DELETE passthrough.
  Confidence: band=high; score=0.8
- Line 54: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  out += "\\\"";
  Confidence: band=high; score=0.74

### src/query/aql_safety_validator.cpp
Total findings: 1

- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::toupper(c)));
  Confidence: band=high; score=0.74

### src/query/cte_cache.cpp
Total findings: 1

- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(nlohmann::json::parse(serialized));
  Confidence: band=high; score=0.74

### src/query/semantic_cache.cpp
Total findings: 1

- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(current);
  Confidence: band=high; score=0.74

### src/query/statistical_aggregator.cpp
Total findings: 1

- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(val.get<double>());
  Confidence: band=high; score=0.74

### src/query/tensor_aware_query_optimizer.cpp
Total findings: 1

- Line 225: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (upper_desc.find(fn) != std::string::npos) {
  Confidence: band=very_high; score=0.9

### src/query/workload_cache_strategy.cpp
Total findings: 1

- Line 444: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hot_queries.push_back(query_frequencies[i].first);
  Confidence: band=high; score=0.74

### src/query/cq_watermark.cpp
Total findings: 0


### src/query/functions/ethics_functions.cpp
Total findings: 0


### src/query/functions/function_registry.cpp
Total findings: 0


### src/query/query_cache_manager.cpp
Total findings: 0


### src/query/query_compiler.cpp
Total findings: 0


### src/query/query_profiler.cpp
Total findings: 0


### src/query/result_stream.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
