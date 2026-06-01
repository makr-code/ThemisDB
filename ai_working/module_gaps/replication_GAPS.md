# replication Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: replication
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 844
- Actionable Findings (Critical + High): 505
- Affected Files: 10

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 205 |
| High | 300 |
| Medium | 339 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| distributed_consistency | 428 |
| container | 119 |
| performance_patterns | 113 |
| reliability | 47 |
| exception_safety | 42 |
| memory | 32 |
| concurrency | 17 |
| performance | 12 |
| raii | 10 |
| legacy_duplication | 9 |
| observability | 5 |
| security | 5 |
| platform | 2 |
| audit_logging | 1 |
| input_validation | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/replication/replication_manager.cpp | 590 | 150 | 177 | 263 | 0 |
| src/replication/conflict_resolution.cpp | 81 | 32 | 38 | 11 | 0 |
| src/replication/logical_replication.cpp | 62 | 7 | 26 | 25 | 4 |
| src/replication/raft_v2.cpp | 33 | 6 | 25 | 2 | 0 |
| src/replication/event_stream.cpp | 28 | 0 | 24 | 4 | 0 |
| src/replication/observability.cpp | 15 | 0 | 0 | 15 | 0 |
| src/replication/replication_slot.cpp | 12 | 4 | 6 | 2 | 0 |
| src/replication/multi_tier_replication.cpp | 10 | 2 | 3 | 5 | 0 |
| src/replication/policy.cpp | 8 | 2 | 0 | 6 | 0 |
| src/replication/schema_cdc.cpp | 5 | 2 | 1 | 2 | 0 |

## Full Scanner Findings

### src/replication/replication_manager.cpp
Total findings: 590

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['', '            // Guard against oversized or corrupt length fields', '            if (len > 64 * 1024 * 1024) {', '                THEMIS_ERROR("WAL segment {}: corrupt record length {}, stopping read", seg, len);', '                break;']
  Confidence: band=very_high; score=0.9
- Line 65: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), s.begin(), s.end());
  Confidence: band=very_high; score=0.99
- Line 189: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# TYPE themisdb_conflicts_resolved_total counter\n"
  Confidence: band=very_high; score=0.99
- Line 191: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 320: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_ERROR("WAL segment {}: incomplete read (expected {} bytes, got {})",
- Line 728: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto entries = wal_->readFrom(next_seq, config_.batch_size);
- Line 1663: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: int64_t LWWConflictResolver::extractTimestamp(const std::string& json_doc) {
  Confidence: band=very_high; score=0.99
- Line 1708: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // CRDTConflictResolver Implementation
  Confidence: band=very_high; score=0.99
- Line 1711: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTConflictResolver::resolve(
  Confidence: band=very_high; score=0.99
- Line 1719: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // For all other content we delegate to LWWConflictResolver.
  Confidence: band=very_high; score=0.99
- Line 1725: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Build merged document: walk the remote document and for numeric fields
  Confidence: band=very_high; score=0.99
- Line 1727: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: LWWConflictResolver lwr;
  Confidence: band=very_high; score=0.99
- Line 1730: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Scan both documents for numeric fields and merge with max semantics.
  Confidence: band=very_high; score=0.99
- Line 1732: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string merged = base;
  Confidence: band=very_high; score=0.99
- Line 1775: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // For each field present in both documents, patch the merged document with max value
  Confidence: band=very_high; score=0.99
- Line 1784: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Replace "key": cur_val with "key": max_val in merged
  Confidence: band=very_high; score=0.99
- Line 1786: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: auto pos = merged.find(search);
  Confidence: band=very_high; score=0.99
- Line 1790: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
  Confidence: band=very_high; score=0.99
- Line 2046: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
  Confidence: band=very_high; score=0.99
- Line 2048: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes.empty()) return MMWriteEntry{};
  Confidence: band=very_high; score=0.99
- Line 2050: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string merged_data;
  Confidence: band=very_high; score=0.99
- Line 2052: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
  Confidence: band=very_high; score=0.99
- Line 2053: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
  Confidence: band=very_high; score=0.99
- Line 2054: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
  Confidence: band=very_high; score=0.99
- Line 2055: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
  Confidence: band=very_high; score=0.99
- Line 2056: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::G_SET:        merged_data = mergeGSet(conflicting_writes);        break;
  Confidence: band=very_high; score=0.99
- Line 2057: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::OR_SET:       merged_data = mergeORSet(conflicting_writes);       break;
  Confidence: band=very_high; score=0.99
- Line 2058: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::LWW_MAP:      merged_data = mergeLWWMap(conflicting_writes);      break;
  Confidence: band=very_high; score=0.99
- Line 2059: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::TWO_P_SET:    merged_data = mergeTwoPSet(conflicting_writes);     break;
  Confidence: band=very_high; score=0.99
- Line 2060: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::RGA:          merged_data = mergeRGA(conflicting_writes);         break;
  Confidence: band=very_high; score=0.99
- Line 2061: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::FLAG_EW:      merged_data = mergeFlagEW(conflicting_writes);      break;
  Confidence: band=very_high; score=0.99
- Line 2062: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::FLAG_DW:      merged_data = mergeFlagDW(conflicting_writes);      break;
  Confidence: band=very_high; score=0.99
- Line 2065: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Base entry is the LWW winner; replace its data with the merged payload
  Confidence: band=very_high; score=0.99
- Line 2067: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 2068: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.data = merged_data;
  Confidence: band=very_high; score=0.99
- Line 2072: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::strategyName() const {
  Confidence: band=very_high; score=0.99
- Line 2098: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeMVRegister(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2099: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Multi-value register: return all concurrent values as a JSON array
  Confidence: band=very_high; score=0.99
- Line 2162: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator qs may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto qs = arr.find('"', p);
- Line 2164: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator qe may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto qe = arr.find('"', qs + 1);
- Line 2166: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(arr.substr(qs + 1, qe - qs - 1));
  Confidence: band=very_high; score=0.99
- Line 2189: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2191: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::map<std::string, int64_t> merged;
  Confidence: band=very_high; score=0.99
- Line 2210: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2213: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge: take max per key for both P and N sub-counters.
  Confidence: band=very_high; score=0.99
- Line 2214: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::map<std::string, int64_t> mergedP, mergedN;
  Confidence: band=very_high; score=0.99
- Line 2219: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: mergedP[k] = std::max(mergedP[k], v);
  Confidence: band=very_high; score=0.99
- Line 2224: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: mergedN[k] = std::max(mergedN[k], v);
  Confidence: band=very_high; score=0.99
- Line 2245: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2255: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(w.data.substr(qs + 1, qe - qs - 1));
  Confidence: band=very_high; score=0.99
- Line 2320: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
  Confidence: band=very_high; score=0.99
- Line 2359: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeTwoPSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2364: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge: union(add) across writes, union(remove) across writes.
  Confidence: band=very_high; score=0.99
- Line 2365: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Result: elements that appear in the merged add-set but NOT in the merged remove-set.
  Confidence: band=very_high; score=0.99
- Line 2463: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge: deletion is irrevocable
  Confidence: band=very_high; score=0.99
- Line 2469: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: // Keep the value from the first observed insert (already stored)
  Confidence: band=very_high; score=0.99
- Line 2490: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeFlagEW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2491: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Enable-Wins Flag: concurrent enable + disable → enabled.
  Confidence: band=very_high; score=0.99
- Line 2497: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.99
- Line 2504: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: enableTags.insert(t);
  Confidence: band=very_high; score=0.99
- Line 2507: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: disableTags.insert(t);
  Confidence: band=very_high; score=0.99
- Line 2518: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeFlagDW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2519: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Disable-Wins Flag: concurrent enable + disable → disabled.
  Confidence: band=very_high; score=0.99
- Line 2524: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.99
- Line 2561: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), s.begin(), s.end());
  Confidence: band=very_high; score=0.99
- Line 2630: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
  Confidence: band=very_high; score=0.99
- Line 2633: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return resolver_(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 2712: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: std::string MultiMasterReplicationManager::write(
- Line 2789: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: MultiMasterReplicationManager::ReadResult MultiMasterReplicationManager::read(
- Line 2901: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Conflict Management
  Confidence: band=very_high; score=0.99
- Line 2919: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<ConflictRecord> result;
  Confidence: band=very_high; score=0.99
- Line 2920: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 2928: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool MultiMasterReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.99
- Line 2929: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::string& conflict_id,
  Confidence: band=very_high; score=0.99
- Line 2932: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 2933: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 2934: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (rec.conflict_id == conflict_id && !rec.resolved) {
  Confidence: band=very_high; score=0.99
- Line 2937: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats_conflicts_resolved_.fetch_add(1);
  Confidence: band=very_high; score=0.99
- Line 2970: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: s.conflicts_detected    = stats_conflicts_detected_.load();
  Confidence: band=very_high; score=0.99
- Line 2971: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: s.conflicts_resolved    = stats_conflicts_resolved_.load();
  Confidence: band=very_high; score=0.99
- Line 3059: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# HELP themisdb_mm_conflicts_detected Conflicts detected\n"
  Confidence: band=very_high; score=0.99
- Line 3060: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# TYPE themisdb_mm_conflicts_detected counter\n"
  Confidence: band=very_high; score=0.99
- Line 3061: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "themisdb_mm_conflicts_detected{node=\"" << config_.node_id << "\"} " << s.conflicts_detected << "\n"
  Confidence: band=very_high; score=0.99
- Line 3062: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
  Confidence: band=very_high; score=0.99
- Line 3063: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# TYPE themisdb_mm_conflicts_resolved counter\n"
  Confidence: band=very_high; score=0.99
- Line 3064: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
  Confidence: band=very_high; score=0.99
- Line 3094: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = write_callbacks_.find(entry.write_id);
- Line 3222: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: vector_clock_->merge(incoming.vector_clock);
  Confidence: band=very_high; score=0.99
- Line 3229: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Check for conflict with any recently-seen write for the same document
  Confidence: band=very_high; score=0.99
- Line 3231: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool has_conflict = false;
  Confidence: band=very_high; score=0.99
- Line 3233: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3234: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 3238: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: has_conflict = true;
  Confidence: band=very_high; score=0.99
- Line 3244: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (has_conflict) {
  Confidence: band=very_high; score=0.99
- Line 3245: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: THEMIS_WARN("MM conflict detected for doc={}/{} from peer={}",
  Confidence: band=very_high; score=0.99
- Line 3247: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // For each unresolved conflict on this document, add the incoming entry
  Confidence: band=very_high; score=0.99
- Line 3248: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3249: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 3253: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: rec.conflicting_writes.push_back(incoming);
  Confidence: band=very_high; score=0.99
- Line 3276: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
  Confidence: band=very_high; score=0.99
- Line 3278: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes.empty()) return;
  Confidence: band=very_high; score=0.99
- Line 3280: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::string& collection = conflicting_writes[0].collection;
  Confidence: band=very_high; score=0.99
- Line 3283: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::shared_ptr<ConflictResolver> resolver;
  Confidence: band=very_high; score=0.99
- Line 3290: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: MMWriteEntry winner = resolver->resolve(document_id, conflicting_writes);
- Line 3305: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3306: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_.push_back(record);
  Confidence: band=very_high; score=0.99
- Line 3408: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = last_done_per_doc_.find(entry.document_id);
- Line 3502: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // has been fully applied, even across concurrent workers.
  Confidence: band=very_high; score=0.99
- Line 3526: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: QuorumReadManager::QuorumReadResult QuorumReadManager::read(
- Line 3979: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::string input(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.99
- Line 4044: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::string input(reinterpret_cast<const char*>(compressed.data()),
  Confidence: band=very_high; score=0.99
- Line 4225: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lag_history_.find(replica_id);
- Line 4881: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
  Confidence: band=very_high; score=0.99
- Line 4892: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.insert(out.end(), iv.begin(), iv.end());
  Confidence: band=very_high; score=0.99
- Line 4893: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.insert(out.end(), tag.begin(), tag.end());
  Confidence: band=very_high; score=0.99
- Line 4894: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.insert(out.end(), ciphertext.begin(), ciphertext.begin() + ct_len);
  Confidence: band=very_high; score=0.99
- Line 4918: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: EVP_DecryptUpdate(ctx, plain.data(), &len, ct, ct_len);
  Confidence: band=very_high; score=0.99
- Line 5367: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: MultiRegionActiveActiveManager::write(
  Confidence: band=very_high; score=0.99
- Line 5367: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: MultiRegionActiveActiveManager::write(
- Line 5402: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: MultiRegionActiveActiveManager::read(
- Line 5661: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: uint64_t BidirectionalReplicationManager::submitWrite(
  Confidence: band=very_high; score=0.99
- Line 5727: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Conflict detection: check whether we have a pending local write for the
  Confidence: band=very_high; score=0.99
- Line 5731: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_writes_.find(key);
- Line 5733: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (detectConflict(entry, it->second)) {
  Confidence: band=very_high; score=0.99
- Line 5734: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: handleConflict(it->second, entry, entry.is_ddl);
  Confidence: band=very_high; score=0.99
- Line 5736: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // the conflict history.
  Confidence: band=very_high; score=0.99
- Line 5787: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::count_if(conflict_timestamps_.begin(), conflict_timestamps_.end(),
  Confidence: band=very_high; score=0.99
- Line 5812: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<BidiConflictRecord> result;
  Confidence: band=very_high; score=0.99
- Line 5813: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& rec : conflict_history_) {
  Confidence: band=very_high; score=0.99
- Line 5814: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (rec.strategy_used == ConflictResolution::CUSTOM
  Confidence: band=very_high; score=0.99
- Line 5822: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // ── Conflict resolution ───────────────────────────────────────────────────────
  Confidence: band=very_high; score=0.99
- Line 5824: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool BidirectionalReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.99
- Line 5833: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lk(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 5834: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Walk in reverse to find the most-recent conflict for this document.
  Confidence: band=very_high; score=0.99
- Line 5835: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto it = conflict_history_.rbegin(); it != conflict_history_.rend(); ++it) {
  Confidence: band=very_high; score=0.99
- Line 5837: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->resolved_write = (winner_node == config_.local_node_id)
- Line 5840: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_.fetch_add(1);
  Confidence: band=very_high; score=0.99
- Line 5851: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: ConflictResolution strategy)
  Confidence: band=very_high; score=0.99
- Line 5856: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: ConflictResolution BidirectionalReplicationManager::getEffectiveStrategy(
  Confidence: band=very_high; score=0.99
- Line 5900: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: return applyRemoteWrite(entry);
  Confidence: band=very_high; score=0.99
- Line 5962: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool BidirectionalReplicationManager::detectConflict(
  Confidence: band=very_high; score=0.99
- Line 5967: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // are always considered concurrent — a true conflict.
  Confidence: band=very_high; score=0.99
- Line 5975: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void BidirectionalReplicationManager::handleConflict(
  Confidence: band=very_high; score=0.99
- Line 6007: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflict_timestamps_.pop_front();
  Confidence: band=very_high; score=0.99
- Line 6012: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (strategy != ConflictResolution::CUSTOM) {
  Confidence: band=very_high; score=0.99
- Line 6013: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_.fetch_add(1);
  Confidence: band=very_high; score=0.99
- Line 6177: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool GeoReplicationManager::write(
  Confidence: band=very_high; score=0.99
- Line 6177: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool GeoReplicationManager::write(
- Line 6215: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: std::optional<std::string> GeoReplicationManager::read(
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 92: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto readUint64 = [&data, &pos]() -> uint64_t {
- Line 100: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto readString = [&data, &pos]() -> std::string {
- Line 189: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(data.data()), len);
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: THEMIS_ERROR("WAL segment {}: incomplete read (expected {} bytes, got {})",
  Confidence: band=very_high; score=0.9
- Line 382: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.wal_directory)) {
- Line 395: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.wal_directory)) {
- Line 401: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
  Confidence: band=very_high; score=0.9
- Line 405: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(data.data()), len);
  Confidence: band=very_high; score=0.9
- Line 447: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread(&LeaderElection::electionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 704: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: stream_thread_ = std::thread(&ReplicationStream::streamLoop, this);
  Confidence: band=very_high; score=0.9
- Line 723: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
- Line 743: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.batch_timeout_ms));
- Line 825: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread(&ReplicationManager::heartbeatLoop, this);
  Confidence: band=very_high; score=0.9
- Line 826: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: compaction_thread_ = std::thread(&ReplicationManager::compactionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 827: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_monitor_thread_ = std::thread(&ReplicationManager::healthMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 857: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function replicate without trace point
  Context: bool ReplicationManager::replicate(const WALEntry& entry) {
  Confidence: band=very_high; score=0.9
- Line 928: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 997: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void ReplicationManager::setConflictResolver(std::shared_ptr<IConflictResolver> resolver) {
  Confidence: band=very_high; score=0.9
- Line 1134: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 1297: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 1416: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ReplicationManager::LeaseReadResult ReplicationManager::leaseRead(
  Confidence: band=very_high; score=0.9
- Line 1725: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Build merged document: walk the remote document and for numeric fields
  Confidence: band=very_high; score=0.9
- Line 1730: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Scan both documents for numeric fields and merge with max semantics.
  Confidence: band=very_high; score=0.9
- Line 1732: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string merged = base;
  Confidence: band=very_high; score=0.9
- Line 1775: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // For each field present in both documents, patch the merged document with max value
  Confidence: band=very_high; score=0.9
- Line 1784: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Replace "key": cur_val with "key": max_val in merged
  Confidence: band=very_high; score=0.9
- Line 1786: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto pos = merged.find(search);
  Confidence: band=very_high; score=0.9
- Line 1790: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
  Confidence: band=very_high; score=0.9
- Line 1793: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (vend < merged.size() && merged[vend] == '-') ++vend;
  Confidence: band=very_high; score=0.9
- Line 1794: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: while (vend < merged.size() &&
  Confidence: band=very_high; score=0.9
- Line 1795: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::isdigit(static_cast<unsigned char>(merged[vend]))) {
  Confidence: band=very_high; score=0.9
- Line 1798: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = merged.substr(0, vp) + std::to_string(max_val) + merged.substr(vend);
  Confidence: band=very_high; score=0.9
- Line 1803: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 1892: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> wlock(mutex_);
- Line 1901: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> wlock(mutex_);
- Line 1913: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=very_high; score=0.9
- Line 1917: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // two threads concurrently call A.merge(B) and B.merge(A).
  Confidence: band=very_high; score=0.9
- Line 1929: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=very_high; score=0.9
- Line 2040: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: CRDTMergeResolver::CRDTMergeResolver(CRDTType type)
  Confidence: band=very_high; score=0.9
- Line 2044: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry CRDTMergeResolver::resolve(
  Confidence: band=very_high; score=0.9
- Line 2050: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string merged_data;
  Confidence: band=very_high; score=0.9
- Line 2052: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
  Confidence: band=very_high; score=0.9
- Line 2053: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
  Confidence: band=very_high; score=0.9
- Line 2054: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
  Confidence: band=very_high; score=0.9
- Line 2055: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
  Confidence: band=very_high; score=0.9
- Line 2067: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 2068: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.data = merged_data;
  Confidence: band=very_high; score=0.9
- Line 2072: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::strategyName() const {
  Confidence: band=very_high; score=0.9
- Line 2089: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeLWWRegister(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2098: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeMVRegister(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2189: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2191: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, int64_t> merged;
  Confidence: band=very_high; score=0.9
- Line 2195: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[k] = std::max(merged[k], v);
  Confidence: band=very_high; score=0.9
- Line 2201: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& [k, v] : merged) {
  Confidence: band=very_high; score=0.9
- Line 2210: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2213: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: take max per key for both P and N sub-counters.
  Confidence: band=very_high; score=0.9
- Line 2214: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, int64_t> mergedP, mergedN;
  Confidence: band=very_high; score=0.9
- Line 2218: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& [k, v] : extractJsonInts(pSub))
- Line 2219: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergedP[k] = std::max(mergedP[k], v);
  Confidence: band=very_high; score=0.9
- Line 2223: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& [k, v] : extractJsonInts(nSub))
- Line 2224: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergedN[k] = std::max(mergedN[k], v);
  Confidence: band=very_high; score=0.9
- Line 2241: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: oss << "{\"P\":" << serializeMap(mergedP) << ",\"N\":" << serializeMap(mergedN) << "}";
  Confidence: band=very_high; score=0.9
- Line 2245: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2250: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto qs = w.data.find('"', p);
- Line 2250: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto qs = w.data.find('"', p);
- Line 2252: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto qe = w.data.find('"', qs + 1);
- Line 2271: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeORSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2279: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: union of all add pairs, union of all tombstones.
  Confidence: band=very_high; score=0.9
- Line 2288: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& t : extractJsonArrayStrings(tsArr))
- Line 2295: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lb = addArr.find('[', p);
- Line 2296: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto lb = addArr.find('[', p);
  Confidence: band=very_high; score=0.9
- Line 2297: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto rb = addArr.find(']', lb + 1);
- Line 2298: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto rb = addArr.find(']', lb + 1);
  Confidence: band=very_high; score=0.9
- Line 2319: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
- Line 2320: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
  Confidence: band=very_high; score=0.9
- Line 2340: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = best.find(k);
- Line 2340: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = best.find(k);
- Line 2341: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = best.find(k);
  Confidence: band=very_high; score=0.9
- Line 2359: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeTwoPSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2364: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: union(add) across writes, union(remove) across writes.
  Confidence: band=very_high; score=0.9
- Line 2365: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Result: elements that appear in the merged add-set but NOT in the merged remove-set.
  Confidence: band=very_high; score=0.9
- Line 2370: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& e : extractJsonArrayStrings(addArr))    addSet.insert(e);
- Line 2371: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& e : extractJsonArrayStrings(removeArr)) removeSet.insert(e);
- Line 2387: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeRGA(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2395: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: union of all elements by unique id; if an id appears in multiple writes, prefer
  Confidence: band=very_high; score=0.9
- Line 2414: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto ob = src.find('{', p);
  Confidence: band=very_high; score=0.9
- Line 2463: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: deletion is irrevocable
  Confidence: band=very_high; score=0.9
- Line 2490: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeFlagEW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2497: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.9
- Line 2503: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& t : extractJsonArrayStrings(eArr))
- Line 2506: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& t : extractJsonArrayStrings(dArr))
- Line 2511: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (disableTags.find(t) == disableTags.end()) { enabled = true; break; }
  Confidence: band=very_high; score=0.9
- Line 2518: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeFlagDW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2524: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.9
- Line 2530: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& t : extractJsonArrayStrings(eArr))
- Line 2533: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& t : extractJsonArrayStrings(dArr))
- Line 2637: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return lwr.resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 2682: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: replication_thread_ = std::thread(&MultiMasterReplicationManager::replicationLoop, this);
  Confidence: band=very_high; score=0.9
- Line 2683: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_   = std::thread(&MultiMasterReplicationManager::heartbeatLoop,   this);
  Confidence: band=very_high; score=0.9
- Line 2684: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sync_thread_        = std::thread(&MultiMasterReplicationManager::syncLoop,        this);
  Confidence: band=very_high; score=0.9
- Line 2745: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
  Confidence: band=very_high; score=0.9
- Line 2782: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return future.wait_for(timeout) == std::future_status::ready && future.get();
  Confidence: band=very_high; score=0.9
- Line 2789: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: MultiMasterReplicationManager::ReadResult MultiMasterReplicationManager::read(
  Confidence: band=very_high; score=0.9
- Line 2917: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<ConflictRecord> MultiMasterReplicationManager::getUnresolvedConflicts() const {
  Confidence: band=very_high; score=0.9
- Line 2920: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.9
- Line 2928: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: bool MultiMasterReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.9
- Line 2971: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: s.conflicts_resolved    = stats_conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 3062: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
  Confidence: band=very_high; score=0.9
- Line 3064: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
  Confidence: band=very_high; score=0.9
- Line 3084: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(writes_mutex_);
- Line 3093: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = write_callbacks_.find(entry.write_id);
- Line 3104: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [entry, cb] : batch) {
  Confidence: band=very_high; score=0.9
- Line 3130: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.heartbeat_interval_ms));
  Confidence: band=very_high; score=0.9
- Line 3130: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.heartbeat_interval_ms));
- Line 3146: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(config_.sync_interval_ms));
- Line 3222: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: vector_clock_->merge(incoming.vector_clock);
  Confidence: band=very_high; score=0.9
- Line 3234: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.9
- Line 3247: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // For each unresolved conflict on this document, add the incoming entry
  Confidence: band=very_high; score=0.9
- Line 3290: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry winner = resolver->resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 3338: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : missing) {
  Confidence: band=very_high; score=0.9
- Line 3390: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : workers_) {
  Confidence: band=very_high; score=0.9
- Line 3433: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 3436: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
- Line 3463: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 3464: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: queue_cv_.wait_for(lock, std::chrono::milliseconds(5),
- Line 3526: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: QuorumReadManager::QuorumReadResult QuorumReadManager::read(
  Confidence: band=very_high; score=0.9
- Line 3616: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto resp = fut.get();
  Confidence: band=very_high; score=0.9
- Line 3651: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<ReplicaResponse> reconcile_set;
  Confidence: band=very_high; score=0.9
- Line 3653: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: reconcile_set = responses;
  Confidence: band=very_high; score=0.9
- Line 3655: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: reconcile_set.reserve(qualifying.size());
  Confidence: band=very_high; score=0.9
- Line 3656: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto* p : qualifying) reconcile_set.push_back(*p);
  Confidence: band=very_high; score=0.9
- Line 3659: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Reconcile: pick the response with the highest version
  Confidence: band=very_high; score=0.9
- Line 3660: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const ReplicaResponse* best = &reconcile_set[0];
  Confidence: band=very_high; score=0.9
- Line 3662: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& r : reconcile_set) {
  Confidence: band=very_high; score=0.9
- Line 3670: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& r : reconcile_set) {
  Confidence: band=very_high; score=0.9
- Line 3682: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.data          = best->data;
- Line 3685: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& r : reconcile_set) {
  Confidence: band=very_high; score=0.9
- Line 3746: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(replicas_mutex_);
- Line 4005: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // compressed inputs whose expansion is dominated by header overhead.
  Confidence: band=very_high; score=0.9
- Line 4109: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: flush_thread_ = std::thread(&BatchedAckTracker::flushLoop, this);
  Confidence: band=very_high; score=0.9
- Line 4177: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(pending_mutex_);
- Line 4178: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: flush_cv_.wait_for(lock,
  Confidence: band=very_high; score=0.9
- Line 4266: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ins.metadata["replica_id"] = replica_id;
- Line 4267: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ins.metadata["lag_ms"]     = std::to_string(last_lag);
- Line 4285: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ins.metadata["replica_id"] = replica_id;
- Line 4286: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ins.metadata["avg_lag_ms"] = std::to_string(avg);
- Line 4315: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // A high value indicates large relative spread (typical of network jitter).
  Confidence: band=very_high; score=0.9
- Line 4330: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: b.details         = "High lag spread (normalized_range=" +
  Confidence: band=very_high; score=0.9
- Line 4649: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& op : include_operations) {
  Confidence: band=very_high; score=0.9
- Line 5371: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::string& data,
- Line 5402: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: MultiRegionActiveActiveManager::read(
  Confidence: band=very_high; score=0.9
- Line 5533: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : region_staleness_) {
  Confidence: band=very_high; score=0.9
- Line 5734: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handleConflict(it->second, entry, entry.is_ddl);
- Line 5778: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: s.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 5813: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : conflict_history_) {
  Confidence: band=very_high; score=0.9
- Line 5824: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: bool BidirectionalReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.9
- Line 5954: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Return an empty placeholder; the application must call resolveConflict().
  Confidence: band=very_high; score=0.9
- Line 6215: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<std::string> GeoReplicationManager::read(
  Confidence: band=very_high; score=0.9
- Line 49: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALEntry::serialize()
  Context: std::vector<uint8_t> WALEntry::serialize() const {
  Confidence: band=medium; score=0.56
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
- Line 61: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>(len & 0xFF));
- Line 240: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALManager::append(const WALEntry& entry)
  Context: uint64_t WALManager::append(const WALEntry& entry) {
  Confidence: band=medium; score=0.56
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(*entry);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(*entry);
- Line 531: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // If term is stale, reject
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: !commit_index_.compare_exchange_weak(expected, new_commit)) {}
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replicas_.push_back(replica);
  Confidence: band=high; score=0.74
- Line 812: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: replicas_.push_back(replica);
- Line 1017: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: listeners_.push_back(listener);
- Line 1083: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replicas_.push_back(replica);
  Confidence: band=high; score=0.74
- Line 1084: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: replicas_.push_back(replica);
- Line 1092: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: streams_.push_back(std::move(stream));
- Line 1223: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: << "# TYPE themisdb_cluster_nodes_healthy gauge\n";
- Line 1230: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: oss << "themisdb_cluster_nodes_healthy " << healthy_count << "\n";
- Line 1313: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::pair<std::string, HealthStatus>> result;
- Line 1315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1331: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: bool counts_as_healthy;
- Line 1343: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: healthy_voting_members++;
- Line 1351: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: healthy_voting_members++;
- Line 1363: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HealthStatus old_status;
- Line 1364: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HealthStatus new_status;
- Line 1373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes.push_back({replica.node_id, old_status, replica.health_status});
  Confidence: band=high; score=0.74
- Line 1374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changes.push_back({replica.node_id, old_status, replica.health_status});
- Line 1489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unreachable_nodes.push_back(replica.node_id);
  Confidence: band=high; score=0.74
- Line 1490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: unreachable_nodes.push_back(replica.node_id);
- Line 1682: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1737: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: -> std::map<std::string, int64_t>
  Confidence: band=high; score=0.74
- Line 1739: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int64_t> fields;
  Confidence: band=high; score=0.74
- Line 1764: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1816: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Context: HybridLogicalClock::Timestamp HybridLogicalClock::now() {
  Confidence: band=medium; score=0.56
- Line 1908: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::increment(const std::string& node_id)
  Context: void VectorClock::increment(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 1913: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::merge(const VectorClock& other)
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=medium; score=0.56
- Line 1929: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::get(const std::string& node_id)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=medium; score=0.56
- Line 1935: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::compare(const VectorClock& other)
  Context: int VectorClock::compare(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 1968: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::happensBefore(const VectorClock& other)
  Context: bool VectorClock::happensBefore(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 1972: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::isConcurrent(const VectorClock& other)
  Context: bool VectorClock::isConcurrent(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 2013: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2113: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static std::map<std::string, int64_t> extractJsonInts(const std::string& doc) {
  Confidence: band=high; score=0.74
- Line 2114: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int64_t> fields;
  Confidence: band=high; score=0.74
- Line 2132: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2214: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int64_t> mergedP, mergedN;
  Confidence: band=high; score=0.74
- Line 2228: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: auto serializeMap = [](const std::map<std::string, int64_t>& m) {
  Confidence: band=high; score=0.74
- Line 2305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allAdds.emplace_back(elem, tag);
  Confidence: band=high; score=0.74
- Line 2313: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> elemTags;
  Confidence: band=high; score=0.74
- Line 2314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elemTags[elem].push_back(tag);
  Confidence: band=high; score=0.74
- Line 2315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elemTags[elem].push_back(tag);
- Line 2337: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::pair<HybridLogicalClock::Timestamp, std::string>> best;
  Confidence: band=high; score=0.74
- Line 2405: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, RGAElem> byId;
  Confidence: band=high; score=0.74
- Line 2550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
- Line 2554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
- Line 2559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2560: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
- Line 2813: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers_snapshot.push_back(info);
  Confidence: band=high; score=0.74
- Line 2814: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: peers_snapshot.push_back(info);
- Line 2821: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // not yet received — that peer signals a potential stale read.
  Confidence: band=high; score=0.74
- Line 2823: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: bool stale_read_detected = false;
  Confidence: band=high; score=0.74
- Line 2843: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: stale_read_detected = true;
  Confidence: band=high; score=0.74
- Line 2853: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (stale_read_detected) {
  Confidence: band=high; score=0.74
- Line 2854: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: THEMIS_WARN("MultiMasterRead: potential stale read detected for node={} "
  Confidence: band=high; score=0.74
- Line 2882: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 2883: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(info);
- Line 2921: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 2922: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(rec);
- Line 3025: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 3026: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.nodes.push_back(std::move(node));
- Line 3032: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.edges.push_back(std::move(edge));
- Line 3039: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.edges.push_back(std::move(reverse_edge));
- Line 3098: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.emplace_back(std::move(entry), std::move(cb));
  Confidence: band=high; score=0.74
- Line 3112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: committed_writes_log_.push_back(entry);
  Confidence: band=high; score=0.74
- Line 3113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: committed_writes_log_.push_back(entry);
- Line 3152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 3152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 3152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 3153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: peer_ids.push_back(id);
- Line 3252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec.conflicting_writes.push_back(incoming);
  Confidence: band=high; score=0.74
- Line 3252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec.conflicting_writes.push_back(incoming);
  Confidence: band=high; score=0.74
- Line 3253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec.conflicting_writes.push_back(incoming);
- Line 3366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(entry);
- Line 3382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&ParallelReplicationWorker::workerLoop, this);
  Confidence: band=high; score=0.74
- Line 3409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: item.deps.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 3410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: item.deps.push_back(it->second);
- Line 3470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(work_queue_.front()));
  Confidence: band=high; score=0.74
- Line 3471: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(std::move(work_queue_.front()));
- Line 3475: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(std::move(work_queue_.front()));
- Line 3596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 3597: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 3606: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // total responses would discard fresh replicas that come after a stale one
  Confidence: band=high; score=0.74
- Line 3616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (resp.ok) responses.push_back(std::move(resp));
  Confidence: band=high; score=0.74
- Line 3617: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (resp.ok) responses.push_back(std::move(resp));
- Line 3637: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qualifying.push_back(&r);
- Line 3650: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // session token was supplied so that stale replicas are not considered).
  Confidence: band=high; score=0.74
- Line 3656: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto* p : qualifying) reconcile_set.push_back(*p);
- Line 3668: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // identify stale replicas and schedule repair.
  Confidence: band=high; score=0.74
- Line 3685: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.sources.push_back(r.endpoint);
  Confidence: band=high; score=0.74
- Line 3685: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.sources.push_back(r.endpoint);
  Confidence: band=high; score=0.74
- Line 3686: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.sources.push_back(r.endpoint);
- Line 3726: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3740: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3860: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3925: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += std::to_string(e.sequence_number) + "|"
  Confidence: band=high; score=0.74
- Line 4231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data_points.push_back(dp);
  Confidence: band=high; score=0.74
- Line 4232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.data_points.push_back(dp);
- Line 4233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(dp.lag_ms);
- Line 4268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: insights.push_back(std::move(ins));
- Line 4274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& dp : history) values.push_back(dp.lag_ms);
- Line 4286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: insights.push_back(std::move(ins));
  Confidence: band=high; score=0.74
- Line 4287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: insights.push_back(std::move(ins));
- Line 4306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& dp : history) values.push_back(dp.lag_ms);
- Line 4544: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies_us.push_back(
  Confidence: band=high; score=0.74
- Line 4545: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latencies_us.push_back(
- Line 4580: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "Throughput:    " << static_cast<uint64_t>(r.writes_per_second) << " writes/sec\n"
- Line 4624: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4715: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4728: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: m += "themisdb_cross_cluster_publication_published_total" + label + " " +
  Confidence: band=high; score=0.74
- Line 4764: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (last_applied_seq_.compare_exchange_weak(expected, e.sequence_number))
  Confidence: band=high; score=0.74
- Line 4767: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bytes.push_back(static_cast<uint8_t>(val));
  Confidence: band=high; score=0.74
- Line 4853: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bytes.push_back(static_cast<uint8_t>(val));
- Line 4887: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 4923: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 4996: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: index_.push_back(seg);
- Line 5035: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { segment_id = 0; }
- Line 5091: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dst.close();
- Line 5104: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: index_.push_back(meta);
- Line 5283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(entry.path().filename().string());
  Confidence: band=high; score=0.74
- Line 5284: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(entry.path().filename().string());
- Line 5314: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Initialise staleness entries for peer regions (unknown at start)
  Confidence: band=high; score=0.74
- Line 5316: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: RegionStalenessInfo info;
  Confidence: band=high; score=0.74
- Line 5318: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms          = std::numeric_limits<int64_t>::max();
  Confidence: band=high; score=0.74
- Line 5322: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: region_staleness_[peer]    = info;
  Confidence: band=high; score=0.74
- Line 5361: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 5379: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Update local region staleness to 0 (we just wrote here)
  Confidence: band=high; score=0.74
- Line 5381: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5382: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto& local = region_staleness_[config_.local_region_id];
  Confidence: band=high; score=0.74
- Line 5383: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: local.staleness_ms           = 0;
  Confidence: band=high; score=0.74
- Line 5411: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t local_staleness_ms = 0;
  Confidence: band=high; score=0.74
- Line 5414: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5415: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(config_.local_region_id);
  Confidence: band=high; score=0.74
- Line 5416: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it != region_staleness_.end()) {
  Confidence: band=high; score=0.74
- Line 5417: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: local_staleness_ms = it->second.staleness_ms;
  Confidence: band=high; score=0.74
- Line 5424: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: result.staleness_ms = local_staleness_ms;
  Confidence: band=high; score=0.74
- Line 5431: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (local_staleness_ms > 0) {
  Confidence: band=high; score=0.74
- Line 5433: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "local staleness={}ms > 0", local_staleness_ms);
  Confidence: band=high; score=0.74
- Line 5434: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++staleness_rejections_;
  Confidence: band=high; score=0.74
- Line 5440: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::BOUNDED_STALENESS:
  Confidence: band=high; score=0.74
- Line 5441: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++bounded_staleness_reads_;
  Confidence: band=high; score=0.74
- Line 5442: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (local_staleness_ms > static_cast<int64_t>(config_.max_staleness_ms)) {
  Confidence: band=high; score=0.74
- Line 5443: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: THEMIS_WARN("MultiRegionActiveActive: BOUNDED_STALENESS read rejected – "
  Confidence: band=high; score=0.74
- Line 5444: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: "local staleness={}ms > bound={}ms",
  Confidence: band=high; score=0.74
- Line 5445: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: local_staleness_ms, config_.max_staleness_ms);
  Confidence: band=high; score=0.74
- Line 5446: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++staleness_rejections_;
  Confidence: band=high; score=0.74
- Line 5469: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Always succeeds regardless of staleness
  Confidence: band=high; score=0.74
- Line 5474: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: THEMIS_INFO("MultiRegionActiveActive: read served region={} staleness={}ms",
  Confidence: band=high; score=0.74
- Line 5475: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: config_.local_region_id, local_staleness_ms);
  Confidence: band=high; score=0.74
- Line 5500: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 5509: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::chrono::milliseconds MultiRegionActiveActiveManager::getStaleness(
  Confidence: band=high; score=0.74
- Line 5512: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5513: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(region_id);
  Confidence: band=high; score=0.74
- Line 5514: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it == region_staleness_.end()) {
  Confidence: band=high; score=0.74
- Line 5517: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return std::chrono::milliseconds(it->second.staleness_ms);
  Confidence: band=high; score=0.74
- Line 5520: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: bool MultiRegionActiveActiveManager::isWithinStalenessBound(
  Confidence: band=high; score=0.74
- Line 5523: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return getStaleness(region_id) <=
  Confidence: band=high; score=0.74
- Line 5524: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::chrono::milliseconds(config_.max_staleness_ms);
  Confidence: band=high; score=0.74
- Line 5527: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::vector<RegionStalenessInfo>
  Confidence: band=high; score=0.74
- Line 5528: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: MultiRegionActiveActiveManager::getAllRegionStaleness() const
  Confidence: band=high; score=0.74
- Line 5530: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5531: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::vector<RegionStalenessInfo> result;
  Confidence: band=high; score=0.74
- Line 5532: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: result.reserve(region_staleness_.size());
  Confidence: band=high; score=0.74
- Line 5533: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& kv : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 5533: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 5534: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.second);
- Line 5539: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void MultiRegionActiveActiveManager::updateRegionStaleness(
  Confidence: band=high; score=0.74
- Line 5541: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t            staleness_ms,
  Confidence: band=high; score=0.74
- Line 5544: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5545: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto& info              = region_staleness_[region_id];
  Confidence: band=high; score=0.74
- Line 5547: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms       = staleness_ms;
  Confidence: band=high; score=0.74
- Line 5550: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.is_healthy         = (staleness_ms >= 0 &&
  Confidence: band=high; score=0.74
- Line 5551: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: staleness_ms <= static_cast<int64_t>(config_.max_staleness_ms) * 2);
  Confidence: band=high; score=0.74
- Line 5568: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_mraaa_staleness_rejections_total Reads rejected due to excessive staleness\n"
  Confidence: band=high; score=0.74
- Line 5569: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_mraaa_staleness_rejections_total counter\n"
  Confidence: band=high; score=0.74
- Line 5570: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "themisdb_mraaa_staleness_rejections_total{region=\"" << r << "\"} "
  Confidence: band=high; score=0.74
- Line 5571: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << staleness_rejections_.load() << "\n\n";
  Confidence: band=high; score=0.74
- Line 5578: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_mraaa_bounded_staleness_reads_total Reads served at BOUNDED_STALENESS\n"
  Confidence: band=high; score=0.74
- Line 5579: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_mraaa_bounded_staleness_reads_total counter\n"
  Confidence: band=high; score=0.74
- Line 5580: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "themisdb_mraaa_bounded_staleness_reads_total{region=\"" << r << "\"} "
  Confidence: band=high; score=0.74
- Line 5581: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << bounded_staleness_reads_.load() << "\n\n";
  Confidence: band=high; score=0.74
- Line 5593: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Per-region staleness gauges
  Confidence: band=high; score=0.74
- Line 5595: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5596: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_mraaa_region_staleness_ms Current replication staleness per region\n"
  Confidence: band=high; score=0.74
- Line 5597: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_mraaa_region_staleness_ms gauge\n";
  Confidence: band=high; score=0.74
- Line 5598: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& kv : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 5599: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "themisdb_mraaa_region_staleness_ms{region=\"" << kv.first << "\"} "
  Confidence: band=high; score=0.74
- Line 5600: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << kv.second.staleness_ms << "\n";
  Confidence: band=high; score=0.74
- Line 5750: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (remote_sequence_.compare_exchange_weak(cur, entry.origin_seq)) {
  Confidence: band=high; score=0.74
- Line 5773: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SyncStatus s;
- Line 5815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 5816: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(rec);
- Line 5872: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (remote_sequence_.compare_exchange_weak(cur, remote_seq)) {
  Confidence: band=high; score=0.74
- Line 6002: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflict_timestamps_.push_back(std::chrono::system_clock::now());
- Line 6025: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: RegionStalenessInfo local;
  Confidence: band=high; score=0.74
- Line 6027: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: local.staleness_ms           = 0;
  Confidence: band=high; score=0.74
- Line 6031: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: region_staleness_[config_.local_region] = local;
  Confidence: band=high; score=0.74
- Line 6036: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: RegionStalenessInfo info;
  Confidence: band=high; score=0.74
- Line 6038: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms           = std::numeric_limits<int64_t>::max();
  Confidence: band=high; score=0.74
- Line 6042: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: region_staleness_[r]       = info;
  Confidence: band=high; score=0.74
- Line 6069: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 6077: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 6087: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // ── Staleness management ──────────────────────────────────────────────────────
  Confidence: band=high; score=0.74
- Line 6089: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void GeoReplicationManager::updateRegionStaleness(const std::string& region,
  Confidence: band=high; score=0.74
- Line 6090: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t            staleness_ms,
  Confidence: band=high; score=0.74
- Line 6093: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6094: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto& info                  = region_staleness_[region];
  Confidence: band=high; score=0.74
- Line 6096: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms            = staleness_ms;
  Confidence: band=high; score=0.74
- Line 6099: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.is_healthy              = (staleness_ms >= 0);
  Confidence: band=high; score=0.74
- Line 6102: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::chrono::milliseconds GeoReplicationManager::getStaleness(
  Confidence: band=high; score=0.74
- Line 6105: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6106: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(region);
  Confidence: band=high; score=0.74
- Line 6107: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it == region_staleness_.end()) {
  Confidence: band=high; score=0.74
- Line 6110: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return std::chrono::milliseconds(it->second.staleness_ms);
  Confidence: band=high; score=0.74
- Line 6119: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6124: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(config_.local_region);
  Confidence: band=high; score=0.74
- Line 6125: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it != region_staleness_.end() && it->second.staleness_ms == 0) {
  Confidence: band=high; score=0.74
- Line 6129: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& [rid, info] : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 6130: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (info.staleness_ms == 0 && info.is_healthy) return rid;
  Confidence: band=high; score=0.74
- Line 6143: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Pick the region with smallest staleness that is within bound.
  Confidence: band=high; score=0.74
- Line 6146: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& [rid, info] : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 6147: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (info.is_healthy && info.staleness_ms <= bound &&
  Confidence: band=high; score=0.74
- Line 6159: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // so it is safe to call while holding staleness_mutex_ as a shared lock.
  Confidence: band=high; score=0.74
- Line 6161: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(config_.local_region);
  Confidence: band=high; score=0.74
- Line 6162: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it != region_staleness_.end() &&
  Confidence: band=high; score=0.74
- Line 6169: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::EVENTUAL:
  Confidence: band=high; score=0.74
- Line 6186: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto lag = getStaleness(config_.local_region);
  Confidence: band=high; score=0.74
- Line 6199: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6200: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto& local               = region_staleness_[config_.local_region];
  Confidence: band=high; score=0.74
- Line 6201: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: local.staleness_ms        = 0;
  Confidence: band=high; score=0.74
- Line 6227: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::BOUNDED_STALENESS:
  Confidence: band=high; score=0.74
- Line 6228: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++bounded_staleness_reads_;
  Confidence: band=high; score=0.74
- Line 6233: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::EVENTUAL:
  Confidence: band=high; score=0.74
- Line 6234: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++eventual_reads_;
  Confidence: band=high; score=0.74
- Line 6281: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_geo_repl_bounded_staleness_reads_total BOUNDED_STALENESS reads\n"
  Confidence: band=high; score=0.74
- Line 6282: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_geo_repl_bounded_staleness_reads_total counter\n"
  Confidence: band=high; score=0.74
- Line 6283: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "themisdb_geo_repl_bounded_staleness_reads_total" << lbl << " "
  Confidence: band=high; score=0.74
- Line 6284: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << bounded_staleness_reads_.load() << "\n";
  Confidence: band=high; score=0.74
- Line 6296: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Per-region staleness gauge
  Confidence: band=high; score=0.74
- Line 6297: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_geo_repl_region_staleness_ms Replication lag per region (ms)\n"
  Confidence: band=high; score=0.74
- Line 6298: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_geo_repl_region_staleness_ms gauge\n";
  Confidence: band=high; score=0.74
- Line 6300: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6301: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& [rid, info] : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 6302: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t lag = (info.staleness_ms == std::numeric_limits<int64_t>::max())
  Confidence: band=high; score=0.74
- Line 6303: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ? -1 : info.staleness_ms;
  Confidence: band=high; score=0.74
- Line 6304: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "themisdb_geo_repl_region_staleness_ms{region=\"" << rid << "\"} "
  Confidence: band=high; score=0.74

### src/replication/conflict_resolution.cpp
Total findings: 81

- Line 2: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * ThemisDB | File: conflict_resolution.cpp | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
  Confidence: band=very_high; score=0.99
- Line 13: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * ThemisDB Advanced Conflict Resolution Implementation
  Confidence: band=very_high; score=0.99
- Line 19: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: #include "replication/conflict_resolution.h"
  Confidence: band=very_high; score=0.99
- Line 195: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string ThreeWayMergeResolver::mergeJson(
  Confidence: band=very_high; score=0.99
- Line 204: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::map<std::string, std::string> merged;
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& kv : base_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.99
- Line 208: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& kv : left_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.99
- Line 209: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& kv : right_f) merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.99
- Line 211: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto& kv : merged) {
  Confidence: band=very_high; score=0.99
- Line 256: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicting_writes[j].vector_clock))
  Confidence: band=very_high; score=0.99
- Line 267: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (size_t i = 0; i < conflicting_writes.size(); ++i) {
  Confidence: band=very_high; score=0.99
- Line 270: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes[i].hlc < conflicting_writes[left_idx].hlc)  left_idx  = i;
  Confidence: band=very_high; score=0.99
- Line 271: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes[right_idx].hlc < conflicting_writes[i].hlc) right_idx = i;
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (first) return conflicting_writes[base_idx]; // no non-base entries
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MMWriteEntry winner = conflicting_writes[right_idx];
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: winner.data = mergeJson(base.data,
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicting_writes[left_idx].data,
  Confidence: band=very_high; score=0.99
- Line 279: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicting_writes[right_idx].data);
  Confidence: band=very_high; score=0.99
- Line 284: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // FieldLevelMergeResolver
  Confidence: band=very_high; score=0.99
- Line 287: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
  Confidence: band=very_high; score=0.99
- Line 331: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (strategy_ == MergeStrategy::INTERSECT) {
  Confidence: band=very_high; score=0.99
- Line 339: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::LEFT_BIAS:
  Confidence: band=very_high; score=0.99
- Line 340: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged[key] = field_maps[present_indices.front()][key];
  Confidence: band=very_high; score=0.99
- Line 342: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::RIGHT_BIAS:
  Confidence: band=very_high; score=0.99
- Line 343: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged[key] = field_maps[present_indices.back()][key];
  Confidence: band=very_high; score=0.99
- Line 345: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::UNION:
  Confidence: band=very_high; score=0.99
- Line 346: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::INTERSECT:
  Confidence: band=very_high; score=0.99
- Line 348: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Latest HLC wins for conflicting fields
  Confidence: band=very_high; score=0.99
- Line 364: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>&  conflicting_writes,
  Confidence: band=very_high; score=0.99
- Line 367: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes.size() == 1) return conflicting_writes[0];
  Confidence: band=very_high; score=0.99
- Line 369: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MMWriteEntry winner = pickLatestHlc(conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 370: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: winner.data = mergeFields(conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    }', '', '    if (first) return conflicting_writes[base_idx]; // no non-base entries', '', '    MMWriteEntry winner = conflicting_writes[right_idx];']
  Confidence: band=high; score=0.81
- Line 164: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // ThreeWayMergeResolver
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry ThreeWayMergeResolver::selectBase(
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string ThreeWayMergeResolver::mergeJson(
  Confidence: band=very_high; score=0.9
- Line 204: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, std::string> merged;
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& kv : base_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& kv : left_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& kv : right_f) merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (auto& kv : merged) {
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto base_it  = base_f.find(key);
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto left_it  = left_f.find(key);
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto right_it = right_f.find(key);
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // else: neither changed — keep base value (already in merged)
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return buildJson(merged);
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry ThreeWayMergeResolver::resolve(
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: winner.data = mergeJson(base.data,
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: conflicting_writes[right_idx].data);
- Line 284: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // FieldLevelMergeResolver
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string FieldLevelMergeResolver::strategyName() const
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::UNION:      return "FIELD_MERGE_UNION";
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::INTERSECT:  return "FIELD_MERGE_INTERSECT";
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::LEFT_BIAS:  return "FIELD_MERGE_LEFT_BIAS";
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::RIGHT_BIAS: return "FIELD_MERGE_RIGHT_BIAS";
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return "FIELD_MERGE_UNKNOWN";
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string FieldLevelMergeResolver::mergeFields(
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, std::string> merged;
  Confidence: band=very_high; score=0.9
- Line 331: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (strategy_ == MergeStrategy::INTERSECT) {
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::LEFT_BIAS:
  Confidence: band=very_high; score=0.9
- Line 340: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[key] = field_maps[present_indices.front()][key];
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::RIGHT_BIAS:
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[key] = field_maps[present_indices.back()][key];
  Confidence: band=very_high; score=0.9
- Line 345: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::UNION:
  Confidence: band=very_high; score=0.9
- Line 346: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::INTERSECT:
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[key] = field_maps[best][key];
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return buildJson(merged);
  Confidence: band=very_high; score=0.9
- Line 362: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry FieldLevelMergeResolver::resolve(
  Confidence: band=very_high; score=0.9
- Line 370: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: winner.data = mergeFields(conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 90: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (*p == '\\' && (p + 1) < end) { ++p; } // skip escape
- Line 129: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: value += '"';
- Line 204: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> merged;
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: key
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: const auto base_it  = base_f.find(key);
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_maps.push_back(parseTopLevelFields(w.data));
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: field_maps.push_back(parseTopLevelFields(w.data));
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);

### src/replication/logical_replication.cpp
Total findings: 62

- Line 121: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: runtime->snapshot_keys.insert(key);
  Confidence: band=very_high; score=0.99
- Line 186: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = slots_.find(slot_name);
- Line 271: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return {0, 0};  // conflict-free initial sync: skip duplicates from snapshot
  Confidence: band=very_high; score=0.99
- Line 633: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(tmp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
- Line 642: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: ::_write(fd, payload.data(), static_cast<unsigned int>(payload.size()));
  Confidence: band=very_high; score=0.99
- Line 644: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ::write(fd, payload.data(), payload.size());
- Line 688: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int dir_fd = ::open(base.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
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
- Line 96: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Unsupported row filter expression: " + expr);
- Line 109: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& snap : initial_snapshot) {
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: runtime->buffer.push_back(std::move(snap));
- Line 128: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("logical replication slot already exists: " + slot_name);
- Line 168: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : slots_) {
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> g(kv.second->mutex);
- Line 185: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = slots_.find(slot_name);
- Line 215: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& kv : slots_) {
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& slot : slots_copy) {
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(slot->mutex);
- Line 226: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 251: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& kv : slots_) {
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto process_slot = [this, entry, change](const std::shared_ptr<SlotRuntime>& slot) -> std::pair<uin
- Line 256: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto process_slot = [this, entry, change](const std::shared_ptr<SlotRuntime>& slot) -> std::pair<uin
- Line 279: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: slot->buffer.push_back(slot_change);
- Line 286: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& slot : slots_copy) {
  Confidence: band=very_high; score=0.9
- Line 315: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t w = 0; w < worker_count; ++w) {
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 328: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
- Line 334: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> elock(worker_err_mutex);
- Line 338: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> elock(worker_err_mutex);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime->buffer.push_back(std::move(snap));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: runtime->buffer.push_back(std::move(snap));
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second->meta);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kv.second->meta);
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(runtime->buffer.front()));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(runtime->buffer.front()));
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slots_copy.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slots_copy.push_back(kv.second);
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slot->buffer.push_back(ddl);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slot->buffer.push_back(ddl);
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slots_copy.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slots_copy.push_back(kv.second);
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slot->buffer.push_back(slot_change);
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers.emplace_back([&next_index, &slots_copy, &process_slot, &worker_error, &worker_err_mutex, this] {
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 557: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 570: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 648: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::_close(fd);
- Line 650: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 667: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::_close(fd);
- Line 669: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 676: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::_close(fd);
- Line 678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 693: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(dir_fd);
- Line 226: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6
- Line 292: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6
- Line 328: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6
- Line 582: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6

### src/replication/raft_v2.cpp
Total findings: 33

- Line 156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto old_members = config_->getNewMembers();
- Line 158: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto new_members = config_->getNewMembers();
- Line 174: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto old_members = config_->getNewMembers();
- Line 176: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto new_members = config_->getNewMembers();
- Line 189: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (pending_->log_index != log_index) {
- Line 303: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: entry.log_index = wal_->getCurrentSequence();
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 45: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 66: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 99: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 113: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& v : votes) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& v : votes) {
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& v : votes) {
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& m : entry.old_members) {
  Confidence: band=very_high; score=0.9
- Line 187: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return;  // Stale callback – ignore
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return;  // Stale callback – ignore
  Confidence: band=high; score=0.74

### src/replication/event_stream.cpp
Total findings: 28

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 84: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ev : buffer_) {
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["from"] = roleToString(from);
- Line 163: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["to"]   = roleToString(to);
- Line 173: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["leader_id"] = leader_id;
- Line 183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["node_id"]  = replica.node_id;
- Line 184: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["endpoint"] = replica.endpoint;
- Line 185: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["role"]     = roleToString(replica.role);
- Line 195: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["node_id"] = node_id;
- Line 204: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["document_id"] = document_id;
- Line 213: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["lag_ms"] = std::to_string(lag_ms);
- Line 226: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["node_id"]    = node_id;
- Line 227: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["old_status"] = healthToString(old_status);
- Line 228: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["new_status"] = healthToString(new_status);
- Line 239: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["failed_node"] = failed_node;
- Line 240: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["new_leader"]  = new_leader;
- Line 252: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["new_leader"] = new_leader;
- Line 253: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["success"]    = success ? "true" : "false";
- Line 277: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["sequence"]   = std::to_string(entry.sequence_number);
- Line 278: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["collection"] = entry.collection;
- Line 279: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ev.data["operation"]  = entry.operation;
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(ev);
- Line 264: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!nodes.empty()) nodes += ',';
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!nodes.empty()) nodes += ',';

### src/replication/observability.cpp
Total findings: 15

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(snap);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(snap);
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(node);
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.downstream_replicas.push_back(r.node_id);
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.downstream_replicas.push_back(r.node_id);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.downstream_replicas.push_back(r.node_id);
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bottlenecks.push_back(std::move(b));
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bottlenecks.push_back(std::move(b));
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score.issues.push_back(std::to_string(failed) + " replica(s) FAILED");
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score.issues.push_back(std::to_string(failed) + " replica(s) FAILED");
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score.issues.push_back(std::to_string(degraded) + " replica(s) DEGRADED");
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score.issues.push_back("Max replication lag " + std::to_string(max_lag_ms) +
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score.issues.push_back("Max replication lag " + std::to_string(max_lag_ms) +
- Line 232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score.issues.push_back("Max replication lag " + std::to_string(max_lag_ms) + "ms is elevated");

### src/replication/replication_slot.cpp
Total findings: 12

- Line 188: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = line.find(search);
- Line 192: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end = line.find(',', pos);
- Line 254: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = slots_.find(name);
- Line 261: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = slots_.find(name);
- Line 255: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != slots_.end()) ? it->second : nullptr;
- Line 255: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != slots_.end()) ? it->second : nullptr;
- Line 274: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : slots_) {
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : slots_) {
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : std::filesystem::directory_iterator(slots_dir)) {
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(slots_dir)) {
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: states.push_back(kv.second->state());
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: states.push_back(kv.second->state());

### src/replication/multi_tier_replication.cpp
Total findings: 10

- Line 131: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.current_tier = config_.default_tier;
- Line 176: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // to avoid data races with concurrent readers.
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 163: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (override_ptr && override_ptr->has_value()) {
- Line 164: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return override_ptr->value();
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.access_timestamps.push_back(now);
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(stats);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(stats);
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(col);
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(col);

### src/replication/policy.cpp
Total findings: 8

- Line 50: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (!r.datacenter.empty()) dcs.insert(r.datacenter);
  Confidence: band=very_high; score=0.99
- Line 111: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pol_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto pol_it = policies_.find(asn_it->second);
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& kv : policies_) names.push_back(kv.first);
- Line 147: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: " are healthy.");
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back(
- Line 178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back(
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back(

### src/replication/schema_cdc.cpp
Total findings: 5

- Line 64: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int32_t schema_id = registry_->ensureSchema(subject, def, reg_cfg_.default_format);
- Line 64: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int32_t schema_id = registry_->ensureSchema(subject, def, reg_cfg_.default_format);
- Line 153: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: themis::cdc::CdcSchemaEncoder encoder(registry_.get());
  Confidence: band=very_high; score=0.9
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subscriptions_.push_back(Subscription{id, collection, std::move(callback)});
- Line 156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
