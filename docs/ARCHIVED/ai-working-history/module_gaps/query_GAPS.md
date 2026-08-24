# query Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: query
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 692
- Actionable Findings (Critical + High): 438
- Affected Files: 52

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 190 |
| High | 248 |
| Medium | 243 |
| Low | 11 |

## Category Summary

| Category | Count |
|---|---:|
| data_race | 83 |
| string_concat_loop | 76 |
| null_dereference | 60 |
| copy_overhead | 35 |
| unordered_container_iter | 33 |
| uninitialized_access | 30 |
| resource_leaked_in_exception | 28 |
| missing_latency_metric | 27 |
| uncaught_exception | 25 |
| o_n_squared | 23 |
| generic_catch | 21 |
| thread_join_no_timeout | 18 |
| blocking_no_timeout | 17 |
| no_timeout | 17 |
| fp_exact_comparison | 16 |
| iterator_invalidation | 15 |
| legacy_or_compat_path | 15 |
| missing_trace_point | 15 |
| array_bounds | 12 |
| array_bounds_violation | 12 |
| lock_contention | 8 |
| function_return_truncation | 7 |
| nested_loop_find | 7 |
| stale_doc_section_reference | 7 |
| unchecked_array_index | 7 |
| map_vs_unordered_map | 6 |
| multiplication_overflow | 6 |
| lock_in_loop | 5 |
| unnecessary_copy | 5 |
| catch_all_swallow | 4 |
| module_doc_linkset_drift | 4 |
| range_temporary | 4 |
| unstructured_log | 4 |
| db_connection_leak | 3 |
| deadlock_risk | 3 |
| hardcoded_output | 3 |
| manual_cleanup | 3 |
| arithmetic_overflow | 2 |
| delete_without_nullptr | 2 |
| duplicate_qualified_signature | 2 |
| explicit_delete | 2 |
| missing_correlation_id | 2 |
| pointer_arithmetic_unbounded | 2 |
| posix_only_api | 2 |
| primitive_no_volatile | 2 |
| repeated_search | 2 |
| allocation_loop | 1 |
| delete_no_nullptr | 1 |
| expensive_copy | 1 |
| memory_order | 1 |
| missing_vector_reserve | 1 |
| sensitive_data_logging | 1 |
| size_assumption | 1 |
| smart_ptr_misuse | 1 |
| uninitialized_array | 1 |
| uninitialized_member_field | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| query/query_engine.cpp | 153 | 54 | 44 | 55 | 0 |
| query/aql_translator.cpp | 93 | 84 | 9 | 0 | 0 |
| query/sparql_parser.cpp | 47 | 3 | 20 | 24 | 0 |
| query/functions/fulltext_functions.cpp | 46 | 2 | 14 | 30 | 0 |
| query/cypher_parser.cpp | 24 | 0 | 5 | 19 | 0 |
| query/parallel_executor.cpp | 24 | 12 | 4 | 8 | 0 |
| query/query_plan_visualizer.cpp | 24 | 2 | 13 | 9 | 0 |
| query/aql_parser.cpp | 20 | 1 | 10 | 9 | 0 |
| query/sql_parser.cpp | 20 | 0 | 2 | 18 | 0 |
| query/adaptive_join.cpp | 16 | 1 | 12 | 2 | 1 |
| query/gremlin_parser.cpp | 16 | 1 | 1 | 14 | 0 |
| query/let_evaluator.cpp | 16 | 0 | 16 | 0 | 0 |
| query/query_optimizer.cpp | 15 | 0 | 11 | 2 | 2 |
| query/tensor_contraction_engine.cpp | 14 | 1 | 13 | 0 | 0 |
| query/optimizer_cost_model.cpp | 10 | 0 | 1 | 7 | 2 |
| query/tensor_aware_query_optimizer.cpp | 10 | 4 | 2 | 4 | 0 |
| query/cte_subquery.cpp | 8 | 1 | 5 | 2 | 0 |
| query/functions/tensor_functions.cpp | 8 | 1 | 3 | 4 | 0 |
| query/query_federation.cpp | 8 | 0 | 4 | 4 | 0 |
| query/cross_cluster_federation.cpp | 7 | 0 | 5 | 2 | 0 |
| query/functions/lora_functions.cpp | 7 | 2 | 0 | 4 | 1 |
| query/materialized_view.cpp | 7 | 1 | 3 | 3 | 0 |
| query/query_cache.cpp | 7 | 3 | 3 | 1 | 0 |
| query/result_type_annotation.cpp | 7 | 1 | 3 | 3 | 0 |
| query/continuous_query_engine.cpp | 6 | 1 | 3 | 2 | 0 |
| query/cte_cache.cpp | 6 | 0 | 2 | 4 | 0 |
| query/workload_cache_strategy.cpp | 6 | 0 | 5 | 1 | 0 |
| query/aql_safety_validator.cpp | 5 | 0 | 3 | 2 | 0 |
| query/plan_cache.cpp | 5 | 1 | 2 | 2 | 0 |
| query/query_cache_manager.cpp | 5 | 5 | 0 | 0 | 0 |
| query/semantic_cache.cpp | 5 | 1 | 4 | 0 | 0 |
| query/window_evaluator.cpp | 5 | 0 | 5 | 0 | 0 |
| query/functions/ethics_functions.cpp | 4 | 3 | 1 | 0 | 0 |
| query/query_canceller.cpp | 4 | 2 | 0 | 2 | 0 |
| query/approximate_aggregator.cpp | 3 | 1 | 1 | 0 | 1 |
| query/aql_runner.cpp | 3 | 0 | 1 | 2 | 0 |
| query/cq_watermark.cpp | 3 | 1 | 2 | 0 | 0 |
| query/functions/process_mining_functions.cpp | 3 | 0 | 2 | 1 | 0 |
| query/functions/udf_registry.cpp | 3 | 0 | 2 | 1 | 0 |
| query/query_rewrite_rule.cpp | 3 | 1 | 1 | 1 | 0 |
| query/adaptive_optimizer.cpp | 2 | 0 | 2 | 0 | 0 |
| query/aql_parser_json.cpp | 2 | 0 | 2 | 0 | 0 |
| query/result_stream.cpp | 2 | 0 | 2 | 0 | 0 |
| query/statistical_aggregator.cpp | 2 | 0 | 2 | 0 | 0 |
| query/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| query/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| query/README.md | 1 | 0 | 0 | 0 | 1 |
| query/ROADMAP.md | 1 | 0 | 0 | 0 | 1 |
| query/functions/function_registry.cpp | 1 | 0 | 1 | 0 | 0 |
| query/materialized_cte.cpp | 1 | 0 | 1 | 0 | 0 |
| query/query_profiler.cpp | 1 | 0 | 1 | 0 | 0 |
| query/vectorized_execution.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### query/query_engine.cpp
Total findings: 153

- Line 603: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: child.setStatus(true);

		});

	}

	tg.wait();



	if (!errors.empty()) {

		std::sort(errors.begin(), errors.end());
- Line 603: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 603: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 826: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: batches[batch_idx] = std::move(local_entities);

			});

		}

		tg.wait();



		logSortedDeserializeFailures(failed_deserialize_pks, "executeAndEntities");
- Line 826: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 826: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 936: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: child.setStatus(true);

		});

	}

	tg.wait();



	if (!errors.empty()) {

		std::sort(errors.begin(), errors.end());
- Line 936: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 936: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 998: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: child.setStatus(true);

		});

	}

	tg.wait();



	if (!errors.empty()) {

		std::sort(errors.begin(), errors.end());
- Line 998: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 998: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 1067: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: batches[batch_idx] = std::move(local_entities);

			});

		}

		tg.wait();

		logSortedDeserializeFailures(failed_deserialize_pks, "executeOrEntitiesWithFallback");

		for (auto& batch : batches) {

			out.insert(out.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
- Line 1067: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 1067: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 1131: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: batches[batch_idx] = std::move(local_entities);

			});

		}

		tg.wait();



		logSortedDeserializeFailures(failed_deserialize_pks, "executeOrEntities");
- Line 1131: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 1131: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 1268: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: batches[batch_idx] = std::move(local_entities);

			});

		}

		tg.wait();



		logSortedDeserializeFailures(failed_deserialize_pks, "executeAndEntitiesSequential");
- Line 1268: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 1268: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 2365: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // per-morsel result bucket.  Results are merged after tg.wait().
- Line 2390: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: morsel_results[m] = std::move(local);

			});

		}

		tg.wait();



		// Merge per-morsel results into the output vector.

		for (auto& bucket : morsel_results) {
- Line 2390: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 2390: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 2739: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto checkFieldAccess = [](const std::shared_ptr<query::Expression>& e) -> std::pair<std::string, st
- Line 2743: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto varExpr = std::static_pointer_cast<query::VariableExpr>(fa->object);
- Line 2792: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: initial_context.cte_cache = parent_context->cte_cache;
- Line 2984: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto bin = std::static_pointer_cast<query::BinaryOpExpr>(filter->condition);
- Line 2989: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->left);
- Line 2990: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto rfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->right);
- Line 2993: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lvar = std::static_pointer_cast<query::VariableExpr>(lfa->object)->name;
- Line 2994: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto rvar = std::static_pointer_cast<query::VariableExpr>(rfa->object)->name;
- Line 3237: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t offset = (limit->offset <= 0) ? 0 : static_cast<size_t>(limit->offset);
- Line 3238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t count  = (limit->count  <= 0) ? 0 : static_cast<size_t>(limit->count);
- Line 3694: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buckets[bi] = std::move(buf);

				});

			}

			tg3.wait();

			for (auto& b : buckets) {

				filteredNodes.insert(filteredNodes.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));

			}
- Line 3694: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg3.wait();
- Line 3694: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg3.wait();
- Line 3996: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto xLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[0]);
- Line 3997: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto yLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[1]);
- Line 4115: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4157: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4188: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it1 may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it1=current.begin(); auto it2=keys.begin();
- Line 4354: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buckets[bi] = std::move(buf);

				});

			}

			tg.wait();

			for (auto& b : buckets) {

				results.insert(results.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));

			}
- Line 4354: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 4354: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 4509: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vgr.entity = cached->second;
- Line 4558: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buckets[bi] = std::move(buf);

		});

	}

	tg2.wait();

	for (auto& b : buckets) {

		vectorResults.insert(vectorResults.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));

	}
- Line 4558: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg2.wait();
- Line 4558: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg2.wait();
- Line 4582: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: r.entity = cached->second;
- Line 4650: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const size_t n = pks.size(); const size_t T = std::max<unsigned>(1u, std::thread::hardware_concurrency()); const size_t CHUNK = std::max<std::size_t>(64,(n+T-1)/T);

		std::vector<std::vector<ContentGeoResult>> buckets((n+CHUNK-1)/CHUNK); tbb::task_group tg;

		for(size_t bi=0; bi<buckets.size(); ++bi){ tg.run([&,bi](){ size_t start=bi*CHUNK; size_t end=std::min(start+CHUNK,n); std::vector<ContentGeoResult> buf; buf.reserve(end-start); for(size_t i=start;i<end;++i){ if(!blobs[i].has_value()) continue; nlohmann::json doc; try { auto entity = BaseEntity::deserialize(pks[i], *blobs[i]); doc = nlohmann::json::parse(entity.toJson()); } catch (...) { continue; } EvaluationContext ctx; ctx.bind("doc", doc); if(!evaluateCondition(q.spatial_filter, ctx)) continue; ContentGeoResult r; r.pk=pks[i]; const auto bm25_it = bm25.find(pks[i]); r.bm25_score = (bm25_it != bm25.end()) ? bm25_it->second : 0.0; r.entity=std::move(doc); if(q.boost_by_distance && q.center_point){ const auto& docRef=r.entity; if(docRef.contains(q.geom_field)){ nlohmann::json geom; if(docRef[q.geom_field].is_string()){ try { geom=nlohmann::json::parse(docRef[q.geom_field].get<std::string>()); } catch (...) {} } else if(docRef[q.geom_field].is_object()){ geom=docRef[q.geom_field]; } if(!geom.is_null() && geom.contains("type") && geom["type"]=="Point" && geom.contains("coordinates") && geom["coordinates"].is_array() && geom["coordinates"].size()>=2){ double x=geom["coordinates"][0].get<double>(); double y=geom["coordinates"][1].get<double>(); double cx=(*q.center_point)[0]; double cy=(*q.center_point)[1]; double dx=x-cx; double dy=y-cy; r.geo_distance=std::sqrt(dx*dx+dy*dy); } } } buf.emplace_back(std::move(r)); } buckets[bi]=std::move(buf); }); }

		tg.wait(); for(auto &b : buckets){ results.insert(results.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end())); }

		child2.setAttribute("spatial_results", static_cast<int64_t>(results.size())); child2.setStatus(true);

	} else {

		// Spatial-first Plan: verwende SpatialIndex zur Kandidatenmenge, dann naive Fulltext-Evaluation
- Line 4650: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait(); for(auto &b : buckets){ results.insert(results.end(), std::make_move_iterator(b.begin()),
- Line 4650: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait(); for(auto &b : buckets){ results.insert(results.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end())); }
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4507 feat(query): v2.0.0 â€“ edg... (2026-04-11) | #4364 docs(query): rewrit
- Line 591: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(errors_mutex);
- Line 812: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = start; i < end; ++i) {
- Line 818: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 923: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(errors_mutex);
- Line 977: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 0; i < q.disjuncts.size(); ++i) {
- Line 985: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> eg(error_mutex);
- Line 1054: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = start; i < end; ++i) {
- Line 1060: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1117: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = start; i < end; ++i) {
- Line 1123: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 1254: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = start; i < end; ++i) {
- Line 1260: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
- Line 2171: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: sorted_fields.reserve(obj->fields.size());
- Line 2172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [k, e] : obj->fields) {
- Line 2206: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (d==0.0) return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Division by zero");
- Line 2211: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (d==0.0) return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Modulo by zero");
- Line 2287: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (pos_a == a.size() && pos_b == b.size()) {
- Line 2299: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (pos_a == a.size() && pos_b == b.size()) {
- Line 2540: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (sel_a == sel_b) {
- Line 2623: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!candSet.empty() && candSet.find(k) == candSet.end()) continue; // filter
- Line 2704: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [key, val] : obj->fields) {
- Line 3015: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto result_or_err = evaluateExpression(return_node->expression, ctx);
- Line 3111: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto result_or_err = evaluateExpression(return_node->expression, ctx);
- Line 3148: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3200: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3238: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['\t\t// Guard against negative int64_t values: cast to size_t only after clamping.', '\t\tsize_t offset = (limit->offset <= 0) ? 0 : static_cast<size_t>(limit->offset);', '\t\tsize_t count  = (limit->count  <= 0) ? 0 : static_cast<size_t>(limit->count);', '\t\tif (offset >= results.size()) {', '\t\t\tresults.clear();']
- Line 3403: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto result_or_err = evaluateExpression(return_node->expression, ctx);
- Line 3566: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto slashPos = vertexPk.find('/');
- Line 3658: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto slashPos = vertexPk.find('/');
- Line 4061: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (eqPrefilter && !eqPrefilter->empty()) {
- Line 4063: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (eqPrefilter->size() < stats.entry_count * 0.05) return VGPlan::VectorThenSpatial;
- Line 4142: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::string tmp = st.column; size_t pos=0; while(true){ size_t n = tmp.find('+', pos); if(n==std::st
- Line 4146: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto &c : cols) { auto it = equalityMap.find(c); if (it==equalityMap.end()) { all=false; break;
- Line 4176: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto itRange = rangeMap.find(column);
- Line 4195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: span.setAttribute("index_prefilter_size", static_cast<int64_t>(indexPrefilter->size()));
- Line 4196: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (indexPrefilter->empty()) { span.setAttribute("result_count", static_cast<int64_t>(0)); span.setS
- Line 4262: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i=0;i<std::min(tmp.size(),k);++i) {
- Line 4294: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 4298: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ci.prefilterSize = indexPrefilter ? indexPrefilter->size() : 0; ci.k = q.k; ci.vectorDim = q.query_v
- Line 4539: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto it = entityCache.find(pk);
- Line 4540: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto it = entityCache.find(pk);
- Line 4684: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 4699: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (sa == sb) {
- Line 115: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 655: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto scoreMap = std::make_shared<std::unordered_map<std::string, double>>();
- Line 703: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto filteredScores = std::make_shared<std::unordered_map<std::string, double>>();
- Line 795: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));

			if (!blob) continue;

			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }

			catch (...) { THEMIS_WARN("executeAndEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }

		}

	} else {

		// Parallel für große Mengen: Batch-Processing mit TBB
- Line 795: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { THEMIS_WARN("executeAndEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
- Line 817: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));

					if (!blob) continue;

					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }

					catch (...) {

						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);

						failed_deserialize_pks.push_back(pk);

					}
- Line 817: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {
- Line 1040: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));

			if (!blob) continue;

			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }

			catch (...) { THEMIS_WARN("executeOrEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}", pk); }

		}

	} else {

		std::vector<std::vector<BaseEntity>> batches((keys.size() + BATCH_SIZE - 1) / BATCH_SIZE);
- Line 1040: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { THEMIS_WARN("executeOrEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}"
- Line 1059: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));

					if (!blob) continue;

					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }

					catch (...) {

						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);

						failed_deserialize_pks.push_back(pk);

					}
- Line 1059: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {
- Line 1101: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));

			if (!blob) continue;

			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }

			catch (...) { THEMIS_WARN("executeOrEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }

		}

	} else {

		std::vector<std::vector<BaseEntity>> batches((keys.size() + BATCH_SIZE - 1) / BATCH_SIZE);
- Line 1101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { THEMIS_WARN("executeOrEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
- Line 1122: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));

					if (!blob) continue;

					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }

					catch (...) {

						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);

						failed_deserialize_pks.push_back(pk);

					}
- Line 1122: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {
- Line 1237: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(table, pk));

			if (!blob) continue;

			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }

			catch (...) { THEMIS_WARN("executeAndEntitiesSequential: Deserialisierung fehlgeschlagen für PK={}", pk); }

		}

	} else {

		// Parallel für große Mengen
- Line 1237: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { THEMIS_WARN("executeAndEntitiesSequential: Deserialisierung fehlgeschlagen für PK={}",
- Line 1259: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = db_->get(KeySchema::makeRelationalKey(table, pk));

					if (!blob) continue;

					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }

					catch (...) {

						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);

						failed_deserialize_pks.push_back(pk);

					}
- Line 1259: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {
- Line 1640: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto coords = g["coordinates"];
- Line 1845: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::string u = trim(wkt); std::string up=u; std::transform(up.begin(), up.end(), up.begin(), ::toup
- Line 2357: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

				BaseEntity e = BaseEntity::deserialize(entry.pk, entry.blob);

				if (matchesPredicates(e)) out.emplace_back(std::move(entry.pk));

			} catch (...) {

				// skip malformed entries

			}

		}
- Line 2357: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2383: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

						BaseEntity e = BaseEntity::deserialize(entry.pk, entry.blob);

						if (matchesPredicates(e)) local.emplace_back(std::move(entry.pk));

					} catch (...) {

						// skip malformed entries

					}

				}
- Line 2383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2608: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> candSet;
- Line 2807: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::shared_ptr<query::FilterNode>>> single_var_filters;
- Line 2834: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
- Line 3271: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> groups;
- Line 3425: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: QueryEngine::executeRecursivePathQuery(const RecursivePathQuery& q) const {
- Line 3447: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> frontier{q.start_node};
- Line 3449: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> next;
- Line 3592: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

					auto entity = BaseEntity::deserialize(vertexPk, *vertexDataOpt);

					vertex = nlohmann::json::parse(entity.toJson());

				} catch (...) {

					pathValid = false;

					break;

				}
- Line 3592: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 3687: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!vertexDataOpt.has_value()) continue;

						nlohmann::json vertex;

						try { auto entity = BaseEntity::deserialize(vertexPk, *vertexDataOpt); vertex = nlohmann::json::parse(entity.toJson()); }

						catch (...) { continue; }

						EvaluationContext ctx; ctx.bind("v", vertex);

						if (evaluateCondition(sc.spatial_filter, ctx)) buf.push_back(vertexPk);

					}
- Line 3687: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { continue; }
- Line 3789: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 3831: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (vertexDataOpt.has_value()) {

				try {

					result.vertex_data = nlohmann::json::parse(*vertexDataOpt);

				} catch (...) {

					// If parsing fails, create minimal JSON

					result.vertex_data = nlohmann::json{{"_key", current.vertex}};

				}
- Line 3831: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 4072: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: QueryEngine::executeVectorGeoQuery(const VectorGeoQuery& q) const {
- Line 4091: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, RangeAcc> rangeMap;
- Line 4093: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> equalityMap;
- Line 4163: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

					span.setAttribute("composite_prefilter_applied", true);

				}

			} catch (...) {

				// defensiv: bei Fehler keine Composite-Nutzung

				THEMIS_WARN("VectorGeoQuery: composite prefilter failed, skipping");

			}
- Line 4163: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 4294: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: double bboxArea = std::max((bbox->maxx - bbox->minx) * (bbox->maxy - bbox->miny), 0.0); 

						ci.bboxRatio = std::min(std::max(bboxArea / totalArea, 0.0), 1.0); 

						ci.spatialIndexEntries = stats.entry_count;

					} catch (...) {

					}

			}

		}
- Line 4294: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 4344: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: BaseEntity entity = BaseEntity::deserialize(vr[i].pk, *blobs[i]);

							doc = nlohmann::json::parse(entity.toJson());

						}

						catch (...) { continue; }

						EvaluationContext ctx; ctx.bind("doc", doc);

						if (evaluateCondition(q.spatial_filter, ctx)) {

							VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc);
- Line 4344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { continue; }
- Line 4380: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, nlohmann::json> entityCache;
- Line 4460: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, nlohmann::json> tmpCache;
- Line 4596: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: QueryEngine::executeContentGeoQuery(const ContentGeoQuery& q) const {
- Line 4644: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string,double> bm25; bm25.reserve(ftResults.size());
- Line 4656: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string,nlohmann::json> cache;
- Line 4678: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> tokenSet(tokens.begin(), tokens.end());
- Line 4685: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto docTokens = SecondaryIndexManager::tokenize(text); std::unordered_set<std::string> docSet(docTokens.begin(), docTokens.end());

### query/aql_translator.cpp
Total findings: 93

- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto varName = std::static_pointer_cast<VariableExpr>(sortExpr)->name;
- Line 108: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(sim->arguments[1]);
- Line 130: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto kLit = std::static_pointer_cast<LiteralExpr>(sim->arguments[2]);
- Line 138: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 175: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (prox->arguments[0]->getType() != ASTNodeType::FieldAccess) {
- Line 175: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // LET ... = PROXIMITY(...)

                    if (letNode.expression->getType() == ASTNodeType::ProximityCall) {

                        auto prox = std::static_pointer_cast<ProximityCallExpr>(letNode.expression);

                        if (prox->arguments.size() == 2) {

                            if (prox->arguments[0]->getType() != ASTNodeType::FieldAccess) {

                                return TranslationResult::Error("PROXIMITY() LET first arg must be field access");

                            }

                            if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                                return TranslationResult::Error("PROXIMITY() LET second arg must array literal");

                            }
- Line 178: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 178: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto prox = std::static_pointer_cast<ProximityCallExpr>(letNode.expression);

                        if (prox->arguments.size() == 2) {

                            if (prox->arguments[0]->getType() != ASTNodeType::FieldAccess) {

                                return TranslationResult::Error("PROXIMITY() LET first arg must be field access");

                            }

                            if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                                return TranslationResult::Error("PROXIMITY() LET second arg must array literal");

                            }



                            std::string geomField = extractColumnName(prox->arguments[0]);

                            auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 182: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: std::string geomField = extractColumnName(prox->arguments[0]);
- Line 182: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

                            if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                                return TranslationResult::Error("PROXIMITY() LET second arg must array literal");

                            }



                            std::string geomField = extractColumnName(prox->arguments[0]);

                            auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);

                            if (arr->elements.size() < 2) {

                                return TranslationResult::Error("PROXIMITY() point needs 2 elements");

                            }
- Line 183: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 183: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                                return TranslationResult::Error("PROXIMITY() LET second arg must array literal");

                            }



                            std::string geomField = extractColumnName(prox->arguments[0]);

                            auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);

                            if (arr->elements.size() < 2) {

                                return TranslationResult::Error("PROXIMITY() point needs 2 elements");

                            }



                            std::vector<float> point;
- Line 183: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 242: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 299: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* fn  = static_cast<FunctionCallExpr*>(bin->left.get());
- Line 300: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* rhs = static_cast<LiteralExpr*>(bin->right.get());
- Line 313: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto* fa0 = static_cast<FieldAccessExpr*>(fn->arguments[0].get());
- Line 314: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto* fa1 = static_cast<FieldAccessExpr*>(fn->arguments[1].get());
- Line 319: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::string var0 = static_cast<VariableExpr*>(fa0->object.get())->name;
- Line 320: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::string var1 = static_cast<VariableExpr*>(fa1->object.get())->name;
- Line 407: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 445: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (args[0]->getType() != ASTNodeType::FieldAccess) {
- Line 445: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto prox = std::static_pointer_cast<ProximityCallExpr>(spec.expression);

                const auto& args = prox->arguments;

                if (args.size() != 2) {

                    return TranslationResult::Error("PROXIMITY() requires exactly 2 arguments: PROXIMITY(doc.location, [lon,lat])");

                }

                if (args[0]->getType() != ASTNodeType::FieldAccess) {

                    return TranslationResult::Error("PROXIMITY() first argument must be field access (e.g. doc.location)");

                }

                if (args[1]->getType() != ASTNodeType::ArrayLiteral) {

                    return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                }
- Line 448: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (args[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 448: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return TranslationResult::Error("PROXIMITY() requires exactly 2 arguments: PROXIMITY(doc.location, [lon,lat])");

                }

                if (args[0]->getType() != ASTNodeType::FieldAccess) {

                    return TranslationResult::Error("PROXIMITY() first argument must be field access (e.g. doc.location)");

                }

                if (args[1]->getType() != ASTNodeType::ArrayLiteral) {

                    return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                }



                std::string geomField = extractColumnName(args[0]);

                auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);
- Line 452: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: std::string geomField = extractColumnName(args[0]);
- Line 452: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

                if (args[1]->getType() != ASTNodeType::ArrayLiteral) {

                    return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                }



                std::string geomField = extractColumnName(args[0]);

                auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);

                if (arr->elements.size() < 2) {

                    return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon, lat]");

                }
- Line 453: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);
- Line 453: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (args[1]->getType() != ASTNodeType::ArrayLiteral) {

                    return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                }



                std::string geomField = extractColumnName(args[0]);

                auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);

                if (arr->elements.size() < 2) {

                    return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon, lat]");

                }



                std::vector<float> point;
- Line 503: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 512: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 534: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 559: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto kLit = std::static_pointer_cast<LiteralExpr>(func->arguments[2]);
- Line 589: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
- Line 625: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (func->arguments[0]->getType() != ASTNodeType::FieldAccess) {
- Line 625: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (name == "proximity") {

                    if (func->arguments.size() != 2) {

                        return TranslationResult::Error("PROXIMITY() requires exactly 2 arguments: PROXIMITY(doc.location, [lon,lat])");

                    }

                    if (func->arguments[0]->getType() != ASTNodeType::FieldAccess) {

                        return TranslationResult::Error("PROXIMITY() first argument must be field access (e.g. doc.location)");

                    }

                    if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                        return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                    }
- Line 628: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
- Line 628: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return TranslationResult::Error("PROXIMITY() requires exactly 2 arguments: PROXIMITY(doc.location, [lon,lat])");

                    }

                    if (func->arguments[0]->getType() != ASTNodeType::FieldAccess) {

                        return TranslationResult::Error("PROXIMITY() first argument must be field access (e.g. doc.location)");

                    }

                    if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                        return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                    }



                    std::string geomField = extractColumnName(func->arguments[0]);

                    auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 632: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: std::string geomField = extractColumnName(func->arguments[0]);
- Line 632: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

                    if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                        return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                    }



                    std::string geomField = extractColumnName(func->arguments[0]);

                    auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);

                    if (arr->elements.size() < 2) {

                        return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon, lat]");

                    }
- Line 633: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 2 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 633: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (func->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {

                        return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");

                    }



                    std::string geomField = extractColumnName(func->arguments[0]);

                    auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);

                    if (arr->elements.size() < 2) {

                        return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon, lat]");

                    }



                    std::vector<float> point;
- Line 633: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto arr = std::static_pointer_cast<ArrayLiteralExpr>(func->arguments[1]);
- Line 683: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
- Line 714: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
- Line 815: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto funcCall = std::static_pointer_cast<FunctionCallExpr>(filter->condition);
- Line 835: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 847: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 879: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto phraseLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 891: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 923: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 935: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 953: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[3]);
- Line 1014: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 1016: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
- Line 1097: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: findFulltext = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1121: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: findSpatial = [&](const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
- Line 1173: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto binOp = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
- Line 1192: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[1]);
- Line 1203: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[2]);
- Line 1276: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto distLiteral = std::static_pointer_cast<LiteralExpr>(spatialFunc->arguments[2]);
- Line 1278: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
- Line 1443: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto literal = std::static_pointer_cast<LiteralExpr>(binOp->right);
- Line 1568: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, limit->offset));
- Line 1569: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, limit->count));
- Line 1618: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
- Line 1619: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
- Line 1627: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
- Line 1628: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
- Line 1638: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1639: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1647: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto eqExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, binOp->left, binOp->right);
- Line 1653: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto gteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gte, binOp->left, binOp->right);
- Line 1659: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lte, binOp->left, binOp->right);
- Line 1665: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1757: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
- Line 1758: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
- Line 1804: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
- Line 1817: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
- Line 449: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
- Line 455: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon,
- Line 541: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: treat legacy FunctionCall nodes for SIMILARITY/PROXIMITY
- Line 629: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
- Line 635: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon,
- Line 761: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // If existing disjuncts = [A, B] and new disjuncts = [C, D], the result is
- Line 822: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("FULLTEXT() requires 2-3 arguments: FULLTEXT(column, query [, limit]
- Line 866: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("PHRASE() requires 2-3 arguments: PHRASE(column, phrase [, limit])")
- Line 910: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return TranslationResult::Error("FUZZY() requires 2-4 arguments: FUZZY(column, query [, maxDistance]

### query/sparql_parser.cpp
Total findings: 47

- Line 871: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = var_bindings.find(tp.subject.value);
- Line 885: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = var_bindings.find(tp.predicate.value);
- Line 899: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = var_bindings.find(tp.object.value);
- Line 10: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // SPARQL compatibility layer – SELECT query parsing and AQL transpilation.
- Line 648: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->op   = "||";
- Line 649: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left = std::move(*left);
- Line 650: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = std::move(*right);
- Line 664: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->op   = "&&";
- Line 665: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left = std::move(*left);
- Line 666: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = std::move(*right);
- Line 689: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->op   = op;
- Line 690: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left = std::move(*left);
- Line 691: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = std::move(*right);
- Line 703: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->op      = "!";
- Line 704: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->operand = std::move(*operand);
- Line 722: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->name = current().value;
- Line 728: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->value = std::string(current().value);
- Line 734: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: try { node->value = std::stoll(current().value); }
- Line 743: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: try { node->value = std::stod(current().value); }
- Line 752: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->value = true;
- Line 758: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->value = false;
- Line 764: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->value = std::string(current().value);
- Line 870: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = var_bindings.find(tp.subject.value);
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"')       out += "\\\"";
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"')       out += "\\\"";
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') out += "\\\\";
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') out += "\\n";
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\r') out += "\\r";
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\t') out += "\\t";
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SPARQLTokenType::VAR,
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SPARQLTokenType::URI, uri, start});
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SPARQLTokenType::LTE, "<=", start});
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SPARQLTokenType::LT, "<", start});
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(readString(c, start));
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(readNumber(start));
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(readIdent(start));
- Line 195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: case '{': tokens.push_back({SPARQLTokenType::LBRACE,  "{",  start}); ++pos_; break;
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: case '}': tokens.push_back({SPARQLTokenType::RBRACE,  "}",  start}); ++pos_; break;
- Line 261: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n':  val += '\n'; break;
- Line 262: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r':  val += '\r'; break;
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't':  val += '\t'; break;
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: default:   val += '\\'; val += esc; break;
- Line 779: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& var_bindings,
- Line 800: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& var_bindings) {
- Line 856: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> var_bindings;
- Line 887: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: constraints.push_back(t_var + ".predicate == " + it->second);
- Line 892: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: constraints.push_back(

### query/functions/fulltext_functions.cpp
Total findings: 46

- Line 670: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        std::string s1 = args[0].get<std::string>();', '        std::string s2 = args[1].get<std::string>();', '        int n = args.size() > 2 ? args[2].get<int>() : 2;', '', '        if (s1.empty() || s2.empty()) return 0.0;']
- Line 784: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '        std::string word = args[0].get<std::string>();', '        int maxLen = args.size() > 1 ? args[1].get<int>() : 6;', '', '        return metaphone(word, maxLen);']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3636 fix(query): build system au... (2026-03-12) | #3377 [WIP] Add highlight
- Line 209: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& t : tokenize(queryArg.get<std::string>()))
- Line 284: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    for (size_t hi = 0; hi < positions.size(); ++hi) {', '        while (positions[hi] - positions[lo] >= windowSize) ++lo;', '        size_t count = hi - lo + 1;', '        if (count > bestCount) {', '            bestCount = count;']
- Line 336: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 396: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 456: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 531: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 533: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 591: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 665: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 718: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 750: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 780: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 808: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ngrams.push_back(s.substr(i, n));
- Line 103: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: while (result.length() < 4) result += '0';
- Line 138: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
- Line 139: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i == 0 || upper[i - 1] != 'M') result += 'B';
- Line 142: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (next == 'H') { result += 'X'; i++; }
- Line 143: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (next == 'I' || next == 'E' || next == 'Y') result += 'S';
- Line 144: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else result += 'K';
- Line 147: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (next == 'G') { result += 'J'; i++; }
- Line 148: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else result += 'T';
- Line 150: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'F': result += 'F'; break;
- Line 153: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (next == 'I' || next == 'E' || next == 'Y') result += 'J';
- Line 154: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else result += 'K';
- Line 157: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i == 0 || !isVowel(upper[i - 1])) result += 'H';
- Line 159: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'J': result += 'J'; break;
- Line 198: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> queryTermSet(const json& queryArg) {
- Line 199: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> terms;
- Line 336: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 338: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto collection = args[0].get<std::string>();
- Line 396: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 398: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto collection = args[0].get<std::string>();
- Line 456: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
- Line 458: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto collection = args[0].get<std::string>();
- Line 531: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 591: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 665: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 678: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> count1, count2;
- Line 718: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 750: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 780: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
- Line 808: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {

### query/cypher_parser.cpp
Total findings: 24

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5329 perf(query): PERF-06 â€” re... (2026-05-27) | #4400 [WIP] Add GNN-based
- Line 584: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
- Line 603: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
- Line 680: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "Expected NULL after IS [NOT]",
- Line 1098: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: edge_var + "._type IN [" + type_list + "]");
- Line 158: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n':  s += '\n'; break;
- Line 159: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't':  s += '\t'; break;
- Line 160: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r':  s += '\r'; break;
- Line 301: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += '.';
- Line 302: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += '.';
- Line 361: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: CypherASTNode parseQuery() {
- Line 758: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: list_literal += ", ";
- Line 857: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > start) expr_text += " ";
- Line 858: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > start) expr_text += " ";
- Line 879: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > start) expr_text += " ";
- Line 880: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > start) expr_text += " ";
- Line 938: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"' || c == '\\') out += '\\';
- Line 939: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"' || c == '\\') out += '\\';
- Line 997: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!filter.empty()) filter += " AND ";
- Line 1093: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i) type_list += ", ";
- Line 1094: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i) type_list += ", ";
- Line 1095: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: type_list += "\"" + rel.types[i] + "\"";
- Line 1096: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: filter_clauses.push_back(
- Line 1097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: filter_clauses.push_back(

### query/parallel_executor.cpp
Total findings: 24

- Line 212: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buckets[m] = std::move(local);

            });

        }

        tg.wait();

    });



    // Merge morsel buckets (preserves input order across morsel boundaries).
- Line 212: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 212: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 278: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            });

        }

        tg.wait();



        // Merge per-morsel buffers into global partitions.

        for (size_t p = 0; p < P; ++p) {
- Line 278: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 278: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 313: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: left_parts[p], right_parts[p], spec);

            });

        }

        tg.wait();

    });



    // Merge partition results.
- Line 313: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 313: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 372: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            });

        }

        tg.wait();

    });



    // Phase 2: merge all partial maps into a single map.
- Line 372: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: tg.wait();
- Line 372: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: tg.wait();
- Line 198: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 297: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 305: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 348: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 70: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!key.empty()) key += '|';
- Line 71: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!key.empty()) key += '|';
- Line 75: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: key += ':';
- Line 198: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 271: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto k = rows[i].getFieldAsString(key_field);
- Line 297: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 305: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {
- Line 348: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: arena.execute([&]() {

### query/query_plan_visualizer.cpp
Total findings: 24

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: filter_node->estimated_rows = static_cast<size_t>(i) < plan.details.size()
- Line 96: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: filter_node->estimated_cost = 50.0 + 10.0 * static_cast<double>(i);
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: filter_node->type = PlanNodeType::Filter;
- Line 91: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: filter_node->description = pred.column + " == " + pred.value;
- Line 92: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: filter_node->selectivity = std::min(1.0, std::max(0.0, selectivity));
- Line 93: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: filter_node->estimated_rows = static_cast<size_t>(i) < plan.details.size()
- Line 96: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: filter_node->estimated_cost = 50.0 + 10.0 * static_cast<double>(i);
- Line 97: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: filter_node->attributes.push_back(pred.column);
- Line 107: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->type = PlanNodeType::IndexScan;
- Line 108: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->description = "IndexScan on " + query.table;
- Line 110: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->index_name = plan.orderedPredicates.front().column + "_idx";
- Line 112: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->type = PlanNodeType::SeqScan;
- Line 113: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->description = "SeqScan on " + query.table;
- Line 115: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->estimated_rows = plan.details.empty() ? 0
- Line 117: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: scan_node->estimated_cost = 200.0;
- Line 141: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += std::string(4, ' ') + "<max depth exceeded>\n";
- Line 269: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  result += "\\\""; break;
- Line 270: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  result += "\\\""; break;
- Line 271: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': result += "\\\\"; break;
- Line 272: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': result += "\\n";  break;
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': result += "\\r";  break;
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': result += "\\t";  break;
- Line 322: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: edges_out += "  n" + std::to_string(my_id)
- Line 323: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: edges_out += "  n" + std::to_string(my_id)

### query/aql_parser.cpp
Total findings: 20

- Line 860: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // std::cerr << "parseComparison current token: " << (int)current().type << " value='" << current().value << "'\n";
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4140 feat(security): AQLInjectio... (2026-03-12) | #3481 [WIP] Synchronize A
- Line 426: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // One or more FOR clauses (first is also stored in for_node for backward compat)
- Line 491: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: also accept SHORTEST_PATH after RETURN
- Line 631: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Still return a ForNode for compatibility (collection = "graph")
- Line 735: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!withNode->ctes.empty()) {
- Line 759: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: withNode->ctes.push_back(std::move(cte));
- Line 779: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->groups.emplace_back(var, expr);
- Line 810: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->aggregations.push_back(std::move(ag));
- Line 956: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: while (match(TokenType::DOT)) {

            advance();

            if (!match(TokenType::IDENTIFIER)) {

                throw std::runtime_error("Expected field name after '.'");

            }

            std::string field = current().value;

            advance();
- Line 1286: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 167: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n': value += '\n'; break;
- Line 168: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't': value += '\t'; break;
- Line 169: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r': value += '\r'; break;
- Line 170: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"': value += '"'; break;
- Line 171: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': value += '\''; break;
- Line 172: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': value += '\\'; break;
- Line 340: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto query = parseQuery(false); // false = not a subquery
- Line 418: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::shared_ptr<Query> parseQuery(bool isSubquery = false) {
- Line 1553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: args.push_back(std::stoll(t));

### query/sql_parser.cpp
Total findings: 20

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3427 feat(query): Per-query reso... (2026-03-12) | #3352 feat(query): SPARQL
- Line 10: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // SQL dialect compatibility layer – SELECT/INSERT/UPDATE/DELETE passthrough.
- Line 54: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"')  out += "\\\"";
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"')  out += "\\\"";
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') out += "\\\\";
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') out += "\\n";
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\r') out += "\\r";
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\t') out += "\\t";
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(readString(c, start));
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(readNumber(start));
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(readIdent(start));
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: case '=': tokens.push_back({SQLTokenType::EQ, "=", start}); ++pos_; break;
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SQLTokenType::LTE, "<=", start}); pos_ += 2;
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SQLTokenType::NEQ, "<>", start}); pos_ += 2;
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SQLTokenType::LT, "<", start}); ++pos_;
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SQLTokenType::GTE, ">=", start}); pos_ += 2;
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({SQLTokenType::GT, ">", start}); ++pos_;
- Line 285: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n': val += '\n'; break;
- Line 286: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r': val += '\r'; break;
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't': val += '\t'; break;

### query/adaptive_join.cpp
Total findings: 16

- Line 489: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = row.find(spec.left_key);
- Line 99: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: return 3.0 * (L + R);
- Line 235: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: result = executeShuffleJoin(spec, left, right);

            break;

        default:

            throw std::logic_error("AdaptiveJoinExecutor: unhandled JoinAlgorithm");

    }



    result.algorithm_used = algo;
- Line 276: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = row.find(build_key);
- Line 285: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto key_it = probe_row.find(probe_key);
- Line 289: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto bucket_it = hash_table.find(key_it->second);
- Line 413: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto rk_it = right_row.find(spec.right_key);
- Line 441: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = row.find(spec.right_key);
- Line 450: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto lk_it = left_row.find(spec.left_key);
- Line 453: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto bucket = index.find(lk_it->second);
- Line 488: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = row.find(spec.left_key);
- Line 497: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = row.find(spec.right_key);
- Line 522: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto bucket = ht.find(lk_it->second);
- Line 273: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<const RowValue*>> hash_table;
- Line 439: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<const RowValue*>> index;
- Line 75: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Scan both sides.  Add sort cost for unsorted inputs.

### query/gremlin_parser.cpp
Total findings: 16

- Line 298: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = opMap.find(opName);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5329 perf(query): PERF-06 â€” re... (2026-05-27) | #4400 [WIP] Add GNN-based
- Line 90: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n': buf += '\n'; break;
- Line 91: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't': buf += '\t'; break;
- Line 92: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r': buf += '\r'; break;
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vals.push_back(parseValue());
- Line 612: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"') out += "\\\"";
- Line 613: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"') out += "\\\"";
- Line 614: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') out += "\\\\";
- Line 645: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i) list += ", ";
- Line 654: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i) list += ", ";
- Line 736: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
- Line 738: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: filters.push_back(vVar + "." + key + " == " + valueToAQL(step.values[0]));
- Line 742: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: filters.push_back(vVar + "." + key + " != null");
- Line 748: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: filters.push_back(vVar + "." + step.strings[0] + " == null");
- Line 752: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: filters.push_back(vVar + "._key == " + "\"" + step.strings[0] + "\"");

### query/let_evaluator.cpp
Total findings: 16

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2853 [geo] Complete GeoJSON spec... (2026-03-12) | #2851 [geo] Implement ST_
- Line 82: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compat: JSON literal wrapper from legacy tests
- Line 87: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compat: path-based field access (supports array indices)
- Line 125: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compat: binary op with string operator
- Line 147: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compat: unary op with string operator
- Line 176: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compat: function call shim with functionName + arguments
- Line 185: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [key, valExpr] : objConstr->fields) {
- Line 229: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compat: numeric string treated as array index
- Line 326: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (rightNum == 0.0) {
- Line 332: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (rightNum == 0.0) {
- Line 450: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy fallback for ST_* functions
- Line 451: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // These remain here for backward compatibility with custom EWKB parsing
- Line 621: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::runtime_error("ST_Within: g1 must be a GeoJSON Point or [x,y] array");
- Line 645: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (cross == 0.0) {
- Line 699: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: return (px == g2j["coordinates"][0].get<double>()
- Line 700: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: && py == g2j["coordinates"][1].get<double>());

### query/query_optimizer.cpp
Total findings: 15

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4269 feat(timeseries): TSStore s... (2026-03-15) | #4166 feat(query): Wire S
- Line 119: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto it = table_stats_ptr->column_stats.find(p.column);
- Line 120: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (it != table_stats_ptr->column_stats.end() &&
- Line 121: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: table_stats_ptr->row_count > 0) {
- Line 124: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: static_cast<double>(table_stats_ptr->row_count));
- Line 159: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ? table_stats_ptr->row_count
- Line 163: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ? static_cast<size_t>(table_stats_ptr->avg_row_size_bytes > 0.0
- Line 164: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ? table_stats_ptr->avg_row_size_bytes
- Line 579: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(plan.recommended_parallelism, size_t(8)); ++i) {
- Line 765: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto it = table_stats_ptr->column_stats.find(pred.column);
- Line 766: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (it != table_stats_ptr->column_stats.end()) {
- Line 94: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: QueryOptimizer::Plan QueryOptimizer::chooseOrderForAndQuery(const ConjunctiveQuery& q, size_t maxProbePerPred) const {
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: pruned_shards.push_back(info.shard_id);
- Line 389: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: vectorSearchCost = std::log(static_cast<double>(universe) + 1.0) * dimScale; // ANN approximation
- Line 418: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double ftPhase = C_fulltext_base * std::log(static_cast<double>(hits) + 5.0);

### query/tensor_contraction_engine.cpp
Total findings: 14

- Line 176: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            throw std::overflow_error("TT-core kron: core dimension product overflows size_t");', '        }', '        cr.data.resize(rl * n * rr, 0.0f);', '', '        // Kronecker product of core matrices for each physical index i']
- Line 110: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 111: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 112: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 113: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 115: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 118: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 123: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 124: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 125: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 174: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: cr.n       = n;

        cr.r_right = rr;

        if (n != 0 && rr != 0 && rl > std::numeric_limits<std::size_t>::max() / n / rr) {

            throw std::overflow_error("TT-core kron: core dimension product overflows size_t");

        }

        cr.data.resize(rl * n * rr, 0.0f);
- Line 266: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 397: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (va == 0.0f) continue;

### query/optimizer_cost_model.cpp
Total findings: 10

- Line 215: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: cost.cpuCost = buildCost + probeCost + constants_.joinOverhead;

    

    // Memory cost: hash table for left table

    size_t hashTableSize = leftRows * (hashKeySize + constants_.hashTablePointerSize);

    cost.memoryCost = calculateMemoryCost(hashTableSize);

    

    const double estimatedD = static_cast<double>(leftRows) * static_cast<double>(rightRows) * selectivity;
- Line 135: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, ColumnStatistics>& columnStats) const {
- Line 592: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Consolidation Phase — Statistics Stubs' that was not found in 'src/query/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: ROADMAP.md § "Consolidation Phase — Statistics Stubs"
- Line 593: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cost Model Statistics' that was not found in 'src/query/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/query/FUTURE_ENHANCEMENTS.md § "Cost Model Statistics"
- Line 628: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Consolidation Phase — Statistics Stubs' that was not found in 'src/query/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: ROADMAP.md § "Consolidation Phase — Statistics Stubs"
- Line 629: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cost Model Statistics' that was not found in 'src/query/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/query/FUTURE_ENHANCEMENTS.md § "Cost Model Statistics"
- Line 655: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Consolidation Phase — Statistics Stubs' that was not found in 'src/query/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: ROADMAP.md § "Consolidation Phase — Statistics Stubs"
- Line 656: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cost Model Statistics' that was not found in 'src/query/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/query/FUTURE_ENHANCEMENTS.md § "Cost Model Statistics"
- Line 235: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Sort both inputs
- Line 241: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Merge phase: single scan of both sorted inputs

### query/tensor_aware_query_optimizer.cpp
Total findings: 10

- Line 133: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        function_name == "TENSOR_CONTRACT") {', '        // Inner-product / transfer-matrix: O(d·n·r³)', '        return d * n * r * r * r;', '    }', '    if (function_name == "TENSOR_SLICE" ||']
- Line 138: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        function_name == "TENSOR_PROJECT") {', '        // Slice / marginalize one core: O(d·n·r²)', '        return d * n * r * r;', '    }', '    if (function_name == "TENSOR_COMPRESS" ||']
- Line 143: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        function_name == "TENSOR_DECOMPOSE") {', '        // TT-rounding / decomposition: O(d·r²·n)', '        return d * r * r * n * std::log2(n + 1.0);', '    }', '    if (function_name == "TENSOR_INFO") {']
- Line 149: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    }', '    // Unknown — use a generic linear estimate.', '    return d * n * r;', '}', '']
- Line 193: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 226: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (upper_desc.find(fn) != std::string::npos) {
- Line 193: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                    return;

                }

            } catch (...) {

                // Visitor threw; fall through to string-scan heuristic.

            }

        }
- Line 193: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 209: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (detector) {

        try {

            detected_fn = detector(node);

        } catch (...) {

            // Fail closed to deterministic description scan below.

            detected_fn.reset();

        }
- Line 209: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### query/cte_subquery.cpp
Total findings: 8

- Line 712: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: optimizedQuery->limit = std::make_shared<query::LimitNode>(0, 1);
- Line 166: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 170: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 199: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 262: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [key, val] : obj->fields) {
- Line 367: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& outerVarNames
- Line 405: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> outerVarNames;

### query/functions/tensor_functions.cpp
Total findings: 8

- Line 472: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                "Marginalize a tensor over one mode: PROJECT(t, mode). "', '                "Sums over all indices along mode, returning a train of order (d-1). "', '                "Operates entirely in the compressed domain (O(d*n*r^2)). "', '                "Ref: tensor marginalization, paper §AQL operators.",', '            .arguments = {']
- Line 61: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto next = path.find('.', pos);
- Line 139: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ptr->is_object() &&
- Line 140: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ptr->contains("data") && ptr->contains("shape")) {
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& s : shape_arr) shape.push_back(s.get<std::size_t>());
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& v : args[2]) modes_a.push_back(v.get<std::size_t>());
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& v : args[0]) data.push_back(v.get<float>());
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());

### query/query_federation.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4364 docs(query): rewrite ROADMA... (2026-03-21) | #4156 [WIP] Implement rea
- Line 617: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = hash_table.find(key);
- Line 963: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::debug("Shard-key range [{}, {}] → {} shard(s)",
- Line 1013: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::debug("QueryFederation: range-lookup [{},{}] → {} shard(s)",
- Line 531: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
- Line 600: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
- Line 866: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else if (m2.size() > 1) {

                    metadata.limit = std::stoull(m2[1].str());

                }

            } catch (...) {

                metadata.limit.reset();

                metadata.offset.reset();

            }
- Line 866: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### query/cross_cluster_federation.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3350 [query] Cross-cluster feder... (2026-03-12)
- Line 162: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto lat_it = latency_cache_.find(id);
- Line 231: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
- Line 257: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(selected.begin(), selected.end(), ep.cluster_id) !=
- Line 295: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (failed == futures.size()) {

            if (!config_.skip_unreachable_clusters) {

                throw std::runtime_error(

                    "CrossClusterFederator: all cluster queries failed");

            }

            spdlog::error(
- Line 231: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json CrossClusterFederator::execute(const std::string& query) {
- Line 276: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool ok = false;

### query/functions/lora_functions.cpp
Total findings: 7

- Line 457: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        std::string start_model = args[0].get<std::string>();', '        std::string end_model = args[1].get<std::string>();', '        int max_depth = args.size() > 2 ? args[2].get<int>() : 5;', '        (void)max_depth;', '']
- Line 731: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        // Parse arguments', '        std::string adapter_id = args[0].get<std::string>();', '        int depth = args.size() > 1 ? args[1].get<int>() : 10;', '', '        // Get orchestrator']
- Line 19: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_storage_service.h"
- Line 20: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_training_service.h"
- Line 226: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "LORA_QUERY('llama-2-7b', 'themis_help_lora', question.text, {max_tokens: 500})"
- Line 378: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> ws;
- Line 825: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "LORA_AUDIT_LOG('legal-lora-v2', 100)"

### query/materialized_view.cpp
Total findings: 7

- Line 143: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: new MaterializedView(def, config));
- Line 171: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 305: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 372: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = row.find(filter_field);
- Line 99: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!s.empty()) s += ',';
- Line 100: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!s.empty()) s += ',';
- Line 191: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: MaterializedView::isStale()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool MaterializedView::isStale() const {

### query/query_cache.cpp
Total findings: 7

- Line 264: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = cache_.find(fingerprint);
- Line 325: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = cache_.find(fingerprint);
- Line 493: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = cache_.find(fingerprint);
- Line 139: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 288: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(cache_mutex_);
- Line 579: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = dependency_index_.find(dep);
- Line 287: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: QueryCache::clear()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: Result<void> QueryCache::clear() {

### query/result_type_annotation.cpp
Total findings: 7

- Line 182: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = row.begin(); it != row.end(); ++it) {
- Line 86: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (std::isfinite(d) && d == std::floor(d) &&
- Line 185: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (seen_fields.find(fname) == seen_fields.end()) {
- Line 203: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (row.find(fname) == row.end()) {
- Line 175: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, ResultFieldType> field_types;
- Line 176: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string>                  nullable_fields;
- Line 177: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string>                  seen_fields;

### query/continuous_query_engine.cpp
Total findings: 6

- Line 140: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: loop_thread_.join();
- Line 73: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return cancelled_.load(std::memory_order_acquire);
- Line 126: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(loop_mutex_);
- Line 254: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> inj_lock(inject_mutex_);
- Line 149: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ContinuousQueryEngineImpl::registerQuery(ContinuousQuerySpec spec) {
- Line 188: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> ContinuousQueryEngineImpl::dropQuery(const std::string& name) {

### query/cte_cache.cpp
Total findings: 6

- Line 155: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 208: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: total_estimate += (sizeof(nlohmann::json) + 64) * data.size();
- Line 155: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (entry.is_spilled && !entry.spill_file_path.empty()) {

            try {

                std::filesystem::remove(entry.spill_file_path);

            } catch (...) {}

        }

    }
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 239: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file.close();
- Line 286: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file.close();

### query/workload_cache_strategy.cpp
Total findings: 6

- Line 174: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 175: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 176: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 177: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 444: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(limit, query_frequencies.size()); ++i) {
- Line 165: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool should_detect = false;

### query/aql_safety_validator.cpp
Total findings: 5

- Line 125: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "enforce_read_only context. This pattern can delete entire "
- Line 125: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: fmt::format(

                    "AQL_READ_ONLY_VIOLATION: Full-collection REMOVE pattern "

                    "'FOR ... REMOVE' detected (FOR at {}, REMOVE at {}) in an "

                    "enforce_read_only context. This pattern can delete entire "

                    "collections and is blocked unconditionally.",

                    forPos, removePos + 1)

            };
- Line 125: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "enforce_read_only context. This pattern can delete entire "
- Line 12: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 5 (ASL-3)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 5 (ASL-3)
- Line 125: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "enforce_read_only context. This pattern can delete entire "

### query/plan_cache.cpp
Total findings: 5

- Line 213: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = cache_.begin(); it != cache_.end(); ++it) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4229 feat(query): Query Plan Cac... (2026-03-15) | #3226 [graph] Register pa
- Line 186: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = cache_.find(fp);
- Line 211: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_map<std::string, Entry>::iterator> to_remove;
- Line 270: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Entry>::iterator it)

### query/query_cache_manager.cpp
Total findings: 5

- Line 133: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cache_result = basic_cache_->get(query, params);
- Line 135: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result = cache_result->result;
- Line 140: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cache_entry = adaptive_cache_->get(fingerprint, "");
- Line 142: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result = cache_entry->result;
- Line 528: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto put_result = basic_cache_->put(query, params, result, dependencies, ttl);

### query/semantic_cache.cpp
Total findings: 5

- Line 455: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = lru_map_.find(queryStr);
- Line 191: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string exactKey = makeExactMatchKey_(query);

    db_.del(exactKey);

    

    // Note: We don't remove from vector index (would need vector index delete API)

    // Vector index entries become stale but won't match on similarity threshold

    

    // Update LRU
- Line 191: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Note: We don't remove from vector index (would need vector index delete API)
- Line 216: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 548: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(tokens.begin(), tokens.end(), kw) != tokens.end()) {

### query/window_evaluator.cpp
Total findings: 5

- Line 139: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        for (size_t i = 0; i < sortedIndices.size(); ++i) {', '            size_t originalIdx = sortedIndices[i];', '            results[originalIdx] = partitionResults[i];', '        }', '    }']
- Line 390: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            // Zugriff auf vorherige Row', '            size_t prevRowIdx = sortedIndices[static_cast<size_t>(lagIdx)];', '            const auto& prevRow = rows[prevRowIdx];', '', '            if (argument) {']
- Line 430: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            // Zugriff auf nächste Row', '            size_t nextRowIdx = sortedIndices[static_cast<size_t>(leadIdx)];', '            const auto& nextRow = rows[nextRowIdx];', '', '            if (argument) {']
- Line 457: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // FIRST_VALUE ist der Wert der ersten Row in der Partition', '    size_t firstRowIdx = sortedIndices[0];', '    const auto& firstRow = rows[firstRowIdx];', '', '    nlohmann::json firstVal = nullptr;']
- Line 512: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        }', '', '        const auto& lastRow = rows[lastRowIdx];', '', '        nlohmann::json lastVal = nullptr;']

### query/functions/ethics_functions.cpp
Total findings: 4

- Line 148: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    [[maybe_unused]] const std::string& philosophy = args[0];', '    [[maybe_unused]] const json& types = args.size() > 1 ? args[1] : json::array();', '    [[maybe_unused]] int limit = args.size() > 2 ? args[2].get<int>() : 20;', '', '    // F-028: throw so the AQL runtime surfaces a real error instead of']
- Line 175: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    [[maybe_unused]] const std::string& query_text = args[0];', '    [[maybe_unused]] double threshold = args.size() > 1 ? args[1].get<double>() : 0.65;', '    [[maybe_unused]] int limit = args.size() > 2 ? args[2].get<int>() : 10;', '', '    // F-028: throw instead of silent empty array.']
- Line 195: severity=CRITICAL; category=function_return_truncation
  Description: Function return (malloc, read, size()) assigned to smaller int type
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // Requires the ethics_ai plugin to create the ethics_arguments_graph.', '    [[maybe_unused]] const std::string& start_id = args[0];', '    [[maybe_unused]] int max_depth = args.size() > 1 ? args[1].get<int>() : 5;', '', '    // F-028: throw instead of silent empty array.']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3574 fix: clear all remaining st... (2026-03-12) | #946 [FEATURE] Ethics AI

### query/query_canceller.cpp
Total findings: 4

- Line 38: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (it == tokens_.end()) {

        return false;

    }

    auto token = it->second.lock();

    if (!token) {

        tokens_.erase(it);

        return false;
- Line 38: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto token = it->second.lock();
- Line 23: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: QueryCanceller::registerQuery(const std::string& request_id) {
- Line 47: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void QueryCanceller::unregisterQuery(const std::string& request_id) {

### query/approximate_aggregator.cpp
Total findings: 3

- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: registers_[static_cast<size_t>(i)] = o->registers_[static_cast<size_t>(i)];
- Line 166: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 105: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::log(static_cast<double>(num_registers_) /

### query/aql_runner.cpp
Total findings: 3

- Line 829: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ── SQL dialect compatibility layer ──────────────────────────────────────────
- Line 81: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: continue;

            }

            out.emplace_back(e.getPrimaryKey(), std::move(geom));

        } catch (...) {

            ++skipped;

            // Skip documents with unparseable geometry

        }
- Line 81: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### query/cq_watermark.cpp
Total findings: 3

- Line 49: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from max_seen never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const int64_t max_seen = max_seen_us_.load(std::memory_order_acquire);
- Line 21: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: int64_t current_max = max_seen_us_.load(std::memory_order_relaxed);
- Line 30: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const int64_t wm = watermark_us_.load(std::memory_order_acquire);

### query/functions/process_mining_functions.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3636 fix(query): build system au... (2026-03-12) | #1100 [WIP] Fix missing a
- Line 137: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!ev.activity.empty() && act_to_id.find(ev.activity) == act_to_id.end()) {
- Line 118: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int> act_to_id;

### query/functions/udf_registry.cpp
Total findings: 3

- Line 264: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": division by zero");
- Line 269: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (r == 0.0) throw std::runtime_error(def_.name + ": modulo by zero");
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: callArgs.push_back(evalExpr(a, args, context, depth + 1));

### query/query_rewrite_rule.cpp
Total findings: 3

- Line 94: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator v may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto v = eq.find("value");
- Line 341: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: else if (t == "div" && rv != 0.0) result = lv / rv;
- Line 362: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> seen;

### query/adaptive_optimizer.cpp
Total findings: 2

- Line 509: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pthread_t thread = pthread_self();
- Line 510: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) == 0;

### query/aql_parser_json.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3427 feat(query): Per-query reso... (2026-03-12) | #3352 feat(query): SPARQL
- Line 45: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: {"object", object->toJSON()},

### query/result_stream.cpp
Total findings: 2

- Line 180: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 184: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### query/statistical_aggregator.cpp
Total findings: 2

- Line 58: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    if (lowerIndex == upperIndex) {', '        return Ok(nlohmann::json(values[lowerIndex]));', '    }', '']
- Line 63: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // Linear interpolation', '    double weight = rank - lowerIndex;', '    double result = values[lowerIndex] * (1.0 - weight) + values[upperIndex] * weight;', '', '    return Ok(nlohmann::json(result));']

### query/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ARCHITECTURE.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### query/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### query/README.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'README.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### query/ROADMAP.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ROADMAP.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, README.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### query/functions/function_registry.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4220 feat(aql): wire detectInten... (2026-03-14) | #2758 [analytics] Advance

### query/materialized_cte.cpp
Total findings: 1

- Line 182: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = vrow.values.find(agg.output_name);

### query/query_profiler.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12) | #3328 [WIP] Add SLO/SLA c

### query/vectorized_execution.cpp
Total findings: 1

- Line 259: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> col_index;

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
