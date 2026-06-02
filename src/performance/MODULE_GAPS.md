# performance Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: performance
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 111
- Actionable Findings (Critical + High): 24
- Affected Files: 29

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 23 |
| Medium | 86 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| concurrency | 90 |
| performance_patterns | 77 |
| reliability | 60 |
| raii | 48 |
| container | 46 |
| exception_safety | 23 |
| determinism | 22 |
| memory | 15 |
| observability | 10 |
| performance | 9 |
| input_validation | 8 |
| security | 7 |
| platform | 6 |
| uninitialized | 3 |
| audit_logging | 2 |
| distributed_consistency | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/performance/adaptive_query_compiler.cpp | 28 | 0 | 10 | 18 | 0 |
| src/performance/hardware_accelerator.cpp | 14 | 0 | 2 | 12 | 0 |
| src/performance/ligra.cpp | 10 | 0 | 0 | 10 | 0 |
| src/performance/numa_topology.cpp | 9 | 0 | 0 | 9 | 0 |
| src/performance/phase3/diskann.cpp | 8 | 0 | 1 | 7 | 0 |
| src/performance/phase3/per_query_cost_model.cpp | 5 | 0 | 0 | 5 | 0 |
| src/performance/cicada.cpp | 4 | 0 | 3 | 1 | 0 |
| src/performance/rabitq.cpp | 4 | 0 | 0 | 4 | 0 |
| src/performance/intelligent_prefetcher.cpp | 3 | 0 | 0 | 3 | 0 |
| src/performance/phase3/adaptive_batch_tuner.cpp | 3 | 0 | 2 | 1 | 0 |
| src/performance/workload_adaptive_optimizer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/performance/advanced_cache_manager.cpp | 2 | 0 | 1 | 1 | 0 |
| src/performance/async_metrics_exporter.cpp | 2 | 1 | 1 | 0 | 0 |
| src/performance/numa_memory_manager.cpp | 2 | 0 | 2 | 0 | 0 |
| src/performance/phase3/gunrock.cpp | 2 | 0 | 0 | 2 | 0 |
| src/performance/phase3/memory_pressure.cpp | 2 | 0 | 0 | 2 | 0 |
| src/performance/chimera_exporter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase2_feature_flags.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase3/bao.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase3/bwtree.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase3/feature_flags.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase3/splinterdb.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase4/io_uring_zero_copy.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/phase4/pmem_storage.cpp | 1 | 0 | 1 | 0 | 0 |
| src/performance/prometheus_exporter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/performance/wisckey.cpp | 1 | 0 | 0 | 0 | 1 |
| src/performance/cycle_metrics.cpp | 0 | 0 | 0 | 0 | 0 |
| src/performance/phase4/feature_flags.cpp | 0 | 0 | 0 | 0 | 0 |
| src/performance/phase4/pmu_counters.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/performance/adaptive_query_compiler.cpp
Total findings: 28

- Line 92: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::EQ:   return lhs == rhs;
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::EQ:   return lhs == rhs;
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::EQ:   return lhs == rhs;
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return lhs == pattern;
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (entry.baseline_row_count == 0.0)
  Confidence: band=very_high; score=0.9
- Line 697: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fn == "MIN")   return acc.min_v != std::numeric_limits<double>::max()
  Confidence: band=very_high; score=0.9
- Line 699: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fn == "MAX")   return acc.max_v != std::numeric_limits<double>::lowest()
  Confidence: band=very_high; score=0.9
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.column_names.push_back(col.name);
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proj_row.column_names.push_back(col_name);
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proj_row.column_names.push_back(col_name);
  Confidence: band=high; score=0.74
- Line 628: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_row.column_names.push_back(query.agg_function + "_result");
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, AggAccum> groups;
  Confidence: band=medium; score=0.66
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_order.push_back(gkey);
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_row.column_names.push_back(query.group_by_column);
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined.column_names.push_back(
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined.column_names.push_back(
  Confidence: band=high; score=0.74
- Line 865: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preds.push_back({p.column, p.op, p.value, p.param_name, ct});
  Confidence: band=high; score=0.74
- Line 949: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) base.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 949: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) base.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 969: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.column_names.push_back(agg_fn + "_result");
  Confidence: band=high; score=0.74
- Line 979: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, AggAccum> groups;
  Confidence: band=medium; score=0.66
- Line 987: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (groups.find(gk) == groups.end()) order.push_back(gk);
  Confidence: band=high; score=0.74
- Line 1009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(out));
  Confidence: band=high; score=0.74

### src/performance/hardware_accelerator.cpp
Total findings: 14

- Line 156: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = ht.find(key);
  Confidence: band=very_high; score=0.9
- Line 700: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
  Confidence: band=very_high; score=0.9
- Line 97: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<size_t>>
  Confidence: band=medium; score=0.66
- Line 99: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<size_t>> ht;
  Confidence: band=medium; score=0.66
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[rows[i][key_col]].push_back(i);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (match) r.match_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 700: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
  Confidence: band=high; score=0.74

### src/performance/ligra.cpp
Total findings: 10

- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([it, chunk_end, &func]() {
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([it, chunk_end, &func]() {
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([start, end, &frontier, &func]() {
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([start, end, &frontier, &func]() {
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([t, it, chunk_end, &adj_list, &func, &thread_buffers]() {
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([t, it, chunk_end, &adj_list, &func, &thread_buffers]() {
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: thread_buffers[t].push_back(dst);  // Collision-free thread index
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: thread_buffers[t].push_back(dst);  // Collision-free thread index
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tasks_.push_back(task);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tasks_.push_back(task);
  Confidence: band=high; score=0.74

### src/performance/numa_topology.cpp
Total findings: 9

- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.cpu_ids.push_back(cpu_base + bit);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.cpu_ids.push_back(cpu_base + bit);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (CPU_ISSET(static_cast<unsigned int>(i), &cs)) cpus.push_back(i);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (CPU_ISSET(static_cast<unsigned int>(i), &cs)) cpus.push_back(i);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cpus.push_back(base + bit);
  Confidence: band=high; score=0.74

### src/performance/phase3/diskann.cpp
Total findings: 8

- Line 108: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& node : nodes) {
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes[i].neighbors.push_back(nodes[nearest.top().second].id);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes[i].neighbors.push_back(nodes[nearest.top().second].id);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.neighbors.push_back(nearest.top().second);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({id, dist});
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.push_back(compute_distance(node->vector, vectors[i].second));
  Confidence: band=high; score=0.74

### src/performance/phase3/per_query_cost_model.cpp
Total findings: 5

- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(records_[pos]);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> factors;
  Confidence: band=medium; score=0.66
- Line 261: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> type_time_sum;
  Confidence: band=medium; score=0.66
- Line 262: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> type_count;
  Confidence: band=medium; score=0.66
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: times_ms.push_back(r.execution_time_ms);
  Confidence: band=high; score=0.74

### src/performance/cicada.cpp
Total findings: 4

- Line 24: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: bool CicadaTransaction::execute(const TransactionFunc& func) {
  Confidence: band=very_high; score=0.9
- Line 68: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool CicadaTransaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void CicadaTransaction::abort() {
  Confidence: band=very_high; score=0.9
- Line 24: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool CicadaTransaction::execute(const TransactionFunc& func) {
  Confidence: band=high; score=0.74

### src/performance/rabitq.cpp
Total findings: 4

- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74

### src/performance/intelligent_prefetcher.cpp
Total findings: 3

- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predictions.push_back(static_cast<uint64_t>(addr));
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int64_t, size_t> stride_counts;
  Confidence: band=medium; score=0.66
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_.addresses.push_back(history_[i].address);
  Confidence: band=high; score=0.74

### src/performance/phase3/adaptive_batch_tuner.cpp
Total findings: 3

- Line 77: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (ema_throughput_ == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 135: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: } else if (throughput_improving || prev_ema_ == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(r.latency_ms);
  Confidence: band=high; score=0.74

### src/performance/workload_adaptive_optimizer.cpp
Total findings: 3

- Line 56: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> table_counts;
  Confidence: band=medium; score=0.66
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.hot_tables.push_back(tvec[i].first);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.hot_tables.push_back(tvec[i].first);
  Confidence: band=high; score=0.74

### src/performance/advanced_cache_manager.cpp
Total findings: 2

- Line 462: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& p : partitions_) {
  Confidence: band=very_high; score=0.9
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions_.push_back(std::move(ps));
  Confidence: band=high; score=0.74

### src/performance/async_metrics_exporter.cpp
Total findings: 2

- Line 228: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: aggregated_metrics_.insert(
  Confidence: band=very_high; score=0.99
- Line 112: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: export_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9

### src/performance/numa_memory_manager.cpp
Total findings: 2

- Line 160: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* NUMAMemoryManager::allocate(size_t size, const AllocationHint& hint) {
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void NUMAMemoryManager::deallocate(void* ptr, size_t size) noexcept {
  Confidence: band=very_high; score=0.9

### src/performance/phase3/gunrock.cpp
Total findings: 2

- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->column_indices.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->column_indices.push_back(neighbor);
  Confidence: band=high; score=0.74

### src/performance/phase3/memory_pressure.cpp
Total findings: 2

- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_call.push_back(entry.callback);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_call.push_back(entry.callback);
  Confidence: band=high; score=0.74

### src/performance/chimera_exporter.cpp
Total findings: 1

- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated[entry.operation_name].push_back(&entry.metrics);
  Confidence: band=high; score=0.74

### src/performance/phase2_feature_flags.cpp
Total findings: 1

- Line 28: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto phase2 = config["performance"]["phase2"];
  Confidence: band=high; score=0.74

### src/performance/phase3/bao.cpp
Total findings: 1

- Line 25: severity=MEDIUM; category=determinism; pattern=random_unseeded
  Description: RNG engine appears default-constructed without explicit seeding
  Context: std::mt19937 rng;
  Confidence: band=high; score=0.74

### src/performance/phase3/bwtree.cpp
Total findings: 1

- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({k, v});
  Confidence: band=high; score=0.74

### src/performance/phase3/feature_flags.cpp
Total findings: 1

- Line 29: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto phase3 = config["performance"]["phase3"];
  Confidence: band=high; score=0.74

### src/performance/phase3/splinterdb.cpp
Total findings: 1

- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74

### src/performance/phase4/io_uring_zero_copy.cpp
Total findings: 1

- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffers_.emplace_back(config_.buffer_size);
  Confidence: band=high; score=0.74

### src/performance/phase4/pmem_storage.cpp
Total findings: 1

- Line 286: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* PMemPool::allocate(size_t size) noexcept {
  Confidence: band=very_high; score=0.9

### src/performance/prometheus_exporter.cpp
Total findings: 1

- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated[entry.operation_name].push_back(&entry.metrics);
  Confidence: band=high; score=0.74

### src/performance/wisckey.cpp
Total findings: 1

- Line 101: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::fstream temp_log(temp_log_path, std::ios::out | std::ios::binary);
  Confidence: band=medium; score=0.6

### src/performance/cycle_metrics.cpp
Total findings: 0


### src/performance/phase4/feature_flags.cpp
Total findings: 0


### src/performance/phase4/pmu_counters.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
