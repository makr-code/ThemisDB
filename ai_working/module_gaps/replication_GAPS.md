# replication Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: replication
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 677
- Actionable Findings (Critical + High): 493
- Affected Files: 11

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 191 |
| High | 302 |
| Medium | 176 |
| Low | 8 |

## Category Summary

| Category | Count |
|---|---:|
| undefined_conflict_resolution | 166 |
| missing_version_tracking | 142 |
| stale_read_undocumented | 133 |
| resource_leaked_in_exception | 44 |
| missing_consensus | 26 |
| unspecified_consistency | 26 |
| range_temporary | 22 |
| map_vs_unordered_map | 11 |
| thread_join_no_timeout | 11 |
| lock_contention | 10 |
| duplicate_qualified_signature | 9 |
| manual_cleanup | 9 |
| nested_loop_find | 9 |
| no_timeout | 9 |
| o_n_squared | 8 |
| pointer_arithmetic_unbounded | 7 |
| copy_overhead | 6 |
| lock_in_loop | 4 |
| unstructured_log | 4 |
| string_concat_loop | 3 |
| hardcoded_output | 2 |
| iterator_invalidation | 2 |
| module_doc_linkset_drift | 2 |
| allocation_loop | 1 |
| arithmetic_overflow | 1 |
| hardcoded_path | 1 |
| missing_move_constructor_defaulted | 1 |
| missing_trace_point | 1 |
| multiplication_overflow | 1 |
| null_dereference | 1 |
| primitive_no_volatile | 1 |
| repeated_lookup | 1 |
| stale_doc_section_reference | 1 |
| unchecked_array_index | 1 |
| uninitialized_array | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| replication/replication_manager.cpp | 509 | 158 | 188 | 162 | 1 |
| replication/conflict_resolution.cpp | 102 | 25 | 72 | 4 | 1 |
| replication/logical_replication.cpp | 36 | 6 | 18 | 8 | 4 |
| replication/raft_v2.cpp | 17 | 0 | 15 | 2 | 0 |
| replication/event_stream.cpp | 4 | 0 | 4 | 0 | 0 |
| replication/multi_tier_replication.cpp | 3 | 1 | 2 | 0 | 0 |
| replication/replication_slot.cpp | 2 | 0 | 2 | 0 | 0 |
| replication/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| replication/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| replication/policy.cpp | 1 | 1 | 0 | 0 | 0 |
| replication/schema_cdc.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### replication/replication_manager.cpp
Total findings: 509

- Line 146: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), s.begin(), s.end());
- Line 329: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
- Line 330: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_conflicts_resolved_total counter\n"
- Line 331: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
- Line 463: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '                    // BATCH D FIX: Guard against oversized or corrupt length fields', '                    if (len > 64 * 1024 * 1024) {', '                        THEMIS_ERROR("WAL segment {}: corrupt record length {}, stopping read",', '                                   segment_path, len);']
- Line 472: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: THEMIS_ERROR("WAL segment {}: incomplete read (expected {} bytes, got {})",
- Line 605: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: election_thread_.join();
- Line 875: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: stream_thread_.join();
- Line 1061: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: heartbeat_thread_.join();
- Line 1064: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: compaction_thread_.join();
- Line 1067: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: health_monitor_thread_.join();
- Line 1930: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int64_t LWWConflictResolver::extractTimestamp(const std::string& json_doc) {
- Line 2005: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // deterministic (sorted) to prevent order-dependent conflicts.
- Line 2011: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Build merged document: walk the remote document and for numeric fields
- Line 2013: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: LWWConflictResolver lwr;
- Line 2016: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Scan both documents for numeric fields and merge with max semantics.
- Line 2018: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string merged = base;
- Line 2061: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // For each field present in both documents, patch the merged document with max value
- Line 2062: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // For each field present in both documents, patch the merged document with max value
- Line 2073: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Replace "key": cur_val with "key": max_val in merged
- Line 2079: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while ((pos = merged.find(search, pos)) != std::string::npos) {
- Line 2082: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
- Line 2086: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (vend < merged.size() && merged[vend] == '-') ++vend;
- Line 2087: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while (vend < merged.size() &&
- Line 2088: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::isdigit(static_cast<unsigned char>(merged[vend]))) {
- Line 2093: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string old_val_str = merged.substr(vp, vend - vp);
- Line 2378: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - The merged vector clock ensures that any future write can detect causality
- Line 2416: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
- Line 2418: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.write_id);
- Line 2448: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
- Line 2450: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (conflicting_writes.empty()) return MMWriteEntry{};
- Line 2452: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string merged_data;
- Line 2454: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
- Line 2455: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
- Line 2456: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
- Line 2457: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
- Line 2458: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::G_SET:        merged_data = mergeGSet(conflicting_writes);        break;
- Line 2459: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::OR_SET:       merged_data = mergeORSet(conflicting_writes);       break;
- Line 2460: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::LWW_MAP:      merged_data = mergeLWWMap(conflicting_writes);      break;
- Line 2461: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::TWO_P_SET:    merged_data = mergeTwoPSet(conflicting_writes);     break;
- Line 2462: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::RGA:          merged_data = mergeRGA(conflicting_writes);         break;
- Line 2463: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::FLAG_EW:      merged_data = mergeFlagEW(conflicting_writes);      break;
- Line 2464: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::FLAG_DW:      merged_data = mergeFlagDW(conflicting_writes);      break;
- Line 2467: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Base entry is the LWW winner; replace its data with the merged payload
- Line 2469: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
- Line 2470: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.data = merged_data;
- Line 2472: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Keep checksum aligned with merged payload and metadata-carrying fields.
- Line 2484: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::strategyName() const {
- Line 2544: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // must include the merged (max) value for each field, along with vector clock metadata
- Line 2610: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator qe may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto qe = arr.find('"', qs + 1);
- Line 2617: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(arr.substr(qs + 1, qe - qs - 1));
- Line 2643: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
- Line 2645: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, int64_t> merged;
- Line 2664: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
- Line 2667: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: take max per key for both P and N sub-counters.
- Line 2668: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, int64_t> mergedP, mergedN;
- Line 2673: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergedP[k] = std::max(mergedP[k], v);
- Line 2678: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergedN[k] = std::max(mergedN[k], v);
- Line 2699: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
- Line 2709: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: seen.insert(w.data.substr(qs + 1, qe - qs - 1));
- Line 2774: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
- Line 2813: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeTwoPSet(const std::vector<MMWriteEntry>& writes) {
- Line 2818: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: union(add) across writes, union(remove) across writes.
- Line 2819: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Result: elements that appear in the merged add-set but NOT in the merged remove-set.
- Line 2917: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: deletion is irrevocable
- Line 2923: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Keep the value from the first observed insert (already stored)
- Line 2944: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeFlagEW(const std::vector<MMWriteEntry>& writes) {
- Line 2945: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Enable-Wins Flag: concurrent enable + disable → enabled.
- Line 2951: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
- Line 2958: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: enableTags.insert(t);
- Line 2961: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: disableTags.insert(t);
- Line 2972: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeFlagDW(const std::vector<MMWriteEntry>& writes) {
- Line 2973: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Disable-Wins Flag: concurrent enable + disable → disabled.
- Line 2978: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
- Line 3015: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.insert(result.end(), s.begin(), s.end());
- Line 3084: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
- Line 3087: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return resolver_(document_id, conflicting_writes);
- Line 3151: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (replication_thread_.joinable()) replication_thread_.join();
- Line 3152: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (heartbeat_thread_.joinable())   heartbeat_thread_.join();
- Line 3153: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (sync_thread_.joinable())        sync_thread_.join();
- Line 3166: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: std::string MultiMasterReplicationManager::write(
- Line 3355: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Conflict Management
- Line 3373: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<ConflictRecord> result;
- Line 3374: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& rec : conflicts_) {
- Line 3382: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool MultiMasterReplicationManager::resolveConflict(
- Line 3383: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::string& conflict_id,
- Line 3386: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
- Line 3387: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (auto& rec : conflicts_) {
- Line 3388: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (rec.conflict_id == conflict_id && !rec.resolved) {
- Line 3391: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stats_conflicts_resolved_.fetch_add(1);
- Line 3424: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: s.conflicts_detected    = stats_conflicts_detected_.load();
- Line 3425: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: s.conflicts_resolved    = stats_conflicts_resolved_.load();
- Line 3513: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# HELP themisdb_mm_conflicts_detected Conflicts detected\n"
- Line 3514: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_mm_conflicts_detected counter\n"
- Line 3515: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_mm_conflicts_detected{node=\"" << config_.node_id << "\"} " << s.conflicts_detected << "\n"
- Line 3516: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
- Line 3517: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_mm_conflicts_resolved counter\n"
- Line 3518: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
- Line 3697: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: vector_clock_->merge(incoming.vector_clock);
- Line 3704: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Check for conflict with any recently-seen write for the same document
- Line 3706: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool has_conflict = false;
- Line 3708: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
- Line 3709: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& rec : conflicts_) {
- Line 3713: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: has_conflict = true;
- Line 3719: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (has_conflict) {
- Line 3720: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_WARN("MM conflict detected for doc={}/{} from peer={}",
- Line 3722: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // For each unresolved conflict on this document, add the incoming entry
- Line 3723: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
- Line 3724: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (auto& rec : conflicts_) {
- Line 3728: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: rec.conflicting_writes.push_back(incoming);
- Line 3751: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
- Line 3753: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (conflicting_writes.empty()) return;
- Line 3755: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::string& collection = conflicting_writes[0].collection;
- Line 3758: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_ptr<ConflictResolver> resolver;
- Line 3780: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
- Line 3781: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflicts_.push_back(record);
- Line 3866: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 3883: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = last_done_per_doc_.find(entry.document_id);
- Line 3977: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // has been fully applied, even across concurrent workers.
- Line 4001: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: QuorumReadManager::QuorumReadResult QuorumReadManager::read(
- Line 4454: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string input(reinterpret_cast<const char*>(data.data()), data.size());
- Line 4519: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string input(reinterpret_cast<const char*>(compressed.data()),
- Line 4590: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (flush_thread_.joinable()) flush_thread_.join();
- Line 5356: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
- Line 5367: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.insert(out.end(), iv.begin(), iv.end());
- Line 5368: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.insert(out.end(), tag.begin(), tag.end());
- Line 5369: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: out.insert(out.end(), ciphertext.begin(), ciphertext.begin() + ct_len);
- Line 5393: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: EVP_DecryptUpdate(ctx, plain.data(), &len, ct, ct_len);
- Line 5842: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MultiRegionActiveActiveManager::write(
- Line 5842: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: MultiRegionActiveActiveManager::write(
- Line 5877: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: MultiRegionActiveActiveManager::read(
- Line 6136: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t BidirectionalReplicationManager::submitWrite(
- Line 6216: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Conflict detection: check whether we have a pending local write for the
- Line 6222: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (detectConflict(entry, it->second)) {
- Line 6223: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: handleConflict(it->second, entry, entry.is_ddl);
- Line 6225: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // the conflict history.
- Line 6276: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::count_if(conflict_timestamps_.begin(), conflict_timestamps_.end(),
- Line 6301: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<BidiConflictRecord> result;
- Line 6302: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& rec : conflict_history_) {
- Line 6303: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (rec.strategy_used == ConflictResolution::CUSTOM
- Line 6311: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // ── Conflict resolution ───────────────────────────────────────────────────────
- Line 6313: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool BidirectionalReplicationManager::resolveConflict(
- Line 6322: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::lock_guard<std::mutex> lk(conflicts_mutex_);
- Line 6323: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Walk in reverse to find the most-recent conflict for this document.
- Line 6324: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (auto it = conflict_history_.rbegin(); it != conflict_history_.rend(); ++it) {
- Line 6329: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflicts_resolved_.fetch_add(1);
- Line 6340: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ConflictResolution strategy)
- Line 6345: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ConflictResolution BidirectionalReplicationManager::getEffectiveStrategy(
- Line 6389: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return applyRemoteWrite(entry);
- Line 6451: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool BidirectionalReplicationManager::detectConflict(
- Line 6456: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // are always considered concurrent — a true conflict.
- Line 6464: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void BidirectionalReplicationManager::handleConflict(
- Line 6496: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflict_timestamps_.pop_front();
- Line 6501: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (strategy != ConflictResolution::CUSTOM) {
- Line 6502: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflicts_resolved_.fetch_add(1);
- Line 6666: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool GeoReplicationManager::write(
- Line 6666: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool GeoReplicationManager::write(
- Line 6704: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: std::optional<std::string> GeoReplicationManager::read(
- Line 81: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto status = future.wait_for(std::chrono::milliseconds(timeout_ms));
- Line 90: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: future.get();
- Line 126: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // BATCH A OPTIMIZATION: Pre-allocate based on typical WAL entry size', '    // Typical entry: 3×8 (header) + 4×(len prefix) + strings (~500 bytes total)', '    size_t estimated_size = 32 + 4 + operation.size() + collection.size() +', '                           document_id.size() + data.size() + checksum.size();', '    result.reserve(estimated_size);']
- Line 329: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
- Line 331: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
- Line 459: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
- Line 470: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ifs.read(reinterpret_cast<char*>(data.data()), len);
- Line 472: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("WAL segment {}: incomplete read (expected {} bytes, got {})",
- Line 547: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.wal_directory)) {
- Line 560: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.wal_directory)) {
- Line 566: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
- Line 570: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ifs.read(reinterpret_cast<char*>(data.data()), len);
- Line 612: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: election_thread_ = std::thread(&LeaderElection::electionLoop, this);
- Line 749: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 751: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 784: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 790: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 869: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stream_thread_ = std::thread(&ReplicationStream::streamLoop, this);
- Line 910: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
- Line 937: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.batch_timeout_ms));
- Line 1046: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: heartbeat_thread_ = std::thread(&ReplicationManager::heartbeatLoop, this);
- Line 1047: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: compaction_thread_ = std::thread(&ReplicationManager::compactionLoop, this);
- Line 1048: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: health_monitor_thread_ = std::thread(&ReplicationManager::healthMonitorLoop, this);
- Line 1078: severity=HIGH; category=missing_trace_point
  Description: Critical function replicate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool ReplicationManager::replicate(const WALEntry& entry) {
- Line 1166: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 1247: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void ReplicationManager::setConflictResolver(std::shared_ptr<IConflictResolver> resolver) {
- Line 1384: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 1547: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 1614: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1633: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1668: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ReplicationManager::LeaseReadResult ReplicationManager::leaseRead(
- Line 1791: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1799: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1800: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1803: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1804: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2001: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // The merged state combines both semantics: max values from counters +
- Line 2011: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Build merged document: walk the remote document and for numeric fields
- Line 2016: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Scan both documents for numeric fields and merge with max semantics.
- Line 2018: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string merged = base;
- Line 2061: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // For each field present in both documents, patch the merged document with max value
- Line 2062: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // For each field present in both documents, patch the merged document with max value
- Line 2073: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Replace "key": cur_val with "key": max_val in merged
- Line 2079: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while ((pos = merged.find(search, pos)) != std::string::npos) {
- Line 2082: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
- Line 2086: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (vend < merged.size() && merged[vend] == '-') ++vend;
- Line 2087: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: while (vend < merged.size() &&
- Line 2088: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::isdigit(static_cast<unsigned char>(merged[vend]))) {
- Line 2093: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string old_val_str = merged.substr(vp, vend - vp);
- Line 2096: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Replace with new value
- Line 2096: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2097: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2098: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2098: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.replace(vp, vend - vp, new_val_str);
- Line 2108: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 2218: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void VectorClock::merge(const VectorClock& other) {
- Line 2222: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // two threads concurrently call A.merge(B) and B.merge(A).
- Line 2234: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
- Line 2267: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2268: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2313: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // The reconstructed vector clock is merged with conflicting writes' clocks in
- Line 2355: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // This resolver implements Last-Write-Wins conflict resolution for multi-master scenarios.
- Line 2366: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 1. Merged Vector Clock: union of all conflicting writes' vector clocks.
- Line 2368: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //    is represented in the merged clock.
- Line 2369: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 2. Merged Dependencies: set union of all write_ids from conflicting entries.
- Line 2372: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //    the resolved entry's timestamp reflects the true end of the conflict window.
- Line 2374: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //    must be recomputed because the merged metadata is included for verification.
- Line 2378: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - The merged vector clock ensures that any future write can detect causality
- Line 2397: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // This lambda enriches the winner with merged causality metadata:
- Line 2398: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - merged_clock: Lattice join of all vector clocks. Represents the frontier
- Line 2400: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - merged_dependencies: Causal dag where winner depends on all conflicting writes.
- Line 2409: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: VectorClock merged_clock = winner.vector_clock;
- Line 2410: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::set<std::string> merged_dependencies(
- Line 2415: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_clock.merge(write.vector_clock);
- Line 2416: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
- Line 2418: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.write_id);
- Line 2425: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: enriched.vector_clock = std::move(merged_clock);
- Line 2426: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: enriched.dependencies.assign(merged_dependencies.begin(), merged_dependencies.end());
- Line 2442: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: CRDTMergeResolver::CRDTMergeResolver(CRDTType type)
- Line 2446: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry CRDTMergeResolver::resolve(
- Line 2452: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string merged_data;
- Line 2454: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
- Line 2455: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
- Line 2456: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
- Line 2457: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
- Line 2469: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
- Line 2470: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.data = merged_data;
- Line 2472: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Keep checksum aligned with merged payload and metadata-carrying fields.
- Line 2484: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::strategyName() const {
- Line 2501: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeLWWRegister(const std::vector<MMWriteEntry>& writes) {
- Line 2510: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeMVRegister(const std::vector<MMWriteEntry>& writes) {
- Line 2528: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // increasing value that should be merged using max() semantics across conflicting writes.
- Line 2533: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // This property must be preserved during merge: merged_value = max(local_val, remote_val).
- Line 2540: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // merge outcomes across all replicas.
- Line 2544: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // must include the merged (max) value for each field, along with vector clock metadata
- Line 2643: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
- Line 2645: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, int64_t> merged;
- Line 2649: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged[k] = std::max(merged[k], v);
- Line 2655: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& [k, v] : merged) {
- Line 2664: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
- Line 2667: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: take max per key for both P and N sub-counters.
- Line 2668: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, int64_t> mergedP, mergedN;
- Line 2672: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& [k, v] : extractJsonInts(pSub))
- Line 2673: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergedP[k] = std::max(mergedP[k], v);
- Line 2677: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& [k, v] : extractJsonInts(nSub))
- Line 2678: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: mergedN[k] = std::max(mergedN[k], v);
- Line 2695: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "{\"P\":" << serializeMap(mergedP) << ",\"N\":" << serializeMap(mergedN) << "}";
- Line 2699: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
- Line 2704: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto qs = w.data.find('"', p);
- Line 2706: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto qe = w.data.find('"', qs + 1);
- Line 2725: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeORSet(const std::vector<MMWriteEntry>& writes) {
- Line 2733: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: union of all add pairs, union of all tombstones.
- Line 2742: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& t : extractJsonArrayStrings(tsArr))
- Line 2749: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto lb = addArr.find('[', p);
- Line 2750: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto lb = addArr.find('[', p);
- Line 2751: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto rb = addArr.find(']', lb + 1);
- Line 2752: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto rb = addArr.find(']', lb + 1);
- Line 2773: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
- Line 2774: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
- Line 2794: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = best.find(k);
- Line 2795: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = best.find(k);
- Line 2813: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeTwoPSet(const std::vector<MMWriteEntry>& writes) {
- Line 2818: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: union(add) across writes, union(remove) across writes.
- Line 2819: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Result: elements that appear in the merged add-set but NOT in the merged remove-set.
- Line 2824: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& e : extractJsonArrayStrings(addArr))    addSet.insert(e);
- Line 2825: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& e : extractJsonArrayStrings(removeArr)) removeSet.insert(e);
- Line 2841: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeRGA(const std::vector<MMWriteEntry>& writes) {
- Line 2849: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: union of all elements by unique id; if an id appears in multiple writes, prefer
- Line 2868: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ob = src.find('{', p);
- Line 2917: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: deletion is irrevocable
- Line 2944: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeFlagEW(const std::vector<MMWriteEntry>& writes) {
- Line 2951: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
- Line 2957: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& t : extractJsonArrayStrings(eArr))
- Line 2960: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& t : extractJsonArrayStrings(dArr))
- Line 2965: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (disableTags.find(t) == disableTags.end()) { enabled = true; break; }
- Line 2972: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string CRDTMergeResolver::mergeFlagDW(const std::vector<MMWriteEntry>& writes) {
- Line 2978: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
- Line 2984: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& t : extractJsonArrayStrings(eArr))
- Line 2987: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& t : extractJsonArrayStrings(dArr))
- Line 3091: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return lwr.resolve(document_id, conflicting_writes);
- Line 3136: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: replication_thread_ = std::thread(&MultiMasterReplicationManager::replicationLoop, this);
- Line 3137: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: heartbeat_thread_   = std::thread(&MultiMasterReplicationManager::heartbeatLoop,   this);
- Line 3138: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: sync_thread_        = std::thread(&MultiMasterReplicationManager::syncLoop,        this);
- Line 3236: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return future.wait_for(timeout) == std::future_status::ready && future.get();
- Line 3243: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MultiMasterReplicationManager::ReadResult MultiMasterReplicationManager::read(
- Line 3371: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<ConflictRecord> MultiMasterReplicationManager::getUnresolvedConflicts() const {
- Line 3382: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool MultiMasterReplicationManager::resolveConflict(
- Line 3425: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: s.conflicts_resolved    = stats_conflicts_resolved_.load();
- Line 3516: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
- Line 3518: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
- Line 3538: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(writes_mutex_);
- Line 3547: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = write_callbacks_.find(entry.write_id);
- Line 3558: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [entry, cb] : batch) {
- Line 3584: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.heartbeat_interval_ms));
- Line 3600: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.sync_interval_ms));
- Line 3697: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: vector_clock_->merge(incoming.vector_clock);
- Line 3722: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // For each unresolved conflict on this document, add the incoming entry
- Line 3765: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry winner = resolver->resolve(document_id, conflicting_writes);
- Line 3908: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 3911: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 3938: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 3939: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: queue_cv_.wait_for(lock, std::chrono::milliseconds(5),
- Line 4001: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: QuorumReadManager::QuorumReadResult QuorumReadManager::read(
- Line 4091: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto resp = fut.get();
- Line 4126: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<ReplicaResponse> reconcile_set;
- Line 4128: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: reconcile_set = responses;
- Line 4130: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: reconcile_set.reserve(qualifying.size());
- Line 4131: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto* p : qualifying) reconcile_set.push_back(*p);
- Line 4134: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Reconcile: pick the response with the highest version
- Line 4135: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const ReplicaResponse* best = &reconcile_set[0];
- Line 4137: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& r : reconcile_set) {
- Line 4145: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& r : reconcile_set) {
- Line 4160: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& r : reconcile_set) {
- Line 4584: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: flush_thread_ = std::thread(&BatchedAckTracker::flushLoop, this);
- Line 4652: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(pending_mutex_);
- Line 4741: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ins.recommendation = "Check network connectivity to " + replica_id +

                                 ", consider increasing batch_size";

            ins.detected_at = now;

            ins.metadata["replica_id"] = replica_id;

            ins.metadata["lag_ms"]     = std::to_string(last_lag);

            insights.push_back(std::move(ins));

        }
- Line 4742: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ", consider increasing batch_size";

            ins.detected_at = now;

            ins.metadata["replica_id"] = replica_id;

            ins.metadata["lag_ms"]     = std::to_string(last_lag);

            insights.push_back(std::move(ins));

        }
- Line 4760: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::to_string(avg) + "ms";

            ins.recommendation = "Investigate disk I/O or CPU on " + replica_id;

            ins.detected_at = now;

            ins.metadata["replica_id"] = replica_id;

            ins.metadata["avg_lag_ms"] = std::to_string(avg);

            insights.push_back(std::move(ins));

        }
- Line 4790: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // A high value indicates large relative spread (typical of network jitter).
- Line 4805: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: b.details         = "High lag spread (normalized_range=" +
- Line 5349: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5387: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5735: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5877: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MultiRegionActiveActiveManager::read(
- Line 6267: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: s.conflicts_resolved = conflicts_resolved_.load();
- Line 6313: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool BidirectionalReplicationManager::resolveConflict(
- Line 6443: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Return an empty placeholder; the application must call resolveConflict().
- Line 6704: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<std::string> GeoReplicationManager::read(
- Line 104: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALEntry::serialize()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::vector<uint8_t> WALEntry::serialize() const {
- Line 282: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // when determining eventual consistency timeline.
- Line 380: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALManager::append(const WALEntry& entry)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint64_t WALManager::append(const WALEntry& entry) {
- Line 696: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // If term is stale, reject
- Line 754: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: !commit_index_.compare_exchange_weak(expected, new_commit)) {}
- Line 902: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //   network storms while maintaining eventual delivery (liveness).
- Line 967: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - This prevents network storms (stability) while ensuring eventual retransmission
- Line 1546: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 300 && running_.load(); ++i) {
- Line 1742: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: unreachable_nodes.push_back(replica.node_id);
- Line 2004: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // on all numeric fields to ensure eventual consistency. Field discovery must be
- Line 2023: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: -> std::map<std::string, int64_t>
- Line 2025: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int64_t> fields;
- Line 2121: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: HybridLogicalClock::Timestamp HybridLogicalClock::now() {
- Line 2213: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::increment(const std::string& node_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void VectorClock::increment(const std::string& node_id) {
- Line 2218: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::merge(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void VectorClock::merge(const VectorClock& other) {
- Line 2234: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::get(const std::string& node_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
- Line 2240: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::compare(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: int VectorClock::compare(const VectorClock& other) const {
- Line 2273: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::happensBefore(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool VectorClock::happensBefore(const VectorClock& other) const {
- Line 2277: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::isConcurrent(const VectorClock& other)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool VectorClock::isConcurrent(const VectorClock& other) const {
- Line 2401: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //   Enables transitive dependency tracking for eventual consistency validation.
- Line 2546: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: static std::map<std::string, int64_t> extractJsonInts(const std::string& doc) {
- Line 2547: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int64_t> fields;
- Line 2668: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int64_t> mergedP, mergedN;
- Line 2682: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto serializeMap = [](const std::map<std::string, int64_t>& m) {
- Line 2767: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> elemTags;
- Line 2791: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::pair<HybridLogicalClock::Timestamp, std::string>> best;
- Line 2859: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, RGAElem> byId;
- Line 3005: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
- Line 3009: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
- Line 3014: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
- Line 3275: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // not yet received — that peer signals a potential stale read.
- Line 3277: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool stale_read_detected = false;
- Line 3297: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: stale_read_detected = true;
- Line 3307: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (stale_read_detected) {
- Line 3308: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_WARN("MultiMasterRead: potential stale read detected for node={} "
- Line 4060: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Single-Node Quorum Read.' that was not found in 'src/replication/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/replication/FUTURE_ENHANCEMENTS.md §Single-Node Quorum Read.
- Line 4081: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // total responses would discard fresh replicas that come after a stale one
- Line 4125: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // session token was supplied so that stale replicas are not considered).
- Line 4143: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // identify stale replicas and schedule repair.
- Line 4400: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buf += std::to_string(e.sequence_number) + "|"
- Line 4708: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: values.push_back(dp.lag_ms);
- Line 5055: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "Throughput:    " << static_cast<uint64_t>(r.writes_per_second) << " writes/sec\n"
- Line 5203: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: m += "themisdb_cross_cluster_publication_published_total" + label + " " +
- Line 5239: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (last_applied_seq_.compare_exchange_weak(expected, e.sequence_number))
- Line 5362: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 5398: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 5759: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidates.push_back(entry.path().filename().string());
- Line 5789: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Initialise staleness entries for peer regions (unknown at start)
- Line 5791: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RegionStalenessInfo info;
- Line 5793: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: info.staleness_ms          = std::numeric_limits<int64_t>::max();
- Line 5797: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: region_staleness_[peer]    = info;
- Line 5854: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Update local region staleness to 0 (we just wrote here)
- Line 5856: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 5857: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto& local = region_staleness_[config_.local_region_id];
- Line 5858: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: local.staleness_ms           = 0;
- Line 5886: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int64_t local_staleness_ms = 0;
- Line 5889: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 5890: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto it = region_staleness_.find(config_.local_region_id);
- Line 5891: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (it != region_staleness_.end()) {
- Line 5892: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: local_staleness_ms = it->second.staleness_ms;
- Line 5899: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.staleness_ms = local_staleness_ms;
- Line 5906: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (local_staleness_ms > 0) {
- Line 5908: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "local staleness={}ms > 0", local_staleness_ms);
- Line 5909: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ++staleness_rejections_;
- Line 5915: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case ConsistencyLevel::BOUNDED_STALENESS:
- Line 5916: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ++bounded_staleness_reads_;
- Line 5917: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (local_staleness_ms > static_cast<int64_t>(config_.max_staleness_ms)) {
- Line 5918: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_WARN("MultiRegionActiveActive: BOUNDED_STALENESS read rejected – "
- Line 5919: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "local staleness={}ms > bound={}ms",
- Line 5920: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: local_staleness_ms, config_.max_staleness_ms);
- Line 5921: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ++staleness_rejections_;
- Line 5944: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Always succeeds regardless of staleness
- Line 5949: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_INFO("MultiRegionActiveActive: read served region={} staleness={}ms",
- Line 5950: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: config_.local_region_id, local_staleness_ms);
- Line 5984: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::chrono::milliseconds MultiRegionActiveActiveManager::getStaleness(
- Line 5987: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 5988: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto it = region_staleness_.find(region_id);
- Line 5989: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (it == region_staleness_.end()) {
- Line 5992: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return std::chrono::milliseconds(it->second.staleness_ms);
- Line 5995: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool MultiRegionActiveActiveManager::isWithinStalenessBound(
- Line 5998: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return getStaleness(region_id) <=
- Line 5999: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::chrono::milliseconds(config_.max_staleness_ms);
- Line 6002: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<RegionStalenessInfo>
- Line 6003: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MultiRegionActiveActiveManager::getAllRegionStaleness() const
- Line 6005: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6006: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<RegionStalenessInfo> result;
- Line 6007: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: result.reserve(region_staleness_.size());
- Line 6008: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : region_staleness_) {
- Line 6014: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void MultiRegionActiveActiveManager::updateRegionStaleness(
- Line 6016: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int64_t            staleness_ms,
- Line 6019: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6020: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto& info              = region_staleness_[region_id];
- Line 6022: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: info.staleness_ms       = staleness_ms;
- Line 6025: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: info.is_healthy         = (staleness_ms >= 0 &&
- Line 6026: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: staleness_ms <= static_cast<int64_t>(config_.max_staleness_ms) * 2);
- Line 6043: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_mraaa_staleness_rejections_total Reads rejected due to excessive staleness\n"
- Line 6044: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_mraaa_staleness_rejections_total counter\n"
- Line 6045: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_mraaa_staleness_rejections_total{region=\"" << r << "\"} "
- Line 6046: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << staleness_rejections_.load() << "\n\n";
- Line 6053: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_mraaa_bounded_staleness_reads_total Reads served at BOUNDED_STALENESS\n"
- Line 6054: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_mraaa_bounded_staleness_reads_total counter\n"
- Line 6055: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_mraaa_bounded_staleness_reads_total{region=\"" << r << "\"} "
- Line 6056: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << bounded_staleness_reads_.load() << "\n\n";
- Line 6068: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Per-region staleness gauges
- Line 6070: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6071: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_mraaa_region_staleness_ms Current replication staleness per region\n"
- Line 6072: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_mraaa_region_staleness_ms gauge\n";
- Line 6073: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : region_staleness_) {
- Line 6074: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "themisdb_mraaa_region_staleness_ms{region=\"" << kv.first << "\"} "
- Line 6075: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << kv.second.staleness_ms << "\n";
- Line 6239: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (remote_sequence_.compare_exchange_weak(cur, entry.origin_seq)) {
- Line 6361: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (remote_sequence_.compare_exchange_weak(cur, remote_seq)) {
- Line 6514: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RegionStalenessInfo local;
- Line 6516: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: local.staleness_ms           = 0;
- Line 6520: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: region_staleness_[config_.local_region] = local;
- Line 6525: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RegionStalenessInfo info;
- Line 6527: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: info.staleness_ms           = std::numeric_limits<int64_t>::max();
- Line 6531: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: region_staleness_[r]       = info;
- Line 6576: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // ── Staleness management ──────────────────────────────────────────────────────
- Line 6578: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void GeoReplicationManager::updateRegionStaleness(const std::string& region,
- Line 6579: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int64_t            staleness_ms,
- Line 6582: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6583: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto& info                  = region_staleness_[region];
- Line 6585: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: info.staleness_ms            = staleness_ms;
- Line 6588: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: info.is_healthy              = (staleness_ms >= 0);
- Line 6591: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::chrono::milliseconds GeoReplicationManager::getStaleness(
- Line 6594: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6595: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto it = region_staleness_.find(region);
- Line 6596: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (it == region_staleness_.end()) {
- Line 6599: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return std::chrono::milliseconds(it->second.staleness_ms);
- Line 6608: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6613: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto it = region_staleness_.find(config_.local_region);
- Line 6614: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (it != region_staleness_.end() && it->second.staleness_ms == 0) {
- Line 6618: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& [rid, info] : region_staleness_) {
- Line 6619: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (info.staleness_ms == 0 && info.is_healthy) return rid;
- Line 6632: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Pick the region with smallest staleness that is within bound.
- Line 6635: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& [rid, info] : region_staleness_) {
- Line 6636: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (info.is_healthy && info.staleness_ms <= bound &&
- Line 6648: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // so it is safe to call while holding staleness_mutex_ as a shared lock.
- Line 6650: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto it = region_staleness_.find(config_.local_region);
- Line 6651: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (it != region_staleness_.end() &&
- Line 6658: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case ConsistencyLevel::EVENTUAL:
- Line 6675: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto lag = getStaleness(config_.local_region);
- Line 6688: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6689: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto& local               = region_staleness_[config_.local_region];
- Line 6690: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: local.staleness_ms        = 0;
- Line 6716: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case ConsistencyLevel::BOUNDED_STALENESS:
- Line 6717: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ++bounded_staleness_reads_;
- Line 6722: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case ConsistencyLevel::EVENTUAL:
- Line 6723: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ++eventual_reads_;
- Line 6770: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_geo_repl_bounded_staleness_reads_total BOUNDED_STALENESS reads\n"
- Line 6771: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_geo_repl_bounded_staleness_reads_total counter\n"
- Line 6772: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "themisdb_geo_repl_bounded_staleness_reads_total" << lbl << " "
- Line 6773: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << bounded_staleness_reads_.load() << "\n";
- Line 6785: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Per-region staleness gauge
- Line 6786: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_geo_repl_region_staleness_ms Replication lag per region (ms)\n"
- Line 6787: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: << "# TYPE themisdb_geo_repl_region_staleness_ms gauge\n";
- Line 6789: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
- Line 6790: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& [rid, info] : region_staleness_) {
- Line 6791: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: int64_t lag = (info.staleness_ms == std::numeric_limits<int64_t>::max())
- Line 6792: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ? -1 : info.staleness_ms;
- Line 6793: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "themisdb_geo_repl_region_staleness_ms{region=\"" << rid << "\"} "
- Line 4480: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // compressed inputs whose expansion is dominated by header overhead.

### replication/conflict_resolution.cpp
Total findings: 102

- Line 2: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: * ThemisDB | File: conflict_resolution.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 20:41:24
- Line 11: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: * ThemisDB Advanced Conflict Resolution Implementation
- Line 17: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: #include "replication/conflict_resolution.h"
- Line 336: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
- Line 344: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.write_id);
- Line 402: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("ThreeWayMergeResolver::selectBase: best_idx {} out of bounds (size {})",
- Line 410: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string ThreeWayMergeResolver::mergeJson(
- Line 421: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, std::string> merged;
- Line 424: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : base_f)  merged[kv.first] = kv.second;
- Line 425: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : left_f)  merged[kv.first] = kv.second;
- Line 426: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : right_f) merged[kv.first] = kv.second;
- Line 428: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (auto& kv : merged) {
- Line 483: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Merged vector clock representing the frontier of all conflicting writes
- Line 485: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Latest HLC to preserve monotonicity across merge
- Line 504: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: conflicting_writes[j].vector_clock))
- Line 528: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return enrichWinnerWithCausality(winner, conflicting_writes);
- Line 532: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // FieldLevelMergeResolver
- Line 535: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
- Line 589: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (strategy_ == MergeStrategy::INTERSECT) {
- Line 614: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("FieldLevelMergeResolver: present_indices[0] {} out of bounds",
- Line 619: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Latest HLC wins for conflicting fields
- Line 623: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("FieldLevelMergeResolver: index {} out of bounds", idx);
- Line 630: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged[key] = field_maps[best][key];
- Line 660: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 1. Merged vector clocks from all conflicting writes
- Line 677: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return enrichWinnerWithCausality(winner, conflicting_writes);
- Line 304: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: * @param conflicting_writes All conflicting writes to merge metadata from
- Line 305: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: * @return Enriched winner with merged vector clock, dependencies, and HLC
- Line 315: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: VectorClock merged_clock = winner.vector_clock;
- Line 316: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::set<std::string> merged_dependencies(
- Line 329: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_clock.merge(write.vector_clock);
- Line 332: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Continue with existing merged clock
- Line 336: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
- Line 344: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged_dependencies.insert(write.write_id);
- Line 352: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Atomically update enriched entry with merged metadata
- Line 353: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: enriched.vector_clock = std::move(merged_clock);
- Line 354: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: enriched.dependencies.assign(merged_dependencies.begin(), merged_dependencies.end());
- Line 369: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // ThreeWayMergeResolver
- Line 372: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry ThreeWayMergeResolver::selectBase(
- Line 377: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: throw std::invalid_argument("ThreeWayMergeResolver::selectBase requires non-empty writes vector");
- Line 402: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("ThreeWayMergeResolver::selectBase: best_idx {} out of bounds (size {})",
- Line 404: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 410: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string ThreeWayMergeResolver::mergeJson(
- Line 421: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, std::string> merged;
- Line 424: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : base_f)  merged[kv.first] = kv.second;
- Line 425: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : left_f)  merged[kv.first] = kv.second;
- Line 426: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (const auto& kv : right_f) merged[kv.first] = kv.second;
- Line 428: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: for (auto& kv : merged) {
- Line 432: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto base_it  = base_f.find(key);
- Line 433: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto left_it  = left_f.find(key);
- Line 434: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto right_it = right_f.find(key);
- Line 451: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // else: neither changed — keep base value (already in merged)
- Line 454: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return buildJson(merged);
- Line 457: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("ThreeWayMergeResolver::mergeJson: exception during merge: {}", e.what());
- Line 462: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry ThreeWayMergeResolver::resolve(
- Line 467: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // BATCH C ANNOTATION: Three-Way Merge with Metadata Enrichment
- Line 468: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // This resolver implements a three-way merge algorithm:
- Line 471: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 3. Merge fields: keep left if only left changed, keep right if only right changed,
- Line 473: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 4. Enrich the winner with merged causality metadata
- Line 475: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Causality Guarantee (RFC 3-Way Merge + Vector Clocks):
- Line 478: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - The resolved entry will have merged_clock = lub(left.vc, right.vc, ...) >= all inputs
- Line 482: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // The merged data (from mergeJson) is combined with enrichWinnerWithCausality to produce:
- Line 483: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Merged vector clock representing the frontier of all conflicting writes
- Line 485: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // - Latest HLC to preserve monotonicity across merge
- Line 489: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: throw std::invalid_argument("ThreeWayMergeResolver::resolve requires at least one conflicting write");
- Line 522: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    }', '', '    if (first) return conflicting_writes[base_idx]; // no non-base entries', '', '    MMWriteEntry winner = conflicting_writes[right_idx];']
- Line 525: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: winner.data = mergeJson(base.data,
- Line 532: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // FieldLevelMergeResolver
- Line 535: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
- Line 539: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string FieldLevelMergeResolver::strategyName() const
- Line 542: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::UNION:      return "FIELD_MERGE_UNION";
- Line 543: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::INTERSECT:  return "FIELD_MERGE_INTERSECT";
- Line 544: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::LEFT_BIAS:  return "FIELD_MERGE_LEFT_BIAS";
- Line 545: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::RIGHT_BIAS: return "FIELD_MERGE_RIGHT_BIAS";
- Line 547: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return "FIELD_MERGE_UNKNOWN";
- Line 550: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string FieldLevelMergeResolver::mergeFields(
- Line 555: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_WARN("FieldLevelMergeResolver::mergeFields: empty writes vector");
- Line 576: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::map<std::string, std::string> merged;
- Line 589: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (strategy_ == MergeStrategy::INTERSECT) {
- Line 597: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_WARN("FieldLevelMergeResolver::mergeFields: present_indices unexpectedly empty");
- Line 603: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::LEFT_BIAS:
- Line 604: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged[key] = field_maps[present_indices.front()][key];
- Line 606: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::RIGHT_BIAS:
- Line 607: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged[key] = field_maps[present_indices.back()][key];
- Line 609: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::UNION:
- Line 610: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: case MergeStrategy::INTERSECT:
- Line 614: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("FieldLevelMergeResolver: present_indices[0] {} out of bounds",
- Line 623: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("FieldLevelMergeResolver: index {} out of bounds", idx);
- Line 630: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged[key] = field_maps[best][key];
- Line 636: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return buildJson(merged);
- Line 639: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_ERROR("FieldLevelMergeResolver::mergeFields: exception during merge: {}", e.what());
- Line 644: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MMWriteEntry FieldLevelMergeResolver::resolve(
- Line 659: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // After field-level merge, the winner is enriched with:
- Line 660: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 1. Merged vector clocks from all conflicting writes
- Line 663: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // 4. Recomputed checksum binding merged data to metadata
- Line 666: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // All replicas must apply the same merge strategy (UNION/INTERSECT/LEFT_BIAS/RIGHT_BIAS)
- Line 671: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: throw std::invalid_argument("FieldLevelMergeResolver::resolve requires at least one conflicting write");
- Line 676: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: winner.data = mergeFields(conflicting_writes);
- Line 201: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: value += '"';
- Line 238: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::string buildJson(const std::map<std::string, std::string>& fields)
- Line 421: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> merged;
- Line 432: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: key
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto base_it  = base_f.find(key);
- Line 478: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // - The resolved entry will have merged_clock = lub(left.vc, right.vc, ...) >= all inputs

### replication/logical_replication.cpp
Total findings: 36

- Line 119: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: runtime->snapshot_keys.insert(key);
- Line 273: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return {0, 0};  // conflict-free initial sync: skip duplicates from snapshot
- Line 346: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 635: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int fd = ::open(tmp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
- Line 644: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ::_write(fd, payload.data(), static_cast<unsigned int>(payload.size()));
- Line 690: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int dir_fd = ::open(base.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
- Line 166: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& kv : slots_) {
- Line 167: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> g(kv.second->mutex);
- Line 183: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = slots_.find(slot_name);
- Line 222: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& slot : slots_copy) {
- Line 223: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(slot->mutex);
- Line 228: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 330: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 336: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> elock(worker_err_mutex);
- Line 340: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> elock(worker_err_mutex);
- Line 415: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 471: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 472: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string LogicalReplicationManager::documentIdFromChange(const LogicalChange& change) const {

    if (change.new_data.is_object()) {

        if (change.new_data.contains("document_id")) {

            const auto& v = change.new_data["document_id"];

            return v.is_string() ? v.get<std::string>() : v.dump();

        }

        if (change.new_data.contains("_id")) {
- Line 472: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 475: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 476: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return v.is_string() ? v.get<std::string>() : v.dump();

        }

        if (change.new_data.contains("_id")) {

            const auto& v = change.new_data["_id"];

            return v.is_string() ? v.get<std::string>() : v.dump();

        }

    }
- Line 476: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 482: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    if (change.old_data.is_object()) {

        if (change.old_data.contains("document_id")) {

            const auto& v = change.old_data["document_id"];

            return v.is_string() ? v.get<std::string>() : v.dump();

        }

        if (change.old_data.contains("_id")) {
- Line 486: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return v.is_string() ? v.get<std::string>() : v.dump();

        }

        if (change.old_data.contains("_id")) {

            const auto& v = change.old_data["_id"];

            return v.is_string() ? v.get<std::string>() : v.dump();

        }

    }
- Line 235: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 650: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::_close(fd);
- Line 652: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 669: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::_close(fd);
- Line 671: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::_close(fd);
- Line 680: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 695: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(dir_fd);
- Line 228: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 294: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 330: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 584: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);

### replication/raft_v2.cpp
Total findings: 17

- Line 31: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 32: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 46: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 47: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 67: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 90: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 109: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 155: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 159: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 177: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 194: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 229: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 291: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 293: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 185: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return;  // Stale callback – ignore
- Line 201: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return;  // Stale callback – ignore

### replication/event_stream.cpp
Total findings: 4

- Line 227: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 239: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 250: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 251: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### replication/multi_tier_replication.cpp
Total findings: 3

- Line 174: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // to avoid data races with concurrent readers.
- Line 162: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return override_ptr->value();
- Line 249: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### replication/replication_slot.cpp
Total findings: 2

- Line 305: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& entry : std::filesystem::directory_iterator(slots_dir)) {
- Line 305: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(slots_dir)) {

### replication/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### replication/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### replication/policy.cpp
Total findings: 1

- Line 48: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (!r.datacenter.empty()) dcs.insert(r.datacenter);

### replication/schema_cdc.cpp
Total findings: 1

- Line 151: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: themis::cdc::CdcSchemaEncoder encoder(registry_.get());

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
