# sharding Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: sharding
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 2272
- Actionable Findings (Critical + High): 1362
- Affected Files: 77

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 324 |
| High | 1038 |
| Medium | 910 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| distributed_consistency | 466 |
| container | 459 |
| performance_patterns | 458 |
| reliability | 179 |
| memory | 168 |
| raii | 110 |
| concurrency | 100 |
| exception_safety | 85 |
| performance | 63 |
| observability | 46 |
| legacy_duplication | 31 |
| determinism | 21 |
| security | 18 |
| input_validation | 17 |
| type_conversion | 13 |
| audit_logging | 12 |
| platform | 10 |
| uninitialized | 7 |
| oop_design | 6 |
| llm_ai_safety | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/sharding/redundancy_strategy.cpp | 227 | 28 | 85 | 114 | 0 |
| src/sharding/cross_shard_transaction.cpp | 151 | 21 | 52 | 78 | 0 |
| src/sharding/shard_router.cpp | 117 | 32 | 43 | 42 | 0 |
| src/sharding/stream_protocol.cpp | 105 | 11 | 65 | 29 | 0 |
| src/sharding/distributed_transaction.cpp | 63 | 11 | 26 | 26 | 0 |
| src/sharding/cloud_backup.cpp | 55 | 0 | 37 | 18 | 0 |
| src/sharding/gossip_protocol.cpp | 53 | 6 | 20 | 27 | 0 |
| src/sharding/paxos_consensus.cpp | 53 | 2 | 37 | 14 | 0 |
| src/sharding/epoch_fencing.cpp | 51 | 4 | 39 | 8 | 0 |
| src/sharding/shard_rpc_client.cpp | 51 | 0 | 21 | 30 | 0 |
| src/sharding/signed_request.cpp | 50 | 7 | 34 | 9 | 0 |
| src/sharding/slo_monitor.cpp | 49 | 10 | 20 | 19 | 0 |
| src/sharding/shard_load_detector.cpp | 42 | 1 | 5 | 36 | 0 |
| src/sharding/gossip_config_manager.cpp | 40 | 18 | 15 | 7 | 0 |
| src/sharding/adaptive_shard_router.cpp | 39 | 4 | 16 | 19 | 0 |
| src/sharding/auto_rebalancer.cpp | 39 | 9 | 6 | 24 | 0 |
| src/sharding/gpu_erasure_coder_opencl.cpp | 38 | 1 | 19 | 17 | 1 |
| src/sharding/shard_topology.cpp | 38 | 0 | 9 | 29 | 0 |
| src/sharding/two_phase_commit_coordinator.cpp | 37 | 6 | 24 | 7 | 0 |
| src/sharding/capability_matcher.cpp | 36 | 3 | 7 | 24 | 2 |
| src/sharding/metadata_shard.cpp | 36 | 12 | 14 | 10 | 0 |
| src/sharding/replica_consistency.cpp | 35 | 5 | 18 | 12 | 0 |
| src/sharding/cloud_agent.cpp | 34 | 6 | 15 | 13 | 0 |
| src/sharding/health_check.cpp | 34 | 6 | 15 | 13 | 0 |
| src/sharding/raft_consensus.cpp | 34 | 2 | 19 | 13 | 0 |
| src/sharding/pki_shard_certificate.cpp | 33 | 4 | 21 | 8 | 0 |
| src/sharding/predictive_detector.cpp | 33 | 1 | 14 | 18 | 0 |
| src/sharding/wal_manager.cpp | 33 | 3 | 9 | 21 | 0 |
| src/sharding/hot_spare_manager.cpp | 30 | 4 | 15 | 11 | 0 |
| src/sharding/raft_log.cpp | 29 | 6 | 12 | 11 | 0 |
| src/sharding/mtls_connection_pool.cpp | 27 | 2 | 24 | 1 | 0 |
| src/sharding/wal_shipper.cpp | 26 | 1 | 17 | 8 | 0 |
| src/sharding/shard_repair_engine.cpp | 25 | 2 | 13 | 10 | 0 |
| src/sharding/paxos_snapshot.cpp | 23 | 1 | 16 | 6 | 0 |
| src/sharding/prometheus_metrics.cpp | 23 | 10 | 4 | 9 | 0 |
| src/sharding/two_phase_commit_participant.cpp | 22 | 0 | 21 | 1 | 0 |
| src/sharding/consistent_hash.cpp | 21 | 8 | 4 | 9 | 0 |
| src/sharding/truetime.cpp | 21 | 2 | 10 | 9 | 0 |
| src/sharding/data_migrator.cpp | 19 | 7 | 6 | 6 | 0 |
| src/sharding/health_monitor.cpp | 19 | 2 | 13 | 4 | 0 |
| src/sharding/raft_consensus_adapter.cpp | 19 | 1 | 15 | 3 | 0 |
| src/sharding/quorum_manager.cpp | 18 | 0 | 3 | 15 | 0 |
| src/sharding/raft_wal_integration.cpp | 18 | 11 | 7 | 0 | 0 |
| src/sharding/shard_resource_manager.cpp | 18 | 5 | 4 | 9 | 0 |
| src/sharding/gossip_consensus_adapter.cpp | 16 | 1 | 11 | 4 | 0 |
| src/sharding/shard_rpc_server.cpp | 16 | 1 | 11 | 4 | 0 |
| src/sharding/admin_api.cpp | 15 | 0 | 5 | 10 | 0 |
| src/sharding/hardware_migration_manager.cpp | 15 | 3 | 7 | 5 | 0 |
| src/sharding/raft_configuration.cpp | 15 | 0 | 15 | 0 | 0 |
| src/sharding/wal_applier.cpp | 15 | 7 | 5 | 3 | 0 |
| src/sharding/locality_aware_router.cpp | 14 | 2 | 3 | 9 | 0 |
| src/sharding/partition_detector.cpp | 13 | 0 | 5 | 8 | 0 |
| src/sharding/shard_durability.cpp | 12 | 0 | 3 | 9 | 0 |
| src/sharding/sharding_manager_edition.cpp | 12 | 1 | 6 | 5 | 0 |
| src/sharding/urn_resolver.cpp | 12 | 7 | 1 | 4 | 0 |
| src/sharding/distributed_coordinator.cpp | 11 | 0 | 10 | 1 | 0 |
| src/sharding/metadata_snapshot.cpp | 11 | 0 | 6 | 5 | 0 |
| src/sharding/multi_primary_coordinator.cpp | 11 | 6 | 3 | 2 | 0 |
| src/sharding/paxos_state_persistence.cpp | 11 | 2 | 8 | 1 | 0 |
| src/sharding/raft_shard_manager.cpp | 11 | 2 | 3 | 6 | 0 |
| src/sharding/circuit_breaker.cpp | 10 | 0 | 8 | 2 | 0 |
| src/sharding/mtls_client.cpp | 10 | 5 | 5 | 0 | 0 |
| src/sharding/remote_executor.cpp | 10 | 2 | 6 | 2 | 0 |
| src/sharding/operational_metrics.cpp | 9 | 2 | 2 | 5 | 0 |
| src/sharding/transaction_snapshot.cpp | 8 | 0 | 2 | 6 | 0 |
| src/sharding/transaction_wal.cpp | 8 | 0 | 6 | 2 | 0 |
| src/sharding/orphan_detector.cpp | 7 | 0 | 0 | 7 | 0 |
| src/sharding/raft_state.cpp | 6 | 1 | 5 | 0 | 0 |
| src/sharding/gpu_erasure_coder.cpp | 5 | 2 | 1 | 2 | 0 |
| src/sharding/metadata_wal.cpp | 5 | 2 | 1 | 2 | 0 |
| src/sharding/replication_coordinator.cpp | 5 | 2 | 3 | 0 | 0 |
| src/sharding/paxos_wal.cpp | 4 | 1 | 1 | 2 | 0 |
| src/sharding/rebalance_operation.cpp | 3 | 0 | 3 | 0 | 0 |
| src/sharding/secure_transport_client.cpp | 3 | 0 | 3 | 0 | 0 |
| src/sharding/admin_operations.cpp | 2 | 0 | 0 | 2 | 0 |
| src/sharding/replica_topology.cpp | 2 | 0 | 0 | 2 | 0 |
| src/sharding/metrics_registry.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/sharding/redundancy_strategy.cpp
Total findings: 227

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        std::vector<uint8_t> chunk(chunk_size, 0);  // Pad with zeros', '        if (offset < data.size()) {', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(chunk);']
  Confidence: band=very_high; score=0.9
- Line 358: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: recovered.insert(recovered.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 417: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 727: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: recovered.insert(recovered.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 798: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 967: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = available_chunks.find(s);
- Line 970: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), it->second.begin(), it->second.end());
  Confidence: band=very_high; score=0.99
- Line 1082: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), shards[s].begin(), shards[s].end());
  Confidence: band=very_high; score=0.99
- Line 1160: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = available_chunks.find(s);
- Line 1165: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), it->second.begin(), it->second.end());
  Confidence: band=very_high; score=0.99
- Line 1262: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), shards[s].begin(), shards[s].end());
  Confidence: band=very_high; score=0.99
- Line 1328: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: WriteResult RedundancyStrategy::write(
  Confidence: band=very_high; score=0.99
- Line 1328: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: WriteResult RedundancyStrategy::write(
- Line 1379: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ReadResult RedundancyStrategy::read(
- Line 1567: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool RedundancyStrategy::proposeRaftWrite(const std::string& shard_id,
  Confidence: band=very_high; score=0.99
- Line 1590: severity=CRITICAL; category=sql_injection; **status=FIXED** (batch2: document_id validated for '|' delimiter at start of proposeRaftWrite; returns false with error log if invalid)
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: SQL injection risk — use prepared statements
  Context: std::string command = "WRITE|" + document_id + "|" +
- Line 1698: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_written_shards.insert(all_written_shards.end(),
  Confidence: band=very_high; score=0.99
- Line 1816: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = region_candidates.find(geo.local_region);
- Line 1953: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: candidates.insert(candidates.end(), replicas.begin(), replicas.end());
  Confidence: band=very_high; score=0.99
- Line 2104: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: available_shards.insert(available_shards.end(), replicas.begin(), replicas.end());
  Confidence: band=very_high; score=0.99
- Line 2161: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge chunks
  Confidence: band=very_high; score=0.99
- Line 2162: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: auto merged = mergeChunks(chunks);
  Confidence: band=very_high; score=0.99
- Line 2165: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.data = std::string(merged.begin(), merged.end());
  Confidence: band=very_high; score=0.99
- Line 2544: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_shards.insert(all_shards.end(), replicas.begin(), replicas.end());
  Confidence: band=very_high; score=0.99
- Line 2606: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: shards.insert(shards.end(), replicas2.begin(), replicas2.end());
  Confidence: band=very_high; score=0.99
- Line 2638: severity=CRITICAL; category=data_race; status=FIXED
  Description: Shared data access without lock protection — FIXED: wrapped in std::shared_lock in recoverDocument (PR #5455)
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto recovered_data = erasure_coder_->decode(available_map, missing_idx_vec, k, m);
- Line 2638: severity=CRITICAL; category=data_race; status=FIXED
  Description: Shared data access without lock protection — FIXED: wrapped in std::shared_lock in recoverDocument (PR #5455)
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto recovered_data = erasure_coder_->decode(available_map, missing_idx_vec, k, m);
- Line 2982: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: config = cfg_it->second;
- Line 0: severity=HIGH; category=uncategorized
  Context: [') {', '    // Calculate chunk size (pad last chunk with zeros if needed)', '    size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '', '    // Split data into k chunks (data shards)']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    // Calculate chunk size', '    size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '', '    // Split data into chunks']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        const uint32_t k = config_.erasure_coding.data_shards;', '        const uint32_t m = config_.erasure_coding.parity_shards;', '        const uint32_t total = k + m;', '', '        // Read all available chunks']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        const uint32_t k = config_.erasure_coding.data_shards;', '        const uint32_t m = config_.erasure_coding.parity_shards;', '        const uint32_t total = k + m;', '        auto replicas = ring.getReplicaNodes(document_id, total - 1);', '        std::vector<std::string> shards{*primary_opt};']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                parity_byte ^= gf_mul(cauchy_matrix[p][d], data_bytes[d]);', '            }', '            parity[byte_pos] = parity_byte;', '        }', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 162: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::optional<ChunkInfo> ChunkInfo::deserialize([[maybe_unused]] const std::vector<uint8_t>& data) {
- Line 191: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool StripeGroup::canRecover(uint32_t data_shards, [[maybe_unused]] uint32_t parity_shards) const {
- Line 240: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Too many shards: rows + cols must be <= 255");
- Line 337: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Too many missing chunks: " +
- Line 343: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Not enough chunks for recovery");
- Line 376: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: full_matrix[data_shards + p] = vandermonde[p];
- Line 395: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to invert decode matrix for Reed-Solomon recovery");
- Line 410: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: recovered_data[i][x] = recovered_bytes[i];
- Line 531: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Too many shards: rows + cols must be <= 256");
- Line 553: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid Cauchy matrix: x[i] == y[j]");
- Line 679: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_bytes[d] = chunks[d][byte_pos];
- Line 685: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: parity_byte ^= gf_mul(cauchy_matrix[p][d], data_bytes[d]);
- Line 704: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Too many missing chunks: " +
- Line 711: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Not enough chunks for recovery");
- Line 750: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: full_matrix[data_shards + i][j] = cauchy_matrix[i][j];
- Line 772: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to invert decode matrix");
- Line 791: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: recovered_data[i][byte_pos] = recovered_bytes[i];
- Line 907: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LRC encode: shard counts must be > 0");
- Line 909: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LRC encode: data must not be empty");
- Line 966: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = available_chunks.find(s);
- Line 967: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = available_chunks.find(s);
  Confidence: band=very_high; score=0.9
- Line 983: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [idx, data] : available_chunks)
- Line 1043: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LRC decode: insufficient shards for recovery");
- Line 1049: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (uint32_t s = 0; s < data_shards; ++s) full_mat[s][s] = 1;
- Line 1050: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (uint32_t p = 0; p < n_global; ++p)   full_mat[data_shards + p] = vand[p];
- Line 1059: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: dec_mat[i] = row_idx < data_shards
- Line 1061: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: : full_mat[data_shards + (row_idx - data_shards)];
- Line 1062: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: rhs[i]     = row_idx < data_shards
- Line 1064: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: : shards[global_start + (row_idx - data_shards)];
- Line 1068: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LRC decode: could not invert recovery matrix");
- Line 1107: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HammingCoder::encode: shard counts must be > 0");
- Line 1109: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HammingCoder::encode: data must not be empty");
- Line 1130: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::vector<uint8_t>& parity = shards[data_shards + p];
- Line 1149: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HammingCoder::decode: no chunks available");
- Line 1159: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = available_chunks.find(s);
- Line 1160: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = available_chunks.find(s);
  Confidence: band=very_high; score=0.9
- Line 1162: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 1253: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 1293: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid redundancy configuration");
- Line 1299: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create erasure coder");
- Line 1331: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: const std::string& collection [[maybe_unused]],
- Line 1379: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ReadResult RedundancyStrategy::read(
  Confidence: band=very_high; score=0.9
- Line 1381: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: const std::string& collection [[maybe_unused]],
- Line 1438: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: ShardTopology& topology [[maybe_unused]],
- Line 1502: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 1605: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool committed = future.get();
  Confidence: band=very_high; score=0.9
- Line 1624: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: ShardTopology& topology [[maybe_unused]],
- Line 1659: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 1710: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: ShardTopology& topology [[maybe_unused]],
- Line 1756: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 1815: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = region_candidates.find(geo.local_region);
- Line 1856: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 2132: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: ShardTopology& topology [[maybe_unused]],
- Line 2161: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge chunks
  Confidence: band=very_high; score=0.9
- Line 2162: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto merged = mergeChunks(chunks);
  Confidence: band=very_high; score=0.9
- Line 2165: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.data = std::string(merged.begin(), merged.end());
  Confidence: band=very_high; score=0.9
- Line 2255: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<uint8_t> RedundancyStrategy::mergeChunks(
  Confidence: band=very_high; score=0.9
- Line 2258: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<uint8_t> merged;
  Confidence: band=very_high; score=0.9
- Line 2261: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.insert(merged.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.9
- Line 2264: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 2272: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("No available shards");
- Line 2333: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("No available shards");
- Line 2405: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: const std::string& collection [[maybe_unused]],
- Line 2506: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 2509: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: from = nullptr;
  Context: spdlog::warn("remove: delete from shard {} failed for doc {}",
- Line 2513: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: from = nullptr;
  Context: spdlog::warn("remove: delete from shard {} threw: {}",
- Line 2524: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: const std::string& collection [[maybe_unused]],
- Line 2638: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto recovered_data = erasure_coder_->decode(available_map, missing_idx_vec, k, m);
- Line 2645: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto all_chunks = erasure_coder_->encode(recovered_data, k, m);
- Line 2673: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: const std::string& collection [[maybe_unused]],
- Line 2771: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid redundancy configuration");
- Line 2994: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, _] : collection_configs_) {
  Confidence: band=very_high; score=0.9
- Line 3010: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility shim: expose under themisdb::sharding
  Confidence: band=high; score=0.8
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(static_cast<uint32_t>(i));
- Line 253: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(parity));
- Line 330: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: available_indices.push_back(idx);
- Line 422: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 436: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 448: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 452: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
  Confidence: band=medium; score=0.56
- Line 481: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 501: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 584: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 664: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 690: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(parity);
- Line 697: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 759: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 759: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 759: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 759: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 759: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 760: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: available_indices.push_back(idx);
- Line 958: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 1028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1029: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!recovered[s]) still_missing.push_back(s);
- Line 1037: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (recovered[s]) avail_rows.push_back(s);
  Confidence: band=high; score=0.74
- Line 1038: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (recovered[s]) avail_rows.push_back(s);
- Line 1039: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (recovered[global_start + p]) avail_rows.push_back(data_shards + p);
  Confidence: band=high; score=0.74
- Line 1040: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (recovered[global_start + p]) avail_rows.push_back(data_shards + p);
- Line 1143: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 1270: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
  Confidence: band=medium; score=0.56
- Line 1484: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id]() {
  Confidence: band=high; score=0.74
- Line 1485: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id]() {
- Line 1495: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(wait_timeout);
  Confidence: band=high; score=0.74
- Line 1497: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1497: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1498: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_shards.push_back(target_shards[i]);
- Line 1503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: written_shards.push_back(target_shards[i]);
- Line 1506: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_shards.push_back(target_shards[i]);
- Line 1510: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_shards.push_back(target_shards[i]);
- Line 1649: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id, chunk]() {
  Confidence: band=high; score=0.74
- Line 1650: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id, chunk]() {
- Line 1659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1660: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: written_shards.push_back(target_shards[i]);
- Line 1746: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id, chunk, chunk_id]() {
  Confidence: band=high; score=0.74
- Line 1747: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id, chunk, chunk_id]() {
- Line 1756: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1757: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: written_shards.push_back(target_shards[i]);
- Line 1803: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> region_candidates;
  Confidence: band=high; score=0.74
- Line 1806: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: region_candidates[region].push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1807: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: region_candidates[region].push_back(shard_id);
- Line 1811: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> write_failed_set(
  Confidence: band=medium; score=0.66
- Line 1818: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : it->second) target_shards.push_back(s);
- Line 1824: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : shards) target_shards.push_back(s);
- Line 1839: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 1840: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 1850: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(config_.replication_timeout);
  Confidence: band=high; score=0.74
- Line 1852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1853: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_shards.push_back(target_shards[i]);
- Line 1857: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: written_shards.push_back(target_shards[i]);
- Line 1860: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_shards.push_back(target_shards[i]);
- Line 1864: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_shards.push_back(target_shards[i]);
- Line 1871: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
  Confidence: band=medium; score=0.66
- Line 1982: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> failed_set;
  Confidence: band=medium; score=0.66
- Line 1987: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, uint32_t> region_reads;
  Confidence: band=high; score=0.74
- Line 2064: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Bounded-staleness / follower-read fallback: try remaining candidates
  Confidence: band=high; score=0.74
- Line 2152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(*data_opt);
  Confidence: band=high; score=0.74
- Line 2153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(*data_opt);
- Line 2204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 2205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing_indices.push_back(i);
- Line 2248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 2249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 2301: severity=MEDIUM; category=determinism; pattern=random_unseeded
  Description: rand() without nearby explicit srand() seeding
  Context: size_t idx = std::rand() % available_shards.size();
  Confidence: band=high; score=0.74
- Line 2369: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
  Confidence: band=medium; score=0.66
- Line 2386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geo.failed_regions.push_back(region);
  Confidence: band=high; score=0.74
- Line 2387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geo.failed_regions.push_back(region);
- Line 2442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.emplace_back(shards[i],
  Confidence: band=high; score=0.74
- Line 2461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.emplace_back(shards[i], key);
  Confidence: band=high; score=0.74
- Line 2468: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(*primary_opt);
- Line 2474: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> failed_set(failed_regions.begin(),
  Confidence: band=medium; score=0.66
- Line 2496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 2497: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 2509: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: spdlog::warn("remove: delete from shard {} failed for doc {}",
- Line 2633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_idx_vec.push_back(i);
  Confidence: band=high; score=0.74
- Line 2633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_idx_vec.push_back(i);
  Confidence: band=high; score=0.74
- Line 2634: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing_idx_vec.push_back(i);
- Line 2706: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health.missing_shards.push_back(all_shards[i]);
  Confidence: band=high; score=0.74
- Line 2707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: health.missing_shards.push_back(all_shards[i]);
- Line 2734: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health.missing_shards.push_back(shards[i]);
  Confidence: band=high; score=0.74
- Line 2735: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: health.missing_shards.push_back(shards[i]);
- Line 2752: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health.missing_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2753: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: health.missing_shards.push_back(shard_id);
- Line 2920: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Bounded-staleness limit
  Confidence: band=high; score=0.74
- Line 2921: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ss << "# HELP themis_geo_max_staleness_ms Maximum accepted replication staleness in ms\n";
  Confidence: band=high; score=0.74
- Line 2922: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ss << "# TYPE themis_geo_max_staleness_ms gauge\n";
  Confidence: band=high; score=0.74
- Line 2923: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ss << "themis_geo_max_staleness_ms{mode=\"geo_mirror\"} " << geo.max_staleness_ms << "\n";
  Confidence: band=high; score=0.74
- Line 2994: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collections.push_back(name);
  Confidence: band=high; score=0.74
- Line 2995: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: collections.push_back(name);

### src/sharding/cross_shard_transaction.cpp
Total findings: 151

- Line 97: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator self_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto self_it = graph.find(component.front());
- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto waiting_state = waiting_it->second.state;
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto blocking_state = blocking_it->second.state;
- Line 589: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: replaced lock.lock() with lock.try_lock_for(config_.lock_timeout); transactions_mutex_ upgraded to std::timed_mutex)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 599: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex upgrade and try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 622: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex upgrade and try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 653: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference: a concurrent abort() could erase the map entry while we are
  Confidence: band=very_high; score=0.99
- Line 683: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: try_lock_for(config_.lock_timeout) with return false on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 728: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference: a concurrent commit() could erase the map entry while we are
  Confidence: band=very_high; score=0.99
- Line 762: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: try_lock_for(config_.lock_timeout) with return false on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 855: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: try_lock_for(config_.lock_timeout) with SAGA compensation and return false on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 992: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: try_lock_for(config_.lock_timeout) with return false on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 1900: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Only merge remote edges that reference known live
  Confidence: band=very_high; score=0.99
- Line 1911: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto waiting_state = waiting_it->second.state;
- Line 1912: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto blocking_state = blocking_it->second.state;
- Line 2028: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto waiting_state = waiting_it->second.state;
- Line 2039: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto blocking_state = blocking_it->second.state;
- Line 2061: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = distributed_wait_for_edges_.begin();
- Line 2089: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = graph.find(start_node);
- Line 2581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats->in_doubt_transactions = static_cast<uint64_t>(std::count_if(
- Line 2625: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats->pending_transactions =
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 72: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: strong_connect(neighbor);
- Line 114: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: strong_connect(node);
- Line 125: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& cycle : collectCycles(graph)) {
- Line 159: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(msg);
- Line 294: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: deadlock_detection_thread_ = std::thread(
  Confidence: band=very_high; score=0.9
- Line 506: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<void>(consensus_->propose("BEGIN_TRANSACTION", data));
- Line 546: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool CrossShardTransactionCoordinator::prepare(const std::string& transaction_id) {
- Line 567: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [shard_id, participant] : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 575: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logPrepare(transaction_id, shard_id, prepare_data);
- Line 643: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool CrossShardTransactionCoordinator::commit(const std::string& transaction_id) {
  Confidence: band=very_high; score=0.9
- Line 718: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool CrossShardTransactionCoordinator::abort(const std::string& transaction_id) {
  Confidence: band=very_high; score=0.9
- Line 946: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logCompensate(transaction_id, shard_id, compensate_data);
- Line 1085: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!prepare(txn.transaction_id)) {
- Line 1104: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: commit_data["participants"].push_back(shard_id);
- Line 1106: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logCommit(txn.transaction_id, commit_data);
- Line 1123: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute2PC [{}]: WAL ABORT log failed during fail-closed handling: {}",
- Line 1137: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute2PC [{}]: fail-closed abort RPC failed for shard {}",
- Line 1147: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute2PC [{}]: WAL ABORTED log failed for shard {}: {}",
- Line 1161: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute2PC [{}]: Commit failed for shard {} - failing closed",
- Line 1176: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute2PC [{}]: WAL COMMITTED log failed for shard {}: {} - failing closed",
- Line 1221: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute3PC [{}]: missing PreCommit RPC callback; failing closed",
- Line 1228: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute3PC [{}]: fail-closed abort RPC failed for shard {}",
- Line 1238: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::error("execute3PC [{}]: failed to log fail-closed ABORT to WAL: {}",
- Line 1257: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: precommit_data["participants"].push_back(shard_id);
- Line 1259: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logCommit(txn.transaction_id, precommit_data);
- Line 1328: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: commit_data["participants"].push_back(shard_id);
- Line 1330: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logCommit(txn.transaction_id, commit_data);
- Line 1394: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = perc_coord.execute(
- Line 1475: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logPrepare(txn.transaction_id, "sequencer", seq_data);
- Line 1530: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::error("Calvin: failed to acquire lock on shard {} for transaction {}",
- Line 1547: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::info("Calvin transaction {}: all locks acquired, proceeding to execution phase",
- Line 1571: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logCommit(txn.transaction_id, exec_data);
- Line 1675: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 1743: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool success = rpc_client.commit(transaction_id, commit_timestamp);
- Line 1758: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 1831: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 1852: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void CrossShardTransactionCoordinator::deadlockDetectionThread() {
  Confidence: band=very_high; score=0.9
- Line 1893: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& edge : remote_edges) {
  Confidence: band=very_high; score=0.9
- Line 1900: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Only merge remote edges that reference known live
  Confidence: band=very_high; score=0.9
- Line 1953: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& cycle_nodes : cycles) {
  Confidence: band=very_high; score=0.9
- Line 1968: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = transactions_.find(txn_id);
- Line 1968: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = transactions_.find(txn_id);
- Line 1969: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = transactions_.find(txn_id);
  Confidence: band=very_high; score=0.9
- Line 2114: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t j = executed_steps.size(); j > 0; --j) {
  Confidence: band=very_high; score=0.9
- Line 2131: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> wal_lock(transactions_mutex_);
- Line 2138: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: transaction_wal_->logCompensate(transaction_id, shard_id, comp_data);
- Line 2805: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto acquire_lock = [&](const std::string& shard_id) -> bool {
- Line 2842: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool locked = acquire_lock(shard_id);
- Line 2861: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool primary_locked = acquire_lock(primary_shard_id);
- Line 2881: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: {"locks_acquired", locked_shards}
- Line 2889: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::info("[Percolator] All locks acquired for txn {}", txn.transaction_id);
- Line 47: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> collectCycles(
  Confidence: band=medium; score=0.66
- Line 48: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& graph
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> cycles;
  Confidence: band=medium; score=0.66
- Line 51: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> index;
  Confidence: band=medium; score=0.66
- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> lowlink;
  Confidence: band=medium; score=0.66
- Line 54: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> on_stack;
  Confidence: band=medium; score=0.66
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: component.push_back(current);
- Line 108: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>(component.begin(), component.end()));
  Confidence: band=medium; score=0.66
- Line 121: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> collectCycleNodes(
  Confidence: band=medium; score=0.66
- Line 124: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> cycle_nodes;
  Confidence: band=medium; score=0.66
- Line 230: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: recovery_result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={}); "
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: recovery_result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: recovery_result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 897: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: operations.push_back(operation);
- Line 960: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: executed_steps.push_back(step);
- Line 1050: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(txn);
  Confidence: band=high; score=0.74
- Line 1051: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active.push_back(txn);
- Line 1104: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: commit_data["participants"].push_back(shard_id);
- Line 1256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: precommit_data["participants"].push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: precommit_data["participants"].push_back(shard_id);
- Line 1280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: commit_data["participants"].push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: commit_data["participants"].push_back(shard_id);
- Line 1388: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: perc_cfg.stale_lock_threshold = std::chrono::seconds(30);
  Confidence: band=high; score=0.74
- Line 1498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_order.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_order.push_back(shard_id);
- Line 1517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1518: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locked_shards.push_back(shard_id);
- Line 1647: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operations.push_back(op);
  Confidence: band=high; score=0.74
- Line 1648: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: operations.push_back(op);
- Line 1885: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: remote_edges.push_back({
  Confidence: band=high; score=0.74
- Line 1886: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: remote_edges.push_back({
- Line 1970: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(txn_id, it->second.start_time);
  Confidence: band=high; score=0.74
- Line 1970: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(txn_id, it->second.start_time);
  Confidence: band=high; score=0.74
- Line 2015: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>>
  Confidence: band=high; score=0.74
- Line 2017: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> graph;
  Confidence: band=high; score=0.74
- Line 2045: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph[waiting_txn_id].push_back(blocking_txn_id);
  Confidence: band=high; score=0.74
- Line 2046: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: graph[waiting_txn_id].push_back(blocking_txn_id);
- Line 2073: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& graph,
  Confidence: band=high; score=0.74
- Line 2298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: participants_json.push_back({
  Confidence: band=high; score=0.74
- Line 2299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: participants_json.push_back({
- Line 2311: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: log_file.close();
- Line 2545: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("Transaction {} is stale (age: {}s), will abort", txn_id, age.count());
  Confidence: band=high; score=0.74
- Line 2545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transactions_to_timeout.push_back(txn_id);
  Confidence: band=high; score=0.74
- Line 2546: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transactions_to_timeout.push_back(txn_id);
- Line 2554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transactions_to_resume.push_back(txn_id);
- Line 2559: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transactions_to_resume.push_back(txn_id);
- Line 2573: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Final states - can be cleaned up eventually
  Confidence: band=high; score=0.74
- Line 2594: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("Aborting {} stale transactions", transactions_to_timeout.size());
  Confidence: band=high; score=0.74
- Line 2606: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::error("Failed to abort stale transaction {}: {}", txn_id, e.what());
  Confidence: band=high; score=0.74
- Line 2623: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: stats->stale_transactions_detected = transactions_to_timeout.size();
  Confidence: band=high; score=0.74
- Line 2626: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: stats->stale_transactions_detected + stats->resume_candidates;
  Confidence: band=high; score=0.74
- Line 2698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.participants.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.participants.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2699: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.participants.push_back(shard_id);
- Line 2701: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ::sharding::ParticipantStatus status;
- Line 2713: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_txns.push_back(entry);
- Line 2852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2853: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locked_shards.push_back(shard_id);
- Line 2870: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(primary_shard_id);
  Confidence: band=high; score=0.74
- Line 2871: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locked_shards.push_back(primary_shard_id);
- Line 2965: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: size_t PercolatorCoordinator::cleanStaleLocks(
  Confidence: band=high; score=0.74
- Line 2966: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: const std::vector<std::string>& stale_txn_ids,
  Confidence: band=high; score=0.74
- Line 2971: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& txn_id : stale_txn_ids) {
  Confidence: band=high; score=0.74
- Line 2990: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (age < config_.stale_lock_threshold) {
  Confidence: band=high; score=0.74
- Line 2994: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("[Percolator] Cleaning stale lock for txn {} (age {}s)",
  Confidence: band=high; score=0.74
- Line 3001: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: wal_->logAbort(txn_id, "stale_lock_cleanup");
  Confidence: band=high; score=0.74
- Line 3009: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("[Percolator] Stale lock cleaned for txn {}", txn_id);
  Confidence: band=high; score=0.74
- Line 3011: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("[Percolator] Failed to clean stale lock for txn {}", txn_id);
  Confidence: band=high; score=0.74
- Line 3015: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("[Percolator] cleanStaleLocks: cleaned {} / {} stale locks",
  Confidence: band=high; score=0.74
- Line 3016: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cleaned, stale_txn_ids.size());
  Confidence: band=high; score=0.74

### src/sharding/shard_router.cpp
Total findings: 117

- Line 140: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool ShardRouter::put(const URN& urn, const nlohmann::json& data) {
  Confidence: band=very_high; score=0.99
- Line 212: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 220: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 227: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 413: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = shard_map.find(id);
- Line 622: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Emit one merged row per matching left-side entry.
  Confidence: band=very_high; score=0.99
- Line 624: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged = nlohmann::json::object();
  Confidence: band=very_high; score=0.99
- Line 626: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["left_" + k] = v;
  Confidence: band=very_high; score=0.99
- Line 630: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (!merged.contains(rk)) merged[rk] = v;
  Confidence: band=very_high; score=0.99
- Line 632: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=very_high; score=0.99
- Line 663: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Execute join locally on each shard and merge results
  Confidence: band=very_high; score=0.99
- Line 671: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Phase 2: Merge results from all shards
  Confidence: band=very_high; score=0.99
- Line 672: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged = mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 677: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: {"data", merged}
  Confidence: band=very_high; score=0.99
- Line 726: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: exec_result = executor_->put(*shard_info, path, *body);
  Confidence: band=very_high; score=0.99
- Line 911: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
  Confidence: band=very_high; score=0.99
- Line 912: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged;
  Confidence: band=very_high; score=0.99
- Line 913: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.99
- Line 914: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["errors"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.99
- Line 915: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["shard_count"] = results.size();
  Confidence: band=very_high; score=0.99
- Line 923: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // If result has data array, merge it
  Confidence: band=very_high; score=0.99
- Line 926: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.99
- Line 930: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.99
- Line 934: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"].push_back(result.data);
  Confidence: band=very_high; score=0.99
- Line 937: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["errors"].push_back(nlohmann::json{
  Confidence: band=very_high; score=0.99
- Line 944: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["success_count"] = success_count;
  Confidence: band=very_high; score=0.99
- Line 945: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["error_count"] = results.size() - success_count;
  Confidence: band=very_high; score=0.99
- Line 947: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return merged;
  Confidence: band=very_high; score=0.99
- Line 951: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const nlohmann::json& merged,
  Confidence: band=very_high; score=0.99
- Line 955: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json paginated = merged;
  Confidence: band=very_high; score=0.99
- Line 957: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (merged.contains("results") && merged["results"].is_array()) {
  Confidence: band=very_high; score=0.99
- Line 958: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const auto& results = merged["results"];
  Confidence: band=very_high; score=0.99
- Line 110: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<nlohmann::json> ShardRouter::get(
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json ShardRouter::executeQuery(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto exec_result = executor_->executeQuery(shard, query);
  Confidence: band=very_high; score=0.9
- Line 356: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: results.push_back(futures[i].get());
  Confidence: band=very_high; score=0.9
- Line 412: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = shard_map.find(id);
- Line 412: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = shard_map.find(id);
- Line 458: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto exec_result = executor_->executeQuery(shard, query);
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: results.push_back(futures[i].get());
  Confidence: band=very_high; score=0.9
- Line 620: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = hash_table.find(key);
  Confidence: band=very_high; score=0.9
- Line 622: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Emit one merged row per matching left-side entry.
  Confidence: band=very_high; score=0.9
- Line 624: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged = nlohmann::json::object();
  Confidence: band=very_high; score=0.9
- Line 626: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["left_" + k] = v;
  Confidence: band=very_high; score=0.9
- Line 630: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (!merged.contains(rk)) merged[rk] = v;
  Confidence: band=very_high; score=0.9
- Line 632: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=very_high; score=0.9
- Line 663: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Execute join locally on each shard and merge results
  Confidence: band=very_high; score=0.9
- Line 671: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Phase 2: Merge results from all shards
  Confidence: band=very_high; score=0.9
- Line 672: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged = mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 677: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: {"data", merged}
  Confidence: band=very_high; score=0.9
- Line 724: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: exec_result = executor_->get(*shard_info, path);
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Execute query (simplified - return empty results)
  Confidence: band=very_high; score=0.9
- Line 911: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
  Confidence: band=very_high; score=0.9
- Line 912: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged;
  Confidence: band=very_high; score=0.9
- Line 913: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.9
- Line 913: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: merged["results"] = nlohmann::json::array();
- Line 914: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["errors"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.9
- Line 914: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: merged["errors"] = nlohmann::json::array();
- Line 915: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["shard_count"] = results.size();
  Confidence: band=very_high; score=0.9
- Line 923: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // If result has data array, merge it
  Confidence: band=very_high; score=0.9
- Line 926: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.9
- Line 930: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.9
- Line 934: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"].push_back(result.data);
  Confidence: band=very_high; score=0.9
- Line 934: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: merged["results"].push_back(result.data);
- Line 937: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["errors"].push_back(nlohmann::json{
  Confidence: band=very_high; score=0.9
- Line 944: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["success_count"] = success_count;
  Confidence: band=very_high; score=0.9
- Line 945: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["error_count"] = results.size() - success_count;
  Confidence: band=very_high; score=0.9
- Line 947: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 951: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const nlohmann::json& merged,
  Confidence: band=very_high; score=0.9
- Line 955: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json paginated = merged;
  Confidence: band=very_high; score=0.9
- Line 957: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (merged.contains("results") && merged["results"].is_array()) {
  Confidence: band=very_high; score=0.9
- Line 958: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const auto& results = merged["results"];
  Confidence: band=very_high; score=0.9
- Line 37: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static std::map<std::string, std::string> parseQueryParams(const std::string& path) {
  Confidence: band=high; score=0.74
- Line 38: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> params;
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json ShardRouter::executeQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RoutingStrategy ShardRouter::analyzeQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_shard_ids.push_back(shard.shard_id);
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 353: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(futures[i].get());
- Line 364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(timeout_result);
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(error_result);
- Line 403: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ShardInfo> shard_map;
  Confidence: band=medium; score=0.66
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_shards.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: target_shards.push_back(it->second);
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_shard_ids.push_back(shard.shard_id);
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_shard_ids.push_back(shard.shard_id);
- Line 445: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(futures[i].get());
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(tr);
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(er);
- Line 549: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash_table[key].push_back(row);
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 632: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: joined_rows.push_back(std::move(merged));
- Line 925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged["results"].push_back(item);
  Confidence: band=high; score=0.74
- Line 925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged["results"].push_back(item);
  Confidence: band=high; score=0.74
- Line 926: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged["results"].push_back(item);
- Line 929: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged["results"].push_back(item);
  Confidence: band=high; score=0.74
- Line 930: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged["results"].push_back(item);
- Line 934: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged["results"].push_back(result.data);
- Line 937: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged["errors"].push_back(nlohmann::json{
- Line 964: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: page.push_back(results[i]);
  Confidence: band=high; score=0.74
- Line 965: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: page.push_back(results[i]);

### src/sharding/stream_protocol.cpp
Total findings: 105

- Line 245: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), data.begin(), data.end());
  Confidence: band=very_high; score=0.99
- Line 923: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Start sessions (up to max_concurrent)
  Confidence: band=very_high; score=0.99
- Line 932: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: while (active_count < config_.max_concurrent_sessions &&
  Confidence: band=very_high; score=0.99
- Line 1045: severity=CRITICAL; category=no_timeout; status=FIXED
  Description: semaphore_wait without timeout — FIXED: replaced cv_.wait with cv_.wait_for(session timeout) in transferLoop (PR #5455)
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cv_.wait(lock, [this] { return !paused_ || !running_; });
- Line 1165: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.chunk_index), sizeof(chunk.chunk_index));
  Confidence: band=very_high; score=0.99
- Line 1166: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.file_offset), sizeof(chunk.file_offset));
  Confidence: band=very_high; score=0.99
- Line 1167: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.uncompressed_size), sizeof(chunk.uncompressed_size));
  Confidence: band=very_high; score=0.99
- Line 1168: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.compressed_size), sizeof(chunk.compressed_size));
  Confidence: band=very_high; score=0.99
- Line 1169: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.checksum), sizeof(chunk.checksum));
  Confidence: band=very_high; score=0.99
- Line 1170: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());
  Confidence: band=very_high; score=0.99
- Line 1333: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(write_data.data()), write_data.size());
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 99: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
- Line 164: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.version = data[pos++];
- Line 165: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.type = static_cast<StreamMessageType>(data[pos++]);
- Line 167: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.session_id = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 168: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 169: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 170: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 175: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.sequence_number = (header.sequence_number << 8) | data[pos++];
- Line 178: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.payload_length = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 179: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 180: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 181: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 184: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.flags = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 185: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 186: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 187: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 190: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header.checksum = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 191: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 192: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 193: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 261: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.file_offset = (chunk.file_offset << 8) | data[pos++];
- Line 265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.chunk_index = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 266: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 267: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 268: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 272: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.uncompressed_size = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 273: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 274: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 275: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 279: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.compressed_size = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 280: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 281: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 282: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 286: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.checksum = (static_cast<uint32_t>(data[pos]) << 24) |
- Line 287: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+1]) << 16) |
- Line 288: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint32_t>(data[pos+2]) << 8) |
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint32_t>(data[pos+3]);
- Line 485: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::chrono::milliseconds StreamRateLimiter::acquire(size_t bytes) {
- Line 597: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: session_thread_ = std::thread(&StreamSession::sessionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 598: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread(&StreamSession::heartbeatLoop, this);
  Confidence: band=very_high; score=0.9
- Line 617: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamSession::abort(const std::string& reason) {
  Confidence: band=very_high; score=0.9
- Line 708: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 752: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Confidence: band=very_high; score=0.9
- Line 752: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 761: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 762: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS), [this] {
- Line 813: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& plan : active_plans_) {
  Confidence: band=very_high; score=0.9
- Line 858: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: bool StreamPlan::execute() {
  Confidence: band=very_high; score=0.9
- Line 863: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: executor_thread_ = std::thread(&StreamPlan::executorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 867: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamPlan::abort() {
  Confidence: band=very_high; score=0.9
- Line 876: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& session : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 892: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lock, timeout, [this] { return complete_.load(); });
  Confidence: band=very_high; score=0.9
- Line 902: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& session : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 918: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& session : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 929: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 931: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Start new sessions if we have capacity
- Line 965: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 999: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: transfer_thread_ = std::thread(&StreamTransferTask::transferLoop, this);
  Confidence: band=very_high; score=0.9
- Line 1012: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamTransferTask::abort() {
  Confidence: band=very_high; score=0.9
- Line 1044: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 1076: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 1105: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(chunk.data.data()), chunk.uncompressed_size);
  Confidence: band=very_high; score=0.9
- Line 1253: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (bool received : chunks_received_) {
  Confidence: band=very_high; score=0.9
- Line 1267: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamReceiveTask::abort() {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['', '    // flags (big-endian)', '    result[pos++] = (flags >> 24) & 0xFF;', '    result[pos++] = (flags >> 16) & 0xFF;', '    result[pos++] = (flags >> 8) & 0xFF;']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    // flags (big-endian)', '    result[pos++] = (flags >> 24) & 0xFF;', '    result[pos++] = (flags >> 16) & 0xFF;', '    result[pos++] = (flags >> 8) & 0xFF;', '    result[pos++] = flags & 0xFF;']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    result[pos++] = (flags >> 24) & 0xFF;', '    result[pos++] = (flags >> 16) & 0xFF;', '    result[pos++] = (flags >> 8) & 0xFF;', '    result[pos++] = flags & 0xFF;', '']
  Confidence: band=medium; score=0.65
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((file_offset >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((file_offset >> (i * 8)) & 0xFF);
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((chunk_index >> 24) & 0xFF);
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((chunk_index >> 16) & 0xFF);
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((chunk_index >> 8) & 0xFF);
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(chunk_index & 0xFF);
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((uncompressed_size >> 24) & 0xFF);
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((uncompressed_size >> 16) & 0xFF);
- Line 229: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((uncompressed_size >> 8) & 0xFF);
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(uncompressed_size & 0xFF);
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((compressed_size >> 24) & 0xFF);
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((compressed_size >> 16) & 0xFF);
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((compressed_size >> 8) & 0xFF);
- Line 585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_.push_back(file);
- Line 659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.file_progress.push_back(file_progress);
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: progress.file_progress.push_back(file_progress);
- Line 669: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.file_progress.push_back(file_progress);
  Confidence: band=high; score=0.74
- Line 670: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: progress.file_progress.push_back(file_progress);
- Line 822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_plans_.push_back(plan);
  Confidence: band=high; score=0.74
- Line 823: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_plans_.push_back(plan);
- Line 858: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool StreamPlan::execute() {
  Confidence: band=high; score=0.74
- Line 902: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.push_back(session->getProgress());
  Confidence: band=high; score=0.74
- Line 902: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.push_back(session->getProgress());
  Confidence: band=high; score=0.74
- Line 903: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: progress.push_back(session->getProgress());
- Line 911: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: listeners_.push_back(listener);
- Line 1216: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: test_file.close();

### src/sharding/distributed_transaction.cpp
Total findings: 63

- Line 191: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: mutex_ upgraded to std::timed_mutex; lock.lock() replaced with lock.try_lock_for(rpc_timeout_ms) + return false)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 228: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex + try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 243: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex + try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 269: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex + try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 292: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex + try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 310: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch2: same timed_mutex + try_lock_for pattern)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 463: severity=CRITICAL; category=no_timeout; status=FIXED
  Description: thread_join without timeout — FIXED: converted to std::async + future.wait_for(prepare_timeout_ms) in preparePhase (PR #5455)
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 500: severity=CRITICAL; category=no_timeout; status=FIXED
  Description: thread_join without timeout — FIXED: converted to std::async + future.wait_for(commit_timeout_ms) in commitPhase (PR #5455)
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 632: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = transactions_.begin();
- Line 813: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: committed_ids.insert(txn_id);
  Confidence: band=very_high; score=0.99
- Line 816: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: aborted_ids.insert(txn_id);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 162: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool DistributedTransactionCoordinator::commit(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool DistributedTransactionCoordinator::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 389: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto shard_results = client.snapshotRead(snapshot_ts.count(), query);
  Confidence: band=very_high; score=0.9
- Line 389: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto shard_results = client.snapshotRead(snapshot_ts.count(), query);
- Line 448: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& participant : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 454: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(error_mutex);
- Line 462: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& thread : threads) {
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& participant : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 491: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(error_mutex);
- Line 499: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& thread : threads) {
  Confidence: band=very_high; score=0.9
- Line 539: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool vote_commit = client.prepare(txn_id, operations);
- Line 539: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool vote_commit = client.prepare(txn_id, operations);
- Line 570: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool committed = client.commit(txn_id, commit_timestamp.count());
- Line 600: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool aborted = client.abort(txn_id);
- Line 669: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
- Line 706: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: recovery_data["participants"].push_back({
- Line 753: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: prepared_data["participants"].push_back({
- Line 806: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::map<std::string, nlohmann::json> prepared_entries; // txn_id -> prepared data
- Line 827: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [txn_id, prepared_data] : prepared_entries) {
- Line 926: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& participant : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 934: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(error_mutex);
- Line 935: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_details.push_back("Shard " + p_ptr->shard_id +
- Line 937: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: p_ptr->error_msg);
- Line 937: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: p_ptr->error_msg);
- Line 942: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : threads) {
  Confidence: band=very_high; score=0.9
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: txn.participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: txn.participants.push_back(participant);
- Line 435: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> map)
  Confidence: band=medium; score=0.66
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &participant, &txn, &all_prepared, &error_mutex, &error_details]() {
  Confidence: band=high; score=0.74
- Line 455: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: error_details.push_back("Shard " + participant.shard_id +
- Line 470: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &participant, &txn, &all_committed, &error_mutex, &error_details]() {
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &participant, &txn, &all_committed, &error_mutex, &error_details]() {
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: error_details.push_back("Shard " + participant.shard_id +
- Line 507: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovery_data["participants"].push_back({
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recovery_data["participants"].push_back({
- Line 752: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prepared_data["participants"].push_back({
  Confidence: band=high; score=0.74
- Line 753: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: prepared_data["participants"].push_back({
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovery_txn.participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovery_txn.participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 847: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recovery_txn.participants.push_back(participant);
- Line 929: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, p_ptr, &txn, &all_committed,
  Confidence: band=high; score=0.74
- Line 935: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: error_details.push_back("Shard " + p_ptr->shard_id +
- Line 949: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += e + "; ";
  Confidence: band=high; score=0.74
- Line 949: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += e + "; ";
  Confidence: band=high; score=0.74
- Line 949: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += e + "; ";
  Confidence: band=high; score=0.74

### src/sharding/cloud_backup.cpp
Total findings: 55

- Line 107: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::map<std::string, std::string>& metadata) override {
- Line 211: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: callback = nullptr;
  Context: THEMIS_ERROR("S3 delete callback failed: {}", e.what());
- Line 232: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: THEMIS_ERROR("S3 delete failed: AWS SDK not integrated");
- Line 329: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::map<std::string, std::string>& metadata) override {
- Line 405: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: callback = nullptr;
  Context: THEMIS_ERROR("Azure delete callback failed: {}", e.what());
- Line 415: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: THEMIS_ERROR("Azure delete failed: Azure SDK not integrated");
- Line 435: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Purpose: Preserve Azure provider list API compatibility before SDK-backed
  Confidence: band=high; score=0.8
- Line 509: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::map<std::string, std::string>& metadata) override {
- Line 593: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: callback = nullptr;
  Context: THEMIS_ERROR("GCS delete callback failed: {}", e.what());
- Line 611: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: THEMIS_ERROR("GCS delete failed: GCS SDK not integrated");
- Line 751: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["backup_id"] = backup_id;
- Line 752: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["shard_id"] = shard_id;
- Line 753: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
- Line 754: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["version"] = "1.0";
- Line 766: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: upload_metadata["backup_id"] = backup_id;
- Line 767: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: upload_metadata["shard_id"] = shard_id;
- Line 867: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: backup = nullptr;
  Context: THEMIS_ERROR("Failed to delete backup object from provider: {}", remote_path);
- Line 903: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool setReplicationTarget(const std::string& datacenter_id,
  Confidence: band=very_high; score=0.9
- Line 914: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: replication_targets_[datacenter_id] = target;
- Line 1050: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool CloudBackupCoordinator::setReplicationTarget(const std::string& datacenter_id,
  Confidence: band=very_high; score=0.9
- Line 1052: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return impl_->setReplicationTarget(datacenter_id, shard_endpoints);
  Confidence: band=very_high; score=0.9
- Line 1052: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->setReplicationTarget(datacenter_id, shard_endpoints);
- Line 1056: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->enableContinuousReplication(datacenter_id);
- Line 1060: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->disableContinuousReplication(datacenter_id);
- Line 1064: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1069: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1074: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1079: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1084: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1089: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1094: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1099: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1104: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1109: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1114: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1119: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 1124: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(g_cloud_backup_fn_mutex);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 227: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_INFO("S3 delete (placeholder): s3://{}/{}", bucket_, remote_path);
- Line 232: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_ERROR("S3 delete failed: AWS SDK not integrated");
- Line 410: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_INFO("Azure delete (placeholder): {}/{}/{}", account_name_, container_, remote_path);
- Line 415: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_ERROR("Azure delete failed: Azure SDK not integrated");
- Line 606: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_INFO("GCS delete (placeholder): gs://{}/{}", bucket_, remote_path);
- Line 611: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_ERROR("GCS delete failed: GCS SDK not integrated");
- Line 760: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: metadata_file.close();
- Line 765: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> upload_metadata;
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_ERROR("Failed to delete backup object from provider: {}", remote_path);
- Line 882: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backups.push_back(entry.second);
  Confidence: band=high; score=0.74
- Line 883: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backups.push_back(entry.second);
- Line 886: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (newest first)
  Confidence: band=high; score=0.74

### src/sharding/gossip_protocol.cpp
Total findings: 53

- Line 154: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = peers_.find(peer_id);
- Line 477: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
  Confidence: band=very_high; score=0.99
- Line 590: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(config_.private_key_path.c_str(), "r");
- Line 616: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (EVP_DigestSignUpdate(ctx, to_sign.c_str(), to_sign.length()) == 1) {
  Confidence: band=very_high; score=0.99
- Line 669: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(key_path.c_str(), "r");
- Line 714: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: EVP_DigestVerifyUpdate(ctx, to_verify.data(), to_verify.size()) == 1 &&
  Confidence: band=very_high; score=0.99
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
- Line 65: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: gossip_thread_ = std::thread(&GossipProtocol::gossipLoop, this);
  Confidence: band=very_high; score=0.9
- Line 68: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread(&GossipProtocol::cleanupLoop, this);
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, peer] : peers_) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: peers_.find(peer.peer_id) == peers_.end()) {
- Line 199: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: custom_handler = it->second;
- Line 231: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge peer list
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergePeerList(peers);
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 325: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 436: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& seed : config_.seed_nodes) {
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void GossipProtocol::mergePeerList(const std::vector<PeerInfo>& peers) {
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& peer : peers) {
  Confidence: band=very_high; score=0.9
- Line 516: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& id : to_remove) {
  Confidence: band=very_high; score=0.9
- Line 549: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // confirmed through Raft joint-consensus).  Without a gate (backward compat),
  Confidence: band=high; score=0.8
- Line 550: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // use the legacy warn+add path.
  Confidence: band=high; score=0.8
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 90: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, PeerInfo> GossipProtocol::getPeers() const {
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<PeerInfo> healthy;
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy.push_back(peer);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy.push_back(peer);
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers.push_back(PeerInfo::fromJson(p));
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: peers.push_back(PeerInfo::fromJson(p));
- Line 369: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 401: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers.push_back(peer);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: peers.push_back(peer);
- Line 429: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(peer);
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(peer);
- Line 506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(id);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_remove.push_back(id);
- Line 515: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Remove stale peers
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 609: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 633: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 634: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 677: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 699: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 724: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 727: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 785: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers_json.push_back(peer.toJson());
  Confidence: band=high; score=0.74
- Line 786: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: peers_json.push_back(peer.toJson());

### src/sharding/paxos_consensus.cpp
Total findings: 53

- Line 207: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = committed_log_.find(i);
- Line 445: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch4: proposal_mutex_ upgraded to std::timed_mutex; condition_variable_any used; lock.lock() replaced with lock.try_lock_for(config_.paxos_prepare_timeout) with break on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (instance.is_committed &&', '                committed_log_.find(slot) == committed_log_.end()) {', '                committed_log_[slot] = instance.accepted_value;', '                spdlog::debug("Acceptor: applied committed slot {} to committed_log_", slot);', '            }']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '        // Get or create instance for this slot', '        auto& instance = instances_[slot];', '        instance.slot = slot;', '        instance.prepare_promises.clear();']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        std::lock_guard<std::mutex> lock(state_mutex_);', '', '        auto& instance = instances_[slot];', '        instance.accept_acks.clear();', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    if (handleAccept(slot, proposal, value)) {', '        std::lock_guard<std::mutex> lock(state_mutex_);', '        instances_[slot].accept_acks.insert(node_id_);', '    }', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    {', '        std::lock_guard<std::mutex> lock(state_mutex_);', '        auto& instance = instances_[slot];', '', '        for (const auto& node : cluster_nodes_) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            for (const auto& [slot_str, instance_json] : state_json["instances"].items()) {', '                uint64_t slot = std::stoull(slot_str);', '                PaxosInstance& instance = instances_[slot];', '', '                instance.slot = slot;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    // Get or create instance for this slot', '    auto& instance = instances_[slot];', '', '    // Phase 1b: Promise not to accept proposals with lower ballot']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    // Get or create instance for this slot', '    auto& instance = instances_[slot];', '', "    // Phase 2b: Accept proposal if it's >= our promised proposal"]
  Confidence: band=high; score=0.81
- Line 79: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy: Load persistent state from JSON if available
  Confidence: band=high; score=0.8
- Line 97: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: proposer_thread_ = std::thread(&PaxosConsensus::runProposer, this);
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: acceptor_thread_ = std::thread(&PaxosConsensus::runAcceptor, this);
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: learner_thread_ = std::thread(&PaxosConsensus::runLearner, this);
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread(&PaxosConsensus::leaderElectionThread, this);
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(proposal_mutex_);
- Line 191: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 387: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(callbacks_mutex_);
- Line 392: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(callbacks_mutex_);
- Line 412: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(proposal_mutex_);
- Line 415: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: proposal_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
- Line 438: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << retry)));
- Line 475: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Confidence: band=very_high; score=0.9
- Line 475: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 484: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: committed_log_.find(slot) == committed_log_.end()) {
  Confidence: band=very_high; score=0.9
- Line 511: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Confidence: band=very_high; score=0.9
- Line 511: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 514: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 518: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (instance.is_committed && committed_log_.find(slot) == committed_log_.end()) {
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void PaxosConsensus::leaderElectionThread() {
  Confidence: band=very_high; score=0.9
- Line 540: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(500));
  Confidence: band=very_high; score=0.9
- Line 540: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(500));
- Line 575: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& peer : nodes_snapshot) {
  Confidence: band=very_high; score=0.9
- Line 645: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& node : cluster_nodes_) {
  Confidence: band=very_high; score=0.9
- Line 719: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: } // state_mutex_ released here — executeAcceptPhase() re-acquires it safely
- Line 790: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& node : cluster_nodes_) {
  Confidence: band=very_high; score=0.9
- Line 970: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: instance.accepted_value.data = value_json["data"];
- Line 993: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry.data = entry_json["data"];
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_slots.push_back(slot);
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_slots.push_back(slot);
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_slots.push_back(slot);
- Line 472: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // are now stale (i.e. the proposer timed out and a higher ballot has been
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Evict stale promises: if the promised round is far behind the
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: constexpr uint64_t kStaleRoundThreshold = 10;
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cur_round > instance.promised_proposal.round + kStaleRoundThreshold) {
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::debug("Acceptor: evicting stale promise for slot {} "
  Confidence: band=high; score=0.74
- Line 927: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 1082: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();

### src/sharding/epoch_fencing.cpp
Total findings: 51

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rec.epoch      = fencing_->currentEpoch();
- Line 305: severity=CRITICAL; category=data_race; **status=FIXED** (batch4: captured `it->second.holder` into local `blocked_holder` string before calling lk.unlock(); iterator no longer accessed after lock release)
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool ok = fencing_->stonithProvider()->fence(
- Line 355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rec.epoch      = fencing_->currentEpoch();
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
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4407 [WIP] Update root documentation for ThemisDB development status (2026-03-24T20:32:47Z)
- Line 43: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 85: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EpochFencingManager: invalid configuration");
- Line 121: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return current_epoch_.load(std::memory_order_acquire);
- Line 126: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: tok.epoch     = current_epoch_.load(std::memory_order_acquire);
- Line 145: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: EpochNumber cur = current_epoch_.load(std::memory_order_acquire);
- Line 208: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (acquire_wait_ms.count() <= 0) {
- Line 209: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::error("[LeaseConfig] acquire_wait_ms must be > 0");
- Line 223: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LeaseManager: invalid configuration");
- Line 226: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LeaseManager: fencing manager must not be null");
- Line 243: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: LeaseAcquireResult LeaseManager::acquire(const LeaseKey& key,
- Line 245: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_wait_ms;
- Line 248: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(mutex_);
- Line 262: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: rec.acquired_at = std::chrono::system_clock::now();
- Line 263: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: rec.expires_at = rec.acquired_at + config_.ttl_ms;
- Line 273: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ++metrics_.acquires;
- Line 287: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ++metrics_.acquires;
- Line 296: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ++metrics_.acquire_failures;
- Line 319: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 393: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<LeaseRecord> LeaseManager::get(const LeaseKey& key) const {
  Confidence: band=very_high; score=0.9
- Line 460: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: << since_epoch(rec.acquired_at) << '|' << since_epoch(rec.expires_at)
- Line 483: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::string state_str, acquired_str, expires_str, gen_str, epoch_str;
- Line 490: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::getline(ss, acquired_str, '|');
- Line 499: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: rec.acquired_at = std::chrono::system_clock::time_point{
- Line 500: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::chrono::milliseconds{std::stoll(acquired_str)}};
- Line 525: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 533: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(20));
  Confidence: band=very_high; score=0.9
- Line 533: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(20));
- Line 153: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Stale epoch
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("[EpochFencing] STALE_EPOCH from='{}' token_epoch={} current={}",
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++metrics_.stale_rejections;
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return issueStonith(source_node, "stale_epoch");
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return FencingResult::STALE_EPOCH;
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(k);
- Line 503: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/shard_rpc_client.cpp
Total findings: 51

- Line 215: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::lock_guard<std::mutex> lk(impl_->handler_mutex);
- Line 265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->in_process_handler = std::move(handler);
- Line 268: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool ShardRPCClient::prepare(
- Line 326: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool ShardRPCClient::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 378: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json ShardRPCClient::snapshotRead(
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown RPC method: " + method);
- Line 606: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.set_transaction_id(params.value("transaction_id", ""));
- Line 607: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.set_coordinator_shard_id(params.value("coordinator_shard_id", ""));
- Line 689: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // snapshotRead() is the lightweight "point-in-time read" path which returns
  Confidence: band=very_high; score=0.9
- Line 698: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 703: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 718: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // snapshotRead() path we return an empty result set with metadata so the
  Confidence: band=very_high; score=0.9
- Line 737: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: grpc::Status status = impl_->stub->HealthCheck(&context, request, &response);
- Line 740: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Health check failed: " + status.error_message());
- Line 757: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.set_shard_id(impl_->config.shard_id.empty()
- Line 761: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto* entity = request.add_entities();
- Line 764: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entity->set_data(params.value("data", nlohmann::json{}).dump());
- Line 871: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::lock_guard<std::mutex> lk(impl_->handler_mutex);
- Line 872: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: injected_handler = impl_->in_process_handler;
- Line 947: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown RPC method: " + method);
- Line 43: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Maximum delay between retry attempts (milliseconds).  Both sendRequestGrpc
  Confidence: band=high; score=0.74
- Line 44: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // and sendRequestInProcess use this cap so that a single constant controls the
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: /// When non-null, sendRequestInProcess() delegates to this function instead
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   When THEMIS_HAS_SHARD_GRPC is 0, all sendRequest() calls are routed
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   to sendRequestInProcess() which returns hardcoded JSON responses.
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   simulation path (sendRequestInProcess) is retained as a fallback for
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::string(override_flag) == "1")
  Context: override_flag && std::string(override_flag) == "1") {
  Confidence: band=medium; score=0.56
- Line 281: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("prepare", params);
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("commit", params);
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("abort", params);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("compensate", params);
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("snapshot_read", params);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("ping", nlohmann::json::object());
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 419: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("collect_wait_for_edges", nlohmann::json::object());
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back({waiting.get<std::string>(),
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back({waiting.get<std::string>(),
- Line 459: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("write_entity", params);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: nlohmann::json ShardRPCClient::sendRequest(
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: return sendRequestGrpc(method, params);
  Confidence: band=high; score=0.74
- Line 479: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: return sendRequestInProcess(method, params);
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: nlohmann::json ShardRPCClient::sendRequestGrpc(
  Confidence: band=high; score=0.74
- Line 693: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: themis::sharding::proto::StatusResponse status_resp;
- Line 805: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_json.push_back({
  Confidence: band=high; score=0.74
- Line 806: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_json.push_back({
- Line 843: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: nlohmann::json ShardRPCClient::sendRequestInProcess(
  Confidence: band=high; score=0.74
- Line 849: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Purpose: Provide a local in-process fallback for sendRequest() that
  Confidence: band=high; score=0.74
- Line 863: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   The sendRequestInProcess() method is retained as a single-node fallback
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Re-check circuit breaker before every attempt (mirrors sendRequestGrpc).
  Confidence: band=high; score=0.74
- Line 973: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Exponential backoff; same overflow-safe calculation as sendRequestGrpc
  Confidence: band=high; score=0.74

### src/sharding/signed_request.cpp
Total findings: 50

- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
- Line 89: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: BIO_write(bio.get(), data, static_cast<int>(len));
  Confidence: band=very_high; score=0.99
- Line 260: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* key_file = fopen(config_.key_path.c_str(), "r");
- Line 291: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
  Confidence: band=very_high; score=0.99
- Line 396: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = seen_nonces_.find(oldest.nonce);
- Line 557: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
  Confidence: band=very_high; score=0.99
- Line 579: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = seen_nonces_.find(oldest.nonce);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 49: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::warn("SignedRequestVerifier reject [{}]: {}", code, details);
- Line 81: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::string base64Encode(const unsigned char* data, size_t len) {
- Line 89: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: BIO_write(bio.get(), data, static_cast<int>(len));
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: BIO_flush(bio.get());
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: BIO_get_mem_ptr(bio.get(), &buffer_ptr);
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string result(buffer_ptr->data, buffer_ptr->length);
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string result(buffer_ptr->data, buffer_ptr->length);
- Line 110: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: int decoded_len = BIO_read(bio.get(), decoded.data(), static_cast<int>(decoded.size()));
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.timestamp_ms = getCurrentTimestampMs();
- Line 213: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.nonce = generateNonce();
- Line 221: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string canonical = request.getCanonicalString();
- Line 286: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignFinal(md_ctx.get(), nullptr, &sig_len) != 1) {
  Confidence: band=very_high; score=0.9
- Line 303: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignFinal(md_ctx.get(), signature.data(), &sig_len) != 1) {
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.key_id.empty() || !std::regex_match(request.key_id, keyIdPattern())) {
- Line 399: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::info("SignedRequestVerifier [{}]: evicted nonce due to cache pressure: {}",
- Line 476: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.key_id, e.what());
- Line 484: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // between weakly_canonical() and the read (the cert_path variable retains
  Confidence: band=very_high; score=0.9
- Line 505: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cert = utils::read_x509_from_bio(bio.get());
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!std::regex_match(request.cert_serial, certSerialPattern())) {
- Line 536: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto pubkey = utils::EVPKeyPtr(X509_get_pubkey(cert.get()));
  Confidence: band=very_high; score=0.9
- Line 540: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const std::string canonical = request.getCanonicalString();
- Line 543: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const int pubkey_type = EVP_PKEY_base_id(pubkey.get());
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, nullptr, nullptr, pubkey.get()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 548: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return EVP_DigestVerify(md_ctx.get(),
  Confidence: band=very_high; score=0.9
- Line 554: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pubkey.get()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 557: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 560: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return EVP_DigestVerifyFinal(md_ctx.get(), signature_bytes->data(), signature_bytes->size()) == 1;
  Confidence: band=very_high; score=0.9
- Line 85: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (!b64) { BIO_free(bmem); return ""; }
- Line 105: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (!b64) { BIO_free(bmem); return std::nullopt; }
- Line 272: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(key_file);
- Line 380: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Fail-closed: reject stale requests outside replay window.
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nonce_fifo_.push_back(NonceEntry{nonce, timestamp_ms});
- Line 456: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Note on TOCTOU: there is an inherent window between weakly_canonical() and
  Confidence: band=high; score=0.74
- Line 472: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: canonical_dir  = fs::weakly_canonical(fs::path(config_.trusted_certs_dir));
  Confidence: band=high; score=0.74
- Line 473: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: canonical_cert = fs::weakly_canonical(cert_path);
  Confidence: band=high; score=0.74
- Line 484: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // between weakly_canonical() and the read (the cert_path variable retains
  Confidence: band=high; score=0.74

### src/sharding/slo_monitor.cpp
Total findings: 49

- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double avg_lag = it->second->getAvgReplicationLag();
- Line 285: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: total_budget += window->getErrorBudget(config_.targets.availability_target);
- Line 308: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double error_budget = window->getErrorBudget(config_.targets.availability_target);
- Line 332: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double avg_lag = window->getAvgReplicationLag();
- Line 365: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: slo["error_budget"] = window->getErrorBudget(config_.targets.availability_target);
- Line 367: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: slo["met"] = window->getAvailability() >= config_.targets.availability_target;
- Line 388: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: slo["avg_replication_lag_ms"] = window->getAvgReplicationLag();
- Line 390: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: slo["met"] = window->getAvgReplicationLag() <= config_.targets.max_replication_lag_ms;
- Line 513: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double error_budget = window->getErrorBudget(config_.targets.availability_target);
- Line 538: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double avg_lag = window->getAvgReplicationLag();
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3328 [WIP] Add SLO/SLA compliance reporting with burn-rate alerts (2026-03-12T06:59:43Z)
- Line 109: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (double lag : replication_lag_samples_) {
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double SLOWindow::getErrorBudget(double target_availability) const {
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double SLOMonitor::getErrorBudget(const std::string& shard_id) const {
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return it->second->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double SLOMonitor::getGlobalErrorBudget() const {
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shard_id, window] : shard_windows_) {
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: total_budget += window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double budget = getErrorBudget(shard_id);
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double error_budget = window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& alert : active_alerts_) {
  Confidence: band=very_high; score=0.9
- Line 365: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: slo["error_budget"] = window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 385: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shard_id, window] : shard_windows_) {
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, window] : shard_windows_) {
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: compliance["error_budget"] = getGlobalErrorBudget();
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 426: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 449: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, prog] : repair_progress_) {
  Confidence: band=very_high; score=0.9
- Line 513: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double error_budget = window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 623: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: availability_slos.push_back(slo);
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: availability_slos.push_back(slo);
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latency_slos.push_back(slo);
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latency_slos.push_back(slo);
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consistency_slos.push_back(slo);
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: consistency_slos.push_back(slo);
- Line 402: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> SLOMonitor::getSLOCompliance() const {
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> compliance;
  Confidence: band=high; score=0.74
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(prog);
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active.push_back(prog);
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(
  Confidence: band=high; score=0.74
- Line 508: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_alerts_.push_back(
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_alerts_.push_back(
- Line 529: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_alerts_.push_back(
- Line 539: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_alerts_.push_back(
- Line 594: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: json_file.close();
- Line 654: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();

### src/sharding/shard_load_detector.cpp
Total findings: 42

- Line 489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: (it_hist->second.size() >= config_.min_samples_per_shard);
- Line 204: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 243: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 267: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 280: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 362: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (double value : values) {
  Confidence: band=very_high; score=0.9
- Line 48: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(load);
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: storage_values.push_back(storage_load);
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: storage_values.push_back(storage_load);
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_ids.push_back(shard_id);
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_ids[i]);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.hotspot_shards.push_back(shard_ids[i]);
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.cold_shards.push_back(shard_ids[i]);
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: request_rates.push_back(static_cast<double>(metrics.requests_per_sec));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: request_rates.push_back(static_cast<double>(metrics.requests_per_sec));
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_ids.push_back(shard_id);
- Line 196: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_ids[i]);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.hotspot_shards.push_back(shard_ids[i]);
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(metrics.p99_latency_ms);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latencies.push_back(metrics.p99_latency_ms);
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_ids.push_back(shard_id);
- Line 238: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 240: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result.reason += "Latency degradation on " + shard_ids[i] +
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_ids[i]);
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.hotspot_shards.push_back(shard_ids[i]);
- Line 262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result.reason += "CPU exhaustion on " + shard_id +
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.hotspot_shards.push_back(shard_id);
- Line 276: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 277: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result.reason += "Storage exhaustion on " + shard_id +
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: load_rankings.push_back({shard_id, load});
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: load_rankings.push_back({shard_id, load});
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cpu_series.push_back(sample.cpu_usage_percent);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cpu_series.push_back(sample.cpu_usage_percent);
- Line 513: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: storage_series.push_back(sample.storage_usage_percent);
- Line 514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: composite_series.push_back(calculateLoad(sample));

### src/sharding/gossip_config_manager.cpp
Total findings: 40

- Line 51: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_shards.insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 54: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_shards.insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 254: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::string GossipConfigManager::publishConfigUpdate(
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: handleConfigUpdate(update);
  Confidence: band=very_high; score=0.99
- Line 293: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: metrics_->recordGossipConfigUpdate("sent");
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
  Confidence: band=very_high; score=0.99
- Line 383: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return (it != current_config_.end()) ? it->second : "";
- Line 416: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.99
- Line 543: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
  Confidence: band=very_high; score=0.99
- Line 606: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void GossipConfigManager::handleConfigUpdate(const ConfigUpdate& update) {
  Confidence: band=very_high; score=0.99
- Line 608: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (!shouldAcceptUpdate(update)) {
  Confidence: band=very_high; score=0.99
- Line 629: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_++;
  Confidence: band=very_high; score=0.99
- Line 631: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Record conflict metric
  Confidence: band=very_high; score=0.99
- Line 633: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: metrics_->recordGossipConfigConflict("last_write_wins");
  Confidence: band=very_high; score=0.99
- Line 705: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool GossipConfigManager::shouldAcceptUpdate(const ConfigUpdate& update) {
  Confidence: band=very_high; score=0.99
- Line 727: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator oldest_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto oldest_it = config_updates_.begin();
- Line 728: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = config_updates_.begin(); it != config_updates_.end(); ++it) {
- Line 791: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: *message.mutable_config_update() = update.toProto();
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 38: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t this_val = get(shard_id);
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t other_val = other.get(shard_id);
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t VectorClock::get(const std::string& shard_id) const {
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: gossip_thread_ = std::thread(&GossipConfigManager::gossipLoop, this);
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: anti_entropy_thread_ = std::thread(&GossipConfigManager::antiEntropyLoop, this);
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge vector clock
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 467: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 728: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = config_updates_.begin(); it != config_updates_.end(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void GossipConfigManager::mergeVectorClock(const VectorClock& other) {
  Confidence: band=very_high; score=0.9
- Line 742: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: local_clock_.merge(other);
  Confidence: band=very_high; score=0.9
- Line 38: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::merge(const VectorClock& other)
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=medium; score=0.56
- Line 44: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::compare(const VectorClock& other)
  Context: VectorClock::Ordering VectorClock::compare(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.warnings.push_back(warning);
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.warnings.push_back(warning);
- Line 386: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> GossipConfigManager::getAllConfigs() const {
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(shard.shard_id);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(shard.shard_id);

### src/sharding/adaptive_shard_router.cpp
Total findings: 39

- Line 100: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pending = load_it->second.pending_requests;
- Line 235: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: already_queried.insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 298: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return merged_results;
  Confidence: band=very_high; score=0.99
- Line 481: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged = nlohmann::json::array();
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid AdaptiveConfig");
- Line 133: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return ShardRouter::executeQuery(query);
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return executeAdaptiveQuery(query, stats);
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json AdaptiveShardRouter::executeAdaptiveQuery(
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return ShardRouter::executeQuery(query);
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge results from all iterations
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto merged_results = mergeIterationResults(all_iteration_results);
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged_results;
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid AdaptiveConfig");
- Line 413: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (already_queried.find(match.shard_id) != already_queried.end()) {
- Line 478: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json AdaptiveShardRouter::mergeIterationResults(
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged = nlohmann::json::array();
  Confidence: band=very_high; score=0.9
- Line 490: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(item);
  Confidence: band=very_high; score=0.9
- Line 496: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 133: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_iteration_results.push_back(iteration_results);
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.iteration_details.push_back(iter_stats);
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.iteration_details.push_back(iter_stats);
- Line 384: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: context.regions.push_back("hamburg");
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: context.regions.push_back("berlin");
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: context.regions.push_back("munich");
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: context.domains.push_back("construction");
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: context.domains.push_back("law");
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(match.shard_id);
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: selected.push_back(match.shard_id);
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(item);
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(match.score);
  Confidence: band=high; score=0.74
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(match.score);
  Confidence: band=high; score=0.74
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(match.score);
  Confidence: band=high; score=0.74
- Line 525: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(match.score);

### src/sharding/auto_rebalancer.cpp
Total findings: 39

- Line 89: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto forecast = detector_->forecastLoad(shard_id, config_.forecast_horizon);
- Line 90: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (forecast && forecast->predicted_composite_load >= config_.predictive_load_threshold) {
- Line 162: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: THEMIS_INFO("AutoRebalancer initialized with check_interval={}s, max_concurrent={}",
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: config_.check_interval.count() / 1000, config_.max_concurrent_operations);
  Confidence: band=very_high; score=0.99
- Line 242: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Check max concurrent operations
  Confidence: band=very_high; score=0.99
- Line 245: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (active_operations_.size() >= config_.max_concurrent_operations) {
  Confidence: band=very_high; score=0.99
- Line 246: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: THEMIS_WARN("Max concurrent operations reached, queuing remaining");
  Confidence: band=very_high; score=0.99
- Line 385: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* key_file = fopen(config_.operator_key_path.c_str(), "r");
- Line 600: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_approvals_.find(operation_id);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Encode signature as Base64 using OpenSSL', '    // Calculate required buffer size: ((input_len + 2) / 3) * 4 + 1 for null terminator', '    size_t b64_len = ((signature.size() + 2) / 3) * 4 + 1;', '    std::vector<unsigned char> b64_buf(b64_len);', '']
  Confidence: band=high; score=0.78
- Line 178: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitor_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Check if we can trigger new rebalances
- Line 241: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : imbalance.recommendations) {
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 254: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 73: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!proposal.reason.empty()) proposal.reason += ", ";
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: proposals.push_back(proposal);
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: proposals.push_back(proposal);
- Line 343: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: OperationStatus status;
- Line 392: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(key_file);
- Line 417: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 423: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 424: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 431: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 432: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 438: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 439: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 447: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 448: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 456: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 457: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 463: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 464: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: completed_ids.push_back(op_id);
  Confidence: band=high; score=0.74
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: completed_ids.push_back(op_id);
- Line 658: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: OperationStatus status;
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: statuses.push_back(status);
  Confidence: band=high; score=0.74
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: statuses.push_back(status);
  Confidence: band=high; score=0.74
- Line 663: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: statuses.push_back(status);

### src/sharding/gpu_erasure_coder_opencl.cpp
Total findings: 38

- Line 73: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 512 > array 255
  Remediation: Fix loop condition or increase array size
  Context: gf_exp[255] = gf_exp[0];
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                        ? static_cast<size_t>(config.device_id) % num_dev', '                        : 0;', '                    chosen_device = devs[dev_idx];', '                    break;', '                }']
  Confidence: band=high; score=0.81
- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (a == 0) throw std::runtime_error("GF division by zero");
- Line 97: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: mat[p * data_shards + d] = val;
- Line 113: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Singular matrix during Gaussian elimination");
- Line 169: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uchar coeff = enc_matrix[p * data_shards + d];
- Line 170: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uchar byte  = data_flat[d * chunk_size + pos];
- Line 291: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::error("OpenCL: failed to allocate GF table buffers "
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("OpenCL decode: no chunks available");
- Line 384: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 413: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aug[r * aug_cols + data_shards + b] = chunk[b];
- Line 423: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.push_back(aug[d * aug_cols + data_shards + b]);
- Line 466: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& block = data_blocks[s];
- Line 543: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: clSetKernelArg(kernel_, 4, sizeof(cl_mem), &buf_gf_log_);
- Line 569: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& block = data_blocks[s];
- Line 637: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_chunks[d].data(), chunk_size);
- Line 676: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: clSetKernelArg(kernel_, 4, sizeof(cl_mem),  &buf_gf_log_);
- Line 704: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(parity_chunks[p].data(),
- Line 726: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: acc ^= gf_mul(enc_matrix[p * data_shards + d],
- Line 727: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_chunks[d][pos]);
- Line 353: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& c : data_chunks)   result.push_back(std::move(c));
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& c : parity_chunks) result.push_back(std::move(c));
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& c : parity_chunks) result.push_back(std::move(c));
- Line 379: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 380: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: present_indices.push_back(i);
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(aug[d * aug_cols + data_shards + b]);
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(encode(block, data_shards, parity_shards));
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(encode(block, data_shards, parity_shards));
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe_chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stripe_chunks.push_back(std::move(chunk));
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe_chunks.push_back(std::move(pchunk));
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stripe_chunks.push_back(std::move(pchunk));
- Line 589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(stripe_chunks));
- Line 594: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(encode(block, data_shards, parity_shards));
- Line 266: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::string log(log_size, '\0');
  Confidence: band=medium; score=0.6

### src/sharding/shard_topology.cpp
Total findings: 38

- Line 114: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 140: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : shards_) {
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : shards_) {
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = client.post(config_.metadata_endpoint, prefix, request_body);
- Line 345: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = client.post(config_.metadata_endpoint, "/v3/kv/put", request_body);
- Line 381: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : shards_) {
  Confidence: band=very_high; score=0.9
- Line 393: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : shards_) {
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : shards_) {
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : shards_) {
  Confidence: band=very_high; score=0.9
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[(n >> 18) & 63]);
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[(n >> 12) & 63]);
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[(n >> 6) & 63]);
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[n & 63]);
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[(n >> 18) & 63]);
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[(n >> 12) & 63]);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back('=');
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back('=');
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(B64_ENCODE_TABLE[(n >> 18) & 63]);
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: range_end.push_back('\x00');
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard.replica_endpoints.push_back(ep.get<std::string>());
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard.replica_endpoints.push_back(ep.get<std::string>());
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard.capabilities.push_back(cap.get<std::string>());
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard.capabilities.push_back(cap.get<std::string>());
- Line 348: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "ShardTopology: Failed to save shard " << shard_id
- Line 382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaders.push_back(id);
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: leaders.push_back(id);
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 414: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 430: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++healthy;

### src/sharding/two_phase_commit_coordinator.cpp
Total findings: 37

- Line 424: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // touching shared state, so that concurrent coordinator operations are
  Confidence: band=very_high; score=0.99
- Line 455: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch4: mutex_ upgraded to std::timed_mutex; lock.lock() replaced with lock.try_lock_for(config_.prepare_timeout) with abort on failure)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();  // re-acquire before touching shared state
- Line 489: severity=CRITICAL; category=double_lock; **status=FIXED** (batch4: lock.lock() replaced with lock.try_lock_for(config_.prepare_timeout); runPhase2 uses RAII unique_lock<timed_mutex>; continue on timeout)
  Description: Potential double-lock on same mutex: lock
  Remediation: Ensure proper lock nesting or use recursive_mutex if needed
  Context: lock.lock();  // re-acquire before accessing shared state
- Line 489: severity=CRITICAL; category=missing_lock; **status=FIXED** (batch4: same fix — try_lock_for with timed_mutex; continue on timeout)
  Description: Raw .lock() without corresponding .unlock() — use RAII instead
  Remediation: Replace with std::lock_guard<std::mutex> guard(mutex);
  Context: lock.lock();  // re-acquire before accessing shared state
- Line 489: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch4: lock.try_lock_for(config_.prepare_timeout))
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();  // re-acquire before accessing shared state
- Line 492: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch4: error path lock.lock() replaced with lock.try_lock_for(config_.prepare_timeout); continue on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();  // ensure lock is re-acquired even on error path
- Line 57: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC coordinator [{}] WAL initialised at {}",
- Line 71: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("participant must not be null");
- Line 75: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC coordinator [{}] registered participant shard {}",
- Line 91: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: participants_[shard_id] = adapter.get();
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC coordinator [{}] registered remote participant shard {} at {}",
- Line 118: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC coordinator [{}] txn {} – no shards, aborting",
- Line 136: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC coordinator [{}] duplicate commit for completed txn {} – "
- Line 152: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC coordinator [{}] txn {} – unknown shard {}",
- Line 172: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [s, _] : ops_per_shard) v.push_back(s);
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC coordinator [{}] txn {} COMMITTED", coordinator_id_, transaction_id);
- Line 242: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC coordinator [{}] txn {} ABORTED", coordinator_id_, transaction_id);
- Line 267: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC coordinator [{}] recovering from WAL…", coordinator_id_);
- Line 327: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC coordinator [{}] in-doubt txn {} has no decision "
- Line 351: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC coordinator [{}] re-driving in-doubt txn {} with decision {}",
- Line 371: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC coordinator [{}] WAL recovery failed: {}", coordinator_id_, e.what());
- Line 374: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC coordinator [{}] recovery complete – {} in-doubt txns resolved",
- Line 431: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 1 – participant {} not found for txn {}",
- Line 451: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 1 – shard {} threw on PREPARE for txn {}: {}",
- Line 459: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC coordinator [{}] shard {} voted ABORT for txn {}",
- Line 474: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC coordinator [{}] Phase 2 – participant {} not found for txn {} "
- Line 489: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: lock.lock();  // re-acquire before accessing shared state
- Line 492: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: lock.lock();  // ensure lock is re-acquired even on error path
- Line 494: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 2 – shard {} threw on {} for txn {}: {}",
- Line 531: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC coordinator [{}] WAL write failed for txn {}: {}",
- Line 108: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, nlohmann::json>& ops_per_shard
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& [s, _] : ops_per_shard) v.push_back(s);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& [s, _] : ops_per_shard) v.push_back(s);
- Line 275: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, CoordinatorTxnRecord> recovered;
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool>                 decisions; // txn_id → commit?
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec.phase2_acked.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec.phase2_acked.push_back(shard_id);

### src/sharding/capability_matcher.cpp
Total findings: 36

- Line 235: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: query_set.insert(normalize(kw));
  Confidence: band=very_high; score=0.99
- Line 240: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: normalized_shard_kw.insert(normalize(kw));
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = shard_tfidf.find(term);
- Line 40: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid CapabilityMatcher configuration: weights must sum to 1.0");
- Line 276: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = shard_tfidf.find(term);
- Line 310: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (query_magnitude == 0.0 || shard_magnitude == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (shard_set.find(qd) != shard_set.end()) {
- Line 368: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (shard_set.find(qo) != shard_set.end()) {
- Line 393: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (shard_set.find(qr) != shard_set.end()) {
- Line 418: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (shard_set.find(qt) != shard_set.end()) {
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(match_result);
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(match_result);
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keywords.push_back(normalized);
- Line 252: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> query_tfidf;
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> shard_tfidf;
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_keywords.push_back(term);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_keywords.push_back(term);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_keywords.push_back(term);
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_domains.push_back(qd);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_domains.push_back(qd);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_domains.push_back(qd);
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_domains.push_back(qd);
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_orgs.push_back(qo);
  Confidence: band=high; score=0.74
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_orgs.push_back(qo);
  Confidence: band=high; score=0.74
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_orgs.push_back(qo);
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_orgs.push_back(qo);
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_regions.push_back(qr);
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_regions.push_back(qr);
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_regions.push_back(qr);
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_regions.push_back(qr);
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_types.push_back(qt);
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_types.push_back(qt);
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_types.push_back(qt);
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched_types.push_back(qt);
- Line 206: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((total_shards_ + config_.idf_smoothing) /
  Confidence: band=medium; score=0.6
- Line 450: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return std::log(total_shards_ + config_.idf_smoothing);
  Confidence: band=medium; score=0.6

### src/sharding/metadata_shard.cpp
Total findings: 36

- Line 151: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cached = cache_->get(cache_key);
- Line 175: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: cache_->put(cache_key, entry_it->second.toJson());
  Confidence: band=very_high; score=0.99
- Line 181: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool MetadataShard::put(
  Confidence: band=very_high; score=0.99
- Line 298: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator partition_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto partition_it = storage_.find(partition);
- Line 317: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator partition_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto partition_it = storage_.find(partition);
- Line 355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_stats = cache_->getStatistics();
- Line 391: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: cache_->put(cache_key, entry.toJson());
  Confidence: band=very_high; score=0.99
- Line 404: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cached = cache_->get(cache_key);
- Line 438: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto log_index = consensus_->propose(operation, log_data);
- Line 493: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool MetadataShardRouter::put(
  Confidence: band=very_high; score=0.99
- Line 509: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: return it->second->put(partition, key, value);
  Confidence: band=very_high; score=0.99
- Line 665: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator partition_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto partition_it = storage_.find(entry.partition);
- Line 110: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_[partition] = std::map<std::string, MetadataEntry>();
- Line 142: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<MetadataEntry> MetadataShard::get(
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cached = cache_->get(cache_key);
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& callback : sub_it->second) {
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : partition_it->second) {
  Confidence: band=very_high; score=0.9
- Line 362: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto partition : config_.partitions) {
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] MetadataPartitionKey partition,
- Line 404: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cached = cache_->get(cache_key);
  Confidence: band=very_high; score=0.9
- Line 438: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto log_index = consensus_->propose(operation, log_data);
- Line 475: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<MetadataEntry> MetadataShardRouter::get(
  Confidence: band=very_high; score=0.9
- Line 490: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return it->second->get(partition, key);
  Confidence: band=very_high; score=0.9
- Line 536: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : shards_) {
  Confidence: band=very_high; score=0.9
- Line 563: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] MetadataPartitionKey partition,
- Line 662: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_[entry.partition][entry.key] = metadata_entry;
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(pair.first);
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partition_stats.push_back(getPartitionStats(partition));
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: partition_stats.push_back(getPartitionStats(partition));
- Line 375: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subscriptions_[partition].push_back(callback);
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_stats.push_back(pair.second->getStatistics());
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_stats.push_back(pair.second->getStatistics());
  Confidence: band=high; score=0.74
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_stats.push_back(pair.second->getStatistics());
- Line 626: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, MetadataEntry> partition_entries;
  Confidence: band=high; score=0.74

### src/sharding/replica_consistency.cpp
Total findings: 35

- Line 32: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void VectorClock::update(const VectorClock& other) {
  Confidence: band=very_high; score=0.99
- Line 204: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return VersionConflict{};  // Empty conflict
  Confidence: band=very_high; score=0.99
- Line 208: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return entries[0];  // No conflict
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<LogEntry> merged;
  Confidence: band=very_high; score=0.99
- Line 293: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(local);
  Confidence: band=very_high; score=0.99
- Line 38: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=very_high; score=0.9
- Line 49: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t other_timestamp = other.get(node_id);
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ReplicaConsistencyManager::mergeReplicas(
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: stats_.merges_performed++;
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (config_.auto_resolve_conflicts) {
  Confidence: band=very_high; score=0.9
- Line 217: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto resolved = autoResolveConflict(*conflict);
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void ReplicaConsistencyManager::resolveConflict(
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<LogEntry> ReplicaConsistencyManager::mergePartitionedLogs(
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge logs using term and index
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<LogEntry> merged;
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(local);
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(remote);
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(local);
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(local_entries[local_idx++]);
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(remote_entries[remote_idx++]);
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: conflict.needs_manual_resolution = !config_.auto_resolve_conflicts;
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: VersionedEntry ReplicaConsistencyManager::autoResolveConflict(
  Confidence: band=very_high; score=0.9
- Line 25: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: VectorClock::VectorClock(const std::map<std::string, uint64_t>& timestamps)
  Confidence: band=high; score=0.74
- Line 28: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::increment(const std::string& node_id)
  Context: void VectorClock::increment(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 38: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::get(const std::string& node_id)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=medium; score=0.56
- Line 43: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::happensBefore(const VectorClock& other)
  Context: bool VectorClock::happensBefore(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 75: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::isConcurrent(const VectorClock& other)
  Context: bool VectorClock::isConcurrent(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 101: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, uint64_t> timestamps;
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(entry);
- Line 285: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(local);
- Line 289: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(remote);
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(local);
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(local_entries[local_idx++]);
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(remote_entries[remote_idx++]);

### src/sharding/cloud_agent.cpp
Total findings: 34

- Line 170: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_operations_.find(operation_id);
- Line 322: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_operations_.begin();
- Line 478: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool a_local = (shard_a->datacenter == config_.datacenter);
- Line 479: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool b_local = (shard_b->datacenter == config_.datacenter);
- Line 484: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool a_same_region = (shard_a->datacenter.find(config_.region) != std::string::npos);
- Line 485: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool b_same_region = (shard_b->datacenter.find(config_.region) != std::string::npos);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #45 [WIP] Delegate tasks to cloud agent (2026-03-11T17:05:12Z)
- Line 64: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: worker_thread_ = std::thread(&CloudAgent::workerLoop, this);
  Confidence: band=very_high; score=0.9
- Line 68: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_thread_ = std::thread(&CloudAgent::healthLoop, this);
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [op_id, op] : pending_operations_) {
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = executor_->get(shard, "/health");
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 313: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
- Line 362: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 531: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: shard_result["datacenter"] = shard_info->datacenter;
- Line 550: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: shard_result["data"] = response.data;
- Line 569: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < futures.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 571: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto [shard_id, shard_result] = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 578: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(results_mutex);
- Line 592: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(results_mutex);
- Line 115: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: operation.on_complete(result.result);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(op_id);
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending.push_back(op_id);
- Line 349: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: operation.on_complete(result.result);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_shards.push_back(shard.shard_id);
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: target_shards.push_back(shard.shard_id);
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_shard_ids.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 508: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_shard_ids.push_back(shard_id);
- Line 510: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 571: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated_result.push_back(shard_result["data"]);
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated_result.push_back(shard_result["data"]);
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aggregated_result.push_back(shard_result["data"]);

### src/sharding/health_check.cpp
Total findings: 34

- Line 201: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 14 > array 0
  Remediation: Fix loop condition or increase array size
  Context: int year = (data[0] - '0') * 10 + (data[1] - '0');
- Line 203: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 14 > array 2
  Remediation: Fix loop condition or increase array size
  Context: tm_expiry.tm_mon = (data[2] - '0') * 10 + (data[3] - '0') - 1;
- Line 204: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 14 > array 4
  Remediation: Fix loop condition or increase array size
  Context: tm_expiry.tm_mday = (data[4] - '0') * 10 + (data[5] - '0');
- Line 205: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 14 > array 6
  Remediation: Fix loop condition or increase array size
  Context: tm_expiry.tm_hour = (data[6] - '0') * 10 + (data[7] - '0');
- Line 206: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 14 > array 8
  Remediation: Fix loop condition or increase array size
  Context: tm_expiry.tm_min = (data[8] - '0') * 10 + (data[9] - '0');
- Line 207: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 14 > array 10
  Remediation: Fix loop condition or increase array size
  Context: tm_expiry.tm_sec = (data[10] - '0') * 10 + (data[11] - '0');
- Line 127: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::thread([this, shard_endpoints]() {
  Confidence: band=very_high; score=0.9
- Line 136: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.check_interval_ms));
- Line 187: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const unsigned char* data = not_after->data;
- Line 205: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tm_expiry.tm_hour = (data[6] - '0') * 10 + (data[7] - '0');
- Line 206: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tm_expiry.tm_min = (data[8] - '0') * 10 + (data[9] - '0');
- Line 207: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tm_expiry.tm_sec = (data[10] - '0') * 10 + (data[11] - '0');
- Line 225: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tm_expiry.tm_hour = (data[8] - '0') * 10 + (data[9] - '0');
- Line 226: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tm_expiry.tm_min = (data[10] - '0') * 10 + (data[11] - '0');
- Line 227: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tm_expiry.tm_sec = (data[12] - '0') * 10 + (data[13] - '0');
- Line 280: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = client.get(endpoint, "/api/v1/metrics/storage");
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = client.get(endpoint, "/api/v1/metrics/storage");
- Line 284: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: response = client.get(endpoint, "/health");
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: response = client.get(endpoint, "/health");
- Line 336: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = client.get(endpoint, "/health");
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = client.get(endpoint, "/health");
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cluster_info.shard_health.push_back(shard_health);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cluster_info.shard_health.push_back(shard_health);
- Line 94: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: cluster_info.healthy_shards++;
- Line 100: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: cluster_info.unhealthy_shards++;
- Line 157: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 167: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 209: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 229: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 234: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 258: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 312: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 348: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 365: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: unhealthy_count++;

### src/sharding/raft_consensus.cpp
Total findings: 34

- Line 72: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // atomically under replica_mutex_ so that a concurrent step-down cannot
  Confidence: band=very_high; score=0.99
- Line 116: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int required = static_cast<int>(self->raft_state_.getQuorumSize());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 39: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread(&RaftConsensus::heartbeatLoop, this);
  Confidence: band=very_high; score=0.9
- Line 40: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread(&RaftConsensus::electionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 43: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: partition_detector_thread_ = std::thread(&RaftConsensus::partitionDetectionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::thread([self, captured_entry, cb, promise]() {
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& member : self->raft_state_.getClusterMembers()) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : replica_states_) {
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 236: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lock, std::chrono::milliseconds(config_.raft_config.heartbeat_interval_ms),
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(config_.raft_config.heartbeat_interval_ms),
- Line 252: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 253: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lock, std::chrono::milliseconds(50),
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(50),
- Line 263: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(partition_mutex_);
- Line 274: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 275: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lock, config_.partition_detection_interval,
  Confidence: band=very_high; score=0.9
- Line 354: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& node : status.reachable_nodes) {
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : replica_states_) {
  Confidence: band=very_high; score=0.9
- Line 408: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& member : config_.raft_config.cluster_members) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: states.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: states.push_back(pair.second);
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hb.reachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hb.reachable_nodes.push_back(pair.first);
- Line 324: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: PartitionStatus status;
- Line 334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.reachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: status.reachable_nodes.push_back(pair.first);
- Line 336: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: healthy_count++;
- Line 338: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: status.unreachable_nodes.push_back(pair.first);
- Line 354: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status.partition_id += ":" + node;
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status.partition_id += ":" + node;
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: status.partition_id += ":" + node;
- Line 398: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: healthy_count++;

### src/sharding/pki_shard_certificate.cpp
Total findings: 33

- Line 91: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 12 > array 8
  Remediation: Fix loop condition or increase array size
  Context: char month_str[8]{};
- Line 214: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cert_bio = utils::make_bio_mem_buf(cert_pem->c_str(), static_cast<int>(cert_pem->size()));
- Line 222: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ca_bio = utils::make_bio_mem_buf(ca_pem->c_str(), static_cast<int>(ca_pem->size()));
- Line 248: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bio = utils::make_bio_mem_buf(crl_pem->c_str(), static_cast<int>(crl_pem->size()));
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 48: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ASN1_TIME_print(bio.get(), time);
  Confidence: band=very_high; score=0.9
- Line 51: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: long len = BIO_get_mem_data(bio.get(), &data);
  Confidence: band=very_high; score=0.9
- Line 76: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: long len = data->length;
- Line 146: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cert = utils::read_x509_from_bio(bio.get());
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: X509_NAME* subject = X509_get_subject_name(cert.get());
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: X509_NAME* issuer = X509_get_issuer_name(cert.get());
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ASN1_INTEGER* serial = X509_get_serialNumber(cert.get());
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: char* hex = BN_bn2hex(bn.get());
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const ASN1_TIME* not_before = X509_get0_notBefore(cert.get());
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const ASN1_TIME* not_after = X509_get0_notAfter(cert.get());
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: parseSAN(cert.get(), info);
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: parseCustomExtensions(cert.get(), info);
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cert = utils::read_x509_from_bio(cert_bio.get());
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto ca_cert = utils::read_x509_from_bio(ca_bio.get());
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto ca_pubkey = utils::EVPKeyPtr(X509_get_pubkey(ca_cert.get()));
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: int result = X509_verify(cert.get(), ca_pubkey.get());
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto crl = utils::read_x509_crl_from_bio(bio.get());
  Confidence: band=very_high; score=0.9
- Line 260: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: STACK_OF(X509_REVOKED)* revoked = X509_CRL_get_REVOKED(crl.get());
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: char* hex = BN_bn2hex(bn.get());
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
  Confidence: band=very_high; score=0.9
- Line 178: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(hex);
- Line 277: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(hex);
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.san_dns.push_back(dns_str);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.san_dns.push_back(dns_str);
- Line 379: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
- Line 381: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.san_ip.push_back(ip_str);
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.san_uri.push_back(uri_str);
- Line 392: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: GENERAL_NAMES_free(san_names);

### src/sharding/predictive_detector.cpp
Total findings: 33

- Line 299: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = metrics_history_.find(shard_id);
- Line 141: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitoring_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(config_.check_interval);
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& shard : shards) {
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(predictions_mutex_);
- Line 193: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 203: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 224: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shard_id, prediction] : cached_predictions_) {
  Confidence: band=very_high; score=0.9
- Line 345: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<double> latencies, throughputs, error_rates;
  Confidence: band=very_high; score=0.9
- Line 348: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: throughputs.push_back(static_cast<double>(m.throughput_ops_per_sec));
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: features.push_back(compute_mean(throughputs));
  Confidence: band=very_high; score=0.9
- Line 399: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: features.push_back(compute_stddev(throughputs));
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: features.push_back(compute_trend(throughputs));
  Confidence: band=very_high; score=0.9
- Line 450: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size_t(5), features.size()); ++i) {
- Line 473: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size_t(5), features.size()); ++i) {
- Line 207: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predictions.push_back(prediction);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: predictions.push_back(prediction);
- Line 237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: high_risk.push_back(prediction);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: high_risk.push_back(prediction);
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(metrics);
  Confidence: band=high; score=0.74
- Line 309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(metrics);
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(m.avg_latency_ms);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latencies.push_back(m.avg_latency_ms);
- Line 348: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: throughputs.push_back(static_cast<double>(m.throughput_ops_per_sec));
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: error_rates.push_back(static_cast<double>(m.read_errors + m.write_errors));
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(compute_mean(latencies));
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(compute_mean(latencies));
- Line 391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(compute_stddev(latencies));
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(compute_trend(latencies));
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(history.back().avg_latency_ms / 100.0));  // Current normalize
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(history.back().p95_latency_ms / 100.0));
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(0.0f);

### src/sharding/wal_manager.cpp
Total findings: 33

- Line 82: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), data_str.begin(), data_str.end());
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: std::optional<WALEntry> WALManager::read(const LSN& lsn) {
- Line 298: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: current_segment_->write(reinterpret_cast<const char*>(write_buffer_.data()),
- Line 28: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid LSN format: " + str);
- Line 129: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_len = (data_len << 8) | bytes[pos++];
- Line 207: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<WALEntry> WALManager::read(const LSN& lsn) {
  Confidence: band=very_high; score=0.9
- Line 242: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(buffer.data()), file_size);
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: current_segment_->write(reinterpret_cast<const char*>(write_buffer_.data()),
- Line 330: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t seg = oldest_lsn_.segment; seg < lsn.segment; ++seg) {
  Confidence: band=very_high; score=0.9
- Line 351: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
- Line 407: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
- Line 445: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    entry.lsn.offset = 0;', '    for (int i = 0; i < 8; ++i) {', '        entry.lsn.offset = (entry.lsn.offset << 8) | bytes[pos++];', '    }', '']
  Confidence: band=medium; score=0.65
- Line 41: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALEntry::serialize()
  Context: std::vector<uint8_t> WALEntry::serialize() const {
  Confidence: band=medium; score=0.56
- Line 46: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>(type));
- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((timestamp >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((timestamp >> (i * 8)) & 0xFF);
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((lsn.segment >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((lsn.segment >> (i * 8)) & 0xFF);
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((lsn.offset >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((lsn.offset >> (i * 8)) & 0xFF);
- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((tx_id_len >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((tx_id_len >> (i * 8)) & 0xFF);
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((data_len >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back((data_len >> (i * 8)) & 0xFF);
- Line 169: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALManager::append(const WALEntry& entry)
  Context: LSN WALManager::append(const WALEntry& entry) {
  Confidence: band=medium; score=0.56
- Line 265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 268: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 375: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: current_segment_->close();
- Line 412: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_num);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments.push_back(seg_num);
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back({seg_num, entry.path().string()});
  Confidence: band=high; score=0.74
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments.push_back({seg_num, entry.path().string()});

### src/sharding/hot_spare_manager.cpp
Total findings: 30

- Line 49: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (max_concurrent_rebuilds == 0) {
  Confidence: band=very_high; score=0.99
- Line 50: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: spdlog::error("Invalid max_concurrent_rebuilds: must be > 0");
  Confidence: band=very_high; score=0.99
- Line 473: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: history.insert(history.end(), start_it, failover_history_.end());
  Confidence: band=very_high; score=0.99
- Line 659: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (active_rebuilds >= config_.max_concurrent_rebuilds) {
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 16: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * with a compatibility shim for themisdb::sharding at the end,
  Confidence: band=high; score=0.8
- Line 75: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid hot spare configuration");
- Line 103: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_check_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: rebuild_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 429: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& spare : status.rebuilding_spares) {
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid hot spare configuration");
- Line 619: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [_, spare] : spares_) {
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(config_.health_check_interval);
  Confidence: band=very_high; score=0.9
- Line 633: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(rebuild_mutex_);
- Line 636: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: rebuild_cv_.wait_for(lock, std::chrono::seconds(10), [this]() {
- Line 652: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, spare] : spares_) {
  Confidence: band=very_high; score=0.9
- Line 765: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < task.documents.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(rebuild_mutex_);
- Line 898: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility shim
  Confidence: band=high; score=0.8
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: available.push_back(shard_id);
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spares.push_back(spare);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: spares.push_back(spare);
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failover_history_.push_back(event);
- Line 413: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: RebuildStatus status;
- Line 418: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.active_rebuilds++;
- Line 418: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.rebuilding_spares.push_back(spare);
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: status.rebuilding_spares.push_back(spare);
- Line 437: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: total_eta_seconds / status.active_rebuilds);
- Line 595: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << "# HELP themis_hot_spare_rebuild_throughput_mbps Rebuild throughput in MB/s\n";

### src/sharding/raft_log.cpp
Total findings: 29

- Line 53: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = log_.find(i);
- Line 106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint64_t last = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
- Line 321: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.99
- Line 324: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.99
- Line 332: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(checksum.data(), static_cast<std::streamsize>(checksum_len));
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(compressed.data()),
  Confidence: band=very_high; score=0.99
- Line 52: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = log_.find(i);
- Line 52: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t i = start_index; i <= end_index; ++i) {
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: constexpr size_t kEntryOverhead = sizeof(uint64_t) * 3;
- Line 149: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [idx, entry] : log_) {
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 372: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: constexpr size_t kMinHeaderBytes = 4 * sizeof(uint64_t) + sizeof(uint32_t);
- Line 392: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.9
- Line 401: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.9
- Line 441: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(snap.checksum.data(), static_cast<std::streamsize>(checksum_len));
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(snap.data.data()),
  Confidence: band=very_high; score=0.9
- Line 513: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry :
  Confidence: band=very_high; score=0.9
- Line 582: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(chunk.data.data()),
  Confidence: band=very_high; score=0.9
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(it->second);
- Line 337: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 463: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(std::stoull(id_str));
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(std::stoull(id_str));
- Line 522: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 584: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(std::stoull(id_str));
  Confidence: band=high; score=0.74
- Line 609: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(std::stoull(id_str));
- Line 610: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/sharding/mtls_connection_pool.cpp
Total findings: 27

- Line 108: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: active_connections_.insert(pooled.ssl.get());
  Confidence: band=very_high; score=0.99
- Line 408: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto pool_stats = pool->getStatistics();
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
- Line 36: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Initializing connection pool for endpoint: " << endpoint << std::endl;
  Confidence: band=very_high; score=0.9
- Line 44: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread([this]() { cleanupLoop(); });
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::optional<std::unique_ptr<SSL, SSLDeleter>> EndpointConnectionPool::getConnection(
- Line 72: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: } else if (pooled.ssl && validateConnection(pooled.ssl.get())) {
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: active_connections_.insert(pooled.ssl.get());
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: active_connections_.insert(new_conn->get());
  Confidence: band=very_high; score=0.9
- Line 106: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (pooled.ssl && validateConnection(pooled.ssl.get())) {
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: active_connections_.insert(pooled.ssl.get());
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: SSL* raw_ptr = connection.get();
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Warmed up " << created << " connections for endpoint: "
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Cleaned up " << removed << " expired connections for endpoint: "
  Confidence: band=very_high; score=0.9
- Line 300: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(config_.health_check_interval);
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::shared_mutex> lock(pool_mutex_);
- Line 315: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Initialized MTLSConnectionPoolManager" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 424: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "MTLSConnectionPoolManager: certificate rotated – draining idle "
  Confidence: band=very_high; score=0.9
- Line 427: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [endpoint, pool] : pools_) {
  Confidence: band=very_high; score=0.9
- Line 429: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // The next getConnection() call will create a new TLS connection which
- Line 438: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Shutting down MTLSConnectionPoolManager with "
  Confidence: band=very_high; score=0.9
- Line 22: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ptr);

### src/sharding/wal_shipper.cpp
Total findings: 26

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        // Approximate lag (segment size * segment difference + offset difference)', '        uint64_t segment_diff = current_lsn.segment - replica.last_confirmed_lsn.segment;', '        replica.lag_bytes = segment_diff * 16 * 1024 * 1024 + current_lsn.offset;', '    }', '']
  Confidence: band=very_high; score=0.9
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // Approximate lag (segment size * segment difference + offset difference)', '        uint64_t segment_diff = current_lsn.segment - replica.last_confirmed_lsn.segment;', '        replica.lag_bytes = segment_diff * 16 * 1024 * 1024 + current_lsn.offset;', '    }', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // Approximate lag (segment size * segment difference + offset difference)', '        uint64_t segment_diff = current_lsn.segment - replica.last_confirmed_lsn.segment;', '        replica.lag_bytes = segment_diff * 16 * 1024 * 1024 + current_lsn.offset;', '    }', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("WAL manager cannot be null");
- Line 94: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : replicas_) {
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(replicas_mutex_);
- Line 122: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, info] : replicas_) {
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& replica_id : replica_ids) {
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(replicas_mutex_);
- Line 131: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(replicas_mutex_);
- Line 145: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 158: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(cv_lock, std::chrono::milliseconds(config_.ship_interval_ms),
- Line 203: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Start new batch
- Line 245: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry_json["data"] = entry.data;
- Line 323: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = mtls_client_->post(endpoint, "/api/v1/wal/apply", request.dump());
- Line 417: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = mtls_client_->get(replica.endpoint, "/api/v1/health");
  Confidence: band=very_high; score=0.9
- Line 563: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& chunk : chunks) {
  Confidence: band=very_high; score=0.9
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replica_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: replica_ids.push_back(id);
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_json.push_back(entry_json);
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_json.push_back(entry_json);
- Line 540: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < data.size()) ? kB64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < data.size()) ? kB64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
  Confidence: band=high; score=0.74

### src/sharding/shard_repair_engine.cpp
Total findings: 25

- Line 362: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch4: jobs_mutex_ upgraded to std::timed_mutex; condition_variable_any used; lock.lock() replaced with lock.try_lock_for(config_.repair_poll_interval) with break on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 477: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch5: replaced unbounded future wait with poll-bounded wait_for loop using repair_poll_interval)
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: f.wait();
- Line 64: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: scan_thread_ = std::thread([this]() { scanLoop(); });
  Confidence: band=very_high; score=0.9
- Line 68: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: repair_thread_ = std::thread([this]() { repairLoop(); });
  Confidence: band=very_high; score=0.9
- Line 204: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, job] : jobs_) {
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, report] : shard_health_) {
  Confidence: band=very_high; score=0.9
- Line 324: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(step);
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(jobs_mutex_);
- Line 520: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& doc_id : doc_ids) {
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!resource_manager_->acquireRepairIOToken() && running_.load()) {
- Line 526: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
  Confidence: band=very_high; score=0.9
- Line 526: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 700: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!resource_manager_->acquireRepairIOToken() && running_.load()) {
- Line 701: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
  Confidence: band=very_high; score=0.9
- Line 701: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active.push_back(job);
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reports.push_back(report);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: reports.push_back(report);
- Line 432: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bands[i % num_workers].push_back(all_shards[i]);
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: band_futures.push_back(promise->get_future());
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: band_futures.push_back(promise->get_future());
- Line 493: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::info("ShardRepairEngine: parallel anti-entropy scan complete ({} shards, {} workers)",
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++report.documents_healthy;
- Line 650: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 684: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : ShardRepairStatus::DEGRADED;

### src/sharding/paxos_snapshot.cpp
Total findings: 23

- Line 249: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(text.data(), static_cast<std::streamsize>(text.size()));
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            }', '', '            snapshot.instances[slot] = instance_json;', '        }', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 79: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["snapshot_id"] = snapshot_id;
- Line 80: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["last_applied_lsn"] = last_applied_lsn.toString();
- Line 81: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["last_committed_slot"] = last_committed_slot;
- Line 82: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["current_round"] = current_round;
- Line 83: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["node_id"] = node_id;
- Line 84: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["timestamp"] = timestamp;
- Line 85: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["instances"] = instances;
- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_json["committed_log"] = committed_log;
- Line 215: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry_json["data"] = entry.data;
- Line 293: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Detect format: binary ZSTD ("PAXZ" magic) or legacy plain JSON
  Confidence: band=high; score=0.8
- Line 295: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(magic, 4);
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(compressed.data()),
  Confidence: band=very_high; score=0.9
- Line 321: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy plain-JSON format – rewind and parse as text
  Confidence: band=high; score=0.8
- Line 355: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
- Line 251: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 312: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 325: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 363: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshots.push_back(snapshot_id);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshots.push_back(snapshot_id);
- Line 365: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/prometheus_metrics.cpp
Total findings: 23

- Line 90: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordResultMergeTime(double time_ms) {
  Confidence: band=very_high; score=0.99
- Line 91: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
  Confidence: band=very_high; score=0.99
- Line 134: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void PrometheusMetrics::recordGossipConfigUpdate(const std::string& operation) {
  Confidence: band=very_high; score=0.99
- Line 142: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordGossipConfigConflict(const std::string& resolution_type) {
  Confidence: band=very_high; score=0.99
- Line 143: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: incrementCounter("themis_gossip_config_conflicts_total", {{"resolution", resolution_type}});
  Confidence: band=very_high; score=0.99
- Line 576: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordPaxosProposalConflict(const std::string& shard_id) {
  Confidence: band=very_high; score=0.99
- Line 577: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: incrementCounter("themis_paxos_proposal_conflicts_total", {{"shard_id", shard_id}});
  Confidence: band=very_high; score=0.99
- Line 685: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordPercolatorConflict(const std::string& transaction_id) {
  Confidence: band=very_high; score=0.99
- Line 686: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: incrementCounter("themis_percolator_conflicts_total", {{"transaction_id", transaction_id}});
  Confidence: band=very_high; score=0.99
- Line 733: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void PrometheusMetrics::recordMvccWrite(double latency_ms) {
  Confidence: band=very_high; score=0.99
- Line 90: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void PrometheusMetrics::recordResultMergeTime(double time_ms) {
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
  Confidence: band=very_high; score=0.9
- Line 345: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, values] : histograms_) {
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void PrometheusMetrics::recordMvccRead(const std::string& read_type, double latency_ms) {
  Confidence: band=very_high; score=0.9
- Line 31: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: incrementCounter("themis_cross_shard_rpc_calls_total",
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p50 = sorted[sorted.size() * 50 / 100];
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p95 = sorted[sorted.size() * 95 / 100];
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p99 = sorted[sorted.size() * 99 / 100];
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "# HELP themis_gossip_messages_total Total gossip messages sent/received\n";
- Line 408: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p50 = sorted[sorted.size() * 50 / 100];
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p95 = sorted[sorted.size() * 95 / 100];
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p99 = sorted[sorted.size() * 99 / 100];
  Confidence: band=high; score=0.74
- Line 724: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"shard_id", shard_id}, {"status", s}});

### src/sharding/two_phase_commit_participant.cpp
Total findings: 22

- Line 53: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC participant [{}] WAL initialised at {}", shard_id_, config_.wal_directory);
- Line 72: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC participant [{}] duplicate PREPARE for {} – returning stored vote {}",
- Line 83: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC participant [{}] PREPARE {}: failed to parse ops – aborting: {}",
- Line 102: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC participant [{}] PREPARE {}: lock/validation error: {}",
- Line 148: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC participant [{}] duplicate COMMIT for {} – idempotent ok",
- Line 155: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC participant [{}] COMMIT for unknown/aborted txn {}",
- Line 171: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC participant [{}] COMMIT {}: apply error: {}",
- Line 178: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC participant [{}] COMMIT {} failed to apply operations",
- Line 189: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC participant [{}] COMMIT {}: lock release error (ignored): {}",
- Line 210: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC participant [{}] COMMIT {} applied successfully", shard_id_, transaction_id);
- Line 222: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC participant [{}] duplicate ABORT for {} – idempotent ok",
- Line 229: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC participant [{}] ABORT for already-committed txn {}",
- Line 238: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC participant [{}] ABORT {}: lock release error (ignored): {}",
- Line 270: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("2PC participant [{}] ABORT {} completed", shard_id_, transaction_id);
- Line 308: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC participant [{}] aborting timed-out prepared txn {}",
- Line 334: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC participant [{}] recovering from WAL…", shard_id_);
- Line 387: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [txn_id, txn] : transactions_) {
  Confidence: band=very_high; score=0.9
- Line 389: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("2PC participant [{}] in-doubt transaction found: {}",
- Line 396: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC participant [{}] WAL recovery failed: {}", shard_id_, e.what());
- Line 399: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("2PC participant [{}] recovery complete – {} in-doubt transactions",
- Line 461: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("2PC participant [{}] WAL write failed for txn {}: {}",
- Line 313: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}

### src/sharding/consistent_hash.cpp
Total findings: 21

- Line 73: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = shard_tokens_.find(shard_id);
- Line 152: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 209: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 223: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (auto it = ring_.begin(); it != ring_.end() && it->first <= hash_end; ++it) {
- Line 223: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = ring_.begin(); it != ring_.end() && it->first <= hash_end; ++it) {
- Line 224: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 232: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: while (it != ring_.end() && it->first <= hash_end) {
- Line 233: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 51: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < virtual_nodes; ++i) {
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t token : it->second) {
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shard_id, _] : shard_tokens_) {
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = ring_.begin(); it != ring_.end() && it->first <= hash_end; ++it) {
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: vnode_key += '#';
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: vnode_key += '#';
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(token);
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);
- Line 108: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::optional<std::string> ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(shard_id);
- Line 204: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66

### src/sharding/truetime.cpp
Total findings: 21

- Line 251: severity=CRITICAL; category=missing_dtor
  Description: Class NTPPacket allocates resources but has no destructor
  Remediation: Add explicit destructor: ~NTPPacket() { /* cleanup */ }
  Context: class/struct NTPPacket
- Line 270: severity=CRITICAL; category=socket_leak
  Description: Socket created but never closed — potential resource leak
  Remediation: Wrap socket in RAII class (e.g., std::unique_ptr with custom deleter)
  Context: SocketHandle sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: startSyncThread();
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: stopSyncThread();
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void TrueTime::startSyncThread() {
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sync_thread_ = std::thread(&TrueTime::syncThreadFunc, this);
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void TrueTime::stopSyncThread() {
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: closesocket(fd_);
- Line 243: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: SocketHandle get() const { return fd_; }
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SocketHandle sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
- Line 426: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 135: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"uncertainty_us\": " << (uncertainty_ns_.load() / 1000) << ", "
- Line 135: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"uncertainty_us\": " << (uncertainty_ns_.load() / 1000) << ", "
- Line 136: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"drift_us\": " << (drift_ns_.load() / 1000) << ", "
- Line 136: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"drift_us\": " << (drift_ns_.load() / 1000) << ", "
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: offsets.push_back(offset);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: offsets.push_back(offset);
- Line 239: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(fd_);
- Line 395: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/data_migrator.cpp
Total findings: 19

- Line 108: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto resp = count_client->get(config_.source_endpoint, count_path.str());
- Line 277: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
- Line 333: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto response = mtls_client->post(config_.target_endpoint, path, request_body);
- Line 438: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_migrations_.insert(migration_id);
  Confidence: band=very_high; score=0.99
- Line 449: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_batches_.insert(batch_id);
  Confidence: band=very_high; score=0.99
- Line 478: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_migrations_.insert(item.get<std::string>());
  Confidence: band=very_high; score=0.99
- Line 494: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_batches_.insert(item.get<std::string>());
  Confidence: band=very_high; score=0.99
- Line 108: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto resp = count_client->get(config_.source_endpoint, count_path.str());
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Write batch to target (atomic operation)
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy format: response body itself might be the array
  Confidence: band=high; score=0.8
- Line 417: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (batch_counter_.fetch_add(1, std::memory_order_relaxed) % 10 == 0) {
- Line 247: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 376: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 518: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: migrations_json.push_back(migration_id);
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: migrations_json.push_back(migration_id);
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches_json.push_back(batch_id);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batches_json.push_back(batch_id);

### src/sharding/health_monitor.cpp
Total findings: 19

- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto replica_set = topology_->getReplicaSet(shard_id);
- Line 394: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto future = http_pool_->get(url);
- Line 80: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Fallback to dedicated thread (backward compatibility)
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fallback to dedicated thread (backward compatibility)
  Confidence: band=high; score=0.8
- Line 81: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitor_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(config_.heartbeat_interval);
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& primary : primaries) {
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 274: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& shard_id : all_shards) {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& replica_id : replica_set->replicas) {
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 286: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 344: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& primary : all_primaries) {
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto future = http_pool_->get(url);
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = future.get();
  Confidence: band=very_high; score=0.9
- Line 128: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, HealthCheckResult> HealthMonitor::getAllHealthStatuses() const {
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(primary.node_id);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(primary.node_id);
- Line 411: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/raft_consensus_adapter.cpp
Total findings: 19

- Line 498: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // against any concurrent state change.
  Confidence: band=very_high; score=0.99
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 182: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 353: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function consensus without trace point
  Context: spdlog::info("Node {} added to cluster via joint consensus (endpoint: {})",
  Confidence: band=very_high; score=0.9
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(convertLogEntry(entry));
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(convertLogEntry(entry));
- Line 647: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/quorum_manager.cpp
Total findings: 18

- Line 115: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: QuorumResult QuorumManager::executeRead(ReadOperation operation,
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [node, data] : results) {
- Line 245: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: T result = future.get();
  Confidence: band=very_high; score=0.9
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back({node, std::move(future)});
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back({node, std::move(future)});
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successful_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successful_nodes.push_back(node);
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_nodes.push_back(node);
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back({node, std::move(future)});
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back({node, std::move(future)});
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back({node, std::move(future)});
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successful_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successful_nodes.push_back(node);
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_nodes.push_back(node);
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({node, result});
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({node, result});
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({node, result});
- Line 264: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/raft_wal_integration.cpp
Total findings: 18

- Line 23 (destructor): severity=CRITICAL; category=data_race; **status=FIXED** (batch4: added std::lock_guard<std::mutex> lock(mutex_) at start of ~RaftWALIntegration() before reading is_leader_)
  Description: Destructor reads is_leader_ without holding mutex_ — data race if other threads access object during destruction

- Line 32: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: RaftWALIntegration::WriteResult RaftWALIntegration::write(const WALEntry& entry) {
- Line 40: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: LSN wal_lsn = config_.wal_manager->append(entry);
- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: log_entry.term = config_.raft_state->getCurrentTerm();
- Line 49: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: log_entry.index = config_.raft_log->getLastLogIndex() + 1;
- Line 52: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t log_index = config_.raft_log->append(log_entry);
- Line 72: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_writes_.find(log_index);
- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return it != pending_writes_.end() && it->second.committed;
- Line 85: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: std::optional<WALEntry> RaftWALIntegration::read(const LSN& lsn) {
- Line 92: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return config_.wal_manager->read(lsn);
- Line 164: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& members = config_.raft_state->getClusterMembers();
- Line 187: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: // Wake up write() waiters whenever a new entry reaches quorum.
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    pending.committed = false;', '', '    pending_writes_[log_index] = pending;', '', '    // CC-2a: The original audit finding flagged a potential self-deadlock where']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 69: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: // in wait_for(), eliminating the deadlock.
  Confidence: band=very_high; score=0.9
- Line 71: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: bool quorum_reached = cv_.wait_for(lock, timeout, [this, log_index]() {
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = pending_writes_.find(log_index);
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<WALEntry> RaftWALIntegration::read(const LSN& lsn) {
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return config_.wal_manager->read(lsn);
  Confidence: band=very_high; score=0.9

### src/sharding/shard_resource_manager.cpp
Total findings: 18

- Line 256: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch5: acquireRepairIOToken now supports bounded wait_timeout and scan/repair callers use repair_poll_interval)
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return repair_io_limiter_->try_acquire(io_ops);
- Line 274: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void ShardResourceManager::broadcastResourceUpdate() {
  Confidence: band=very_high; score=0.99
- Line 315: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void ShardResourceManager::receiveResourceUpdate(const std::string& shard_id,
  Confidence: band=very_high; score=0.99
- Line 437: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: broadcastResourceUpdate();
  Confidence: band=very_high; score=0.99
- Line 481: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = peer_resources_.begin();
- Line 173: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitoring_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool ShardResourceManager::acquireRepairIOToken(double io_ops) {
- Line 256: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return repair_io_limiter_->try_acquire(io_ops);
- Line 440: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(
  Confidence: band=very_high; score=0.9
- Line 102: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = j["timestamp_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ShardResourceManager::ResourceSnapshot>
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::string> healthy_peers;
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_peers.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy_peers.push_back(shard_id);
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overloaded_peers.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: overloaded_peers.push_back(shard_id);
- Line 434: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cleanupStaleSnapshots();
  Confidence: band=high; score=0.74
- Line 475: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void ShardResourceManager::cleanupStaleSnapshots() {
  Confidence: band=high; score=0.74

### src/sharding/gossip_consensus_adapter.cpp
Total findings: 16

- Line 179: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = log_entries_.find(i);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    // Calculate quorum (majority)', '    size_t quorum_size = (cluster_nodes_.size() / 2) + 1;', '    return it->second.size() >= quorum_size;', '}']
  Confidence: band=high; score=0.81
- Line 70: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: gossip_thread_ = std::thread(&GossipConsensusAdapter::gossipThread, this);
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 336: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void GossipConsensusAdapter::gossipThread() {
  Confidence: band=very_high; score=0.9
- Line 340: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(state_mutex_);
- Line 343: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: gossip_cv_.wait_for(lock, config_.gossip_interval, [this] {
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> log_lock(log_mutex_);
- Line 356: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [index, entry] : log_entries_) {
  Confidence: band=very_high; score=0.9
- Line 361: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> cb_lock(callbacks_mutex_);
- Line 361: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> cb_lock(callbacks_mutex_);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);

### src/sharding/shard_rpc_server.cpp
Total findings: 16

- Line 379: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void ShardRPCServer::wait() {
- Line 68: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool vote_commit = handler_->onPrepare(
- Line 71: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: request->transaction_data()
- Line 98: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool success = handler_->onCommit(request->transaction_id());
- Line 123: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool success = handler_->onAbort(request->transaction_id());
- Line 143: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto health_info = handler_->onHealthCheck();
- Line 172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto edges = handler_->onCollectWaitForEdges();
- Line 205: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto health = handler_->onHealthCheck();
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->handler = handler;
- Line 270: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->service = std::make_unique<ShardServiceImpl>(impl_->handler);
- Line 324: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 336: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: builder.RegisterService(impl_->service.get());
  Confidence: band=very_high; score=0.9
- Line 294: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
- Line 320: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::string(override_flag) == "1")
  Context: override_flag && std::string(override_flag) == "1") {
  Confidence: band=medium; score=0.56
- Line 336: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: builder.RegisterService(impl_->service.get());
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: impl_->service.reset();
  Confidence: band=high; score=0.74

### src/sharding/admin_api.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Context: ['        nlohmann::json shard_entry;', '        shard_entry["shard_id"] = r.shard_id;', '        shard_entry["status"] = kStatusStr[status_idx];', '        shard_entry["documents_scanned"] = r.documents_scanned;', '        shard_entry["documents_healthy"] = r.documents_healthy;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 126: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: Endpoints
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: } else if (path.find(Endpoints::REPAIR_STATUS) == 0 && method == "GET") {
- Line 168: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
- Line 178: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(ctx);
- Line 194: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(store);
- Line 197: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 235: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: GENERAL_NAMES_free(sans);
- Line 239: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(std::move(shard_entry));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(std::move(shard_entry));

### src/sharding/hardware_migration_manager.cpp
Total findings: 15

- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto shards = ring_->getAllShards();
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t total = ring_->getVirtualNodeCount();
- Line 347: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = in_flight_counts_.find(shard_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 263: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& sid : shards) {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto before_it  = before_vnode_counts.find(sid);
- Line 279: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto current_it = current.find(sid);
- Line 202: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> before_vnodes;
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t>
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t>
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> snapshot;
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, size_t>&  before_vnode_counts
  Confidence: band=high; score=0.74

### src/sharding/raft_configuration.cpp
Total findings: 15

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
- Line 28: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Membership change already in progress");
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Membership change already in progress");
- Line 53: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 82: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& vote : votes) {
  Confidence: band=very_high; score=0.9

### src/sharding/wal_applier.cpp
Total findings: 15

- Line 135: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch5: removed ExponentialBackoff::wait call and switched to explicitly bounded sleep_for backoff)
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backoff.wait();
- Line 158: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool WALApplier::handleConflict(const WALEntry& entry) {
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (!config_.enable_conflict_detection) {
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return true;  // Conflicts ignored
  Confidence: band=very_high; score=0.99
- Line 165: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats_.conflicts_detected++;
  Confidence: band=very_high; score=0.99
- Line 168: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Conflict resolution strategy (can be extended)
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::cerr << "WALApplier: Conflict detected for entry at LSN "
  Confidence: band=very_high; score=0.99
- Line 40: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : entries) {
  Confidence: band=very_high; score=0.9
- Line 52: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> stats_lock(stats_mutex_);
- Line 68: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> stats_lock(stats_mutex_);
- Line 92: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 97: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(error);
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(error);
- Line 129: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "WALApplier: Exception applying entry at LSN "

### src/sharding/locality_aware_router.cpp
Total findings: 14

- Line 210: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: placement_cache_[cache_key].insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 441: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = placement_cache_.begin();
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #777 [YARN-Inspired] Implement Locality-Aware Query Router for Data Affi... (2026-03-11T18:05:
- Line 333: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = placement_cache_.find(cache_key);
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: it->second.find(shard_id) != it->second.end()) {
  Confidence: band=very_high; score=0.9
- Line 71: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string LocalityAwareRouter::routeQuery(const QuerySpec& spec) {
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<std::string> LocalityAwareRouter::routeMultiShardQuery(const QuerySpec& spec) {
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(local_shard_id_);
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(affinity.shard_id);
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(affinity.shard_id);
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(affinity);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(affinity);
- Line 214: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cleanupStaleEntries();
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void LocalityAwareRouter::cleanupStaleEntries() {
  Confidence: band=high; score=0.74

### src/sharding/partition_detector.cpp
Total findings: 13

- Line 35: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_check_thread_ = std::thread(&PartitionDetector::healthCheckLoop, this);
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(nodes_mutex_);
- Line 175: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(config_.health_check_interval);
  Confidence: band=very_high; score=0.9
- Line 240: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.second);
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_ids.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node_ids.push_back(pair.first);
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unreachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unreachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: unreachable_nodes.push_back(pair.first);
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: reachable_nodes.push_back(pair.first);

### src/sharding/shard_durability.cpp
Total findings: 12

- Line 135: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::recursive_directory_iterator(info.path)) {
- Line 321: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.checkpoint_dir)) {
- Line 342: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& file : std::filesystem::recursive_directory_iterator(info.path)) {
- Line 140: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checkpoints_.push_back(info);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: checkpoints_.push_back(info);
- Line 254: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 305: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 336: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checkpoints_.push_back(info);
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: checkpoints_.push_back(info);
- Line 351: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/sharding_manager_edition.cpp
Total findings: 12

- Line 84: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = shard_nodes_.begin(); it != shard_nodes_.end(); ++it) {
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(error);
- Line 64: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(error);
- Line 76: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& node : shard_nodes_) {
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const auto info = edition::EditionInfo::Get();
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const auto info = edition::EditionInfo::Get();
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const auto info = edition::EditionInfo::Get();
  Confidence: band=very_high; score=0.9
- Line 97: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += " | Max Shard Nodes: ";
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += " | Max Shard Nodes: ";
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(all_shards[idx]);
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(all_shards[idx]);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(all_shards[idx]);

### src/sharding/urn_resolver.cpp
Total findings: 12

- Line 28: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string shard_id = hash_ring_->getShardForURN(urn);
- Line 51: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::vector<std::string> successor_ids = hash_ring_->getSuccessors(hash, replica_count + 1);
- Line 69: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string shard_id = hash_ring_->getShardForURN(urn);
- Line 81: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto node = hash_ring_->getNode(key);
- Line 90: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t h_min = hash_ring_->hashKey(min_key);
- Line 91: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t h_max = hash_ring_->hashKey(max_key);
- Line 92: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto shards = hash_ring_->getShardsInRange(h_min, h_max);
- Line 115: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: topology_->refresh();
- Line 56: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*replica);
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*replica);
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(s.shard_id);
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(s.shard_id);

### src/sharding/distributed_coordinator.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 104: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Start heartbeat thread (only active if leader)
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: task_executor_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 198: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& shard : all_shards) {
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Only leader can schedule tasks");
- Line 381: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& task : tasks_to_execute) {
  Confidence: band=very_high; score=0.9
- Line 500: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending_tasks_.push_back(task);

### src/sharding/metadata_snapshot.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 80: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [key, metadata_entry] : entries) {
- Line 81: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: partition_data[key] = metadata_entry.toJson();
- Line 84: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: snapshot.partitions[partition_key] = partition_data;
- Line 188: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
- Line 103: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 154: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot_ids.push_back(snapshot_id);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot_ids.push_back(snapshot_id);
- Line 202: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/sharding/multi_primary_coordinator.cpp
Total findings: 11

- Line 153: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: LSN MultiPrimaryCoordinator::resolveConflict(const WriteConflict& conflict) const {
  Confidence: band=very_high; score=0.99
- Line 154: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_++;
  Confidence: band=very_high; score=0.99
- Line 157: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return conflict.resolveLastWriteWins();
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return (conflict.lsn2 > conflict.lsn1) ? conflict.lsn2 : conflict.lsn1;
  Confidence: band=very_high; score=0.99
- Line 164: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void MultiPrimaryCoordinator::recordWrite(const LSN& lsn) {
  Confidence: band=very_high; score=0.99
- Line 199: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.99
- Line 122: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [node_id, info] : primaries_) {
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [node_id, info] : primaries_) {
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);

### src/sharding/paxos_state_persistence.cpp
Total findings: 11

- Line 105: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool PaxosStatePersistence::open(const std::string& node_id) {
- Line 308: severity=CRITICAL; category=no_timeout; **status=FIXED** (batch4: mutex_ upgraded to std::timed_mutex; persistCommit restructured to use unique_lock; mutex_.lock() replaced with lock.try_lock_for(30s) with return false on timeout)
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: mutex_.lock();
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        uint64_t slot = entry.slot;', '', '        DurableAcceptorState& s = slot_cache_[slot];', '        s.slot = slot;', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    if (!is_open_.load()) return false;', '', '    DurableAcceptorState& s = slot_cache_[slot];', '    s.slot           = slot;', '    s.promised_round = ballot_round;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    if (!is_open_.load()) return false;', '', '    DurableAcceptorState& s = slot_cache_[slot];', '    s.slot          = slot;', '    s.accepted_round = ballot_round;']
  Confidence: band=high; score=0.81
- Line 47: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry.data["accepted_round"] = ballot_round;
- Line 48: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry.data["slot"] = slot;
- Line 97: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!wal_)          throw std::invalid_argument("PaxosStatePersistence: wal cannot be null");
- Line 98: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!snapshot_mgr_) throw std::invalid_argument("PaxosStatePersistence: snapshot_mgr cannot be null"
- Line 177: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s.accepted_value = logged_value["data"]["raw_command"].get<std::string>();
- Line 143: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PaxosStatePersistence::close() {

### src/sharding/raft_shard_manager.cpp
Total findings: 11

- Line 86: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = raft_instances_.find(shard_id);
- Line 147: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::future<bool> RaftShardManager::proposeWrite(const std::string& shard_id,
  Confidence: band=very_high; score=0.99
- Line 32: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [shard_id, raft] : raft_instances_) {
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& replica : replica_states) {
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& replica : replica_states) {
  Confidence: band=very_high; score=0.9
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.replica_ids.push_back(replica.node_id);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.replica_ids.push_back(replica.node_id);
- Line 195: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ShardRaftInfo> RaftShardManager::getAllShardRaftInfo() const {
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ShardRaftInfo> all_info;
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.replica_ids.push_back(replica.node_id);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.replica_ids.push_back(replica.node_id);

### src/sharding/circuit_breaker.cpp
Total findings: 10

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 105: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 203: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 214: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [shard_id, cb] : circuit_breakers_) {
  Confidence: band=very_high; score=0.9
- Line 224: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shard_id, _] : circuit_breakers_) {
  Confidence: band=very_high; score=0.9
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_ids.push_back(shard_id);

### src/sharding/mtls_client.cpp
Total findings: 10

- Line 125: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: MTLSClient::Response MTLSClient::put(const std::string& endpoint,
  Confidence: band=very_high; score=0.99
- Line 216: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: http::write(stream, req);
  Confidence: band=very_high; score=0.99
- Line 216: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(stream, req);
- Line 221: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::read(stream, buffer, res);
- Line 381: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto stats = pool_manager_->getStatistics();
- Line 107: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: MTLSClient::Response MTLSClient::get(const std::string& endpoint, const std::string& path) {
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: beast::get_lowest_layer(stream).connect(results);
- Line 192: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: req.target(path);
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::read(stream, buffer, res);
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay));

### src/sharding/remote_executor.cpp
Total findings: 10

- Line 64: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: RemoteExecutor::Result RemoteExecutor::put(const ShardInfo& shard_info,
  Confidence: band=very_high; score=0.99
- Line 200: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: response = mtls_client_->put(endpoint, path, request_body);
  Confidence: band=very_high; score=0.99
- Line 53: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RemoteExecutor::Result RemoteExecutor::get(const ShardInfo& shard_info,
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RemoteExecutor::Result RemoteExecutor::executeQuery(const ShardInfo& shard_info,
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto b0 = data[i];
- Line 103: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto b1 = (i + 1 < size) ? data[i + 1] : std::uint8_t{0};
- Line 104: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto b2 = (i + 2 < size) ? data[i + 2] : std::uint8_t{0};
- Line 196: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: response = mtls_client_->get(endpoint, path);
  Confidence: band=very_high; score=0.9
- Line 102: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto b0 = data[i];
  Confidence: band=high; score=0.74
- Line 107: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: encoded += (i + 1 < size) ? kBase64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
  Confidence: band=high; score=0.74

### src/sharding/operational_metrics.cpp
Total findings: 9

- Line 302: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: {"conflicts", metrics->transaction_conflicts.load()}
  Confidence: band=very_high; score=0.99
- Line 538: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool had_conflict
  Confidence: band=very_high; score=0.99
- Line 48: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return it->second.get();
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, _] : shard_metrics_) {
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 375: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: unhealthy_count++;
- Line 439: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (metrics->min_latency_us.compare_exchange_weak(current_min, latency_us)) {
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (metrics->max_latency_us.compare_exchange_weak(current_max, latency_us)) {
  Confidence: band=high; score=0.74

### src/sharding/transaction_snapshot.cpp
Total findings: 8

- Line 357: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
- Line 394: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: snapshot = nullptr;
  Context: spdlog::error("Failed to delete snapshot {}: {}", snapshot_id, e.what());
- Line 290: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 312: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot_ids.push_back(snapshot_id);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot_ids.push_back(snapshot_id);
- Line 368: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 407: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t to_delete = snapshots.size() - max_snapshots_;

### src/sharding/transaction_wal.cpp
Total findings: 8

- Line 85: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["transaction_id"] = transaction_id;
- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["protocol"] = static_cast<int>(protocol);
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["participants"] = participants;
- Line 134: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["vote"] = vote;
- Line 135: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["response"] = response;
- Line 195: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["reason"] = reason;
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(txn_entry.value());
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(txn_entry.value());

### src/sharding/orphan_detector.cpp
Total findings: 7

- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: orphaned_txns.push_back(txn.transaction_id);
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: orphaned_txns.push_back(txn.transaction_id);
- Line 137: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Skip transactions that haven't yet hit the stale threshold.
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("OrphanDetector: Reclaiming stale Percolator lock for txn {} "
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("OrphanDetector: Stale Percolator lock reclaimed for txn {}",
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("OrphanDetector: Failed to abort stale Percolator txn {}",
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("OrphanDetector::cleanPercolatorLocks: reclaimed {} stale lock(s)",
  Confidence: band=high; score=0.74

### src/sharding/raft_state.cpp
Total findings: 6

- Line 271: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // 3. If an existing entry conflicts with a new one (same index but different terms),
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 225: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& vote : votes_received_) {
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (request.leader_commit > log_.getCommitIndex()) {
- Line 290: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: uint64_t new_commit = std::min(request.leader_commit, log_.getLastLogIndex());
- Line 318: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& vote : votes_received_) {
  Confidence: band=very_high; score=0.9

### src/sharding/gpu_erasure_coder.cpp
Total findings: 5

- Line 206: severity=CRITICAL; category=data_race; **status=FIXED** (batch2: stats_mutex_ added; all stats_ updates now happen under std::lock_guard<std::mutex> lk(stats_mutex_))
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = impl_->decode(available_chunks, missing_indices,
- Line 224: severity=CRITICAL; category=data_race; **status=FIXED** (batch2: same stats_mutex_ fix covers both encode/decode paths)
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = cpu_coder_->decode(available_chunks, missing_indices,
- Line 171: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result = cpu_coder_->encode(data, data_shards, parity_shards);
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(encode(block, data_shards, parity_shards));

### src/sharding/metadata_wal.cpp
Total findings: 5

- Line 61: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: LSN MetadataWAL::logPut(
  Confidence: band=very_high; score=0.99
- Line 96: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: LSN MetadataWAL::logUpdate(
  Confidence: band=very_high; score=0.99
- Line 129: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& wal_entry : wal_entries) {
  Confidence: band=very_high; score=0.9
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(MetadataWALEntry::fromWALEntry(wal_entry));
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(MetadataWALEntry::fromWALEntry(wal_entry));

### src/sharding/replication_coordinator.cpp
Total findings: 5

- Line 118: severity=CRITICAL; category=data_race; **status=FIXED** (batch3: PendingWrite::completed upgraded to std::atomic<bool>; all completed reads/writes now use atomic load/store under pending_mutex_ synchronization)
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != pending_writes_.end() && !it->second.completed) {
- Line 169: severity=CRITICAL; category=iterator_invalidation; **status=FIXED** (batch3: cleanupPendingWrites now snapshots keys first and erases by key after iteration, avoiding iterator invalidation risk)
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_writes_.begin();
- Line 83: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: result.replicas_acknowledged = it->second.ack_count.load(std::memory_order_acquire);
- Line 144: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return enabled_.load(std::memory_order_acquire);
- Line 163: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: size_t current_acks = write.ack_count.load(std::memory_order_acquire);

### src/sharding/paxos_wal.cpp
Total findings: 4

- Line 33: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // We use type 100+ for Paxos entries to avoid conflicts
  Confidence: band=very_high; score=0.99
- Line 202: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& wal_entry : wal_entries) {
  Confidence: band=very_high; score=0.9
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paxos_entries.push_back(std::move(paxos_entry));
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paxos_entries.push_back(std::move(paxos_entry));

### src/sharding/rebalance_operation.cpp
Total findings: 3

- Line 23: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Source and target shard IDs must not be empty");
- Line 27: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid token range");
- Line 85: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);

### src/sharding/secure_transport_client.cpp
Total findings: 3

- Line 204: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: request["data"] = data_base64;
- Line 244: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay));
- Line 264: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay));

### src/sharding/admin_operations.cpp
Total findings: 2

- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["shards"].push_back(shard_json);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["shards"].push_back(shard_json);

### src/sharding/replica_topology.cpp
Total findings: 2

- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replica_set.replicas.push_back(replica.get<std::string>());
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: replica_set.replicas.push_back(replica.get<std::string>());

### src/sharding/metrics_registry.cpp
Total findings: 1

- Line 19: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
