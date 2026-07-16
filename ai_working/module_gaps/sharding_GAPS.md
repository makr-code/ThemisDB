# sharding Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: sharding
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 1371
- Actionable Findings (Critical + High): 997
- Affected Files: 75

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 257 |
| High | 740 |
| Medium | 354 |
| Low | 20 |

## Category Summary

| Category | Count |
|---|---:|
| unspecified_consistency | 171 |
| missing_version_tracking | 86 |
| missing_consensus | 84 |
| resource_leaked_in_exception | 79 |
| undefined_conflict_resolution | 71 |
| uninitialized_access | 71 |
| stale_read_undocumented | 67 |
| copy_overhead | 54 |
| range_temporary | 54 |
| lock_contention | 42 |
| thread_join_no_timeout | 38 |
| db_connection_leak | 35 |
| map_vs_unordered_map | 35 |
| manual_cleanup | 29 |
| no_retry_logic | 28 |
| missing_correlation_id | 26 |
| data_race | 23 |
| explicit_delete | 22 |
| stale_doc_section_reference | 21 |
| duplicate_qualified_signature | 20 |
| pointer_arithmetic_unbounded | 20 |
| string_concat_loop | 20 |
| unordered_container_iter | 20 |
| delete_without_nullptr | 17 |
| unchecked_array_index | 16 |
| hardcoded_output | 15 |
| lock_in_loop | 12 |
| o_n_squared | 12 |
| unnecessary_copy | 12 |
| deadlock_risk | 11 |
| legacy_or_compat_path | 11 |
| missing_trace_point | 11 |
| delete_no_nullptr | 10 |
| no_timeout | 8 |
| arithmetic_overflow | 7 |
| nested_loop_find | 7 |
| null_dereference | 7 |
| uncaught_exception | 7 |
| missing_latency_metric | 6 |
| primitive_no_volatile | 6 |
| missing_move_constructor_defaulted | 5 |
| repeated_search | 5 |
| uninitialized_array | 5 |
| allocation_loop | 4 |
| generic_catch | 4 |
| hardcoded_path | 4 |
| iterator_invalidation | 4 |
| missing_override_keyword | 4 |
| shift_overflow | 4 |
| size_assumption | 4 |
| expensive_inner_op | 3 |
| missing_resource_limits | 3 |
| unstructured_log | 3 |
| blocking_no_timeout | 2 |
| fp_exact_comparison | 2 |
| module_doc_linkset_drift | 2 |
| multiplication_overflow | 2 |
| new_without_raii | 2 |
| pure_virtual_unimplemented | 2 |
| array_bounds | 1 |
| array_bounds_violation | 1 |
| coupling_risk_sharding_storage | 1 |
| crypto_weakness | 1 |
| exception_in_destructor | 1 |
| explicit_lock_unlock | 1 |
| memory_order | 1 |
| missing_dtor | 1 |
| missing_vector_reserve | 1 |
| pointer_without_null_check | 1 |
| random_unseeded | 1 |
| repeated_lookup | 1 |
| socket_leak | 1 |
| timestamp_sorting_unstable | 1 |
| unchecked_memcpy | 1 |
| uninitialized_pointer | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| sharding/redundancy_strategy.cpp | 115 | 20 | 53 | 41 | 1 |
| sharding/shard_router.cpp | 78 | 32 | 37 | 9 | 0 |
| sharding/cross_shard_transaction.cpp | 77 | 6 | 32 | 38 | 1 |
| sharding/cloud_backup.cpp | 69 | 0 | 43 | 26 | 0 |
| sharding/paxos_consensus.cpp | 45 | 5 | 30 | 10 | 0 |
| sharding/stream_protocol.cpp | 44 | 15 | 21 | 7 | 1 |
| sharding/signed_request.cpp | 42 | 4 | 33 | 5 | 0 |
| sharding/distributed_transaction.cpp | 39 | 3 | 30 | 6 | 0 |
| sharding/epoch_fencing.cpp | 37 | 1 | 31 | 5 | 0 |
| sharding/shard_rpc_client.cpp | 37 | 0 | 10 | 27 | 0 |
| sharding/gossip_config_manager.cpp | 35 | 17 | 14 | 4 | 0 |
| sharding/replica_consistency.cpp | 31 | 5 | 18 | 8 | 0 |
| sharding/pki_shard_certificate.cpp | 30 | 5 | 19 | 5 | 1 |
| sharding/gossip_protocol.cpp | 28 | 6 | 14 | 8 | 0 |
| sharding/mtls_connection_pool.cpp | 28 | 3 | 17 | 2 | 6 |
| sharding/two_phase_commit_coordinator.cpp | 27 | 1 | 23 | 3 | 0 |
| sharding/auto_rebalancer.cpp | 26 | 8 | 7 | 11 | 0 |
| sharding/slo_monitor.cpp | 24 | 0 | 13 | 11 | 0 |
| sharding/adaptive_shard_router.cpp | 22 | 3 | 15 | 4 | 0 |
| sharding/prometheus_metrics.cpp | 21 | 10 | 3 | 8 | 0 |
| sharding/cloud_agent.cpp | 20 | 6 | 10 | 4 | 0 |
| sharding/two_phase_commit_participant.cpp | 20 | 0 | 20 | 0 | 0 |
| sharding/hot_spare_manager.cpp | 19 | 6 | 12 | 1 | 0 |
| sharding/raft_consensus_adapter.cpp | 18 | 1 | 17 | 0 | 0 |
| sharding/wal_manager.cpp | 18 | 3 | 5 | 10 | 0 |
| sharding/raft_consensus.cpp | 17 | 5 | 10 | 2 | 0 |
| sharding/data_migrator.cpp | 16 | 8 | 5 | 3 | 0 |
| sharding/health_check.cpp | 16 | 4 | 8 | 4 | 0 |
| sharding/raft_log.cpp | 16 | 5 | 8 | 3 | 0 |
| sharding/gossip_consensus_adapter.cpp | 15 | 2 | 11 | 2 | 0 |
| sharding/metadata_shard.cpp | 15 | 7 | 5 | 3 | 0 |
| sharding/predictive_detector.cpp | 15 | 1 | 7 | 2 | 5 |
| sharding/shard_load_detector.cpp | 15 | 0 | 4 | 11 | 0 |
| sharding/shard_resource_manager.cpp | 14 | 4 | 5 | 5 | 0 |
| sharding/truetime.cpp | 14 | 3 | 8 | 3 | 0 |
| sharding/capability_matcher.cpp | 12 | 2 | 6 | 2 | 2 |
| sharding/health_monitor.cpp | 12 | 1 | 9 | 2 | 0 |
| sharding/shard_rpc_server.cpp | 12 | 2 | 7 | 3 | 0 |
| sharding/distributed_coordinator.cpp | 11 | 3 | 7 | 1 | 0 |
| sharding/hardware_migration_manager.cpp | 11 | 0 | 6 | 5 | 0 |
| sharding/paxos_snapshot.cpp | 11 | 1 | 7 | 3 | 0 |
| sharding/wal_shipper.cpp | 11 | 1 | 9 | 1 | 0 |
| sharding/raft_configuration.cpp | 10 | 0 | 10 | 0 | 0 |
| sharding/wal_applier.cpp | 10 | 6 | 2 | 2 | 0 |
| sharding/consistent_hash.cpp | 9 | 5 | 0 | 4 | 0 |
| sharding/locality_aware_router.cpp | 9 | 1 | 3 | 5 | 0 |
| sharding/mtls_client.cpp | 9 | 3 | 6 | 0 | 0 |
| sharding/partition_detector.cpp | 9 | 1 | 4 | 4 | 0 |
| sharding/admin_api.cpp | 8 | 0 | 5 | 3 | 0 |
| sharding/gpu_erasure_coder_opencl.cpp | 8 | 2 | 5 | 0 | 1 |
| sharding/raft_state.cpp | 8 | 2 | 6 | 0 | 0 |
| sharding/raft_wal_integration.cpp | 8 | 3 | 5 | 0 | 0 |
| sharding/shard_repair_engine.cpp | 8 | 2 | 5 | 1 | 0 |
| sharding/urn_resolver.cpp | 8 | 7 | 0 | 1 | 0 |
| sharding/multi_primary_coordinator.cpp | 7 | 6 | 1 | 0 | 0 |
| sharding/remote_executor.cpp | 7 | 2 | 3 | 2 | 0 |
| sharding/shard_durability.cpp | 7 | 0 | 3 | 4 | 0 |
| sharding/sharding_manager_edition.cpp | 7 | 0 | 5 | 2 | 0 |
| sharding/metadata_snapshot.cpp | 6 | 0 | 4 | 2 | 0 |
| sharding/orphan_detector.cpp | 6 | 0 | 0 | 6 | 0 |
| sharding/paxos_state_persistence.cpp | 6 | 1 | 4 | 1 | 0 |
| sharding/operational_metrics.cpp | 5 | 2 | 1 | 2 | 0 |
| sharding/transaction_snapshot.cpp | 5 | 0 | 4 | 1 | 0 |
| sharding/circuit_breaker.cpp | 4 | 0 | 4 | 0 | 0 |
| sharding/metadata_wal.cpp | 4 | 2 | 1 | 1 | 0 |
| sharding/replication_coordinator.cpp | 4 | 1 | 3 | 0 | 0 |
| sharding/shard_topology.cpp | 4 | 0 | 2 | 2 | 0 |
| sharding/raft_shard_manager.cpp | 3 | 1 | 0 | 2 | 0 |
| sharding/quorum_manager.cpp | 2 | 0 | 2 | 0 | 0 |
| sharding/secure_transport_client.cpp | 2 | 0 | 2 | 0 | 0 |
| sharding/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| sharding/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| sharding/gpu_erasure_coder.cpp | 1 | 0 | 1 | 0 | 0 |
| sharding/paxos_wal.cpp | 1 | 1 | 0 | 0 | 0 |
| sharding/transaction_wal.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### sharding/redundancy_strategy.cpp
Total findings: 115

- Line 356: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: recovered.insert(recovered.end(), chunk.begin(), chunk.end());
- Line 415: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
- Line 660: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        std::vector<uint8_t> chunk(chunk_size, 0);  // Pad with zeros', '        if (offset < data.size()) {', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(chunk);']
- Line 725: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: recovered.insert(recovered.end(), chunk.begin(), chunk.end());
- Line 796: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
- Line 968: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
- Line 1083: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), shards[s].begin(), shards[s].end());
- Line 1164: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
- Line 1266: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), shards[s].begin(), shards[s].end());
- Line 1332: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: WriteResult RedundancyStrategy::write(
- Line 1332: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: WriteResult RedundancyStrategy::write(
- Line 1639: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool RedundancyStrategy::proposeRaftWrite(const std::string& shard_id,
- Line 2115: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: candidates.insert(candidates.end(), replicas.begin(), replicas.end());
- Line 2295: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: available_shards.insert(available_shards.end(), replicas.begin(), replicas.end());
- Line 2352: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge chunks
- Line 2353: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto merged = mergeChunks(chunks);
- Line 2356: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.data = std::string(merged.begin(), merged.end());
- Line 2370: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // concurrent configure() resetting erasure_coder_ (data race fix).
- Line 2754: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: all_shards.insert(all_shards.end(), replicas.begin(), replicas.end());
- Line 2821: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: shards.insert(shards.end(), replicas2.begin(), replicas2.end());
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #1285 GEO_MIRROR: Configurable ge... (2026-03-11) | #1247 Implement Raft cons
- Line 295: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: [') {', '    // Calculate chunk size (pad last chunk with zeros if needed)', '    size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '', '    // Split data into k chunks (data shards)']
- Line 393: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (!invertMatrix(decode_matrix)) {

        throw std::runtime_error("Failed to invert decode matrix for Reed-Solomon recovery");

    }



    // Apply inverse matrix byte-by-byte to recover original data chunks
- Line 408: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<uint8_t> recovered_bytes;

        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);

        for (size_t i = 0; i < data_shards; ++i) {

            recovered_data[i][x] = recovered_bytes[i];

        }

    }
- Line 651: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    // Calculate chunk size', '    size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '', '    // Split data into chunks']
- Line 685: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                parity_byte ^= gf_mul(cauchy_matrix[p][d], data_bytes[d]);', '            }', '            parity[byte_pos] = parity_byte;', '        }', '']
- Line 789: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Store recovered bytes

        for (size_t i = 0; i < data_shards; ++i) {

            recovered_data[i][byte_pos] = recovered_bytes[i];

        }

    }
- Line 1327: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1335: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const std::string& collection [[maybe_unused]],
- Line 1407: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ReadResult RedundancyStrategy::read(
- Line 1409: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const std::string& collection [[maybe_unused]],
- Line 1474: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ShardTopology& topology [[maybe_unused]],
- Line 1574: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (futures[i].get()) {
- Line 1717: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool committed = future.get();
- Line 1736: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ShardTopology& topology [[maybe_unused]],
- Line 1771: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (futures[i].get()) {
- Line 1822: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ShardTopology& topology [[maybe_unused]],
- Line 1875: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (futures[i].get()) {
- Line 1933: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = region_candidates.find(geo.local_region);
- Line 2018: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (futures[i].get()) {
- Line 2323: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ShardTopology& topology [[maybe_unused]],
- Line 2352: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge chunks
- Line 2353: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto merged = mergeChunks(chunks);
- Line 2356: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.data = std::string(merged.begin(), merged.end());
- Line 2465: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<uint8_t> RedundancyStrategy::mergeChunks(
- Line 2468: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<uint8_t> merged;
- Line 2471: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.insert(merged.end(), chunk.begin(), chunk.end());
- Line 2474: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 2511: severity=HIGH; category=crypto_weakness
  Description: weak_rng_rand_family: Weak RNG — use OpenSSL RAND or std::random_device
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: size_t idx = std::rand() % available_shards.size();
- Line 2517: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 2519: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 2524: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 2526: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 2530: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 2615: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const std::string& collection [[maybe_unused]],
- Line 2634: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Build the list of shard IDs and the doc-keys to delete
- Line 2702: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    // Delete from all targets in parallel (send a "write" with empty payload

    // — the WriteHandler interprets an empty payload as a delete command)

    const std::vector<uint8_t> empty_payload;

    std::vector<std::future<bool>> futures;

    futures.reserve(targets.size());
- Line 2702: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // — the WriteHandler interprets an empty payload as a delete command)
- Line 2716: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (futures[i].get()) {
- Line 2719: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::warn("remove: delete from shard {} failed for doc {}",
- Line 2719: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (futures[i].get()) {

                ++successes;

            } else {

                spdlog::warn("remove: delete from shard {} failed for doc {}",

                             targets[i].first, document_id);

            }

        } catch (const std::exception& e) {
- Line 2719: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::warn("remove: delete from shard {} failed for doc {}",
- Line 2723: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::warn("remove: delete from shard {} threw: {}",
- Line 2723: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: targets[i].first, document_id);

            }

        } catch (const std::exception& e) {

            spdlog::warn("remove: delete from shard {} threw: {}",

                         targets[i].first, e.what());

        }

    }
- Line 2723: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::warn("remove: delete from shard {} threw: {}",
- Line 2728: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }



    // Succeed if at least one replica was deleted (soft-delete semantics)

    return successes > 0;

}
- Line 2728: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Succeed if at least one replica was deleted (soft-delete semantics)
- Line 2734: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const std::string& collection [[maybe_unused]],
- Line 2813: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            m = config_.erasure_coding.parity_shards;', '        }', '        const uint32_t total = k + m;', '', '        // Read all available chunks']
- Line 2896: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const std::string& collection [[maybe_unused]],
- Line 2944: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        const uint32_t k = config_.erasure_coding.data_shards;', '        const uint32_t m = config_.erasure_coding.parity_shards;', '        const uint32_t total = k + m;', '        auto replicas = ring.getReplicaNodes(document_id, total - 1);', '        std::vector<std::string> shards{*primary_opt};']
- Line 3201: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3233: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility shim: expose under themisdb::sharding
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: missing.push_back(static_cast<uint32_t>(i));
- Line 251: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
- Line 328: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
- Line 420: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
- Line 434: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
- Line 446: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
- Line 450: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
- Line 479: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
- Line 499: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
- Line 582: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
- Line 695: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
- Line 1041: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (recovered[global_start + p]) avail_rows.push_back(data_shards + p);
- Line 1274: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
- Line 1485: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: target_shards.push_back(*primary_shard);
- Line 1530: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool ok = false;
- Line 1567: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto status = futures[i].wait_for(wait_timeout);
- Line 1569: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: failed_shards.push_back(target_shards[i]);
- Line 1570: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: failed_shards.push_back(target_shards[i]);
- Line 1575: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: written_shards.push_back(target_shards[i]);
- Line 1578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: failed_shards.push_back(target_shards[i]);
- Line 1582: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: failed_shards.push_back(target_shards[i]);
- Line 1749: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: target_shards.push_back(*primary_shard);
- Line 1772: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: written_shards.push_back(target_shards[i]);
- Line 1876: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: written_shards.push_back(target_shards[i]);
- Line 1913: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidates.push_back(*primary_opt);
- Line 1916: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string> write_failed_set(
- Line 1925: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> region_candidates;
- Line 2012: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto status = futures[i].wait_for(config_.replication_timeout);
- Line 2019: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: written_shards.push_back(target_shards[i]);
- Line 2033: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
- Line 2144: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> failed_set;
- Line 2167: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, uint32_t> region_reads;
- Line 2255: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Bounded-staleness / follower-read fallback: try remaining candidates
- Line 2292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: available_shards.push_back(*primary_shard);
- Line 2511: severity=MEDIUM; category=random_unseeded
  Description: rand() without nearby explicit srand() seeding
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: size_t idx = std::rand() % available_shards.size();
- Line 2579: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
- Line 2684: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string> failed_set(failed_regions.begin(),
- Line 3143: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Bounded-staleness limit
- Line 3144: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ss << "# HELP themis_geo_max_staleness_ms Maximum accepted replication staleness in ms\n";
- Line 3145: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ss << "# TYPE themis_geo_max_staleness_ms gauge\n";
- Line 3146: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ss << "themis_geo_max_staleness_ms{mode=\"geo_mirror\"} " << geo.max_staleness_ms << "\n";
- Line 1642: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // W2-S02: Input validation guards — fail-closed on invalid inputs

### sharding/shard_router.cpp
Total findings: 78

- Line 222: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeResults(results);
- Line 230: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeResults(results);
- Line 237: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeResults(results);
- Line 641: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Emit one merged row per matching left-side entry.
- Line 643: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged = nlohmann::json::object();
- Line 645: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["left_" + k] = v;
- Line 649: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (!merged.contains(rk)) merged[rk] = v;
- Line 651: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: joined_rows.push_back(std::move(merged));
- Line 682: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Execute join locally on each shard and merge results
- Line 690: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Phase 2: Merge results from all shards
- Line 691: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged = mergeResults(results);
- Line 696: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: {"data", merged}
- Line 769: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: exec_result = executor_->put(*shard_info, path, *body);
- Line 969: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
- Line 970: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // W2-S07: Merge strategy for distributed query results
- Line 971: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Conflict resolution: Last-write-wins based on shard response order
- Line 976: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged;
- Line 977: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"] = nlohmann::json::array();
- Line 978: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["errors"] = nlohmann::json::array();
- Line 979: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["shard_count"] = results.size();
- Line 987: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // If result has data array, merge it
- Line 990: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"].push_back(item);
- Line 994: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"].push_back(item);
- Line 998: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"].push_back(result.data);
- Line 1001: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["errors"].push_back(nlohmann::json{
- Line 1008: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["success_count"] = success_count;
- Line 1009: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["error_count"] = results.size() - success_count;
- Line 1011: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 1015: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const nlohmann::json& merged,
- Line 1019: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json paginated = merged;
- Line 1021: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (merged.contains("results") && merged["results"].is_array()) {
- Line 1022: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const auto& results = merged["results"];
- Line 109: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<nlohmann::json> ShardRouter::get(
- Line 182: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json ShardRouter::executeQuery(const std::string& query) {
- Line 222: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeResults(results);
- Line 230: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeResults(results);
- Line 237: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeResults(results);
- Line 337: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto exec_result = executor_->executeQuery(shard, query);
- Line 370: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: results.push_back(futures[i].get());
- Line 476: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto exec_result = executor_->executeQuery(shard, query);
- Line 500: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: results.push_back(futures[i].get());
- Line 639: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = hash_table.find(key);
- Line 641: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Emit one merged row per matching left-side entry.
- Line 643: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged = nlohmann::json::object();
- Line 645: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["left_" + k] = v;
- Line 649: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (!merged.contains(rk)) merged[rk] = v;
- Line 651: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: joined_rows.push_back(std::move(merged));
- Line 682: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Execute join locally on each shard and merge results
- Line 690: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Phase 2: Merge results from all shards
- Line 691: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged = mergeResults(results);
- Line 696: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: {"data", merged}
- Line 767: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: exec_result = executor_->get(*shard_info, path);
- Line 876: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Execute query (simplified - return empty results)
- Line 976: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged;
- Line 977: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"] = nlohmann::json::array();
- Line 978: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["errors"] = nlohmann::json::array();
- Line 979: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["shard_count"] = results.size();
- Line 987: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // If result has data array, merge it
- Line 990: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"].push_back(item);
- Line 994: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"].push_back(item);
- Line 998: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["results"].push_back(result.data);
- Line 1001: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["errors"].push_back(nlohmann::json{
- Line 1008: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["success_count"] = success_count;
- Line 1009: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged["error_count"] = results.size() - success_count;
- Line 1011: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 1015: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const nlohmann::json& merged,
- Line 1019: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json paginated = merged;
- Line 1021: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (merged.contains("results") && merged["results"].is_array()) {
- Line 1022: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const auto& results = merged["results"];
- Line 36: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: static std::map<std::string, std::string> parseQueryParams(const std::string& path) {
- Line 37: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> params;
- Line 118: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Default: Read from primary shard (eventual consistency)
- Line 182: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json ShardRouter::executeQuery(const std::string& query) {
- Line 244: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: RoutingStrategy ShardRouter::analyzeQuery(const std::string& query) const {
- Line 367: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto status = futures[i].wait_for(timeout);
- Line 417: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, ShardInfo> shard_map;
- Line 568: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
- Line 972: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Consistency model: Read committed (eventual) from multiple shards

### sharding/cross_shard_transaction.cpp
Total findings: 77

- Line 286: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return terminal_decisions_in_progress_.insert(transaction_id).second;
- Line 441: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: deadlock_detection_thread_.join();
- Line 759: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // reference: a concurrent abort() could erase the map entry while we are
- Line 873: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // reference: a concurrent commit() could erase the map entry while we are
- Line 2058: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Only merge remote edges that reference known live
- Line 2783: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats->pending_transactions =
- Line 123: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& cycle : collectCycles(graph)) {
- Line 306: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: deadlock_detection_thread_ = std::thread(
- Line 467: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 721: severity=HIGH; category=missing_trace_point
  Description: Critical function commit without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool CrossShardTransactionCoordinator::commit(const std::string& transaction_id) {
- Line 828: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool CrossShardTransactionCoordinator::abort(const std::string& transaction_id) {
- Line 1262: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"participants", nlohmann::json::array()}

            };

            for (const auto& [shard_id, participant] : txn.participants) {

                commit_data["participants"].push_back(shard_id);

            }

            transaction_wal_->logCommit(txn.transaction_id, commit_data);

            operations_since_snapshot_++;
- Line 1281: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute2PC [{}]: WAL ABORT log failed during fail-closed handling: {}",
- Line 1295: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute2PC [{}]: fail-closed abort RPC failed for shard {}",
- Line 1305: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute2PC [{}]: WAL ABORTED log failed for shard {}: {}",
- Line 1319: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute2PC [{}]: Commit failed for shard {} - failing closed",
- Line 1334: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute2PC [{}]: WAL COMMITTED log failed for shard {}: {} - failing closed",
- Line 1379: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute3PC [{}]: missing PreCommit RPC callback; failing closed",
- Line 1386: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute3PC [{}]: fail-closed abort RPC failed for shard {}",
- Line 1396: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("execute3PC [{}]: failed to log fail-closed ABORT to WAL: {}",
- Line 1415: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"participants", nlohmann::json::array()}

            };

            for (const auto& [shard_id, participant] : txn.participants) {

                precommit_data["participants"].push_back(shard_id);

            }

            transaction_wal_->logCommit(txn.transaction_id, precommit_data);

            operations_since_snapshot_++;
- Line 1486: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"participants", nlohmann::json::array()}

            };

            for (const auto& [shard_id, participant] : txn.participants) {

                commit_data["participants"].push_back(shard_id);

            }

            transaction_wal_->logCommit(txn.transaction_id, commit_data);

            operations_since_snapshot_++;
- Line 1688: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Calvin: failed to acquire lock on shard {} for transaction {}",
- Line 1705: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::info("Calvin transaction {}: all locks acquired, proceeding to execution phase",
- Line 1833: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 1901: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: while (retries <= rpc_config.max_retries) {

            try {

                bool success = rpc_client.commit(transaction_id, commit_timestamp);

                if (success) {

                    spdlog::info("Shard {} committed transaction {} at timestamp {}", 

                               shard_id, transaction_id, commit_timestamp);
- Line 1916: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 1989: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 2010: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void CrossShardTransactionCoordinator::deadlockDetectionThread() {
- Line 2058: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Only merge remote edges that reference known live
- Line 2126: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = transactions_.find(txn_id);
- Line 2127: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = transactions_.find(txn_id);
- Line 2289: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::timed_mutex> wal_lock(transactions_mutex_);
- Line 2963: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto acquire_lock = [&](const std::string& shard_id) -> bool {
- Line 3000: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool locked = acquire_lock(shard_id);
- Line 3019: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool primary_locked = acquire_lock(primary_shard_id);
- Line 3039: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: {"locks_acquired", locked_shards}
- Line 3047: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::info("[Percolator] All locks acquired for txn {}", txn.transaction_id);
- Line 45: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_set<std::string>> collectCycles(
- Line 46: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::vector<std::string>>& graph
- Line 48: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_set<std::string>> cycles;
- Line 49: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> index;
- Line 50: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> lowlink;
- Line 52: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> on_stack;
- Line 106: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string>(component.begin(), component.end()));
- Line 119: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> collectCycleNodes(
- Line 122: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> cycle_nodes;
- Line 228: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
- Line 235: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: recovery_result.details.stale_transactions_detected,
- Line 340: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={}); "
- Line 349: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: recovery_result.details.stale_transactions_detected,
- Line 360: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
- Line 369: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: recovery_result.details.stale_transactions_detected,
- Line 417: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
- Line 424: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.details.stale_transactions_detected,
- Line 1546: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: perc_cfg.stale_lock_threshold = std::chrono::seconds(30);
- Line 2173: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>>
- Line 2175: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> graph;
- Line 2231: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::vector<std::string>>& graph,
- Line 2469: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: log_file.close();
- Line 2703: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::warn("Transaction {} is stale (age: {}s), will abort", txn_id, age.count());
- Line 2731: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Final states - can be cleaned up eventually
- Line 2752: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("Aborting {} stale transactions", transactions_to_timeout.size());
- Line 2764: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::error("Failed to abort stale transaction {}: {}", txn_id, e.what());
- Line 2781: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats->stale_transactions_detected = transactions_to_timeout.size();
- Line 2784: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats->stale_transactions_detected + stats->resume_candidates;
- Line 3123: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: size_t PercolatorCoordinator::cleanStaleLocks(
- Line 3124: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::vector<std::string>& stale_txn_ids,
- Line 3129: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& txn_id : stale_txn_ids) {
- Line 3148: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (age < config_.stale_lock_threshold) {
- Line 3152: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("[Percolator] Cleaning stale lock for txn {} (age {}s)",
- Line 3159: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: wal_->logAbort(txn_id, "stale_lock_cleanup");
- Line 3167: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("[Percolator] Stale lock cleaned for txn {}", txn_id);
- Line 3169: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::warn("[Percolator] Failed to clean stale lock for txn {}", txn_id);
- Line 3173: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("[Percolator] cleanStaleLocks: cleaned {} / {} stale locks",
- Line 3174: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cleaned, stale_txn_ids.size());
- Line 538: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // W2-S04: Fail-closed on empty inputs

### sharding/cloud_backup.cpp
Total findings: 69

- Line 209: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("S3 delete callback failed: {}", e.what());
- Line 209: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

                return fn(bucket_, remote_path);

            } catch (const std::exception& e) {

                THEMIS_ERROR("S3 delete callback failed: {}", e.what());

                return false;

            }

        }
- Line 209: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("S3 delete callback failed: {}", e.what());
- Line 215: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        // STUB/SIMULATION NOTE (stub #313):

        // Purpose: Keep cloud-backup deletion API callable before S3 SDK delete wiring

        //          is integrated in this provider path.

        // Activation: S3 provider active without injected SDK-backed delete callback.

        // Production Delta: Delete logs a placeholder action and returns false,
- Line 215: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Purpose: Keep cloud-backup deletion API callable before S3 SDK delete wiring
- Line 217: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // STUB/SIMULATION NOTE (stub #313):

        // Purpose: Keep cloud-backup deletion API callable before S3 SDK delete wiring

        //          is integrated in this provider path.

        // Activation: S3 provider active without injected SDK-backed delete callback.

        // Production Delta: Delete logs a placeholder action and returns false,

        //                   so remote backup objects

        //                   are not actually removed from S3-compatible storage.
- Line 217: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Activation: S3 provider active without injected SDK-backed delete callback.
- Line 221: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Production Delta: Delete logs a placeholder action and returns false,

        //                   so remote backup objects

        //                   are not actually removed from S3-compatible storage.

        // Removal Plan: Integrate Aws::S3::DeleteObject (or injected delete bridge)

        //               for production deletion behavior.

        //               See src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.

        //               Target: v2.3.0.
- Line 221: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Removal Plan: Integrate Aws::S3::DeleteObject (or injected delete bridge)
- Line 225: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_INFO("S3 delete (placeholder): s3://{}/{}", bucket_, remote_path);
- Line 230: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("S3 delete failed: AWS SDK not integrated");
- Line 230: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Fail closed: placeholder providers must never report success without

        // a real SDK-backed callback wired via setS3DeleteFn().

        THEMIS_ERROR("S3 delete failed: AWS SDK not integrated");

        return false;

    }
- Line 230: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("S3 delete failed: AWS SDK not integrated");
- Line 403: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Azure delete callback failed: {}", e.what());
- Line 403: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

                return fn(account_name_, container_, remote_path);

            } catch (const std::exception& e) {

                THEMIS_ERROR("Azure delete callback failed: {}", e.what());

                return false;

            }

        }
- Line 403: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Azure delete callback failed: {}", e.what());
- Line 408: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_INFO("Azure delete (placeholder): {}/{}/{}", account_name_, container_, remote_path);
- Line 413: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Azure delete failed: Azure SDK not integrated");
- Line 413: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Fail closed: placeholder providers must never report success without

        // a real SDK-backed callback wired via setAzureDeleteFn().

        THEMIS_ERROR("Azure delete failed: Azure SDK not integrated");

        return false;

    }
- Line 413: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Azure delete failed: Azure SDK not integrated");
- Line 433: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Purpose: Preserve Azure provider list API compatibility before SDK-backed
- Line 591: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("GCS delete callback failed: {}", e.what());
- Line 591: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

                return fn(bucket_, remote_path);

            } catch (const std::exception& e) {

                THEMIS_ERROR("GCS delete callback failed: {}", e.what());

                return false;

            }

        }
- Line 591: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("GCS delete callback failed: {}", e.what());
- Line 597: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        // STUB/SIMULATION NOTE (GCSStorageProvider::deleteObject):

        // Purpose: Placeholder delete path inside the GCS stub class (same stub block

        //          documented at the class-level STUB/SIMULATION NOTE above).

        // Activation: THEMIS_ENABLE_GCS not defined; same condition as upload/download().

        // Production Delta: Returns false without contacting GCS.
- Line 597: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Purpose: Placeholder delete path inside the GCS stub class (same stub block
- Line 604: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_INFO("GCS delete (placeholder): gs://{}/{}", bucket_, remote_path);
- Line 609: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("GCS delete failed: GCS SDK not integrated");
- Line 609: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Fail closed: placeholder providers must never report success without

        // a real SDK-backed callback wired via setGCSDeleteFn().

        THEMIS_ERROR("GCS delete failed: GCS SDK not integrated");

        return false;

    }
- Line 609: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("GCS delete failed: GCS SDK not integrated");
- Line 749: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Create backup metadata

                nlohmann::json metadata;

                metadata["backup_id"] = backup_id;

                metadata["shard_id"] = shard_id;

                metadata["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);

                metadata["version"] = "1.0";
- Line 750: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Create backup metadata

                nlohmann::json metadata;

                metadata["backup_id"] = backup_id;

                metadata["shard_id"] = shard_id;

                metadata["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);

                metadata["version"] = "1.0";
- Line 751: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: nlohmann::json metadata;

                metadata["backup_id"] = backup_id;

                metadata["shard_id"] = shard_id;

                metadata["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);

                metadata["version"] = "1.0";

                

                // Save metadata
- Line 752: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["backup_id"] = backup_id;

                metadata["shard_id"] = shard_id;

                metadata["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);

                metadata["version"] = "1.0";

                

                // Save metadata

                std::string metadata_path = local_backup_dir + "/" + shard_id + ".json";
- Line 764: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string remote_path = config_.backup_prefix + "/" + backup_id + "/" + shard_id;

                

                std::map<std::string, std::string> upload_metadata;

                upload_metadata["backup_id"] = backup_id;

                upload_metadata["shard_id"] = shard_id;

                

                if (storage_provider_) {
- Line 765: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::map<std::string, std::string> upload_metadata;

                upload_metadata["backup_id"] = backup_id;

                upload_metadata["shard_id"] = shard_id;

                

                if (storage_provider_) {

                    bool uploaded = storage_provider_->upload(snapshot_path,
- Line 838: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(it->second.shard_ids.begin(), it->second.shard_ids.end(), shard_id)
- Line 894: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Failed to delete backup object from provider: {}", remote_path);
- Line 894: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto& shard_id : it->second.shard_ids) {

            std::string remote_path = config_.backup_prefix + "/" + backup_id + "/" + shard_id;

            if (!storage_provider_->deleteObject(remote_path)) {

                THEMIS_ERROR("Failed to delete backup object from provider: {}", remote_path);

                return false;

            }

        }
- Line 894: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Failed to delete backup object from provider: {}", remote_path);
- Line 930: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool setReplicationTarget(const std::string& datacenter_id,
- Line 1097: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool CloudBackupCoordinator::setReplicationTarget(const std::string& datacenter_id,
- Line 1099: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return impl_->setReplicationTarget(datacenter_id, shard_endpoints);
- Line 14: severity=MEDIUM; category=coupling_risk_sharding_storage
  Description: Potential coupling risk between sharding/ and storage/ (validate no circular dependency)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_architecture_rules
  Context: #include "storage/backup_manager.h"
- Line 62: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 66: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 69: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 69: severity=MEDIUM; category=pure_virtual_unimplemented
  Description: Ensure all derived classes provide implementation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 73: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 73: severity=MEDIUM; category=pure_virtual_unimplemented
  Description: Ensure all derived classes provide implementation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 88: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 183: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 223: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 225: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("S3 delete (placeholder): s3://{}/{}", bucket_, remote_path);
- Line 257: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 287: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 313: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 350: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 440: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 469: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 493: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 530: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 570: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 603: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 604: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("GCS delete (placeholder): gs://{}/{}", bucket_, remote_path);
- Line 636: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 665: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Storage.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Cloud Storage.
- Line 758: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: metadata_file.close();
- Line 913: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp (newest first)

### sharding/paxos_consensus.cpp
Total findings: 45

- Line 115: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (proposer_thread_.joinable()) proposer_thread_.join();
- Line 116: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (acceptor_thread_.joinable()) acceptor_thread_.join();
- Line 117: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (learner_thread_.joinable()) learner_thread_.join();
- Line 118: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (election_thread_.joinable()) election_thread_.join();
- Line 205: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = committed_log_.find(i);
- Line 77: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy: Load persistent state from JSON if available
- Line 95: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: proposer_thread_ = std::thread(&PaxosConsensus::runProposer, this);
- Line 96: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: acceptor_thread_ = std::thread(&PaxosConsensus::runAcceptor, this);
- Line 97: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: learner_thread_ = std::thread(&PaxosConsensus::runLearner, this);
- Line 98: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: election_thread_ = std::thread(&PaxosConsensus::leaderElectionThread, this);
- Line 178: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::timed_mutex> lock(proposal_mutex_);
- Line 189: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 300: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: uint64_t restored_index = 0;

    uint64_t restored_term  = 0;

    if (snapshot_data.contains("_snapshot_index"))

        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();

    if (snapshot_data.contains("_snapshot_term"))

        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();
- Line 302: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (snapshot_data.contains("_snapshot_index"))

        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();

    if (snapshot_data.contains("_snapshot_term"))

        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();



    {

        std::lock_guard<std::mutex> lock(state_mutex_);
- Line 410: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::timed_mutex> lock(proposal_mutex_);
- Line 413: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: proposal_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
- Line 436: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << retry)));
- Line 477: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 486: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: committed_log_.find(slot) == committed_log_.end()) {
- Line 487: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            if (instance.is_committed &&', '                committed_log_.find(slot) == committed_log_.end()) {', '                committed_log_[slot] = instance.accepted_value;', '                spdlog::debug("Acceptor: applied committed slot {} to committed_log_", slot);', '            }']
- Line 513: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 516: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 520: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (instance.is_committed && committed_log_.find(slot) == committed_log_.end()) {
- Line 533: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PaxosConsensus::leaderElectionThread() {
- Line 542: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(500));
- Line 626: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 629: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '        // Get or create instance for this slot', '        auto& instance = instances_[slot];', '        instance.slot = slot;', '        instance.prepare_promises.clear();']
- Line 721: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: } // state_mutex_ released here — executeAcceptPhase() re-acquires it safely
- Line 768: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        std::lock_guard<std::mutex> lock(state_mutex_);', '', '        auto& instance = instances_[slot];', '        instance.accept_acks.clear();', '']
- Line 780: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    if (handleAccept(slot, proposal, value)) {', '        std::lock_guard<std::mutex> lock(state_mutex_);', '        instances_[slot].accept_acks.insert(node_id_);', '    }', '']
- Line 789: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(state_mutex_);
- Line 790: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    {', '        std::lock_guard<std::mutex> lock(state_mutex_);', '        auto& instance = instances_[slot];', '', '        for (const auto& node : cluster_nodes_) {']
- Line 948: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            for (const auto& [slot_str, instance_json] : state_json["instances"].items()) {', '                uint64_t slot = std::stoull(slot_str);', '                PaxosInstance& instance = instances_[slot];', '', '                instance.slot = slot;']
- Line 1108: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    // Get or create instance for this slot', '    auto& instance = instances_[slot];', '', '    // Phase 1b: Promise not to accept proposals with lower ballot']
- Line 1145: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    // Get or create instance for this slot', '    auto& instance = instances_[slot];', '', "    // Phase 2b: Accept proposal if it's >= our promised proposal"]
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(it->second);
- Line 369: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 428: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool success = false;
- Line 429: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const int max_retries = 3;
- Line 431: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int retry = 0; retry < max_retries && running_.load(); ++retry) {
- Line 474: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // are now stale (i.e. the proposer timed out and a higher ballot has been
- Line 491: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Evict stale promises: if the promised round is far behind the
- Line 494: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: constexpr uint64_t kStaleRoundThreshold = 10;
- Line 497: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cur_round > instance.promised_proposal.round + kStaleRoundThreshold) {
- Line 498: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::debug("Acceptor: evicting stale promise for slot {} "

### sharding/stream_protocol.cpp
Total findings: 44

- Line 244: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), data.begin(), data.end());
- Line 654: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: session_thread_.join();
- Line 657: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: heartbeat_thread_.join();
- Line 901: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: executor_thread_.join();
- Line 942: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Start sessions (up to max_concurrent)
- Line 951: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while (active_count < config_.max_concurrent_sessions &&
- Line 1012: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: transfer_thread_.join();
- Line 1187: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        return false;', '    }', '    if (chunk.data.size() > 1024 * 1024 * 1024) {  // 1GB max chunk', '        spdlog::error("Chunk data exceeds maximum size ({})", chunk.data.size());', '        return false;']
- Line 1203: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.write(reinterpret_cast<const char*>(&chunk.chunk_index), sizeof(chunk.chunk_index));
- Line 1204: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.write(reinterpret_cast<const char*>(&chunk.file_offset), sizeof(chunk.file_offset));
- Line 1205: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.write(reinterpret_cast<const char*>(&chunk.uncompressed_size), sizeof(chunk.uncompressed_size));
- Line 1206: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.write(reinterpret_cast<const char*>(&chunk.compressed_size), sizeof(chunk.compressed_size));
- Line 1207: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.write(reinterpret_cast<const char*>(&chunk.checksum), sizeof(chunk.checksum));
- Line 1208: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.write(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());
- Line 1442: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.write(reinterpret_cast<const char*>(write_data.data()), write_data.size());
- Line 504: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::chrono::milliseconds StreamRateLimiter::acquire(size_t bytes) {
- Line 616: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: session_thread_ = std::thread(&StreamSession::sessionLoop, this);
- Line 617: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: heartbeat_thread_ = std::thread(&StreamSession::heartbeatLoop, this);
- Line 636: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void StreamSession::abort(const std::string& reason) {
- Line 727: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 771: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 780: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 781: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS), [this] {
- Line 793: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 877: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool StreamPlan::execute() {
- Line 882: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: executor_thread_ = std::thread(&StreamPlan::executorLoop, this);
- Line 886: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void StreamPlan::abort() {
- Line 948: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 950: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Start new sessions if we have capacity
- Line 984: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 1018: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: transfer_thread_ = std::thread(&StreamTransferTask::transferLoop, this);
- Line 1031: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void StreamTransferTask::abort() {
- Line 1063: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 1099: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 1128: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(chunk.data.data()), chunk.uncompressed_size);
- Line 1356: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void StreamReceiveTask::abort() {
- Line 141: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    // flags (big-endian)', '    result[pos++] = (flags >> 24) & 0xFF;', '    result[pos++] = (flags >> 16) & 0xFF;', '    result[pos++] = (flags >> 8) & 0xFF;']
- Line 142: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // flags (big-endian)', '    result[pos++] = (flags >> 24) & 0xFF;', '    result[pos++] = (flags >> 16) & 0xFF;', '    result[pos++] = (flags >> 8) & 0xFF;', '    result[pos++] = flags & 0xFF;']
- Line 143: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    result[pos++] = (flags >> 24) & 0xFF;', '    result[pos++] = (flags >> 16) & 0xFF;', '    result[pos++] = (flags >> 8) & 0xFF;', '    result[pos++] = flags & 0xFF;', '']
- Line 598: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stream Protocol PrepareTransfer.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §Stream Protocol PrepareTransfer.
- Line 877: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool StreamPlan::execute() {
- Line 922: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: progress.push_back(session->getProgress());
- Line 1298: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::cerr << "Rejecting stale or duplicate chunk " << chunk.chunk_index
- Line 250: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // W2-S03: Chunk metadata validation - fail-closed on malformed inputs

### sharding/signed_request.cpp
Total findings: 42

- Line 93: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int written = BIO_write(bio.get(), data, static_cast<int>(len));
- Line 105: severity=CRITICAL; category=uninitialized_pointer
  Description: Undefined behavior: potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer declared but not initialized
- Line 309: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
- Line 575: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
- Line 47: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::warn("SignedRequestVerifier reject [{}]: {}", code, details);
- Line 85: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 87: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 93: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int written = BIO_write(bio.get(), data, static_cast<int>(len));
- Line 99: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int flush_result = BIO_flush(bio.get());
- Line 106: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: BIO_get_mem_ptr(bio.get(), &buffer_ptr);
- Line 108: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!buffer_ptr || !buffer_ptr->data) {
- Line 113: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string result(buffer_ptr->data, buffer_ptr->length);
- Line 113: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 120: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 122: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 128: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int decoded_len = BIO_read(bio.get(), decoded.data(), static_cast<int>(decoded.size()));
- Line 228: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.shard_id = config_.shard_id;

    

    // Set timestamp

    request.timestamp_ms = getCurrentTimestampMs();

    

    // Generate nonce

    request.nonce = generateNonce();
- Line 231: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.timestamp_ms = getCurrentTimestampMs();

    

    // Generate nonce

    request.nonce = generateNonce();

    

    // Set certificate serial

    request.cert_serial = cert_serial_;
- Line 239: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.signature_format = SignedRequest::kSignatureFormatV1;

    

    // Create canonical string

    std::string canonical = request.getCanonicalString();

    

    // Sign

    auto signature = signData(canonical);
- Line 304: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestSignInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) {
- Line 309: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
- Line 315: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestSignFinal(md_ctx.get(), nullptr, &sig_len) != 1) {
- Line 321: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestSignFinal(md_ctx.get(), signature.data(), &sig_len) != 1) {
- Line 345: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: kAuditInvalidSignatureFormat,

            "unsupported signature_format='" + request.signature_format + "'");

    }

    if (request.key_id.empty() || !std::regex_match(request.key_id, keyIdPattern())) {

        return rejectWithAuditCode(

            kAuditInvalidKeyId,

            "invalid key_id='" + request.key_id + "'");
- Line 417: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::info("SignedRequestVerifier [{}]: evicted nonce due to cache pressure: {}",
- Line 494: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: canonical_cert = fs::weakly_canonical(cert_path);

    } catch (const fs::filesystem_error& e) {

        spdlog::warn("SignedRequestVerifier: path canonicalization failed for key_id='{}': {}",

                     request.key_id, e.what());

        return false;

    }

    if (canonical_cert.parent_path() != canonical_dir) {
- Line 502: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // between weakly_canonical() and the read (the cert_path variable retains
- Line 523: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto cert = utils::read_x509_from_bio(bio.get());
- Line 543: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Step 4: Check Certificate Revocation List if configured.

    if (!config_.crl_path.empty()) {

        if (!std::regex_match(request.cert_serial, certSerialPattern())) {

            return rejectWithAuditCode(

                kAuditInvalidCertSerial,

                "CRL check requires valid cert_serial, got '" + request.cert_serial + "'");
- Line 554: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto pubkey = utils::EVPKeyPtr(X509_get_pubkey(cert.get()));
- Line 558: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!pubkey) return false;



    // Step 6: Verify RSA/ECDSA-SHA-256 signature against the canonical request string.

    const std::string canonical = request.getCanonicalString();

    auto md_ctx = utils::make_evp_md_ctx();

    if (!md_ctx) return false;

    const int pubkey_type = EVP_PKEY_base_id(pubkey.get());
- Line 561: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const int pubkey_type = EVP_PKEY_base_id(pubkey.get());
- Line 563: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, nullptr, nullptr, pubkey.get()) != 1) {
- Line 566: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return EVP_DigestVerify(md_ctx.get(),
- Line 572: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pubkey.get()) != 1) {
- Line 575: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
- Line 578: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return EVP_DigestVerifyFinal(md_ctx.get(), signature_bytes->data(), signature_bytes->size()) == 1;
- Line 398: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Fail-closed: reject stale requests outside replay window.
- Line 474: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Note on TOCTOU: there is an inherent window between weakly_canonical() and
- Line 490: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: canonical_dir  = fs::weakly_canonical(fs::path(config_.trusted_certs_dir));
- Line 491: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: canonical_cert = fs::weakly_canonical(cert_path);
- Line 502: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // between weakly_canonical() and the read (the cert_path variable retains

### sharding/distributed_transaction.cpp
Total findings: 39

- Line 840: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: committed_ids.insert(txn_id);
- Line 843: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: aborted_ids.insert(txn_id);
- Line 970: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 161: severity=HIGH; category=missing_trace_point
  Description: Critical function commit without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool DistributedTransactionCoordinator::commit(const std::string& txn_id) {
- Line 190: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
- Line 230: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
- Line 248: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
- Line 278: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
- Line 304: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
- Line 326: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
- Line 354: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool DistributedTransactionCoordinator::abort(const std::string& txn_id) {
- Line 408: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {"snapshot_timestamp", snapshot_ts.count()}

            });

            

            auto shard_results = client.snapshotRead(snapshot_ts.count(), query);

            

            results[shard_id] = {

                {"status", "success"},
- Line 408: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto shard_results = client.snapshotRead(snapshot_ts.count(), query);
- Line 468: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& participant : txn.participants) {
- Line 473: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(error_mutex);
- Line 482: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (f.wait_for(deadline) == std::future_status::timeout) {
- Line 484: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(error_mutex);
- Line 509: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& participant : txn.participants) {
- Line 514: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(error_mutex);
- Line 523: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (f.wait_for(deadline) == std::future_status::timeout) {
- Line 525: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(error_mutex);
- Line 566: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        

        // Send PREPARE request

        bool vote_commit = client.prepare(txn_id, operations);

        participant.prepared = vote_commit;

        

        THEMIS_DEBUG("PREPARE shard {}: vote={}",
- Line 597: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ShardRPCClient client(rpc_config);

        

        // Send COMMIT request with timestamp for MVCC

        bool committed = client.commit(txn_id, commit_timestamp.count());

        participant.committed = committed;

        

        THEMIS_DEBUG("COMMIT shard {}: success={}",
- Line 627: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ShardRPCClient client(rpc_config);

        

        // Send ABORT request

        bool aborted = client.abort(txn_id);

        participant.prepared = false;

        participant.committed = false;
- Line 696: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
- Line 733: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: };

    

    for (const auto& participant : txn.participants) {

        recovery_data["participants"].push_back({

            {"shard_id", participant.shard_id},

            {"endpoint", participant.endpoint},

            {"prepared", participant.prepared},
- Line 780: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: };

    

    for (const auto& participant : txn.participants) {

        prepared_data["participants"].push_back({

            {"shard_id", participant.shard_id},

            {"endpoint", participant.endpoint},

            {"prepared", participant.prepared}
- Line 870: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: recovery_txn.state = TransactionState::ABORTING;

                

                if (prepared_data.contains("participants")) {

                    for (const auto& p : prepared_data["participants"]) {

                        TransactionParticipant participant;

                        participant.shard_id = p.value("shard_id", "");

                        participant.endpoint = p.value("endpoint", "");
- Line 929: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 953: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& participant : txn.participants) {
- Line 961: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(error_mutex);
- Line 962: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: error_details.push_back("Shard " + p_ptr->shard_id +
- Line 964: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: p_ptr->error_msg);
- Line 454: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> map)
- Line 493: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: txn.error_detail += err + "; ";
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: error_details.push_back("Shard " + participant.shard_id +
- Line 526: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: error_details.push_back("commit timed out after " +
- Line 534: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: txn.error_detail += err + "; ";
- Line 976: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: txn.error_detail += e + "; ";

### sharding/epoch_fencing.cpp
Total findings: 37

- Line 235: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4407 [WIP] Update root documenta... (2026-03-24)
- Line 111: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 119: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return current_epoch_.load(std::memory_order_acquire);
- Line 124: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: tok.epoch     = current_epoch_.load(std::memory_order_acquire);
- Line 143: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EpochNumber cur = current_epoch_.load(std::memory_order_acquire);
- Line 202: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 203: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 206: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (acquire_wait_ms.count() <= 0) {
- Line 207: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("[LeaseConfig] acquire_wait_ms must be > 0");
- Line 231: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 241: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: LeaseAcquireResult LeaseManager::acquire(const LeaseKey& key,
- Line 243: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_wait_ms;
- Line 246: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(mutex_);
- Line 260: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rec.acquired_at = std::chrono::system_clock::now();
- Line 261: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rec.expires_at = rec.acquired_at + config_.ttl_ms;
- Line 296: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ++metrics_.acquire_failures;
- Line 319: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 333: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 334: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 342: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 349: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 356: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 359: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 393: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<LeaseRecord> LeaseManager::get(const LeaseKey& key) const {
- Line 460: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << since_epoch(rec.acquired_at) << '|' << since_epoch(rec.expires_at)
- Line 483: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::string state_str, acquired_str, expires_str, gen_str, epoch_str;
- Line 490: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::getline(ss, acquired_str, '|');
- Line 499: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rec.acquired_at = std::chrono::system_clock::time_point{
- Line 500: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::chrono::milliseconds{std::stoll(acquired_str)}};
- Line 525: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 533: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(20));
- Line 151: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Stale epoch
- Line 154: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::warn("[EpochFencing] STALE_EPOCH from='{}' token_epoch={} current={}",
- Line 160: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ++metrics_.stale_rejections;
- Line 164: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return issueStonith(source_node, "stale_epoch");
- Line 166: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return FencingResult::STALE_EPOCH;

### sharding/shard_rpc_client.cpp
Total findings: 37

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4259 feat(sharding): Wire Orphan... (2026-03-15) | #3090 sharding: integrate
- Line 324: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool ShardRPCClient::abort(const std::string& txn_id) {
- Line 376: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json ShardRPCClient::snapshotRead(
- Line 604: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const nlohmann::json& params

) {

    themis::sharding::proto::PrepareRequest request;

    request.set_transaction_id(params.value("transaction_id", ""));

    request.set_coordinator_shard_id(params.value("coordinator_shard_id", ""));

    

    // Serialize operations as JSON
- Line 605: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ) {

    themis::sharding::proto::PrepareRequest request;

    request.set_transaction_id(params.value("transaction_id", ""));

    request.set_coordinator_shard_id(params.value("coordinator_shard_id", ""));

    

    // Serialize operations as JSON

    if (params.contains("operations")) {
- Line 687: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // snapshotRead() is the lightweight "point-in-time read" path which returns
- Line 716: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // snapshotRead() path we return an empty result set with metadata so the
- Line 735: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::sharding::proto::HealthRequest request;

    themis::sharding::proto::HealthResponse response;

    

    grpc::Status status = impl_->stub->HealthCheck(&context, request, &response);

    

    if (!status.ok()) {

        throw std::runtime_error("Health check failed: " + status.error_message());
- Line 755: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const nlohmann::json& params

) {

    themis::sharding::proto::ReplicateRequest request;

    request.set_shard_id(impl_->config.shard_id.empty()

                         ? impl_->config.endpoint

                         : impl_->config.shard_id);
- Line 759: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ? impl_->config.endpoint

                         : impl_->config.shard_id);



    auto* entity = request.add_entities();

    entity->set_uuid(params.value("uuid", std::string{}));

    entity->set_collection(params.value("collection", std::string{}));

    entity->set_data(params.value("data", nlohmann::json{}).dump());
- Line 41: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // Maximum delay between retry attempts (milliseconds).  Both sendRequestGrpc
- Line 42: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // and sendRequestInProcess use this cap so that a single constant controls the
- Line 52: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: /// When non-null, sendRequestInProcess() delegates to this function instead
- Line 119: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: //   When THEMIS_HAS_SHARD_GRPC is 0, all sendRequest() calls are routed
- Line 120: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: //   to sendRequestInProcess() which returns hardcoded JSON responses.
- Line 129: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: //   simulation path (sendRequestInProcess) is retained as a fallback for
- Line 131: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'WAL gRPC Replication' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §"WAL gRPC Replication"
- Line 209: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::string(override_flag) == "1")
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: override_flag && std::string(override_flag) == "1") {
- Line 279: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("prepare", params);
- Line 308: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("commit", params);
- Line 332: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("abort", params);
- Line 360: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("compensate", params);
- Line 389: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("snapshot_read", params);
- Line 408: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("ping", nlohmann::json::object());
- Line 417: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("collect_wait_for_edges", nlohmann::json::object());
- Line 432: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: edges.push_back({waiting.get<std::string>(),
- Line 457: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto response = sendRequest("write_entity", params);
- Line 465: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json ShardRPCClient::sendRequest(
- Line 472: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return sendRequestGrpc(method, params);
- Line 477: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return sendRequestInProcess(method, params);
- Line 481: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json ShardRPCClient::sendRequestGrpc(
- Line 841: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json ShardRPCClient::sendRequestInProcess(
- Line 847: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // Purpose: Provide a local in-process fallback for sendRequest() that
- Line 861: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: //   The sendRequestInProcess() method is retained as a single-node fallback
- Line 863: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'WAL gRPC Replication' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §"WAL gRPC Replication"
- Line 880: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // Re-check circuit breaker before every attempt (mirrors sendRequestGrpc).
- Line 971: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // Exponential backoff; same overflow-safe calculation as sendRequestGrpc

### sharding/gossip_config_manager.cpp
Total findings: 35

- Line 50: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: all_shards.insert(shard_id);
- Line 53: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: all_shards.insert(shard_id);
- Line 241: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: gossip_thread_.join();
- Line 244: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: anti_entropy_thread_.join();
- Line 260: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: gossip_thread_.join();
- Line 264: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: anti_entropy_thread_.join();
- Line 268: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string GossipConfigManager::publishConfigUpdate(
- Line 302: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: handleConfigUpdate(update);
- Line 319: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: metrics_->recordGossipConfigUpdate("sent");
- Line 406: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
- Line 512: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
- Line 637: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
- Line 707: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void GossipConfigManager::handleConfigUpdate(const ConfigUpdate& update) {
- Line 709: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (!shouldAcceptUpdate(update)) {
- Line 730: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflicts_resolved_++;
- Line 832: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool GossipConfigManager::shouldAcceptUpdate(const ConfigUpdate& update) {
- Line 914: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: *message.mutable_config_update() = update.toProto();
- Line 37: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void VectorClock::merge(const VectorClock& other) {
- Line 58: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t this_val = get(shard_id);
- Line 59: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t other_val = other.get(shard_id);
- Line 79: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t VectorClock::get(const std::string& shard_id) const {
- Line 234: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: gossip_thread_ = std::thread(&GossipConfigManager::gossipLoop, this);
- Line 237: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: anti_entropy_thread_ = std::thread(&GossipConfigManager::antiEntropyLoop, this);
- Line 404: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge vector clock
- Line 406: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
- Line 512: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
- Line 547: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 563: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 804: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 863: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void GossipConfigManager::mergeVectorClock(const VectorClock& other) {
- Line 865: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: local_clock_.merge(other);
- Line 37: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::merge(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void VectorClock::merge(const VectorClock& other) {
- Line 43: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::compare(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: VectorClock::Ordering VectorClock::compare(const VectorClock& other) const {
- Line 482: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> GossipConfigManager::getAllConfigs() const {
- Line 624: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidates.push_back(shard.shard_id);

### sharding/replica_consistency.cpp
Total findings: 31

- Line 30: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void VectorClock::update(const VectorClock& other) {
- Line 202: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return VersionConflict{};  // Empty conflict
- Line 206: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return entries[0];  // No conflict
- Line 272: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<LogEntry> merged;
- Line 291: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(local);
- Line 36: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
- Line 47: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t other_timestamp = other.get(node_id);
- Line 194: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ReplicaConsistencyManager::mergeReplicas(
- Line 199: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats_.merges_performed++;
- Line 214: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (config_.auto_resolve_conflicts) {
- Line 215: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto resolved = autoResolveConflict(*conflict);
- Line 227: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void ReplicaConsistencyManager::resolveConflict(
- Line 267: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<LogEntry> ReplicaConsistencyManager::mergePartitionedLogs(
- Line 271: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge logs using term and index
- Line 272: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<LogEntry> merged;
- Line 283: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(local);
- Line 287: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(remote);
- Line 291: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(local);
- Line 299: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(local_entries[local_idx++]);
- Line 302: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(remote_entries[remote_idx++]);
- Line 305: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 321: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflict.needs_manual_resolution = !config_.auto_resolve_conflicts;
- Line 330: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: VersionedEntry ReplicaConsistencyManager::autoResolveConflict(
- Line 23: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: VectorClock::VectorClock(const std::map<std::string, uint64_t>& timestamps)
- Line 26: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::increment(const std::string& node_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void VectorClock::increment(const std::string& node_id) {
- Line 36: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::get(const std::string& node_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
- Line 41: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::happensBefore(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool VectorClock::happensBefore(const VectorClock& other) const {
- Line 73: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::isConcurrent(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool VectorClock::isConcurrent(const VectorClock& other) const {
- Line 99: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, uint64_t> timestamps;
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: merged.push_back(local_entries[local_idx++]);
- Line 302: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: merged.push_back(remote_entries[remote_idx++]);

### sharding/pki_shard_certificate.cpp
Total findings: 30

- Line 81: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::istringstream input(value);
- Line 109: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::istringstream time_input(time_of_day);
- Line 241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cert_bio = utils::make_bio_mem_buf(cert_pem->c_str(), static_cast<int>(cert_pem->size()));
- Line 249: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto ca_bio = utils::make_bio_mem_buf(ca_pem->c_str(), static_cast<int>(ca_pem->size()));
- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto bio = utils::make_bio_mem_buf(crl_pem->c_str(), static_cast<int>(crl_pem->size()));
- Line 46: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 47: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ASN1_TIME_print(bio.get(), time);
- Line 50: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: long len = BIO_get_mem_data(bio.get(), &data);
- Line 173: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto cert = utils::read_x509_from_bio(bio.get());
- Line 182: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: X509_NAME* subject = X509_get_subject_name(cert.get());
- Line 190: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: X509_NAME* issuer = X509_get_issuer_name(cert.get());
- Line 198: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ASN1_INTEGER* serial = X509_get_serialNumber(cert.get());
- Line 202: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: char* hex = BN_bn2hex(bn.get());
- Line 211: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const ASN1_TIME* not_before = X509_get0_notBefore(cert.get());
- Line 212: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const ASN1_TIME* not_after = X509_get0_notAfter(cert.get());
- Line 217: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: parseSAN(cert.get(), info);
- Line 222: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: parseCustomExtensions(cert.get(), info);
- Line 242: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto cert = utils::read_x509_from_bio(cert_bio.get());
- Line 250: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto ca_cert = utils::read_x509_from_bio(ca_bio.get());
- Line 257: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto ca_pubkey = utils::EVPKeyPtr(X509_get_pubkey(ca_cert.get()));
- Line 263: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int result = X509_verify(cert.get(), ca_pubkey.get());
- Line 280: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto crl = utils::read_x509_crl_from_bio(bio.get());
- Line 287: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: STACK_OF(X509_REVOKED)* revoked = X509_CRL_get_REVOKED(crl.get());
- Line 299: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: char* hex = BN_bn2hex(bn.get());
- Line 93: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: static const std::map<std::string, int> months = {
- Line 205: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(hex);
- Line 304: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(hex);
- Line 406: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
- Line 419: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: GENERAL_NAMES_free(san_names);
- Line 406: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",

### sharding/gossip_protocol.cpp
Total findings: 28

- Line 80: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: gossip_thread_.join();
- Line 84: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: cleanup_thread_.join();
- Line 491: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
- Line 630: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (EVP_DigestSignUpdate(ctx, to_sign.c_str(), to_sign.length()) == 1) {
- Line 683: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* fp = fopen(key_path.c_str(), "r");
- Line 728: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: EVP_DigestVerifyUpdate(ctx, to_verify.data(), to_verify.size()) == 1 &&
- Line 63: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: gossip_thread_ = std::thread(&GossipProtocol::gossipLoop, this);
- Line 66: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cleanup_thread_ = std::thread(&GossipProtocol::cleanupLoop, this);
- Line 127: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 128: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 129: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 130: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 243: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge peer list
- Line 249: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergePeerList(peers);
- Line 327: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 339: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 496: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void GossipProtocol::mergePeerList(const std::vector<PeerInfo>& peers) {
- Line 563: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // confirmed through Raft joint-consensus).  Without a gate (backward compat),
- Line 564: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // use the legacy warn+add path.
- Line 723: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 88: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, PeerInfo> GossipProtocol::getPeers() const {
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: peers.push_back(PeerInfo::fromJson(p));
- Line 292: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 529: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Remove stale peers
- Line 647: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 648: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 738: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 741: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);

### sharding/mtls_connection_pool.cpp
Total findings: 28

- Line 52: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: cleanup_thread_.join();
- Line 106: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: active_connections_.insert(pooled.ssl.get());
- Line 337: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }

    

    // Create new pool (write lock)

    std::unique_lock<std::shared_mutex> lock(pools_mutex_);

    

    // Double-check in case another thread created it
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4259 feat(sharding): Wire Orphan... (2026-03-15) | #1035 [WIP] Implement dyn
- Line 42: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cleanup_thread_ = std::thread([this]() { cleanupLoop(); });
- Line 56: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::optional<std::unique_ptr<SSL, SSLDeleter>> EndpointConnectionPool::getConnection(
- Line 70: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: } else if (pooled.ssl && validateConnection(pooled.ssl.get())) {
- Line 73: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: active_connections_.insert(pooled.ssl.get());
- Line 79: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 83: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 84: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 84: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: active_connections_.insert(new_conn->get());
- Line 104: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (pooled.ssl && validateConnection(pooled.ssl.get())) {
- Line 106: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: active_connections_.insert(pooled.ssl.get());
- Line 119: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: SSL* raw_ptr = connection.get();
- Line 274: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 302: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::shared_mutex> lock(pool_mutex_);
- Line 337: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 419: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 427: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // The next getConnection() call will create a new TLS connection which
- Line 20: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SSL_free(ptr);
- Line 232: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'mTLS Pool Connection Ownership' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md § "mTLS Pool Connection Ownership"
- Line 34: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Initializing connection pool for endpoint: " << endpoint << std::endl;
- Line 187: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Warmed up " << created << " connections for endpoint: "
- Line 291: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Cleaned up " << removed << " expired connections for endpoint: "
- Line 313: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Initialized MTLSConnectionPoolManager" << std::endl;
- Line 422: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "MTLSConnectionPoolManager: certificate rotated – draining idle "
- Line 436: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Shutting down MTLSConnectionPoolManager with "

### sharding/two_phase_commit_coordinator.cpp
Total findings: 27

- Line 422: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // touching shared state, so that concurrent coordinator operations are
- Line 55: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC coordinator [{}] WAL initialised at {}",
- Line 73: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC coordinator [{}] registered participant shard {}",
- Line 89: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: participants_[shard_id] = adapter.get();
- Line 91: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC coordinator [{}] registered remote participant shard {} at {}",
- Line 116: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC coordinator [{}] txn {} – no shards, aborting",
- Line 134: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC coordinator [{}] duplicate commit for completed txn {} – "
- Line 150: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] txn {} – unknown shard {}",
- Line 235: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC coordinator [{}] txn {} COMMITTED", coordinator_id_, transaction_id);
- Line 240: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC coordinator [{}] txn {} ABORTED", coordinator_id_, transaction_id);
- Line 265: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC coordinator [{}] recovering from WAL…", coordinator_id_);
- Line 325: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC coordinator [{}] in-doubt txn {} has no decision "
- Line 349: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC coordinator [{}] re-driving in-doubt txn {} with decision {}",
- Line 369: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] WAL recovery failed: {}", coordinator_id_, e.what());
- Line 372: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC coordinator [{}] recovery complete – {} in-doubt txns resolved",
- Line 429: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 1 – participant {} not found for txn {}",
- Line 449: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 1 – shard {} threw on PREPARE for txn {}: {}",
- Line 454: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 1 – timed out re-acquiring mutex for txn {}; aborting",
- Line 459: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC coordinator [{}] shard {} voted ABORT for txn {}",
- Line 474: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC coordinator [{}] Phase 2 – participant {} not found for txn {} "
- Line 490: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 2 – timed out re-acquiring mutex for txn {} after {}; conti
- Line 498: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 2 – timed out re-acquiring mutex for txn {} on error path",
- Line 503: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] Phase 2 – shard {} threw on {} for txn {}: {}",
- Line 540: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC coordinator [{}] WAL write failed for txn {}: {}",
- Line 106: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, nlohmann::json>& ops_per_shard
- Line 273: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, CoordinatorTxnRecord> recovered;
- Line 274: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, bool>                 decisions; // txn_id → commit?

### sharding/auto_rebalancer.cpp
Total findings: 26

- Line 87: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto forecast = detector_->forecastLoad(shard_id, config_.forecast_horizon);
- Line 88: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (forecast && forecast->predicted_composite_load >= config_.predictive_load_threshold) {
- Line 160: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_INFO("AutoRebalancer initialized with check_interval={}s, max_concurrent={}",
- Line 161: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: config_.check_interval.count() / 1000, config_.max_concurrent_operations);
- Line 195: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitor_thread_.join();
- Line 243: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Check max concurrent operations
- Line 246: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (active_operations_.size() >= config_.max_concurrent_operations) {
- Line 247: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_WARN("Max concurrent operations reached, queuing remaining");
- Line 176: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: monitor_thread_ = std::thread([this]() {
- Line 214: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 221: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Check if we can trigger new rebalances
- Line 242: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& rec : imbalance.recommendations) {
- Line 245: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 255: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 469: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // Encode signature as Base64 using OpenSSL', '    // Calculate required buffer size: ((input_len + 2) / 3) * 4 + 1 for null terminator', '    size_t b64_len = ((signature.size() + 2) / 3) * 4 + 1;', '    std::vector<unsigned char> b64_buf(b64_len);', '']
- Line 71: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!proposal.reason.empty()) proposal.reason += ", ";
- Line 432: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 433: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 439: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 440: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 448: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 449: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 457: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 458: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 464: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 465: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);

### sharding/slo_monitor.cpp
Total findings: 24

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4181 feat(sharding): Reed-Solomo... (2026-03-13) | #3328 [WIP] Add SLO/SLA c
- Line 114: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double SLOWindow::getErrorBudget(double target_availability) const {
- Line 287: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double SLOMonitor::getErrorBudget(const std::string& shard_id) const {
- Line 304: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return window->getErrorBudget(availability_target);
- Line 307: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double SLOMonitor::getGlobalErrorBudget() const {
- Line 328: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: total_budget += window->getErrorBudget(availability_target);
- Line 340: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double budget = getErrorBudget(shard_id);
- Line 369: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double error_budget = window->getErrorBudget(targets.availability_target);
- Line 439: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: slo["error_budget"] = window->getErrorBudget(targets.availability_target);
- Line 496: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: total_budget += window->getErrorBudget(availability_target);
- Line 608: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double error_budget = window->getErrorBudget(config_snapshot.targets.availability_target);
- Line 619: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: double target = (query_type.find("single") != std::string::npos)
- Line 723: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 476: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> SLOMonitor::getSLOCompliance() const {
- Line 477: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows;
- Line 484: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> compliance;
- Line 584: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows;
- Line 585: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::shared_ptr<SLOWindow>> query_latency_windows;
- Line 603: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: active_alerts.push_back(
- Line 610: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: active_alerts.push_back(
- Line 625: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: active_alerts.push_back(
- Line 635: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: active_alerts.push_back(
- Line 694: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: json_file.close();
- Line 754: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file.close();

### sharding/adaptive_shard_router.cpp
Total findings: 22

- Line 256: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: already_queried.insert(shard_id);
- Line 319: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged_results;
- Line 531: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged = nlohmann::json::array();
- Line 154: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
- Line 158: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return ShardRouter::executeQuery(query);
- Line 162: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return executeAdaptiveQuery(query, stats);
- Line 165: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json AdaptiveShardRouter::executeAdaptiveQuery(
- Line 205: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return ShardRouter::executeQuery(query);
- Line 303: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge results from all iterations
- Line 304: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto merged_results = mergeIterationResults(all_iteration_results);
- Line 319: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged_results;
- Line 440: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (already_queried.find(match.shard_id) != already_queried.end()) {
- Line 465: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (lhs.score != rhs.score) {
- Line 516: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 528: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json AdaptiveShardRouter::mergeIterationResults(
- Line 531: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: nlohmann::json merged = nlohmann::json::array();
- Line 540: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(item);
- Line 546: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 89: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int best_freshness_rank = 2;  // 0=fresh, 1=stale, 2=missing
- Line 154: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
- Line 455: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Skip stale topology entries (e.g., shard became unhealthy after scoring).
- Line 575: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: scores.push_back(match.score);

### sharding/prometheus_metrics.cpp
Total findings: 21

- Line 88: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordResultMergeTime(double time_ms) {
- Line 89: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
- Line 132: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordGossipConfigUpdate(const std::string& operation) {
- Line 140: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordGossipConfigConflict(const std::string& resolution_type) {
- Line 141: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: incrementCounter("themis_gossip_config_conflicts_total", {{"resolution", resolution_type}});
- Line 574: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordPaxosProposalConflict(const std::string& shard_id) {
- Line 575: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: incrementCounter("themis_paxos_proposal_conflicts_total", {{"shard_id", shard_id}});
- Line 683: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordPercolatorConflict(const std::string& transaction_id) {
- Line 684: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: incrementCounter("themis_percolator_conflicts_total", {{"transaction_id", transaction_id}});
- Line 731: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordMvccWrite(double latency_ms) {
- Line 88: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordResultMergeTime(double time_ms) {
- Line 89: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
- Line 736: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void PrometheusMetrics::recordMvccRead(const std::string& read_type, double latency_ms) {
- Line 29: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: incrementCounter("themis_cross_shard_rpc_calls_total",
- Line 350: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p50 = sorted[sorted.size() * 50 / 100];
- Line 351: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p95 = sorted[sorted.size() * 95 / 100];
- Line 352: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p99 = sorted[sorted.size() * 99 / 100];
- Line 371: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "# HELP themis_gossip_messages_total Total gossip messages sent/received\n";
- Line 406: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p50 = sorted[sorted.size() * 50 / 100];
- Line 407: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p95 = sorted[sorted.size() * 95 / 100];
- Line 408: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p99 = sorted[sorted.size() * 99 / 100];

### sharding/cloud_agent.cpp
Total findings: 20

- Line 79: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker_thread_.join();
- Line 83: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: health_thread_.join();
- Line 476: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool a_local = (shard_a->datacenter == config_.datacenter);
- Line 477: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool b_local = (shard_b->datacenter == config_.datacenter);
- Line 482: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool a_same_region = (shard_a->datacenter.find(config_.region) != std::string::npos);
- Line 483: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool b_same_region = (shard_b->datacenter.find(config_.region) != std::string::npos);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #45 [WIP] Delegate tasks to clo... (2026-03-11) | #52 Implement horizontal/ve
- Line 62: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: worker_thread_ = std::thread(&CloudAgent::workerLoop, this);
- Line 66: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: health_thread_ = std::thread(&CloudAgent::healthLoop, this);
- Line 279: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto response = executor_->get(shard, "/health");
- Line 308: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 311: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
- Line 360: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 572: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto [shard_id, shard_result] = futures[i].get();
- Line 576: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(results_mutex);
- Line 590: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(results_mutex);
- Line 113: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: operation.on_complete(result.result);
- Line 347: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: operation.on_complete(result.result);
- Line 402: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: target_shards.push_back(shard.shard_id);
- Line 569: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto status = futures[i].wait_for(timeout);

### sharding/two_phase_commit_participant.cpp
Total findings: 20

- Line 51: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC participant [{}] WAL initialised at {}", shard_id_, config_.wal_directory);
- Line 70: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC participant [{}] duplicate PREPARE for {} – returning stored vote {}",
- Line 81: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC participant [{}] PREPARE {}: failed to parse ops – aborting: {}",
- Line 100: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC participant [{}] PREPARE {}: lock/validation error: {}",
- Line 146: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC participant [{}] duplicate COMMIT for {} – idempotent ok",
- Line 153: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC participant [{}] COMMIT for unknown/aborted txn {}",
- Line 169: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC participant [{}] COMMIT {}: apply error: {}",
- Line 176: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC participant [{}] COMMIT {} failed to apply operations",
- Line 187: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC participant [{}] COMMIT {}: lock release error (ignored): {}",
- Line 208: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC participant [{}] COMMIT {} applied successfully", shard_id_, transaction_id);
- Line 220: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC participant [{}] duplicate ABORT for {} – idempotent ok",
- Line 227: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC participant [{}] ABORT for already-committed txn {}",
- Line 236: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC participant [{}] ABORT {}: lock release error (ignored): {}",
- Line 268: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("2PC participant [{}] ABORT {} completed", shard_id_, transaction_id);
- Line 306: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC participant [{}] aborting timed-out prepared txn {}",
- Line 332: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC participant [{}] recovering from WAL…", shard_id_);
- Line 387: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("2PC participant [{}] in-doubt transaction found: {}",
- Line 394: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC participant [{}] WAL recovery failed: {}", shard_id_, e.what());
- Line 397: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("2PC participant [{}] recovery complete – {} in-doubt transactions",
- Line 459: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("2PC participant [{}] WAL write failed for txn {}: {}",

### sharding/hot_spare_manager.cpp
Total findings: 19

- Line 47: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (max_concurrent_rebuilds == 0) {
- Line 48: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::error("Invalid max_concurrent_rebuilds: must be > 0");
- Line 125: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: health_check_thread_.join();
- Line 128: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: rebuild_thread_.join();
- Line 471: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: history.insert(history.end(), start_it, failover_history_.end());
- Line 657: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (active_rebuilds >= config_.max_concurrent_rebuilds) {
- Line 14: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * with a compatibility shim for themisdb::sharding at the end,
- Line 101: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: health_check_thread_ = std::thread([this]() {
- Line 107: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: rebuild_thread_ = std::thread([this]() {
- Line 350: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 390: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(rebuild_mutex_);
- Line 492: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 631: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(rebuild_mutex_);
- Line 631: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(rebuild_mutex_);
- Line 634: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: rebuild_cv_.wait_for(lock, std::chrono::seconds(10), [this]() {
- Line 763: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 0; i < task.documents.size(); ++i) {
- Line 768: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(rebuild_mutex_);
- Line 896: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility shim
- Line 593: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ss << "# HELP themis_hot_spare_rebuild_throughput_mbps Rebuild throughput in MB/s\n";

### sharding/raft_consensus_adapter.cpp
Total findings: 18

- Line 497: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // against any concurrent state change.
- Line 181: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 293: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 321: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 332: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 333: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 338: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 352: severity=HIGH; category=missing_trace_point
  Description: Critical function consensus without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: spdlog::info("Node {} added to cluster via joint consensus (endpoint: {})",
- Line 400: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 428: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 439: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 440: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 445: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 501: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 502: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 507: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 541: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: uint64_t restored_index = 0;

    uint64_t restored_term  = 0;

    if (snapshot_data.contains("_snapshot_index"))

        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();

    if (snapshot_data.contains("_snapshot_term"))

        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();
- Line 543: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (snapshot_data.contains("_snapshot_index"))

        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();

    if (snapshot_data.contains("_snapshot_term"))

        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();



    {

        std::lock_guard<std::mutex> lock(snapshot_mutex_);

### sharding/wal_manager.cpp
Total findings: 18

- Line 81: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), data_str.begin(), data_str.end());
- Line 215: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: std::optional<WALEntry> WALManager::read(const LSN& lsn) {
- Line 306: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: current_segment_->write(reinterpret_cast<const char*>(write_buffer_.data()),
- Line 215: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<WALEntry> WALManager::read(const LSN& lsn) {
- Line 250: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(buffer.data()), file_size);
- Line 359: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
- Line 415: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
- Line 453: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.wal_directory)) {
- Line 40: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALEntry::serialize()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::vector<uint8_t> WALEntry::serialize() const {
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(static_cast<uint8_t>(type));
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back((timestamp >> (i * 8)) & 0xFF);
- Line 54: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back((lsn.segment >> (i * 8)) & 0xFF);
- Line 59: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back((lsn.offset >> (i * 8)) & 0xFF);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back((tx_id_len >> (i * 8)) & 0xFF);
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back((data_len >> (i * 8)) & 0xFF);
- Line 112: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    entry.lsn.offset = 0;', '    for (int i = 0; i < 8; ++i) {', '        entry.lsn.offset = (entry.lsn.offset << 8) | bytes[pos++];', '    }', '']
- Line 177: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALManager::append(const WALEntry& entry)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: LSN WALManager::append(const WALEntry& entry) {
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: segments.push_back({seg_num, entry.path().string()});

### sharding/raft_consensus.cpp
Total findings: 17

- Line 55: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: heartbeat_thread_.join();
- Line 58: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: election_thread_.join();
- Line 61: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: partition_detector_thread_.join();
- Line 70: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // atomically under replica_mutex_ so that a concurrent step-down cannot
- Line 131: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // path must hold replica_mutex_ so that concurrent proposeEntry()
- Line 37: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: heartbeat_thread_ = std::thread(&RaftConsensus::heartbeatLoop, this);
- Line 38: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: election_thread_ = std::thread(&RaftConsensus::electionLoop, this);
- Line 41: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: partition_detector_thread_ = std::thread(&RaftConsensus::partitionDetectionLoop, this);
- Line 236: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 237: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::milliseconds(config_.raft_config.heartbeat_interval_ms),
- Line 253: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 254: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::milliseconds(50),
- Line 264: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(partition_mutex_);
- Line 275: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 454: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 361: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: status.partition_id += ":" + node;
- Line 362: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: status.partition_id += ":" + node;

### sharding/data_migrator.cpp
Total findings: 16

- Line 106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto resp = count_client->get(config_.source_endpoint, count_path.str());
- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
- Line 331: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto response = mtls_client->post(config_.target_endpoint, path, request_body);
- Line 437: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: completed_migrations_.insert(migration_id);
- Line 452: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: completed_batches_.insert(batch_id);
- Line 484: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: loaded_migrations.insert(item.get<std::string>());
- Line 500: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: loaded_batches.insert(item.get<std::string>());
- Line 519: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // that file I/O does not block concurrent migration threads.
- Line 106: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto resp = count_client->get(config_.source_endpoint, count_path.str());
- Line 147: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Write batch to target (atomic operation)
- Line 275: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
- Line 294: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy format: response body itself might be the array
- Line 454: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: should_persist = (batch_counter_.fetch_add(1, std::memory_order_relaxed) % 10 == 0);
- Line 449: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool should_persist = false;
- Line 464: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> loaded_migrations;
- Line 465: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> loaded_batches;

### sharding/health_check.cpp
Total findings: 16

- Line 146: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (stale_thread.joinable()) {

        stale_thread.join();

    }



    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
- Line 146: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: stale_thread.join();
- Line 146: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: stale_thread.join();
- Line 192: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread_to_join.join();
- Line 149: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
- Line 153: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: periodic_thread_ = std::thread([this, shard_endpoints]() {
- Line 259: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: sharding::MTLSClient client(client_config);

        

        // Request storage metrics from shard

        auto response = client.get(endpoint, "/api/v1/metrics/storage");

        

        if (!response.success) {

            // Try fallback health endpoint
- Line 259: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto response = client.get(endpoint, "/api/v1/metrics/storage");
- Line 263: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!response.success) {

            // Try fallback health endpoint

            response = client.get(endpoint, "/health");

            

            if (!response.success) {

                usage_percent = 0.0;
- Line 263: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: response = client.get(endpoint, "/health");
- Line 315: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto start = std::chrono::steady_clock::now();

        

        // Ping the health endpoint

        auto response = client.get(endpoint, "/health");

        

        auto end = std::chrono::steady_clock::now();

        response_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
- Line 315: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto response = client.get(endpoint, "/health");
- Line 128: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::thread stale_thread;
- Line 141: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stale_thread = std::move(periodic_thread_);
- Line 145: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (stale_thread.joinable()) {
- Line 146: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stale_thread.join();

### sharding/raft_log.cpp
Total findings: 16

- Line 51: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = log_.find(i);
- Line 321: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.write(reinterpret_cast<const char*>(&v), sizeof(v));
- Line 325: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.write(reinterpret_cast<const char*>(&v), sizeof(v));
- Line 341: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.write(checksum.data(), static_cast<std::streamsize>(checksum_len));
- Line 351: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.write(reinterpret_cast<const char*>(compressed.data()),
- Line 50: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = log_.find(i);
- Line 145: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr size_t kEntryOverhead = sizeof(uint64_t) * 3;
- Line 425: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr size_t kMinHeaderBytes = 4 * sizeof(uint64_t) + sizeof(uint32_t);
- Line 445: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(&v), sizeof(v));
- Line 454: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(&v), sizeof(v));
- Line 494: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(snap.checksum.data(), static_cast<std::streamsize>(checksum_len));
- Line 509: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(snap.data.data()),
- Line 642: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(chunk.data.data()),
- Line 53: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(it->second);
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(std::stoull(id_str));
- Line 672: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(std::stoull(id_str));

### sharding/gossip_consensus_adapter.cpp
Total findings: 15

- Line 83: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: gossip_thread_.join();
- Line 177: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = log_entries_.find(i);
- Line 68: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: gossip_thread_ = std::thread(&GossipConsensusAdapter::gossipThread, this);
- Line 161: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 261: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: uint64_t restored_index = 0;

    if (snapshot_data.contains("_snapshot_index"))

        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();



    {

        std::lock_guard<std::mutex> lock(state_mutex_);
- Line 334: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void GossipConsensusAdapter::gossipThread() {
- Line 338: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(state_mutex_);
- Line 338: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(state_mutex_);
- Line 351: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> log_lock(log_mutex_);
- Line 351: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> log_lock(log_mutex_);
- Line 354: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [index, entry] : log_entries_) {
- Line 359: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> cb_lock(callbacks_mutex_);
- Line 380: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    // Calculate quorum (majority)', '    size_t quorum_size = (cluster_nodes_.size() / 2) + 1;', '    return it->second.size() >= quorum_size;', '}']
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(it->second);
- Line 312: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### sharding/metadata_shard.cpp
Total findings: 15

- Line 149: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cached = cache_->get(cache_key);
- Line 173: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cache_->put(cache_key, entry_it->second.toJson());
- Line 179: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool MetadataShard::put(
- Line 385: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cache_->put(cache_key, entry.toJson());
- Line 432: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto log_index = consensus_->propose(operation, log_data);
- Line 487: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool MetadataShardRouter::put(
- Line 503: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return it->second->put(partition, key, value);
- Line 140: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<MetadataEntry> MetadataShard::get(
- Line 149: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto cached = cache_->get(cache_key);
- Line 398: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto cached = cache_->get(cache_key);
- Line 469: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<MetadataEntry> MetadataShardRouter::get(
- Line 484: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return it->second->get(partition, key);
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: keys.push_back(pair.first);
- Line 366: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 620: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, MetadataEntry> partition_entries;

### sharding/predictive_detector.cpp
Total findings: 15

- Line 151: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitoring_thread_.join();
- Line 140: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: monitoring_thread_ = std::thread([this]() {
- Line 181: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& shard : shards) {
- Line 186: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(predictions_mutex_);
- Line 192: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 202: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 449: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(size_t(5), features.size()); ++i) {
- Line 472: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(size_t(5), features.size()); ++i) {
- Line 45: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'PredictiveDetector ONNX Model.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §PredictiveDetector ONNX Model.
- Line 346: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: latencies.push_back(m.avg_latency_ms);
- Line 344: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<double> latencies, throughputs, error_rates;
- Line 347: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: throughputs.push_back(static_cast<double>(m.throughput_ops_per_sec));
- Line 397: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: features.push_back(compute_mean(throughputs));
- Line 398: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: features.push_back(compute_stddev(throughputs));
- Line 399: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: features.push_back(compute_trend(throughputs));

### sharding/shard_load_detector.cpp
Total findings: 15

- Line 202: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 241: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 265: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 278: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(),
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: request_rates.push_back(static_cast<double>(metrics.requests_per_sec));
- Line 194: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: latencies.push_back(metrics.p99_latency_ms);
- Line 236: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 237: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 238: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result.reason += "Latency degradation on " + shard_ids[i] +
- Line 260: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 261: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 262: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result.reason += "CPU exhaustion on " + shard_id +
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.reason.empty()) result.reason += "; ";
- Line 275: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result.reason += "Storage exhaustion on " + shard_id +

### sharding/shard_resource_manager.cpp
Total findings: 14

- Line 182: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitoring_thread_.join();
- Line 289: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void ShardResourceManager::broadcastResourceUpdate() {
- Line 330: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void ShardResourceManager::receiveResourceUpdate(const std::string& shard_id,
- Line 452: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: broadcastResourceUpdate();
- Line 171: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: monitoring_thread_ = std::thread([this]() {
- Line 250: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool ShardResourceManager::acquireRepairIOToken(double io_ops,
- Line 255: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (repair_io_limiter_->try_acquire(io_ops)) {
- Line 265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 266: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (repair_io_limiter_->try_acquire(io_ops)) {
- Line 100: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ms = j["timestamp_ms"].get<int64_t>();
- Line 340: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, ShardResourceManager::ResourceSnapshot>
- Line 449: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cleanupStaleSnapshots();
- Line 490: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void ShardResourceManager::cleanupStaleSnapshots() {
- Line 627: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'VRAM Usage Monitoring.' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md §VRAM Usage Monitoring.

### sharding/truetime.cpp
Total findings: 14

- Line 156: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: sync_thread_.join();
- Line 250: severity=CRITICAL; category=missing_dtor
  Description: Class NTPPacket allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct NTPPacket
- Line 269: severity=CRITICAL; category=socket_leak
  Description: Socket created but never closed — potential resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: SocketHandle sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
- Line 51: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: startSyncThread();
- Line 56: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stopSyncThread();
- Line 142: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void TrueTime::startSyncThread() {
- Line 147: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: sync_thread_ = std::thread(&TrueTime::syncThreadFunc, this);
- Line 150: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void TrueTime::stopSyncThread() {
- Line 207: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 242: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: SocketHandle get() const { return fd_; }
- Line 425: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\"uncertainty_us\": " << (uncertainty_ns_.load() / 1000) << ", "
- Line 135: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\"drift_us\": " << (drift_ns_.load() / 1000) << ", "
- Line 244: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### sharding/capability_matcher.cpp
Total findings: 12

- Line 233: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: query_set.insert(normalize(kw));
- Line 238: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: normalized_shard_kw.insert(normalize(kw));
- Line 274: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = shard_tfidf.find(term);
- Line 308: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (query_magnitude == 0.0 || shard_magnitude == 0.0) {
- Line 341: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (shard_set.find(qd) != shard_set.end()) {
- Line 366: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (shard_set.find(qo) != shard_set.end()) {
- Line 391: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (shard_set.find(qr) != shard_set.end()) {
- Line 416: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (shard_set.find(qt) != shard_set.end()) {
- Line 250: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> query_tfidf;
- Line 260: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> shard_tfidf;
- Line 204: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double idf = std::log((total_shards_ + config_.idf_smoothing) /
- Line 448: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return std::log(total_shards_ + config_.idf_smoothing);

### sharding/health_monitor.cpp
Total findings: 12

- Line 92: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitor_thread_.join();
- Line 79: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Fallback to dedicated thread (backward compatibility)
- Line 79: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Fallback to dedicated thread (backward compatibility)
- Line 80: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: monitor_thread_ = std::thread([this]() {
- Line 206: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& primary : primaries) {
- Line 214: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 277: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& replica_id : replica_set->replicas) {
- Line 285: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 393: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto future = http_pool_->get(url);
- Line 402: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto response = future.get();
- Line 127: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, HealthCheckResult> HealthMonitor::getAllHealthStatuses() const {
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidates.push_back(primary.node_id);

### sharding/shard_rpc_server.cpp
Total findings: 12

- Line 377: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #endif

}



void ShardRPCServer::wait() {

#if THEMIS_HAS_SHARD_GRPC

    if (impl_->server) {

        impl_->server->Wait();
- Line 377: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void ShardRPCServer::wait() {
- Line 60: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::sharding::proto::PrepareResponse* response

    ) override {

        if (!handler_) {

            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");

        }

        

        THEMIS_DEBUG("gRPC PrepareTransaction: txn_id={}", request->transaction_id());
- Line 90: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::sharding::proto::CommitResponse* response

    ) override {

        if (!handler_) {

            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");

        }

        

        THEMIS_DEBUG("gRPC CommitTransaction: txn_id={}", request->transaction_id());
- Line 115: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::sharding::proto::AbortResponse* response

    ) override {

        if (!handler_) {

            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");

        }

        

        THEMIS_DEBUG("gRPC AbortTransaction: txn_id={}", request->transaction_id());
- Line 141: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto health_info = handler_->onHealthCheck();
- Line 170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const auto edges = handler_->onCollectWaitForEdges();
- Line 203: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto health = handler_->onHealthCheck();
- Line 334: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: builder.RegisterService(impl_->service.get());
- Line 318: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::string(override_flag) == "1")
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: override_flag && std::string(override_flag) == "1") {
- Line 334: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: builder.RegisterService(impl_->service.get());
- Line 373: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: impl_->service.reset();

### sharding/distributed_coordinator.cpp
Total findings: 11

- Line 145: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: election_thread_.join();
- Line 148: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: heartbeat_thread_.join();
- Line 151: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: task_executor_thread_.join();
- Line 102: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: election_thread_ = std::thread([this]() {
- Line 111: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Start heartbeat thread (only active if leader)
- Line 112: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: heartbeat_thread_ = std::thread([this]() {
- Line 124: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: task_executor_thread_ = std::thread([this]() {
- Line 129: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 436: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 511: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 195: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool won = true;

### sharding/hardware_migration_manager.cpp
Total findings: 11

- Line 184: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 185: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 207: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 230: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 276: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto before_it  = before_vnode_counts.find(sid);
- Line 277: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto current_it = current.find(sid);
- Line 200: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, size_t> before_vnodes;
- Line 238: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, size_t>
- Line 244: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, size_t>
- Line 247: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, size_t> snapshot;
- Line 269: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, size_t>&  before_vnode_counts

### sharding/paxos_snapshot.cpp
Total findings: 11

- Line 247: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.write(text.data(), static_cast<std::streamsize>(text.size()));
- Line 206: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            }', '', '            snapshot.instances[slot] = instance_json;', '        }', '']
- Line 315: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Detect format: binary ZSTD ("PAXZ" magic) or legacy plain JSON
- Line 317: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(magic, 4);
- Line 340: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: file.read(reinterpret_cast<char*>(compressed.data()),
- Line 355: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy plain-JSON format – rewind and parse as text
- Line 394: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
- Line 411: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 359: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file.close();
- Line 404: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                        uint64_t snapshot_id = std::stoull(id_str);

                        snapshots.push_back(snapshot_id);

                    } catch (...) {

                        // Skip invalid filenames

                    }

                }
- Line 404: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### sharding/wal_shipper.cpp
Total findings: 11

- Line 383: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        // Approximate lag (segment size * segment difference + offset difference)', '        uint64_t segment_diff = current_lsn.segment - replica.last_confirmed_lsn.segment;', '        replica.lag_bytes = segment_diff * 16 * 1024 * 1024 + current_lsn.offset;', '    }', '']
- Line 119: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(replicas_mutex_);
- Line 126: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& replica_id : replica_ids) {
- Line 129: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(replicas_mutex_);
- Line 143: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 156: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(cv_lock, std::chrono::milliseconds(config_.ship_interval_ms),
- Line 201: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Start new batch
- Line 201: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 383: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        // Approximate lag (segment size * segment difference + offset difference)', '        uint64_t segment_diff = current_lsn.segment - replica.last_confirmed_lsn.segment;', '        replica.lag_bytes = segment_diff * 16 * 1024 * 1024 + current_lsn.offset;', '    }', '']
- Line 415: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto response = mtls_client_->get(replica.endpoint, "/api/v1/health");
- Line 538: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += (i + 1 < data.size()) ? kB64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';

### sharding/raft_configuration.cpp
Total findings: 10

- Line 28: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 29: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 38: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 39: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 46: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 73: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 93: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 103: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 113: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### sharding/wal_applier.cpp
Total findings: 10

- Line 185: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool WALApplier::handleConflict(const WALEntry& entry) {
- Line 186: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (!config_.enable_conflict_detection) {
- Line 187: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return true;  // Conflicts ignored
- Line 192: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats_.conflicts_detected++;
- Line 195: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Conflict resolution strategy (can be extended)
- Line 197: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::cerr << "WALApplier: Conflict detected for entry at LSN "
- Line 31: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 164: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 62: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string error = "LSN stale or duplicate: current " + current_lsn_.toString() +
- Line 151: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "WALApplier: Exception applying entry at LSN "

### sharding/consistent_hash.cpp
Total findings: 9

- Line 150: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: seen.insert(it->second);
- Line 207: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: seen.insert(it->second);
- Line 221: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = ring_.begin(); it != ring_.end() && it->first <= hash_end; ++it) {
- Line 222: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: seen.insert(it->second);
- Line 231: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: seen.insert(it->second);
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: vnode_key += '#';
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: vnode_key += '#';
- Line 106: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::optional<std::string> ConsistentHashRing::getNode(const std::string& key) const {
- Line 202: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;

### sharding/locality_aware_router.cpp
Total findings: 9

- Line 208: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: placement_cache_[cache_key].insert(shard_id);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #777 [YARN-Inspired] Implement L... (2026-03-11)
- Line 331: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = placement_cache_.find(cache_key);
- Line 334: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: it->second.find(shard_id) != it->second.end()) {
- Line 69: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string LocalityAwareRouter::routeQuery(const QuerySpec& spec) {
- Line 118: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<std::string> LocalityAwareRouter::routeMultiShardQuery(const QuerySpec& spec) {
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(affinity.shard_id);
- Line 212: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cleanupStaleEntries();
- Line 424: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void LocalityAwareRouter::cleanupStaleEntries() {

### sharding/mtls_client.cpp
Total findings: 9

- Line 123: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MTLSClient::Response MTLSClient::put(const std::string& endpoint,
- Line 214: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: http::write(stream, req);
- Line 379: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto stats = pool_manager_->getStatistics();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4259 feat(sharding): Wire Orphan... (2026-03-15) | #1035 [WIP] Implement dyn
- Line 105: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MTLSClient::Response MTLSClient::get(const std::string& endpoint, const std::string& path) {
- Line 169: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: beast::get_lowest_layer(stream).expires_after(

                std::chrono::milliseconds(config_.connect_timeout_ms)

            );

            beast::get_lowest_layer(stream).connect(results);

            

            // Perform SSL handshake

            stream.handshake(ssl::stream_base::client);
- Line 190: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: req.target(path);
- Line 219: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: http::read(stream, buffer, res);
- Line 252: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay));

### sharding/partition_detector.cpp
Total findings: 9

- Line 42: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: health_check_thread_.join();
- Line 33: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: health_check_thread_ = std::thread(&PartitionDetector::healthCheckLoop, this);
- Line 145: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(nodes_mutex_);
- Line 178: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(nodes_mutex_);
- Line 234: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(nodes_mutex_);
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(pair.second);
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: node_ids.push_back(pair.first);
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: unreachable_nodes.push_back(pair.first);
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: reachable_nodes.push_back(pair.first);

### sharding/admin_api.cpp
Total findings: 8

- Line 161: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 182: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 307: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        nlohmann::json shard_entry;', '        shard_entry["shard_id"] = r.shard_id;', '        shard_entry["status"] = kStatusStr[status_idx];', '        shard_entry["documents_scanned"] = r.documents_scanned;', '        shard_entry["documents_healthy"] = r.documents_healthy;']
- Line 340: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 343: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 124: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: Endpoints
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: } else if (path.find(Endpoints::REPAIR_STATUS) == 0 && method == "GET") {
- Line 233: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: GENERAL_NAMES_free(sans);
- Line 237: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);

### sharding/gpu_erasure_coder_opencl.cpp
Total findings: 8

- Line 71: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 512 > array 255
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: gf_exp[255] = gf_exp[0];
- Line 71: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 512 > array size 255
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // multiply by 2 in GF(2^8)

        bool carry = (x & 0x80) != 0;

        x = static_cast<uint8_t>(x << 1);

        if (carry) x ^= 0x1D;   // reduce mod 0x11D

    }

    gf_exp[255] = gf_exp[0];

    for (int i = 256; i < 512; ++i) gf_exp[i] = gf_exp[i - 255];

    gf_log[0] = 0;  // undefined, but set to 0 to avoid UB

}



static uint8_t gf_mul(uint8_t a, uint8_t b) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4265 feat(sharding): implement G... (2026-03-15) | #4181 feat(sharding): Ree
- Line 222: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                        ? static_cast<size_t>(config.device_id) % num_dev', '                        : 0;', '                    chosen_device = devs[dev_idx];', '                    break;', '                }']
- Line 289: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("OpenCL: failed to allocate GF table buffers "
- Line 541: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: clSetKernelArg(kernel_, 4, sizeof(cl_mem), &buf_gf_log_);
- Line 674: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: clSetKernelArg(kernel_, 4, sizeof(cl_mem),  &buf_gf_log_);
- Line 264: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string log(log_size, '\0');

### sharding/raft_state.cpp
Total findings: 8

- Line 271: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 3. If an existing entry conflicts with a new one (same index but different terms),
- Line 271: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return response;

    }

    

    // 3. If an existing entry conflicts with a new one (same index but different terms),

    //    delete the existing entry and all that follow it

    for (const auto& entry : request.entries) {

        auto existing = log_.getEntry(entry.index);
- Line 127: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 272: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    // 3. If an existing entry conflicts with a new one (same index but different terms),

    //    delete the existing entry and all that follow it

    for (const auto& entry : request.entries) {

        auto existing = log_.getEntry(entry.index);

        if (existing.has_value() && existing->term != entry.term) {
- Line 272: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: //    delete the existing entry and all that follow it
- Line 289: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    // 5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)

    if (request.leader_commit > log_.getCommitIndex()) {

        uint64_t new_commit = std::min(request.leader_commit, log_.getLastLogIndex());

        log_.setCommitIndex(new_commit);

    }
- Line 290: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // 5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)

    if (request.leader_commit > log_.getCommitIndex()) {

        uint64_t new_commit = std::min(request.leader_commit, log_.getLastLogIndex());

        log_.setCommitIndex(new_commit);

    }
- Line 290: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### sharding/raft_wal_integration.cpp
Total findings: 8

- Line 31: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: RaftWALIntegration::WriteResult RaftWALIntegration::write(const WALEntry& entry) {
- Line 87: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // blocking disk read would prevent concurrent writes from progressing.
- Line 189: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Wake up write() waiters whenever a new entry reaches quorum.
- Line 60: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    pending.committed = false;', '', '    pending_writes_[log_index] = pending;', '', '    // CC-2a: The original audit finding flagged a potential self-deadlock where']
- Line 71: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = pending_writes_.find(log_index);
- Line 84: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<WALEntry> RaftWALIntegration::read(const LSN& lsn) {
- Line 94: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return config_.wal_manager->read(lsn);
- Line 190: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### sharding/shard_repair_engine.cpp
Total findings: 8

- Line 82: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: scan_thread_.join();
- Line 85: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: repair_thread_.join();
- Line 62: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: scan_thread_ = std::thread([this]() { scanLoop(); });
- Line 66: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: repair_thread_ = std::thread([this]() { repairLoop(); });
- Line 336: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::timed_mutex> lock(jobs_mutex_);
- Line 537: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: !resource_manager_->acquireRepairIOToken(1.0, config_.repair_poll_interval)) {
- Line 712: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: !resource_manager_->acquireRepairIOToken(1.0, config_.repair_poll_interval)) {
- Line 505: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::info("ShardRepairEngine: parallel anti-entropy scan complete ({} shards, {} workers)",

### sharding/urn_resolver.cpp
Total findings: 8

- Line 26: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string shard_id = hash_ring_->getShardForURN(urn);
- Line 49: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::vector<std::string> successor_ids = hash_ring_->getSuccessors(hash, replica_count + 1);
- Line 67: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string shard_id = hash_ring_->getShardForURN(urn);
- Line 84: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto node = hash_ring_->getNode(key);
- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t h_min = hash_ring_->hashKey(min_key);
- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t h_max = hash_ring_->hashKey(max_key);
- Line 95: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto shards = hash_ring_->getShardsInRange(h_min, h_max);
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(*replica);

### sharding/multi_primary_coordinator.cpp
Total findings: 7

- Line 151: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: LSN MultiPrimaryCoordinator::resolveConflict(const WriteConflict& conflict) const {
- Line 152: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflicts_resolved_++;
- Line 155: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return conflict.resolveLastWriteWins();
- Line 159: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return (conflict.lsn2 > conflict.lsn1) ? conflict.lsn2 : conflict.lsn1;
- Line 162: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void MultiPrimaryCoordinator::recordWrite(const LSN& lsn) {
- Line 197: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
- Line 197: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats.conflicts_resolved = conflicts_resolved_.load();

### sharding/remote_executor.cpp
Total findings: 7

- Line 62: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RemoteExecutor::Result RemoteExecutor::put(const ShardInfo& shard_info,
- Line 198: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: response = mtls_client_->put(endpoint, path, request_body);
- Line 51: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RemoteExecutor::Result RemoteExecutor::get(const ShardInfo& shard_info,
- Line 73: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RemoteExecutor::Result RemoteExecutor::executeQuery(const ShardInfo& shard_info,
- Line 194: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: response = mtls_client_->get(endpoint, path);
- Line 100: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto b0 = data[i];
- Line 105: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: encoded += (i + 1 < size) ? kBase64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';

### sharding/shard_durability.cpp
Total findings: 7

- Line 133: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::recursive_directory_iterator(info.path)) {
- Line 319: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.checkpoint_dir)) {
- Line 340: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& file : std::filesystem::recursive_directory_iterator(info.path)) {
- Line 138: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: info.size_bytes += entry.file_size();

            }

        }

    } catch (...) {

        info.size_bytes = 0;

    }
- Line 138: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 334: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::chrono::system_clock::now()

                    );

                    info.created_at = sctp;

                } catch (...) {

                    info.created_at = std::chrono::system_clock::now();

                }
- Line 334: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### sharding/sharding_manager_edition.cpp
Total findings: 7

- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: error += " nodes maximum (";

        error += std::string(edition::EDITION_STRING);

        error += " edition)";

        throw std::runtime_error(error);

    }



    shard_nodes_.push_back(node);
- Line 62: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: error += std::to_string(max_nodes);

        error += "). Edition: ";

        error += std::string(edition::EDITION_STRING);

        throw std::runtime_error(error);

    }

}
- Line 93: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const auto info = edition::EditionInfo::Get();
- Line 195: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const auto info = edition::EditionInfo::Get();
- Line 216: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const auto info = edition::EditionInfo::Get();
- Line 95: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += " | Max Shard Nodes: ";
- Line 176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(all_shards[idx]);

### sharding/metadata_snapshot.cpp
Total findings: 6

- Line 79: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto& [partition_key, entries] : storage) {

            std::map<std::string, nlohmann::json> partition_data;

            for (const auto& [key, metadata_entry] : entries) {

                partition_data[key] = metadata_entry.toJson();

                snapshot.total_entries++;

            }

            snapshot.partitions[partition_key] = partition_data;
- Line 129: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 186: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
- Line 207: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 200: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                        uint64_t snapshot_id = std::stoull(id_str);

                        snapshot_ids.push_back(snapshot_id);

                    } catch (...) {

                        // Skip invalid filenames

                    }

                }
- Line 200: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### sharding/orphan_detector.cpp
Total findings: 6

- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: orphaned_txns.push_back(txn.transaction_id);
- Line 135: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Skip transactions that haven't yet hit the stale threshold.
- Line 146: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("OrphanDetector: Reclaiming stale Percolator lock for txn {} "
- Line 154: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("OrphanDetector: Stale Percolator lock reclaimed for txn {}",
- Line 157: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::warn("OrphanDetector: Failed to abort stale Percolator txn {}",
- Line 162: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: spdlog::info("OrphanDetector::cleanPercolatorLocks: reclaimed {} stale lock(s)",

### sharding/paxos_state_persistence.cpp
Total findings: 6

- Line 103: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool PaxosStatePersistence::open(const std::string& node_id) {
- Line 156: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        uint64_t slot = entry.slot;', '', '        DurableAcceptorState& s = slot_cache_[slot];', '        s.slot = slot;', '']
- Line 218: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    if (!is_open_.load()) return false;', '', '    DurableAcceptorState& s = slot_cache_[slot];', '    s.slot           = slot;', '    s.promised_round = ballot_round;']
- Line 244: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    if (!is_open_.load()) return false;', '', '    DurableAcceptorState& s = slot_cache_[slot];', '    s.slot          = slot;', '    s.accepted_round = ballot_round;']
- Line 307: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(30))) {
- Line 141: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PaxosStatePersistence::close() {

### sharding/operational_metrics.cpp
Total findings: 5

- Line 300: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: {"conflicts", metrics->transaction_conflicts.load()}
- Line 536: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool had_conflict
- Line 46: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return it->second.get();
- Line 437: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (metrics->min_latency_us.compare_exchange_weak(current_min, latency_us)) {
- Line 444: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (metrics->max_latency_us.compare_exchange_weak(current_max, latency_us)) {

### sharding/transaction_snapshot.cpp
Total findings: 5

- Line 356: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
- Line 393: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::error("Failed to delete snapshot {}: {}", snapshot_id, e.what());
- Line 393: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        return false;

    } catch (const std::exception& e) {

        spdlog::error("Failed to delete snapshot {}: {}", snapshot_id, e.what());

        return false;

    }

}
- Line 393: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::error("Failed to delete snapshot {}: {}", snapshot_id, e.what());
- Line 406: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t to_delete = snapshots.size() - max_snapshots_;

### sharding/circuit_breaker.cpp
Total findings: 4

- Line 135: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 137: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 138: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 192: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### sharding/metadata_wal.cpp
Total findings: 4

- Line 59: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: LSN MetadataWAL::logPut(
- Line 94: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: LSN MetadataWAL::logUpdate(
- Line 88: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: entry.value = nullptr;  // No value for delete
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(MetadataWALEntry::fromWALEntry(wal_entry));

### sharding/replication_coordinator.cpp
Total findings: 4

- Line 83: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from replicas_acknowledged never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: result.replicas_acknowledged = it->second.ack_count.load(std::memory_order_acquire);
- Line 152: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return enabled_.load(std::memory_order_acquire);
- Line 171: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t current_acks = write.ack_count.load(std::memory_order_acquire);
- Line 185: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: pending.completed.load(std::memory_order_acquire)) {

### sharding/shard_topology.cpp
Total findings: 4

- Line 227: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: };

    

    try {

        auto response = client.post(config_.metadata_endpoint, prefix, request_body);

        

        if (!response.success) {

            std::cerr << "ShardTopology: Failed to query metadata store: "
- Line 343: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: };

        

        try {

            auto response = client.post(config_.metadata_endpoint, "/v3/kv/put", request_body);

            

            if (!response.success) {

                std::cerr << "ShardTopology: Failed to save shard " << shard_id
- Line 346: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "ShardTopology: Failed to save shard " << shard_id
- Line 412: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;

### sharding/raft_shard_manager.cpp
Total findings: 3

- Line 145: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::future<bool> RaftShardManager::proposeWrite(const std::string& shard_id,
- Line 193: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, ShardRaftInfo> RaftShardManager::getAllShardRaftInfo() const {
- Line 196: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, ShardRaftInfo> all_info;

### sharding/quorum_manager.cpp
Total findings: 2

- Line 113: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: QuorumResult QuorumManager::executeRead(ReadOperation operation,
- Line 243: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: T result = future.get();

### sharding/secure_transport_client.cpp
Total findings: 2

- Line 241: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay));
- Line 261: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay));

### sharding/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### sharding/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### sharding/gpu_erasure_coder.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4181 feat(sharding): Reed-Solomo... (2026-03-13) | #250 [v1.5.0] GPU-Acceler

### sharding/paxos_wal.cpp
Total findings: 1

- Line 31: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // We use type 100+ for Paxos entries to avoid conflicts

### sharding/transaction_wal.cpp
Total findings: 1

- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(txn_entry.value());

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
