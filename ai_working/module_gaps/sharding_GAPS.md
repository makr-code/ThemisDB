# sharding Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: sharding
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 904
- Actionable Findings (Critical + High): 444
- Affected Files: 75

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 162 |
| High | 282 |
| Medium | 457 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| distributed_consistency | 470 |
| performance_patterns | 342 |
| container | 244 |
| reliability | 116 |
| exception_safety | 85 |
| raii | 74 |
| memory | 66 |
| performance | 62 |
| observability | 46 |
| concurrency | 40 |
| legacy_duplication | 31 |
| determinism | 22 |
| input_validation | 17 |
| type_conversion | 13 |
| audit_logging | 12 |
| platform | 10 |
| security | 7 |
| uninitialized | 7 |
| oop_design | 6 |
| llm_ai_safety | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/sharding/redundancy_strategy.cpp | 110 | 18 | 17 | 75 | 0 |
| src/sharding/shard_router.cpp | 91 | 30 | 37 | 24 | 0 |
| src/sharding/cross_shard_transaction.cpp | 63 | 3 | 6 | 54 | 0 |
| src/sharding/gossip_config_manager.cpp | 30 | 15 | 10 | 5 | 0 |
| src/sharding/shard_rpc_client.cpp | 30 | 0 | 4 | 26 | 0 |
| src/sharding/stream_protocol.cpp | 28 | 10 | 10 | 8 | 0 |
| src/sharding/adaptive_shard_router.cpp | 27 | 3 | 13 | 11 | 0 |
| src/sharding/distributed_transaction.cpp | 27 | 2 | 6 | 19 | 0 |
| src/sharding/replica_consistency.cpp | 25 | 4 | 15 | 6 | 0 |
| src/sharding/signed_request.cpp | 25 | 3 | 17 | 5 | 0 |
| src/sharding/capability_matcher.cpp | 22 | 2 | 1 | 17 | 2 |
| src/sharding/prometheus_metrics.cpp | 20 | 10 | 3 | 7 | 0 |
| src/sharding/pki_shard_certificate.cpp | 19 | 0 | 18 | 1 | 0 |
| src/sharding/slo_monitor.cpp | 19 | 0 | 10 | 9 | 0 |
| src/sharding/gossip_protocol.cpp | 17 | 3 | 6 | 8 | 0 |
| src/sharding/paxos_consensus.cpp | 16 | 0 | 7 | 9 | 0 |
| src/sharding/metadata_shard.cpp | 15 | 5 | 5 | 5 | 0 |
| src/sharding/shard_load_detector.cpp | 14 | 0 | 0 | 14 | 0 |
| src/sharding/cloud_agent.cpp | 12 | 0 | 4 | 8 | 0 |
| src/sharding/hot_spare_manager.cpp | 12 | 4 | 5 | 3 | 0 |
| src/sharding/predictive_detector.cpp | 12 | 0 | 7 | 5 | 0 |
| src/sharding/raft_log.cpp | 12 | 4 | 5 | 3 | 0 |
| src/sharding/wal_manager.cpp | 12 | 1 | 2 | 9 | 0 |
| src/sharding/auto_rebalancer.cpp | 11 | 5 | 2 | 4 | 0 |
| src/sharding/raft_consensus.cpp | 10 | 1 | 4 | 5 | 0 |
| src/sharding/shard_resource_manager.cpp | 10 | 3 | 1 | 6 | 0 |
| src/sharding/consistent_hash.cpp | 9 | 4 | 0 | 5 | 0 |
| src/sharding/data_migrator.cpp | 9 | 4 | 3 | 2 | 0 |
| src/sharding/health_monitor.cpp | 9 | 0 | 7 | 2 | 0 |
| src/sharding/quorum_manager.cpp | 9 | 0 | 2 | 7 | 0 |
| src/sharding/shard_topology.cpp | 9 | 0 | 0 | 9 | 0 |
| src/sharding/gpu_erasure_coder_opencl.cpp | 8 | 0 | 0 | 7 | 1 |
| src/sharding/mtls_connection_pool.cpp | 8 | 1 | 7 | 0 | 0 |
| src/sharding/multi_primary_coordinator.cpp | 8 | 6 | 1 | 1 | 0 |
| src/sharding/wal_applier.cpp | 8 | 6 | 0 | 2 | 0 |
| src/sharding/epoch_fencing.cpp | 7 | 0 | 1 | 6 | 0 |
| src/sharding/remote_executor.cpp | 7 | 2 | 3 | 2 | 0 |
| src/sharding/sharding_manager_edition.cpp | 7 | 0 | 3 | 4 | 0 |
| src/sharding/truetime.cpp | 7 | 0 | 6 | 1 | 0 |
| src/sharding/two_phase_commit_coordinator.cpp | 7 | 1 | 1 | 5 | 0 |
| src/sharding/wal_shipper.cpp | 7 | 0 | 2 | 5 | 0 |
| src/sharding/cloud_backup.cpp | 6 | 0 | 4 | 2 | 0 |
| src/sharding/locality_aware_router.cpp | 6 | 0 | 2 | 4 | 0 |
| src/sharding/operational_metrics.cpp | 6 | 2 | 1 | 3 | 0 |
| src/sharding/orphan_detector.cpp | 6 | 0 | 0 | 6 | 0 |
| src/sharding/gossip_consensus_adapter.cpp | 5 | 0 | 3 | 2 | 0 |
| src/sharding/hardware_migration_manager.cpp | 5 | 0 | 0 | 5 | 0 |
| src/sharding/health_check.cpp | 5 | 0 | 4 | 1 | 0 |
| src/sharding/mtls_client.cpp | 5 | 2 | 3 | 0 | 0 |
| src/sharding/partition_detector.cpp | 5 | 0 | 1 | 4 | 0 |
| src/sharding/raft_shard_manager.cpp | 5 | 1 | 0 | 4 | 0 |
| src/sharding/shard_repair_engine.cpp | 5 | 0 | 2 | 3 | 0 |
| src/sharding/distributed_coordinator.cpp | 4 | 0 | 4 | 0 | 0 |
| src/sharding/paxos_snapshot.cpp | 4 | 1 | 2 | 1 | 0 |
| src/sharding/raft_wal_integration.cpp | 4 | 1 | 3 | 0 | 0 |
| src/sharding/shard_rpc_server.cpp | 4 | 0 | 1 | 3 | 0 |
| src/sharding/metadata_wal.cpp | 3 | 2 | 0 | 1 | 0 |
| src/sharding/raft_consensus_adapter.cpp | 3 | 1 | 1 | 1 | 0 |
| src/sharding/paxos_wal.cpp | 2 | 1 | 0 | 1 | 0 |
| src/sharding/shard_durability.cpp | 2 | 0 | 0 | 2 | 0 |
| src/sharding/urn_resolver.cpp | 2 | 0 | 0 | 2 | 0 |
| src/sharding/admin_api.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/admin_operations.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/circuit_breaker.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/gpu_erasure_coder.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/metadata_snapshot.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/raft_state.cpp | 1 | 1 | 0 | 0 | 0 |
| src/sharding/replica_topology.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/transaction_snapshot.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/transaction_wal.cpp | 1 | 0 | 0 | 1 | 0 |
| src/sharding/paxos_state_persistence.cpp | 0 | 0 | 0 | 0 | 0 |
| src/sharding/raft_configuration.cpp | 0 | 0 | 0 | 0 | 0 |
| src/sharding/replication_coordinator.cpp | 0 | 0 | 0 | 0 | 0 |
| src/sharding/secure_transport_client.cpp | 0 | 0 | 0 | 0 | 0 |
| src/sharding/two_phase_commit_participant.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/sharding/redundancy_strategy.cpp
Total findings: 110

- Line 356: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: recovered.insert(recovered.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 415: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 725: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: recovered.insert(recovered.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 796: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.99
- Line 968: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), it->second.begin(), it->second.end());
  Confidence: band=very_high; score=0.99
- Line 1080: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), shards[s].begin(), shards[s].end());
  Confidence: band=very_high; score=0.99
- Line 1163: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), it->second.begin(), it->second.end());
  Confidence: band=very_high; score=0.99
- Line 1260: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), shards[s].begin(), shards[s].end());
  Confidence: band=very_high; score=0.99
- Line 1326: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: WriteResult RedundancyStrategy::write(
  Confidence: band=very_high; score=0.99
- Line 1565: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool RedundancyStrategy::proposeRaftWrite(const std::string& shard_id,
  Confidence: band=very_high; score=0.99
- Line 1696: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_written_shards.insert(all_written_shards.end(),
  Confidence: band=very_high; score=0.99
- Line 1951: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: candidates.insert(candidates.end(), replicas.begin(), replicas.end());
  Confidence: band=very_high; score=0.99
- Line 2102: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: available_shards.insert(available_shards.end(), replicas.begin(), replicas.end());
  Confidence: band=very_high; score=0.99
- Line 2159: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge chunks
  Confidence: band=very_high; score=0.99
- Line 2160: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: auto merged = mergeChunks(chunks);
  Confidence: band=very_high; score=0.99
- Line 2163: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.data = std::string(merged.begin(), merged.end());
  Confidence: band=very_high; score=0.99
- Line 2542: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_shards.insert(all_shards.end(), replicas.begin(), replicas.end());
  Confidence: band=very_high; score=0.99
- Line 2604: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: shards.insert(shards.end(), replicas2.begin(), replicas2.end());
  Confidence: band=very_high; score=0.99
- Line 965: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = available_chunks.find(s);
  Confidence: band=very_high; score=0.9
- Line 1158: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = available_chunks.find(s);
  Confidence: band=very_high; score=0.9
- Line 1377: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ReadResult RedundancyStrategy::read(
  Confidence: band=very_high; score=0.9
- Line 1500: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 1603: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool committed = future.get();
  Confidence: band=very_high; score=0.9
- Line 1657: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 1754: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 1854: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 2159: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge chunks
  Confidence: band=very_high; score=0.9
- Line 2160: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto merged = mergeChunks(chunks);
  Confidence: band=very_high; score=0.9
- Line 2163: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.data = std::string(merged.begin(), merged.end());
  Confidence: band=very_high; score=0.9
- Line 2253: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<uint8_t> RedundancyStrategy::mergeChunks(
  Confidence: band=very_high; score=0.9
- Line 2256: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<uint8_t> merged;
  Confidence: band=very_high; score=0.9
- Line 2259: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.insert(merged.end(), chunk.begin(), chunk.end());
  Confidence: band=very_high; score=0.9
- Line 2262: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 2504: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (futures[i].get()) {
  Confidence: band=very_high; score=0.9
- Line 3008: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility shim: expose under themisdb::sharding
  Confidence: band=high; score=0.8
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 434: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 446: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 450: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
  Confidence: band=medium; score=0.56
- Line 479: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 499: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 582: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 687: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 687: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 687: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 687: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(parity);
  Confidence: band=high; score=0.74
- Line 695: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 956: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!recovered[s]) still_missing.push_back(s);
  Confidence: band=high; score=0.74
- Line 1035: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (recovered[s]) avail_rows.push_back(s);
  Confidence: band=high; score=0.74
- Line 1037: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (recovered[global_start + p]) avail_rows.push_back(data_shards + p);
  Confidence: band=high; score=0.74
- Line 1141: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 1268: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
  Confidence: band=medium; score=0.56
- Line 1482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id]() {
  Confidence: band=high; score=0.74
- Line 1493: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(wait_timeout);
  Confidence: band=high; score=0.74
- Line 1495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1647: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id, chunk]() {
  Confidence: band=high; score=0.74
- Line 1657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1744: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id, chunk, chunk_id]() {
  Confidence: band=high; score=0.74
- Line 1754: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1801: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> region_candidates;
  Confidence: band=high; score=0.74
- Line 1804: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: region_candidates[region].push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1809: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> write_failed_set(
  Confidence: band=medium; score=0.66
- Line 1837: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 1848: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(config_.replication_timeout);
  Confidence: band=high; score=0.74
- Line 1850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_shards.push_back(target_shards[i]);
  Confidence: band=high; score=0.74
- Line 1869: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
  Confidence: band=medium; score=0.66
- Line 1980: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> failed_set;
  Confidence: band=medium; score=0.66
- Line 1985: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, uint32_t> region_reads;
  Confidence: band=high; score=0.74
- Line 2062: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Bounded-staleness / follower-read fallback: try remaining candidates
  Confidence: band=high; score=0.74
- Line 2150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(*data_opt);
  Confidence: band=high; score=0.74
- Line 2202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 2246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 2299: severity=MEDIUM; category=determinism; pattern=random_unseeded
  Description: rand() without nearby explicit srand() seeding
  Context: size_t idx = std::rand() % available_shards.size();
  Confidence: band=high; score=0.74
- Line 2367: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
  Confidence: band=medium; score=0.66
- Line 2384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geo.failed_regions.push_back(region);
  Confidence: band=high; score=0.74
- Line 2440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.emplace_back(shards[i],
  Confidence: band=high; score=0.74
- Line 2459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.emplace_back(shards[i], key);
  Confidence: band=high; score=0.74
- Line 2472: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> failed_set(failed_regions.begin(),
  Confidence: band=medium; score=0.66
- Line 2494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 2631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_idx_vec.push_back(i);
  Confidence: band=high; score=0.74
- Line 2631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_idx_vec.push_back(i);
  Confidence: band=high; score=0.74
- Line 2704: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health.missing_shards.push_back(all_shards[i]);
  Confidence: band=high; score=0.74
- Line 2732: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health.missing_shards.push_back(shards[i]);
  Confidence: band=high; score=0.74
- Line 2750: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health.missing_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2918: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Bounded-staleness limit
  Confidence: band=high; score=0.74
- Line 2919: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ss << "# HELP themis_geo_max_staleness_ms Maximum accepted replication staleness in ms\n";
  Confidence: band=high; score=0.74
- Line 2920: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ss << "# TYPE themis_geo_max_staleness_ms gauge\n";
  Confidence: band=high; score=0.74
- Line 2921: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ss << "themis_geo_max_staleness_ms{mode=\"geo_mirror\"} " << geo.max_staleness_ms << "\n";
  Confidence: band=high; score=0.74
- Line 2992: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collections.push_back(name);
  Confidence: band=high; score=0.74

### src/sharding/shard_router.cpp
Total findings: 91

- Line 139: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool ShardRouter::put(const URN& urn, const nlohmann::json& data) {
  Confidence: band=very_high; score=0.99
- Line 211: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 219: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 226: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 630: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Emit one merged row per matching left-side entry.
  Confidence: band=very_high; score=0.99
- Line 632: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged = nlohmann::json::object();
  Confidence: band=very_high; score=0.99
- Line 634: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["left_" + k] = v;
  Confidence: band=very_high; score=0.99
- Line 638: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (!merged.contains(rk)) merged[rk] = v;
  Confidence: band=very_high; score=0.99
- Line 640: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=very_high; score=0.99
- Line 679: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Phase 2: Merge results from all shards
  Confidence: band=very_high; score=0.99
- Line 680: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged = mergeResults(results);
  Confidence: band=very_high; score=0.99
- Line 685: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: {"data", merged}
  Confidence: band=very_high; score=0.99
- Line 758: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: exec_result = executor_->put(*shard_info, path, *body);
  Confidence: band=very_high; score=0.99
- Line 958: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
  Confidence: band=very_high; score=0.99
- Line 959: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged;
  Confidence: band=very_high; score=0.99
- Line 960: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.99
- Line 961: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["errors"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.99
- Line 962: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["shard_count"] = results.size();
  Confidence: band=very_high; score=0.99
- Line 970: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // If result has data array, merge it
  Confidence: band=very_high; score=0.99
- Line 973: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.99
- Line 977: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.99
- Line 981: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["results"].push_back(result.data);
  Confidence: band=very_high; score=0.99
- Line 984: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["errors"].push_back(nlohmann::json{
  Confidence: band=very_high; score=0.99
- Line 991: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["success_count"] = success_count;
  Confidence: band=very_high; score=0.99
- Line 992: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged["error_count"] = results.size() - success_count;
  Confidence: band=very_high; score=0.99
- Line 994: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return merged;
  Confidence: band=very_high; score=0.99
- Line 998: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const nlohmann::json& merged,
  Confidence: band=very_high; score=0.99
- Line 1002: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json paginated = merged;
  Confidence: band=very_high; score=0.99
- Line 1004: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (merged.contains("results") && merged["results"].is_array()) {
  Confidence: band=very_high; score=0.99
- Line 1005: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const auto& results = merged["results"];
  Confidence: band=very_high; score=0.99
- Line 109: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<nlohmann::json> ShardRouter::get(
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json ShardRouter::executeQuery(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto exec_result = executor_->executeQuery(shard, query);
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: results.push_back(futures[i].get());
  Confidence: band=very_high; score=0.9
- Line 465: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto exec_result = executor_->executeQuery(shard, query);
  Confidence: band=very_high; score=0.9
- Line 489: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: results.push_back(futures[i].get());
  Confidence: band=very_high; score=0.9
- Line 628: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = hash_table.find(key);
  Confidence: band=very_high; score=0.9
- Line 630: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Emit one merged row per matching left-side entry.
  Confidence: band=very_high; score=0.9
- Line 632: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged = nlohmann::json::object();
  Confidence: band=very_high; score=0.9
- Line 634: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["left_" + k] = v;
  Confidence: band=very_high; score=0.9
- Line 638: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (!merged.contains(rk)) merged[rk] = v;
  Confidence: band=very_high; score=0.9
- Line 640: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=very_high; score=0.9
- Line 679: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Phase 2: Merge results from all shards
  Confidence: band=very_high; score=0.9
- Line 680: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged = mergeResults(results);
  Confidence: band=very_high; score=0.9
- Line 685: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: {"data", merged}
  Confidence: band=very_high; score=0.9
- Line 756: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: exec_result = executor_->get(*shard_info, path);
  Confidence: band=very_high; score=0.9
- Line 865: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Execute query (simplified - return empty results)
  Confidence: band=very_high; score=0.9
- Line 958: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json ShardRouter::mergeResults(const std::vector<ShardResult>& results) {
  Confidence: band=very_high; score=0.9
- Line 959: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged;
  Confidence: band=very_high; score=0.9
- Line 960: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.9
- Line 961: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["errors"] = nlohmann::json::array();
  Confidence: band=very_high; score=0.9
- Line 962: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["shard_count"] = results.size();
  Confidence: band=very_high; score=0.9
- Line 970: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // If result has data array, merge it
  Confidence: band=very_high; score=0.9
- Line 973: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.9
- Line 977: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"].push_back(item);
  Confidence: band=very_high; score=0.9
- Line 981: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["results"].push_back(result.data);
  Confidence: band=very_high; score=0.9
- Line 984: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["errors"].push_back(nlohmann::json{
  Confidence: band=very_high; score=0.9
- Line 991: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["success_count"] = success_count;
  Confidence: band=very_high; score=0.9
- Line 992: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged["error_count"] = results.size() - success_count;
  Confidence: band=very_high; score=0.9
- Line 994: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 998: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const nlohmann::json& merged,
  Confidence: band=very_high; score=0.9
- Line 1002: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json paginated = merged;
  Confidence: band=very_high; score=0.9
- Line 1004: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (merged.contains("results") && merged["results"].is_array()) {
  Confidence: band=very_high; score=0.9
- Line 1005: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const auto& results = merged["results"];
  Confidence: band=very_high; score=0.9
- Line 36: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static std::map<std::string, std::string> parseQueryParams(const std::string& path) {
  Confidence: band=high; score=0.74
- Line 37: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> params;
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json ShardRouter::executeQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RoutingStrategy ShardRouter::analyzeQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=high; score=0.74
- Line 358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ShardInfo> shard_map;
  Confidence: band=medium; score=0.66
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_shards.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_shard_ids.push_back(shard.shard_id);
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 557: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
  Confidence: band=medium; score=0.66
- Line 570: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 570: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_table[key].push_back(row);
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined_rows.push_back(std::move(merged));
  Confidence: band=high; score=0.74
- Line 972: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged["results"].push_back(item);
  Confidence: band=high; score=0.74
- Line 972: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged["results"].push_back(item);
  Confidence: band=high; score=0.74
- Line 976: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged["results"].push_back(item);
  Confidence: band=high; score=0.74
- Line 1011: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: page.push_back(results[i]);
  Confidence: band=high; score=0.74

### src/sharding/cross_shard_transaction.cpp
Total findings: 63

- Line 651: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference: a concurrent abort() could erase the map entry while we are
  Confidence: band=very_high; score=0.99
- Line 726: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference: a concurrent commit() could erase the map entry while we are
  Confidence: band=very_high; score=0.99
- Line 1898: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Only merge remote edges that reference known live
  Confidence: band=very_high; score=0.99
- Line 292: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: deadlock_detection_thread_ = std::thread(
  Confidence: band=very_high; score=0.9
- Line 641: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool CrossShardTransactionCoordinator::commit(const std::string& transaction_id) {
  Confidence: band=very_high; score=0.9
- Line 716: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool CrossShardTransactionCoordinator::abort(const std::string& transaction_id) {
  Confidence: band=very_high; score=0.9
- Line 1850: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void CrossShardTransactionCoordinator::deadlockDetectionThread() {
  Confidence: band=very_high; score=0.9
- Line 1898: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Only merge remote edges that reference known live
  Confidence: band=very_high; score=0.9
- Line 1967: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = transactions_.find(txn_id);
  Confidence: band=very_high; score=0.9
- Line 45: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> collectCycles(
  Confidence: band=medium; score=0.66
- Line 46: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& graph
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> cycles;
  Confidence: band=medium; score=0.66
- Line 49: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> index;
  Confidence: band=medium; score=0.66
- Line 50: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> lowlink;
  Confidence: band=medium; score=0.66
- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> on_stack;
  Confidence: band=medium; score=0.66
- Line 106: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>(component.begin(), component.end()));
  Confidence: band=medium; score=0.66
- Line 119: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> collectCycleNodes(
  Confidence: band=medium; score=0.66
- Line 122: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> cycle_nodes;
  Confidence: band=medium; score=0.66
- Line 228: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: recovery_result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={}); "
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: recovery_result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: recovery_result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: result.details.stale_transactions_detected,
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(txn);
  Confidence: band=high; score=0.74
- Line 1254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: precommit_data["participants"].push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: commit_data["participants"].push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1386: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: perc_cfg.stale_lock_threshold = std::chrono::seconds(30);
  Confidence: band=high; score=0.74
- Line 1496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_order.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1515: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 1645: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operations.push_back(op);
  Confidence: band=high; score=0.74
- Line 1883: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: remote_edges.push_back({
  Confidence: band=high; score=0.74
- Line 1968: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(txn_id, it->second.start_time);
  Confidence: band=high; score=0.74
- Line 1968: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(txn_id, it->second.start_time);
  Confidence: band=high; score=0.74
- Line 2013: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>>
  Confidence: band=high; score=0.74
- Line 2015: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> graph;
  Confidence: band=high; score=0.74
- Line 2043: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph[waiting_txn_id].push_back(blocking_txn_id);
  Confidence: band=high; score=0.74
- Line 2071: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& graph,
  Confidence: band=high; score=0.74
- Line 2296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: participants_json.push_back({
  Confidence: band=high; score=0.74
- Line 2543: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("Transaction {} is stale (age: {}s), will abort", txn_id, age.count());
  Confidence: band=high; score=0.74
- Line 2543: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transactions_to_timeout.push_back(txn_id);
  Confidence: band=high; score=0.74
- Line 2571: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Final states - can be cleaned up eventually
  Confidence: band=high; score=0.74
- Line 2592: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("Aborting {} stale transactions", transactions_to_timeout.size());
  Confidence: band=high; score=0.74
- Line 2604: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::error("Failed to abort stale transaction {}: {}", txn_id, e.what());
  Confidence: band=high; score=0.74
- Line 2621: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: stats->stale_transactions_detected = transactions_to_timeout.size();
  Confidence: band=high; score=0.74
- Line 2624: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: stats->stale_transactions_detected + stats->resume_candidates;
  Confidence: band=high; score=0.74
- Line 2696: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.participants.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2696: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.participants.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 2868: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locked_shards.push_back(primary_shard_id);
  Confidence: band=high; score=0.74
- Line 2963: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: size_t PercolatorCoordinator::cleanStaleLocks(
  Confidence: band=high; score=0.74
- Line 2964: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: const std::vector<std::string>& stale_txn_ids,
  Confidence: band=high; score=0.74
- Line 2969: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& txn_id : stale_txn_ids) {
  Confidence: band=high; score=0.74
- Line 2988: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (age < config_.stale_lock_threshold) {
  Confidence: band=high; score=0.74
- Line 2992: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("[Percolator] Cleaning stale lock for txn {} (age {}s)",
  Confidence: band=high; score=0.74
- Line 2999: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: wal_->logAbort(txn_id, "stale_lock_cleanup");
  Confidence: band=high; score=0.74
- Line 3007: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("[Percolator] Stale lock cleaned for txn {}", txn_id);
  Confidence: band=high; score=0.74
- Line 3009: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("[Percolator] Failed to clean stale lock for txn {}", txn_id);
  Confidence: band=high; score=0.74
- Line 3013: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("[Percolator] cleanStaleLocks: cleaned {} / {} stale locks",
  Confidence: band=high; score=0.74
- Line 3014: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cleaned, stale_txn_ids.size());
  Confidence: band=high; score=0.74

### src/sharding/gossip_config_manager.cpp
Total findings: 30

- Line 49: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_shards.insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 52: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: all_shards.insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 252: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::string GossipConfigManager::publishConfigUpdate(
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: handleConfigUpdate(update);
  Confidence: band=very_high; score=0.99
- Line 291: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: metrics_->recordGossipConfigUpdate("sent");
  Confidence: band=very_high; score=0.99
- Line 332: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
  Confidence: band=very_high; score=0.99
- Line 414: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.99
- Line 541: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
  Confidence: band=very_high; score=0.99
- Line 604: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void GossipConfigManager::handleConfigUpdate(const ConfigUpdate& update) {
  Confidence: band=very_high; score=0.99
- Line 606: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (!shouldAcceptUpdate(update)) {
  Confidence: band=very_high; score=0.99
- Line 627: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_++;
  Confidence: band=very_high; score=0.99
- Line 629: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Record conflict metric
  Confidence: band=very_high; score=0.99
- Line 631: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: metrics_->recordGossipConfigConflict("last_write_wins");
  Confidence: band=very_high; score=0.99
- Line 703: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool GossipConfigManager::shouldAcceptUpdate(const ConfigUpdate& update) {
  Confidence: band=very_high; score=0.99
- Line 789: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: *message.mutable_config_update() = update.toProto();
  Confidence: band=very_high; score=0.99
- Line 36: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t this_val = get(shard_id);
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t other_val = other.get(shard_id);
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t VectorClock::get(const std::string& shard_id) const {
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: gossip_thread_ = std::thread(&GossipConfigManager::gossipLoop, this);
  Confidence: band=very_high; score=0.9
- Line 233: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: anti_entropy_thread_ = std::thread(&GossipConfigManager::antiEntropyLoop, this);
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge vector clock
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergeVectorClock(VectorClock::fromProto(message.vector_clock()));
  Confidence: band=very_high; score=0.9
- Line 414: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void GossipConfigManager::mergeVectorClock(const VectorClock& other) {
  Confidence: band=very_high; score=0.9
- Line 36: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::merge(const VectorClock& other)
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=medium; score=0.56
- Line 42: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::compare(const VectorClock& other)
  Context: VectorClock::Ordering VectorClock::compare(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.warnings.push_back(warning);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> GossipConfigManager::getAllConfigs() const {
  Confidence: band=high; score=0.74
- Line 525: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(shard.shard_id);
  Confidence: band=high; score=0.74

### src/sharding/shard_rpc_client.cpp
Total findings: 30

- Line 324: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool ShardRPCClient::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 376: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json ShardRPCClient::snapshotRead(
  Confidence: band=very_high; score=0.9
- Line 687: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // snapshotRead() is the lightweight "point-in-time read" path which returns
  Confidence: band=very_high; score=0.9
- Line 716: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // snapshotRead() path we return an empty result set with metadata so the
  Confidence: band=very_high; score=0.9
- Line 41: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Maximum delay between retry attempts (milliseconds).  Both sendRequestGrpc
  Confidence: band=high; score=0.74
- Line 42: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // and sendRequestInProcess use this cap so that a single constant controls the
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: /// When non-null, sendRequestInProcess() delegates to this function instead
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   When THEMIS_HAS_SHARD_GRPC is 0, all sendRequest() calls are routed
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   to sendRequestInProcess() which returns hardcoded JSON responses.
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   simulation path (sendRequestInProcess) is retained as a fallback for
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::string(override_flag) == "1")
  Context: override_flag && std::string(override_flag) == "1") {
  Confidence: band=medium; score=0.56
- Line 279: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("prepare", params);
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("commit", params);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("abort", params);
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("compensate", params);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("snapshot_read", params);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("ping", nlohmann::json::object());
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("collect_wait_for_edges", nlohmann::json::object());
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back({waiting.get<std::string>(),
  Confidence: band=high; score=0.74
- Line 457: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto response = sendRequest("write_entity", params);
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: nlohmann::json ShardRPCClient::sendRequest(
  Confidence: band=high; score=0.74
- Line 472: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: return sendRequestGrpc(method, params);
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: return sendRequestInProcess(method, params);
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: nlohmann::json ShardRPCClient::sendRequestGrpc(
  Confidence: band=high; score=0.74
- Line 803: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_json.push_back({
  Confidence: band=high; score=0.74
- Line 841: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: nlohmann::json ShardRPCClient::sendRequestInProcess(
  Confidence: band=high; score=0.74
- Line 847: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Purpose: Provide a local in-process fallback for sendRequest() that
  Confidence: band=high; score=0.74
- Line 861: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //   The sendRequestInProcess() method is retained as a single-node fallback
  Confidence: band=high; score=0.74
- Line 880: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Re-check circuit breaker before every attempt (mirrors sendRequestGrpc).
  Confidence: band=high; score=0.74
- Line 971: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // Exponential backoff; same overflow-safe calculation as sendRequestGrpc
  Confidence: band=high; score=0.74

### src/sharding/stream_protocol.cpp
Total findings: 28

- Line 243: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), data.begin(), data.end());
  Confidence: band=very_high; score=0.99
- Line 921: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Start sessions (up to max_concurrent)
  Confidence: band=very_high; score=0.99
- Line 930: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: while (active_count < config_.max_concurrent_sessions &&
  Confidence: band=very_high; score=0.99
- Line 1163: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.chunk_index), sizeof(chunk.chunk_index));
  Confidence: band=very_high; score=0.99
- Line 1164: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.file_offset), sizeof(chunk.file_offset));
  Confidence: band=very_high; score=0.99
- Line 1165: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.uncompressed_size), sizeof(chunk.uncompressed_size));
  Confidence: band=very_high; score=0.99
- Line 1166: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.compressed_size), sizeof(chunk.compressed_size));
  Confidence: band=very_high; score=0.99
- Line 1167: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(&chunk.checksum), sizeof(chunk.checksum));
  Confidence: band=very_high; score=0.99
- Line 1168: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.write(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());
  Confidence: band=very_high; score=0.99
- Line 1402: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(write_data.data()), write_data.size());
  Confidence: band=very_high; score=0.99
- Line 595: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: session_thread_ = std::thread(&StreamSession::sessionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 596: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread(&StreamSession::heartbeatLoop, this);
  Confidence: band=very_high; score=0.9
- Line 615: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamSession::abort(const std::string& reason) {
  Confidence: band=very_high; score=0.9
- Line 856: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: bool StreamPlan::execute() {
  Confidence: band=very_high; score=0.9
- Line 861: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: executor_thread_ = std::thread(&StreamPlan::executorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 865: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamPlan::abort() {
  Confidence: band=very_high; score=0.9
- Line 997: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: transfer_thread_ = std::thread(&StreamTransferTask::transferLoop, this);
  Confidence: band=very_high; score=0.9
- Line 1010: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamTransferTask::abort() {
  Confidence: band=very_high; score=0.9
- Line 1103: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(chunk.data.data()), chunk.uncompressed_size);
  Confidence: band=very_high; score=0.9
- Line 1316: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void StreamReceiveTask::abort() {
  Confidence: band=very_high; score=0.9
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((file_offset >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.file_progress.push_back(file_progress);
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.file_progress.push_back(file_progress);
  Confidence: band=high; score=0.74
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_plans_.push_back(plan);
  Confidence: band=high; score=0.74
- Line 856: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool StreamPlan::execute() {
  Confidence: band=high; score=0.74
- Line 900: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.push_back(session->getProgress());
  Confidence: band=high; score=0.74
- Line 900: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: progress.push_back(session->getProgress());
  Confidence: band=high; score=0.74
- Line 1258: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::cerr << "Rejecting stale or duplicate chunk " << chunk.chunk_index
  Confidence: band=high; score=0.74

### src/sharding/adaptive_shard_router.cpp
Total findings: 27

- Line 256: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: already_queried.insert(shard_id);
  Confidence: band=very_high; score=0.99
- Line 319: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return merged_results;
  Confidence: band=very_high; score=0.99
- Line 531: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: nlohmann::json merged = nlohmann::json::array();
  Confidence: band=very_high; score=0.99
- Line 154: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return ShardRouter::executeQuery(query);
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return executeAdaptiveQuery(query, stats);
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: nlohmann::json AdaptiveShardRouter::executeAdaptiveQuery(
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return ShardRouter::executeQuery(query);
  Confidence: band=very_high; score=0.9
- Line 303: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge results from all iterations
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto merged_results = mergeIterationResults(all_iteration_results);
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged_results;
  Confidence: band=very_high; score=0.9
- Line 465: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (lhs.score != rhs.score) {
  Confidence: band=very_high; score=0.9
- Line 528: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json AdaptiveShardRouter::mergeIterationResults(
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: nlohmann::json merged = nlohmann::json::array();
  Confidence: band=very_high; score=0.9
- Line 540: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(item);
  Confidence: band=very_high; score=0.9
- Line 546: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 89: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int best_freshness_rank = 2;  // 0=fresh, 1=stale, 2=missing
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json AdaptiveShardRouter::executeQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.iteration_details.push_back(iter_stats);
  Confidence: band=high; score=0.74
- Line 455: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Skip stale topology entries (e.g., shard became unhealthy after scoring).
  Confidence: band=high; score=0.74
- Line 473: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(candidate.shard_id);
  Confidence: band=high; score=0.74
- Line 539: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 539: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 539: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(item);
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(match.score);
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(match.score);
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(match.score);
  Confidence: band=high; score=0.74

### src/sharding/distributed_transaction.cpp
Total findings: 27

- Line 811: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: committed_ids.insert(txn_id);
  Confidence: band=very_high; score=0.99
- Line 814: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: aborted_ids.insert(txn_id);
  Confidence: band=very_high; score=0.99
- Line 160: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool DistributedTransactionCoordinator::commit(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 333: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool DistributedTransactionCoordinator::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 387: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto shard_results = client.snapshotRead(snapshot_ts.count(), query);
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& participant : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& participant : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 924: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& participant : txn.participants) {
  Confidence: band=very_high; score=0.9
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: txn.participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> map)
  Confidence: band=medium; score=0.66
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &participant, &txn, &all_prepared, &error_mutex, &error_details]() {
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &participant, &txn, &all_committed, &error_mutex, &error_details]() {
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &participant, &txn, &all_committed, &error_mutex, &error_details]() {
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += err + "; ";
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovery_data["participants"].push_back({
  Confidence: band=high; score=0.74
- Line 750: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prepared_data["participants"].push_back({
  Confidence: band=high; score=0.74
- Line 844: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovery_txn.participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 844: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovery_txn.participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 927: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, p_ptr, &txn, &all_committed,
  Confidence: band=high; score=0.74
- Line 947: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += e + "; ";
  Confidence: band=high; score=0.74
- Line 947: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += e + "; ";
  Confidence: band=high; score=0.74
- Line 947: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: txn.error_detail += e + "; ";
  Confidence: band=high; score=0.74

### src/sharding/replica_consistency.cpp
Total findings: 25

- Line 30: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void VectorClock::update(const VectorClock& other) {
  Confidence: band=very_high; score=0.99
- Line 202: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return VersionConflict{};  // Empty conflict
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return entries[0];  // No conflict
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<LogEntry> merged;
  Confidence: band=very_high; score=0.99
- Line 36: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=very_high; score=0.9
- Line 47: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t other_timestamp = other.get(node_id);
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ReplicaConsistencyManager::mergeReplicas(
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: stats_.merges_performed++;
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (config_.auto_resolve_conflicts) {
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto resolved = autoResolveConflict(*conflict);
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void ReplicaConsistencyManager::resolveConflict(
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<LogEntry> ReplicaConsistencyManager::mergePartitionedLogs(
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge logs using term and index
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<LogEntry> merged;
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(remote);
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(remote_entries[remote_idx++]);
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 321: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: conflict.needs_manual_resolution = !config_.auto_resolve_conflicts;
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: VersionedEntry ReplicaConsistencyManager::autoResolveConflict(
  Confidence: band=very_high; score=0.9
- Line 23: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: VectorClock::VectorClock(const std::map<std::string, uint64_t>& timestamps)
  Confidence: band=high; score=0.74
- Line 26: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::increment(const std::string& node_id)
  Context: void VectorClock::increment(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 36: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::get(const std::string& node_id)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=medium; score=0.56
- Line 41: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::happensBefore(const VectorClock& other)
  Context: bool VectorClock::happensBefore(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 73: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::isConcurrent(const VectorClock& other)
  Context: bool VectorClock::isConcurrent(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 99: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, uint64_t> timestamps;
  Confidence: band=high; score=0.74

### src/sharding/signed_request.cpp
Total findings: 25

- Line 87: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: BIO_write(bio.get(), data, static_cast<int>(len));
  Confidence: band=very_high; score=0.99
- Line 289: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
  Confidence: band=very_high; score=0.99
- Line 555: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
  Confidence: band=very_high; score=0.99
- Line 87: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: BIO_write(bio.get(), data, static_cast<int>(len));
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: BIO_flush(bio.get());
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: BIO_get_mem_ptr(bio.get(), &buffer_ptr);
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: int decoded_len = BIO_read(bio.get(), decoded.data(), static_cast<int>(decoded.size()));
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignFinal(md_ctx.get(), nullptr, &sig_len) != 1) {
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestSignFinal(md_ctx.get(), signature.data(), &sig_len) != 1) {
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // between weakly_canonical() and the read (the cert_path variable retains
  Confidence: band=very_high; score=0.9
- Line 503: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cert = utils::read_x509_from_bio(bio.get());
  Confidence: band=very_high; score=0.9
- Line 534: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto pubkey = utils::EVPKeyPtr(X509_get_pubkey(cert.get()));
  Confidence: band=very_high; score=0.9
- Line 541: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const int pubkey_type = EVP_PKEY_base_id(pubkey.get());
  Confidence: band=very_high; score=0.9
- Line 543: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, nullptr, nullptr, pubkey.get()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 546: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return EVP_DigestVerify(md_ctx.get(),
  Confidence: band=very_high; score=0.9
- Line 552: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pubkey.get()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 555: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
  Confidence: band=very_high; score=0.9
- Line 558: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return EVP_DigestVerifyFinal(md_ctx.get(), signature_bytes->data(), signature_bytes->size()) == 1;
  Confidence: band=very_high; score=0.9
- Line 378: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Fail-closed: reject stale requests outside replay window.
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Note on TOCTOU: there is an inherent window between weakly_canonical() and
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: canonical_dir  = fs::weakly_canonical(fs::path(config_.trusted_certs_dir));
  Confidence: band=high; score=0.74
- Line 471: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: canonical_cert = fs::weakly_canonical(cert_path);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // between weakly_canonical() and the read (the cert_path variable retains
  Confidence: band=high; score=0.74

### src/sharding/capability_matcher.cpp
Total findings: 22

- Line 233: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: query_set.insert(normalize(kw));
  Confidence: band=very_high; score=0.99
- Line 238: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: normalized_shard_kw.insert(normalize(kw));
  Confidence: band=very_high; score=0.99
- Line 308: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (query_magnitude == 0.0 || shard_magnitude == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(match_result);
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> query_tfidf;
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> shard_tfidf;
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_keywords.push_back(term);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_keywords.push_back(term);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_domains.push_back(qd);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_domains.push_back(qd);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_domains.push_back(qd);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_orgs.push_back(qo);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_orgs.push_back(qo);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_orgs.push_back(qo);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_regions.push_back(qr);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_regions.push_back(qr);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_regions.push_back(qr);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_types.push_back(qt);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_types.push_back(qt);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_types.push_back(qt);
  Confidence: band=high; score=0.74
- Line 204: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((total_shards_ + config_.idf_smoothing) /
  Confidence: band=medium; score=0.6
- Line 448: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return std::log(total_shards_ + config_.idf_smoothing);
  Confidence: band=medium; score=0.6

### src/sharding/prometheus_metrics.cpp
Total findings: 20

- Line 88: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordResultMergeTime(double time_ms) {
  Confidence: band=very_high; score=0.99
- Line 89: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
  Confidence: band=very_high; score=0.99
- Line 132: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void PrometheusMetrics::recordGossipConfigUpdate(const std::string& operation) {
  Confidence: band=very_high; score=0.99
- Line 140: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordGossipConfigConflict(const std::string& resolution_type) {
  Confidence: band=very_high; score=0.99
- Line 141: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: incrementCounter("themis_gossip_config_conflicts_total", {{"resolution", resolution_type}});
  Confidence: band=very_high; score=0.99
- Line 574: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordPaxosProposalConflict(const std::string& shard_id) {
  Confidence: band=very_high; score=0.99
- Line 575: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: incrementCounter("themis_paxos_proposal_conflicts_total", {{"shard_id", shard_id}});
  Confidence: band=very_high; score=0.99
- Line 683: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void PrometheusMetrics::recordPercolatorConflict(const std::string& transaction_id) {
  Confidence: band=very_high; score=0.99
- Line 684: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: incrementCounter("themis_percolator_conflicts_total", {{"transaction_id", transaction_id}});
  Confidence: band=very_high; score=0.99
- Line 731: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void PrometheusMetrics::recordMvccWrite(double latency_ms) {
  Confidence: band=very_high; score=0.99
- Line 88: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void PrometheusMetrics::recordResultMergeTime(double time_ms) {
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
  Confidence: band=very_high; score=0.9
- Line 736: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void PrometheusMetrics::recordMvccRead(const std::string& read_type, double latency_ms) {
  Confidence: band=very_high; score=0.9
- Line 29: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: incrementCounter("themis_cross_shard_rpc_calls_total",
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p50 = sorted[sorted.size() * 50 / 100];
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p95 = sorted[sorted.size() * 95 / 100];
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p99 = sorted[sorted.size() * 99 / 100];
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p50 = sorted[sorted.size() * 50 / 100];
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p95 = sorted[sorted.size() * 95 / 100];
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p99 = sorted[sorted.size() * 99 / 100];
  Confidence: band=high; score=0.74

### src/sharding/pki_shard_certificate.cpp
Total findings: 19

- Line 46: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ASN1_TIME_print(bio.get(), time);
  Confidence: band=very_high; score=0.9
- Line 49: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: long len = BIO_get_mem_data(bio.get(), &data);
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cert = utils::read_x509_from_bio(bio.get());
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: X509_NAME* subject = X509_get_subject_name(cert.get());
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: X509_NAME* issuer = X509_get_issuer_name(cert.get());
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ASN1_INTEGER* serial = X509_get_serialNumber(cert.get());
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: char* hex = BN_bn2hex(bn.get());
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const ASN1_TIME* not_before = X509_get0_notBefore(cert.get());
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const ASN1_TIME* not_after = X509_get0_notAfter(cert.get());
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: parseSAN(cert.get(), info);
  Confidence: band=very_high; score=0.9
- Line 193: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: parseCustomExtensions(cert.get(), info);
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cert = utils::read_x509_from_bio(cert_bio.get());
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto ca_cert = utils::read_x509_from_bio(ca_bio.get());
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto ca_pubkey = utils::EVPKeyPtr(X509_get_pubkey(ca_cert.get()));
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: int result = X509_verify(cert.get(), ca_pubkey.get());
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto crl = utils::read_x509_crl_from_bio(bio.get());
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: STACK_OF(X509_REVOKED)* revoked = X509_CRL_get_REVOKED(crl.get());
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: char* hex = BN_bn2hex(bn.get());
  Confidence: band=very_high; score=0.9
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.san_dns.push_back(dns_str);
  Confidence: band=high; score=0.74

### src/sharding/slo_monitor.cpp
Total findings: 19

- Line 114: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double SLOWindow::getErrorBudget(double target_availability) const {
  Confidence: band=very_high; score=0.9
- Line 263: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double SLOMonitor::getErrorBudget(const std::string& shard_id) const {
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return it->second->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double SLOMonitor::getGlobalErrorBudget() const {
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: total_budget += window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double budget = getErrorBudget(shard_id);
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double error_budget = window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 363: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: slo["error_budget"] = window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: compliance["error_budget"] = getGlobalErrorBudget();
  Confidence: band=very_high; score=0.9
- Line 511: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double error_budget = window->getErrorBudget(config_.targets.availability_target);
  Confidence: band=very_high; score=0.9
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: availability_slos.push_back(slo);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latency_slos.push_back(slo);
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consistency_slos.push_back(slo);
  Confidence: band=high; score=0.74
- Line 400: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> SLOMonitor::getSLOCompliance() const {
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> compliance;
  Confidence: band=high; score=0.74
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(prog);
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(
  Confidence: band=high; score=0.74
- Line 537: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(
  Confidence: band=high; score=0.74

### src/sharding/gossip_protocol.cpp
Total findings: 17

- Line 475: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
  Confidence: band=very_high; score=0.99
- Line 614: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (EVP_DigestSignUpdate(ctx, to_sign.c_str(), to_sign.length()) == 1) {
  Confidence: band=very_high; score=0.99
- Line 712: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: EVP_DigestVerifyUpdate(ctx, to_verify.data(), to_verify.size()) == 1 &&
  Confidence: band=very_high; score=0.99
- Line 63: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: gossip_thread_ = std::thread(&GossipProtocol::gossipLoop, this);
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread(&GossipProtocol::cleanupLoop, this);
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge peer list
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergePeerList(peers);
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void GossipProtocol::mergePeerList(const std::vector<PeerInfo>& peers) {
  Confidence: band=very_high; score=0.9
- Line 547: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // confirmed through Raft joint-consensus).  Without a gate (backward compat),
  Confidence: band=high; score=0.8
- Line 88: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, PeerInfo> GossipProtocol::getPeers() const {
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy.push_back(peer);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers.push_back(PeerInfo::fromJson(p));
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers.push_back(peer);
  Confidence: band=high; score=0.74
- Line 460: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(peer);
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(id);
  Confidence: band=high; score=0.74
- Line 513: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Remove stale peers
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers_json.push_back(peer.toJson());
  Confidence: band=high; score=0.74

### src/sharding/paxos_consensus.cpp
Total findings: 16

- Line 95: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: proposer_thread_ = std::thread(&PaxosConsensus::runProposer, this);
  Confidence: band=very_high; score=0.9
- Line 96: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: acceptor_thread_ = std::thread(&PaxosConsensus::runAcceptor, this);
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: learner_thread_ = std::thread(&PaxosConsensus::runLearner, this);
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread(&PaxosConsensus::leaderElectionThread, this);
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: committed_log_.find(slot) == committed_log_.end()) {
  Confidence: band=very_high; score=0.9
- Line 516: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (instance.is_committed && committed_log_.find(slot) == committed_log_.end()) {
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void PaxosConsensus::leaderElectionThread() {
  Confidence: band=very_high; score=0.9
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_slots.push_back(slot);
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_slots.push_back(slot);
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // are now stale (i.e. the proposer timed out and a higher ballot has been
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Evict stale promises: if the promised round is far behind the
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: constexpr uint64_t kStaleRoundThreshold = 10;
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cur_round > instance.promised_proposal.round + kStaleRoundThreshold) {
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::debug("Acceptor: evicting stale promise for slot {} "
  Confidence: band=high; score=0.74

### src/sharding/metadata_shard.cpp
Total findings: 15

- Line 173: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: cache_->put(cache_key, entry_it->second.toJson());
  Confidence: band=very_high; score=0.99
- Line 179: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool MetadataShard::put(
  Confidence: band=very_high; score=0.99
- Line 389: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: cache_->put(cache_key, entry.toJson());
  Confidence: band=very_high; score=0.99
- Line 491: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool MetadataShardRouter::put(
  Confidence: band=very_high; score=0.99
- Line 507: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: return it->second->put(partition, key, value);
  Confidence: band=very_high; score=0.99
- Line 140: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<MetadataEntry> MetadataShard::get(
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cached = cache_->get(cache_key);
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cached = cache_->get(cache_key);
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<MetadataEntry> MetadataShardRouter::get(
  Confidence: band=very_high; score=0.9
- Line 488: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return it->second->get(partition, key);
  Confidence: band=very_high; score=0.9
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partition_stats.push_back(getPartitionStats(partition));
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_stats.push_back(pair.second->getStatistics());
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_stats.push_back(pair.second->getStatistics());
  Confidence: band=high; score=0.74
- Line 624: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, MetadataEntry> partition_entries;
  Confidence: band=high; score=0.74

### src/sharding/shard_load_detector.cpp
Total findings: 14

- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: storage_values.push_back(storage_load);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_ids[i]);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: request_rates.push_back(static_cast<double>(metrics.requests_per_sec));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_ids[i]);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(metrics.p99_latency_ms);
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_ids[i]);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.reason.empty()) result.reason += "; ";
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.hotspot_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: load_rankings.push_back({shard_id, load});
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cpu_series.push_back(sample.cpu_usage_percent);
  Confidence: band=high; score=0.74

### src/sharding/cloud_agent.cpp
Total findings: 12

- Line 62: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: worker_thread_ = std::thread(&CloudAgent::workerLoop, this);
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_thread_ = std::thread(&CloudAgent::healthLoop, this);
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = executor_->get(shard, "/health");
  Confidence: band=very_high; score=0.9
- Line 572: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto [shard_id, shard_result] = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 113: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: operation.on_complete(result.result);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(op_id);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: operation.on_complete(result.result);
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_shards.push_back(shard.shard_id);
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_shard_ids.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated_result.push_back(shard_result["data"]);
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated_result.push_back(shard_result["data"]);
  Confidence: band=high; score=0.74

### src/sharding/hot_spare_manager.cpp
Total findings: 12

- Line 47: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (max_concurrent_rebuilds == 0) {
  Confidence: band=very_high; score=0.99
- Line 48: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: spdlog::error("Invalid max_concurrent_rebuilds: must be > 0");
  Confidence: band=very_high; score=0.99
- Line 471: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: history.insert(history.end(), start_it, failover_history_.end());
  Confidence: band=very_high; score=0.99
- Line 657: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (active_rebuilds >= config_.max_concurrent_rebuilds) {
  Confidence: band=very_high; score=0.99
- Line 14: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * with a compatibility shim for themisdb::sharding at the end,
  Confidence: band=high; score=0.8
- Line 101: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_check_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 107: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: rebuild_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 763: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < task.documents.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 896: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility shim
  Confidence: band=high; score=0.8
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spares.push_back(spare);
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.rebuilding_spares.push_back(spare);
  Confidence: band=high; score=0.74

### src/sharding/predictive_detector.cpp
Total findings: 12

- Line 139: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitoring_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& shard : shards) {
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<double> latencies, throughputs, error_rates;
  Confidence: band=very_high; score=0.9
- Line 346: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: throughputs.push_back(static_cast<double>(m.throughput_ops_per_sec));
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: features.push_back(compute_mean(throughputs));
  Confidence: band=very_high; score=0.9
- Line 397: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: features.push_back(compute_stddev(throughputs));
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: features.push_back(compute_trend(throughputs));
  Confidence: band=very_high; score=0.9
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predictions.push_back(prediction);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: high_risk.push_back(prediction);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(metrics);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(m.avg_latency_ms);
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(compute_mean(latencies));
  Confidence: band=high; score=0.74

### src/sharding/raft_log.cpp
Total findings: 12

- Line 319: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.99
- Line 322: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.99
- Line 330: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(checksum.data(), static_cast<std::streamsize>(checksum_len));
  Confidence: band=very_high; score=0.99
- Line 332: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(reinterpret_cast<const char*>(compressed.data()),
  Confidence: band=very_high; score=0.99
- Line 390: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.9
- Line 399: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(&v), sizeof(v));
  Confidence: band=very_high; score=0.9
- Line 439: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(snap.checksum.data(), static_cast<std::streamsize>(checksum_len));
  Confidence: band=very_high; score=0.9
- Line 454: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(snap.data.data()),
  Confidence: band=very_high; score=0.9
- Line 580: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(chunk.data.data()),
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(std::stoull(id_str));
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(std::stoull(id_str));
  Confidence: band=high; score=0.74

### src/sharding/wal_manager.cpp
Total findings: 12

- Line 80: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), data_str.begin(), data_str.end());
  Confidence: band=very_high; score=0.99
- Line 205: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<WALEntry> WALManager::read(const LSN& lsn) {
  Confidence: band=very_high; score=0.9
- Line 240: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(buffer.data()), file_size);
  Confidence: band=very_high; score=0.9
- Line 39: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALEntry::serialize()
  Context: std::vector<uint8_t> WALEntry::serialize() const {
  Confidence: band=medium; score=0.56
- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((timestamp >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((lsn.segment >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((lsn.offset >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((tx_id_len >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back((data_len >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALManager::append(const WALEntry& entry)
  Context: LSN WALManager::append(const WALEntry& entry) {
  Confidence: band=medium; score=0.56
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_num);
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back({seg_num, entry.path().string()});
  Confidence: band=high; score=0.74

### src/sharding/auto_rebalancer.cpp
Total findings: 11

- Line 160: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: THEMIS_INFO("AutoRebalancer initialized with check_interval={}s, max_concurrent={}",
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: config_.check_interval.count() / 1000, config_.max_concurrent_operations);
  Confidence: band=very_high; score=0.99
- Line 240: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Check max concurrent operations
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (active_operations_.size() >= config_.max_concurrent_operations) {
  Confidence: band=very_high; score=0.99
- Line 244: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: THEMIS_WARN("Max concurrent operations reached, queuing remaining");
  Confidence: band=very_high; score=0.99
- Line 176: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitor_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : imbalance.recommendations) {
  Confidence: band=very_high; score=0.9
- Line 71: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!proposal.reason.empty()) proposal.reason += ", ";
  Confidence: band=high; score=0.74
- Line 547: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: completed_ids.push_back(op_id);
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: statuses.push_back(status);
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: statuses.push_back(status);
  Confidence: band=high; score=0.74

### src/sharding/raft_consensus.cpp
Total findings: 10

- Line 70: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // atomically under replica_mutex_ so that a concurrent step-down cannot
  Confidence: band=very_high; score=0.99
- Line 37: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread(&RaftConsensus::heartbeatLoop, this);
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread(&RaftConsensus::electionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 41: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: partition_detector_thread_ = std::thread(&RaftConsensus::partitionDetectionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::thread([self, captured_entry, cb, promise]() {
  Confidence: band=very_high; score=0.9
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: states.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hb.reachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.reachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status.partition_id += ":" + node;
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status.partition_id += ":" + node;
  Confidence: band=high; score=0.74

### src/sharding/shard_resource_manager.cpp
Total findings: 10

- Line 272: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void ShardResourceManager::broadcastResourceUpdate() {
  Confidence: band=very_high; score=0.99
- Line 313: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void ShardResourceManager::receiveResourceUpdate(const std::string& shard_id,
  Confidence: band=very_high; score=0.99
- Line 435: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: broadcastResourceUpdate();
  Confidence: band=very_high; score=0.99
- Line 171: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitoring_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 100: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = j["timestamp_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ShardResourceManager::ResourceSnapshot>
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_peers.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overloaded_peers.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: cleanupStaleSnapshots();
  Confidence: band=high; score=0.74
- Line 473: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void ShardResourceManager::cleanupStaleSnapshots() {
  Confidence: band=high; score=0.74

### src/sharding/consistent_hash.cpp
Total findings: 9

- Line 150: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 222: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 231: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(it->second);
  Confidence: band=very_high; score=0.99
- Line 50: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: vnode_key += '#';
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(token);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::optional<std::string> ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66

### src/sharding/data_migrator.cpp
Total findings: 9

- Line 436: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_migrations_.insert(migration_id);
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_batches_.insert(batch_id);
  Confidence: band=very_high; score=0.99
- Line 476: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_migrations_.insert(item.get<std::string>());
  Confidence: band=very_high; score=0.99
- Line 492: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: completed_batches_.insert(item.get<std::string>());
  Confidence: band=very_high; score=0.99
- Line 106: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto resp = count_client->get(config_.source_endpoint, count_path.str());
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Write batch to target (atomic operation)
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
  Confidence: band=very_high; score=0.9
- Line 516: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: migrations_json.push_back(migration_id);
  Confidence: band=high; score=0.74
- Line 526: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches_json.push_back(batch_id);
  Confidence: band=high; score=0.74

### src/sharding/health_monitor.cpp
Total findings: 9

- Line 78: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Fallback to dedicated thread (backward compatibility)
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fallback to dedicated thread (backward compatibility)
  Confidence: band=high; score=0.8
- Line 79: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitor_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& primary : primaries) {
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& replica_id : replica_set->replicas) {
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto future = http_pool_->get(url);
  Confidence: band=very_high; score=0.9
- Line 401: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = future.get();
  Confidence: band=very_high; score=0.9
- Line 126: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, HealthCheckResult> HealthMonitor::getAllHealthStatuses() const {
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(primary.node_id);
  Confidence: band=high; score=0.74

### src/sharding/quorum_manager.cpp
Total findings: 9

- Line 113: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: QuorumResult QuorumManager::executeRead(ReadOperation operation,
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: T result = future.get();
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back({node, std::move(future)});
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successful_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back({node, std::move(future)});
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back({node, std::move(future)});
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successful_nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({node, result});
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({node, result});
  Confidence: band=high; score=0.74

### src/sharding/shard_topology.cpp
Total findings: 9

- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard.replica_endpoints.push_back(ep.get<std::string>());
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard.capabilities.push_back(cap.get<std::string>());
  Confidence: band=high; score=0.74
- Line 380: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaders.push_back(id);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66

### src/sharding/gpu_erasure_coder_opencl.cpp
Total findings: 8

- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& c : parity_chunks) result.push_back(std::move(c));
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe_chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe_chunks.push_back(std::move(pchunk));
  Confidence: band=high; score=0.74
- Line 592: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74
- Line 264: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::string log(log_size, '\0');
  Confidence: band=medium; score=0.6

### src/sharding/mtls_connection_pool.cpp
Total findings: 8

- Line 106: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: active_connections_.insert(pooled.ssl.get());
  Confidence: band=very_high; score=0.99
- Line 42: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread([this]() { cleanupLoop(); });
  Confidence: band=very_high; score=0.9
- Line 70: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: } else if (pooled.ssl && validateConnection(pooled.ssl.get())) {
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: active_connections_.insert(pooled.ssl.get());
  Confidence: band=very_high; score=0.9
- Line 84: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: active_connections_.insert(new_conn->get());
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (pooled.ssl && validateConnection(pooled.ssl.get())) {
  Confidence: band=very_high; score=0.9
- Line 106: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: active_connections_.insert(pooled.ssl.get());
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: SSL* raw_ptr = connection.get();
  Confidence: band=very_high; score=0.9

### src/sharding/multi_primary_coordinator.cpp
Total findings: 8

- Line 151: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: LSN MultiPrimaryCoordinator::resolveConflict(const WriteConflict& conflict) const {
  Confidence: band=very_high; score=0.99
- Line 152: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_++;
  Confidence: band=very_high; score=0.99
- Line 155: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return conflict.resolveLastWriteWins();
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return (conflict.lsn2 > conflict.lsn1) ? conflict.lsn2 : conflict.lsn1;
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void MultiPrimaryCoordinator::recordWrite(const LSN& lsn) {
  Confidence: band=very_high; score=0.99
- Line 197: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.99
- Line 197: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: stats.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74

### src/sharding/wal_applier.cpp
Total findings: 8

- Line 172: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool WALApplier::handleConflict(const WALEntry& entry) {
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (!config_.enable_conflict_detection) {
  Confidence: band=very_high; score=0.99
- Line 174: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return true;  // Conflicts ignored
  Confidence: band=very_high; score=0.99
- Line 179: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats_.conflicts_detected++;
  Confidence: band=very_high; score=0.99
- Line 182: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Conflict resolution strategy (can be extended)
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::cerr << "WALApplier: Conflict detected for entry at LSN "
  Confidence: band=very_high; score=0.99
- Line 47: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::string error = "LSN stale or duplicate: current " + current_lsn_.toString() +
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(error);
  Confidence: band=high; score=0.74

### src/sharding/epoch_fencing.cpp
Total findings: 7

- Line 391: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<LeaseRecord> LeaseManager::get(const LeaseKey& key) const {
  Confidence: band=very_high; score=0.9
- Line 151: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Stale epoch
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("[EpochFencing] STALE_EPOCH from='{}' token_epoch={} current={}",
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++metrics_.stale_rejections;
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return issueStonith(source_node, "stale_epoch");
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return FencingResult::STALE_EPOCH;
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74

### src/sharding/remote_executor.cpp
Total findings: 7

- Line 62: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: RemoteExecutor::Result RemoteExecutor::put(const ShardInfo& shard_info,
  Confidence: band=very_high; score=0.99
- Line 198: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: response = mtls_client_->put(endpoint, path, request_body);
  Confidence: band=very_high; score=0.99
- Line 51: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RemoteExecutor::Result RemoteExecutor::get(const ShardInfo& shard_info,
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RemoteExecutor::Result RemoteExecutor::executeQuery(const ShardInfo& shard_info,
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: response = mtls_client_->get(endpoint, path);
  Confidence: band=very_high; score=0.9
- Line 100: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto b0 = data[i];
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: encoded += (i + 1 < size) ? kBase64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
  Confidence: band=high; score=0.74

### src/sharding/sharding_manager_edition.cpp
Total findings: 7

- Line 93: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const auto info = edition::EditionInfo::Get();
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const auto info = edition::EditionInfo::Get();
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const auto info = edition::EditionInfo::Get();
  Confidence: band=very_high; score=0.9
- Line 95: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += " | Max Shard Nodes: ";
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += " | Max Shard Nodes: ";
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(all_shards[idx]);
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(all_shards[idx]);
  Confidence: band=high; score=0.74

### src/sharding/truetime.cpp
Total findings: 7

- Line 50: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: startSyncThread();
  Confidence: band=very_high; score=0.9
- Line 55: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: stopSyncThread();
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void TrueTime::startSyncThread() {
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sync_thread_ = std::thread(&TrueTime::syncThreadFunc, this);
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void TrueTime::stopSyncThread() {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: SocketHandle get() const { return fd_; }
  Confidence: band=very_high; score=0.9
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: offsets.push_back(offset);
  Confidence: band=high; score=0.74

### src/sharding/two_phase_commit_coordinator.cpp
Total findings: 7

- Line 422: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // touching shared state, so that concurrent coordinator operations are
  Confidence: band=very_high; score=0.99
- Line 89: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: participants_[shard_id] = adapter.get();
  Confidence: band=very_high; score=0.9
- Line 106: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, nlohmann::json>& ops_per_shard
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& [s, _] : ops_per_shard) v.push_back(s);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, CoordinatorTxnRecord> recovered;
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool>                 decisions; // txn_id → commit?
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec.phase2_acked.push_back(shard_id);
  Confidence: band=high; score=0.74

### src/sharding/wal_shipper.cpp
Total findings: 7

- Line 126: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& replica_id : replica_ids) {
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = mtls_client_->get(replica.endpoint, "/api/v1/health");
  Confidence: band=very_high; score=0.9
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replica_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_json.push_back(entry_json);
  Confidence: band=high; score=0.74
- Line 538: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < data.size()) ? kB64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
  Confidence: band=high; score=0.74
- Line 538: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < data.size()) ? kB64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
  Confidence: band=high; score=0.74

### src/sharding/cloud_backup.cpp
Total findings: 6

- Line 433: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Purpose: Preserve Azure provider list API compatibility before SDK-backed
  Confidence: band=high; score=0.8
- Line 930: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool setReplicationTarget(const std::string& datacenter_id,
  Confidence: band=very_high; score=0.9
- Line 1097: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool CloudBackupCoordinator::setReplicationTarget(const std::string& datacenter_id,
  Confidence: band=very_high; score=0.9
- Line 1099: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return impl_->setReplicationTarget(datacenter_id, shard_endpoints);
  Confidence: band=very_high; score=0.9
- Line 909: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backups.push_back(entry.second);
  Confidence: band=high; score=0.74
- Line 913: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (newest first)
  Confidence: band=high; score=0.74

### src/sharding/locality_aware_router.cpp
Total findings: 6

- Line 331: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = placement_cache_.find(cache_key);
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: it->second.find(shard_id) != it->second.end()) {
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string LocalityAwareRouter::routeQuery(const QuerySpec& spec) {
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<std::string> LocalityAwareRouter::routeMultiShardQuery(const QuerySpec& spec) {
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(affinity.shard_id);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(affinity);
  Confidence: band=high; score=0.74

### src/sharding/operational_metrics.cpp
Total findings: 6

- Line 300: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: {"conflicts", metrics->transaction_conflicts.load()}
  Confidence: band=very_high; score=0.99
- Line 536: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool had_conflict
  Confidence: band=very_high; score=0.99
- Line 46: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return it->second.get();
  Confidence: band=very_high; score=0.9
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (metrics->min_latency_us.compare_exchange_weak(current_min, latency_us)) {
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (metrics->max_latency_us.compare_exchange_weak(current_max, latency_us)) {
  Confidence: band=high; score=0.74

### src/sharding/orphan_detector.cpp
Total findings: 6

- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: orphaned_txns.push_back(txn.transaction_id);
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Skip transactions that haven't yet hit the stale threshold.
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("OrphanDetector: Reclaiming stale Percolator lock for txn {} "
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("OrphanDetector: Stale Percolator lock reclaimed for txn {}",
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::warn("OrphanDetector: Failed to abort stale Percolator txn {}",
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: spdlog::info("OrphanDetector::cleanPercolatorLocks: reclaimed {} stale lock(s)",
  Confidence: band=high; score=0.74

### src/sharding/gossip_consensus_adapter.cpp
Total findings: 5

- Line 68: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: gossip_thread_ = std::thread(&GossipConsensusAdapter::gossipThread, this);
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: void GossipConsensusAdapter::gossipThread() {
  Confidence: band=very_high; score=0.9
- Line 354: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [index, entry] : log_entries_) {
  Confidence: band=very_high; score=0.9
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74

### src/sharding/hardware_migration_manager.cpp
Total findings: 5

- Line 200: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> before_vnodes;
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t>
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t>
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> snapshot;
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, size_t>&  before_vnode_counts
  Confidence: band=high; score=0.74

### src/sharding/health_check.cpp
Total findings: 5

- Line 125: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::thread([this, shard_endpoints]() {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = client.get(endpoint, "/api/v1/metrics/storage");
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: response = client.get(endpoint, "/health");
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto response = client.get(endpoint, "/health");
  Confidence: band=very_high; score=0.9
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cluster_info.shard_health.push_back(shard_health);
  Confidence: band=high; score=0.74

### src/sharding/mtls_client.cpp
Total findings: 5

- Line 123: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: MTLSClient::Response MTLSClient::put(const std::string& endpoint,
  Confidence: band=very_high; score=0.99
- Line 214: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: http::write(stream, req);
  Confidence: band=very_high; score=0.99
- Line 105: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: MTLSClient::Response MTLSClient::get(const std::string& endpoint, const std::string& path) {
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: req.target(path);
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::read(stream, buffer, res);
  Confidence: band=very_high; score=0.9

### src/sharding/partition_detector.cpp
Total findings: 5

- Line 33: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_check_thread_ = std::thread(&PartitionDetector::healthCheckLoop, this);
  Confidence: band=very_high; score=0.9
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_ids.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unreachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unreachable_nodes.push_back(pair.first);
  Confidence: band=high; score=0.74

### src/sharding/raft_shard_manager.cpp
Total findings: 5

- Line 145: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::future<bool> RaftShardManager::proposeWrite(const std::string& shard_id,
  Confidence: band=very_high; score=0.99
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.replica_ids.push_back(replica.node_id);
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ShardRaftInfo> RaftShardManager::getAllShardRaftInfo() const {
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ShardRaftInfo> all_info;
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.replica_ids.push_back(replica.node_id);
  Confidence: band=high; score=0.74

### src/sharding/shard_repair_engine.cpp
Total findings: 5

- Line 62: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: scan_thread_ = std::thread([this]() { scanLoop(); });
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: repair_thread_ = std::thread([this]() { repairLoop(); });
  Confidence: band=very_high; score=0.9
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reports.push_back(report);
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: band_futures.push_back(promise->get_future());
  Confidence: band=high; score=0.74
- Line 497: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::info("ShardRepairEngine: parallel anti-entropy scan complete ({} shards, {} workers)",
  Confidence: band=high; score=0.74

### src/sharding/distributed_coordinator.cpp
Total findings: 4

- Line 102: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 111: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Start heartbeat thread (only active if leader)
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: task_executor_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9

### src/sharding/paxos_snapshot.cpp
Total findings: 4

- Line 247: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: file.write(text.data(), static_cast<std::streamsize>(text.size()));
  Confidence: band=very_high; score=0.99
- Line 293: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(magic, 4);
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: file.read(reinterpret_cast<char*>(compressed.data()),
  Confidence: band=very_high; score=0.9
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshots.push_back(snapshot_id);
  Confidence: band=high; score=0.74

### src/sharding/raft_wal_integration.cpp
Total findings: 4

- Line 185: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: // Wake up write() waiters whenever a new entry reaches quorum.
  Confidence: band=very_high; score=0.99
- Line 70: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = pending_writes_.find(log_index);
  Confidence: band=very_high; score=0.9
- Line 83: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<WALEntry> RaftWALIntegration::read(const LSN& lsn) {
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return config_.wal_manager->read(lsn);
  Confidence: band=very_high; score=0.9

### src/sharding/shard_rpc_server.cpp
Total findings: 4

- Line 334: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: builder.RegisterService(impl_->service.get());
  Confidence: band=very_high; score=0.9
- Line 318: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::string(override_flag) == "1")
  Context: override_flag && std::string(override_flag) == "1") {
  Confidence: band=medium; score=0.56
- Line 334: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: builder.RegisterService(impl_->service.get());
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: impl_->service.reset();
  Confidence: band=high; score=0.74

### src/sharding/metadata_wal.cpp
Total findings: 3

- Line 59: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: LSN MetadataWAL::logPut(
  Confidence: band=very_high; score=0.99
- Line 94: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: LSN MetadataWAL::logUpdate(
  Confidence: band=very_high; score=0.99
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(MetadataWALEntry::fromWALEntry(wal_entry));
  Confidence: band=high; score=0.74

### src/sharding/raft_consensus_adapter.cpp
Total findings: 3

- Line 496: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // against any concurrent state change.
  Confidence: band=very_high; score=0.99
- Line 351: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function consensus without trace point
  Context: spdlog::info("Node {} added to cluster via joint consensus (endpoint: {})",
  Confidence: band=very_high; score=0.9
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(convertLogEntry(entry));
  Confidence: band=high; score=0.74

### src/sharding/paxos_wal.cpp
Total findings: 2

- Line 31: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // We use type 100+ for Paxos entries to avoid conflicts
  Confidence: band=very_high; score=0.99
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paxos_entries.push_back(std::move(paxos_entry));
  Confidence: band=high; score=0.74

### src/sharding/shard_durability.cpp
Total findings: 2

- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checkpoints_.push_back(info);
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checkpoints_.push_back(info);
  Confidence: band=high; score=0.74

### src/sharding/urn_resolver.cpp
Total findings: 2

- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*replica);
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(s.shard_id);
  Confidence: band=high; score=0.74

### src/sharding/admin_api.cpp
Total findings: 1

- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(std::move(shard_entry));
  Confidence: band=high; score=0.74

### src/sharding/admin_operations.cpp
Total findings: 1

- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["shards"].push_back(shard_json);
  Confidence: band=high; score=0.74

### src/sharding/circuit_breaker.cpp
Total findings: 1

- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(shard_id);
  Confidence: band=high; score=0.74

### src/sharding/gpu_erasure_coder.cpp
Total findings: 1

- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(encode(block, data_shards, parity_shards));
  Confidence: band=high; score=0.74

### src/sharding/metadata_snapshot.cpp
Total findings: 1

- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot_ids.push_back(snapshot_id);
  Confidence: band=high; score=0.74

### src/sharding/raft_state.cpp
Total findings: 1

- Line 269: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // 3. If an existing entry conflicts with a new one (same index but different terms),
  Confidence: band=very_high; score=0.99

### src/sharding/replica_topology.cpp
Total findings: 1

- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replica_set.replicas.push_back(replica.get<std::string>());
  Confidence: band=high; score=0.74

### src/sharding/transaction_snapshot.cpp
Total findings: 1

- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot_ids.push_back(snapshot_id);
  Confidence: band=high; score=0.74

### src/sharding/transaction_wal.cpp
Total findings: 1

- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(txn_entry.value());
  Confidence: band=high; score=0.74

### src/sharding/paxos_state_persistence.cpp
Total findings: 0


### src/sharding/raft_configuration.cpp
Total findings: 0


### src/sharding/replication_coordinator.cpp
Total findings: 0


### src/sharding/secure_transport_client.cpp
Total findings: 0


### src/sharding/two_phase_commit_participant.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
