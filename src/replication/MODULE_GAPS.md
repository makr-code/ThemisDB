# replication Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: replication
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 525
- Actionable Findings (Critical + High): 327
- Affected Files: 10

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 164 |
| High | 163 |
| Medium | 194 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| distributed_consistency | 441 |
| performance_patterns | 91 |
| container | 60 |
| exception_safety | 42 |
| reliability | 39 |
| performance | 12 |
| legacy_duplication | 9 |
| memory | 9 |
| raii | 9 |
| observability | 5 |
| platform | 2 |
| audit_logging | 1 |
| concurrency | 1 |
| input_validation | 1 |
| security | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/replication/replication_manager.cpp | 414 | 134 | 114 | 166 | 0 |
| src/replication/conflict_resolution.cpp | 76 | 25 | 45 | 6 | 0 |
| src/replication/logical_replication.cpp | 16 | 3 | 2 | 7 | 4 |
| src/replication/observability.cpp | 7 | 0 | 0 | 7 | 0 |
| src/replication/multi_tier_replication.cpp | 3 | 1 | 0 | 2 | 0 |
| src/replication/event_stream.cpp | 2 | 0 | 0 | 2 | 0 |
| src/replication/policy.cpp | 2 | 1 | 0 | 1 | 0 |
| src/replication/raft_v2.cpp | 2 | 0 | 0 | 2 | 0 |
| src/replication/replication_slot.cpp | 2 | 0 | 1 | 1 | 0 |
| src/replication/schema_cdc.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/replication/replication_manager.cpp
Total findings: 414

- Line 64: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), s.begin(), s.end());
  Confidence: band=very_high; score=0.99
- Line 188: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
  Confidence: band=very_high; score=0.99
- Line 189: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# TYPE themisdb_conflicts_resolved_total counter\n"
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 1691: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: int64_t LWWConflictResolver::extractTimestamp(const std::string& json_doc) {
  Confidence: band=very_high; score=0.99
- Line 1736: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // CRDTConflictResolver Implementation
  Confidence: band=very_high; score=0.99
- Line 1739: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTConflictResolver::resolve(
  Confidence: band=very_high; score=0.99
- Line 1747: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // For all other content we delegate to LWWConflictResolver.
  Confidence: band=very_high; score=0.99
- Line 1753: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Build merged document: walk the remote document and for numeric fields
  Confidence: band=very_high; score=0.99
- Line 1755: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: LWWConflictResolver lwr;
  Confidence: band=very_high; score=0.99
- Line 1758: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Scan both documents for numeric fields and merge with max semantics.
  Confidence: band=very_high; score=0.99
- Line 1760: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string merged = base;
  Confidence: band=very_high; score=0.99
- Line 1803: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // For each field present in both documents, patch the merged document with max value
  Confidence: band=very_high; score=0.99
- Line 1812: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Replace "key": cur_val with "key": max_val in merged
  Confidence: band=very_high; score=0.99
- Line 1814: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: auto pos = merged.find(search);
  Confidence: band=very_high; score=0.99
- Line 1818: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
  Confidence: band=very_high; score=0.99
- Line 2079: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
  Confidence: band=very_high; score=0.99
- Line 2081: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: merged_dependencies.insert(write.write_id);
  Confidence: band=very_high; score=0.99
- Line 2111: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
  Confidence: band=very_high; score=0.99
- Line 2113: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes.empty()) return MMWriteEntry{};
  Confidence: band=very_high; score=0.99
- Line 2115: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string merged_data;
  Confidence: band=very_high; score=0.99
- Line 2117: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
  Confidence: band=very_high; score=0.99
- Line 2118: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
  Confidence: band=very_high; score=0.99
- Line 2119: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
  Confidence: band=very_high; score=0.99
- Line 2120: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
  Confidence: band=very_high; score=0.99
- Line 2121: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::G_SET:        merged_data = mergeGSet(conflicting_writes);        break;
  Confidence: band=very_high; score=0.99
- Line 2122: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::OR_SET:       merged_data = mergeORSet(conflicting_writes);       break;
  Confidence: band=very_high; score=0.99
- Line 2123: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::LWW_MAP:      merged_data = mergeLWWMap(conflicting_writes);      break;
  Confidence: band=very_high; score=0.99
- Line 2124: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::TWO_P_SET:    merged_data = mergeTwoPSet(conflicting_writes);     break;
  Confidence: band=very_high; score=0.99
- Line 2125: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::RGA:          merged_data = mergeRGA(conflicting_writes);         break;
  Confidence: band=very_high; score=0.99
- Line 2126: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::FLAG_EW:      merged_data = mergeFlagEW(conflicting_writes);      break;
  Confidence: band=very_high; score=0.99
- Line 2127: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case CRDTType::FLAG_DW:      merged_data = mergeFlagDW(conflicting_writes);      break;
  Confidence: band=very_high; score=0.99
- Line 2130: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Base entry is the LWW winner; replace its data with the merged payload
  Confidence: band=very_high; score=0.99
- Line 2132: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 2133: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.data = merged_data;
  Confidence: band=very_high; score=0.99
- Line 2135: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Keep checksum aligned with merged payload and metadata-carrying fields.
  Confidence: band=very_high; score=0.99
- Line 2147: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::strategyName() const {
  Confidence: band=very_high; score=0.99
- Line 2173: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeMVRegister(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2174: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Multi-value register: return all concurrent values as a JSON array
  Confidence: band=very_high; score=0.99
- Line 2241: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(arr.substr(qs + 1, qe - qs - 1));
  Confidence: band=very_high; score=0.99
- Line 2264: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2266: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::map<std::string, int64_t> merged;
  Confidence: band=very_high; score=0.99
- Line 2285: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2288: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge: take max per key for both P and N sub-counters.
  Confidence: band=very_high; score=0.99
- Line 2289: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::map<std::string, int64_t> mergedP, mergedN;
  Confidence: band=very_high; score=0.99
- Line 2294: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: mergedP[k] = std::max(mergedP[k], v);
  Confidence: band=very_high; score=0.99
- Line 2299: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: mergedN[k] = std::max(mergedN[k], v);
  Confidence: band=very_high; score=0.99
- Line 2320: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2330: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(w.data.substr(qs + 1, qe - qs - 1));
  Confidence: band=very_high; score=0.99
- Line 2395: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
  Confidence: band=very_high; score=0.99
- Line 2434: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeTwoPSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2439: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge: union(add) across writes, union(remove) across writes.
  Confidence: band=very_high; score=0.99
- Line 2440: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Result: elements that appear in the merged add-set but NOT in the merged remove-set.
  Confidence: band=very_high; score=0.99
- Line 2538: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge: deletion is irrevocable
  Confidence: band=very_high; score=0.99
- Line 2544: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: // Keep the value from the first observed insert (already stored)
  Confidence: band=very_high; score=0.99
- Line 2565: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeFlagEW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2566: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Enable-Wins Flag: concurrent enable + disable → enabled.
  Confidence: band=very_high; score=0.99
- Line 2572: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.99
- Line 2579: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: enableTags.insert(t);
  Confidence: band=very_high; score=0.99
- Line 2582: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: disableTags.insert(t);
  Confidence: band=very_high; score=0.99
- Line 2593: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string CRDTMergeResolver::mergeFlagDW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.99
- Line 2594: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Disable-Wins Flag: concurrent enable + disable → disabled.
  Confidence: band=very_high; score=0.99
- Line 2599: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.99
- Line 2636: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: result.insert(result.end(), s.begin(), s.end());
  Confidence: band=very_high; score=0.99
- Line 2705: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
  Confidence: band=very_high; score=0.99
- Line 2708: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return resolver_(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 2976: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Conflict Management
  Confidence: band=very_high; score=0.99
- Line 2994: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<ConflictRecord> result;
  Confidence: band=very_high; score=0.99
- Line 2995: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 3003: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool MultiMasterReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.99
- Line 3004: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::string& conflict_id,
  Confidence: band=very_high; score=0.99
- Line 3007: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3008: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 3009: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (rec.conflict_id == conflict_id && !rec.resolved) {
  Confidence: band=very_high; score=0.99
- Line 3012: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: stats_conflicts_resolved_.fetch_add(1);
  Confidence: band=very_high; score=0.99
- Line 3045: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: s.conflicts_detected    = stats_conflicts_detected_.load();
  Confidence: band=very_high; score=0.99
- Line 3046: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: s.conflicts_resolved    = stats_conflicts_resolved_.load();
  Confidence: band=very_high; score=0.99
- Line 3134: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# HELP themisdb_mm_conflicts_detected Conflicts detected\n"
  Confidence: band=very_high; score=0.99
- Line 3135: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# TYPE themisdb_mm_conflicts_detected counter\n"
  Confidence: band=very_high; score=0.99
- Line 3136: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "themisdb_mm_conflicts_detected{node=\"" << config_.node_id << "\"} " << s.conflicts_detected << "\n"
  Confidence: band=very_high; score=0.99
- Line 3137: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
  Confidence: band=very_high; score=0.99
- Line 3138: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "# TYPE themisdb_mm_conflicts_resolved counter\n"
  Confidence: band=very_high; score=0.99
- Line 3139: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
  Confidence: band=very_high; score=0.99
- Line 3318: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: vector_clock_->merge(incoming.vector_clock);
  Confidence: band=very_high; score=0.99
- Line 3325: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Check for conflict with any recently-seen write for the same document
  Confidence: band=very_high; score=0.99
- Line 3327: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool has_conflict = false;
  Confidence: band=very_high; score=0.99
- Line 3329: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3330: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 3334: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: has_conflict = true;
  Confidence: band=very_high; score=0.99
- Line 3340: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (has_conflict) {
  Confidence: band=very_high; score=0.99
- Line 3341: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: THEMIS_WARN("MM conflict detected for doc={}/{} from peer={}",
  Confidence: band=very_high; score=0.99
- Line 3343: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // For each unresolved conflict on this document, add the incoming entry
  Confidence: band=very_high; score=0.99
- Line 3344: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3345: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto& rec : conflicts_) {
  Confidence: band=very_high; score=0.99
- Line 3349: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: rec.conflicting_writes.push_back(incoming);
  Confidence: band=very_high; score=0.99
- Line 3372: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>& conflicting_writes)
  Confidence: band=very_high; score=0.99
- Line 3374: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (conflicting_writes.empty()) return;
  Confidence: band=very_high; score=0.99
- Line 3376: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::string& collection = conflicting_writes[0].collection;
  Confidence: band=very_high; score=0.99
- Line 3379: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::shared_ptr<ConflictResolver> resolver;
  Confidence: band=very_high; score=0.99
- Line 3401: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lock(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 3402: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_.push_back(record);
  Confidence: band=very_high; score=0.99
- Line 3598: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // has been fully applied, even across concurrent workers.
  Confidence: band=very_high; score=0.99
- Line 4075: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::string input(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.99
- Line 4140: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: std::string input(reinterpret_cast<const char*>(compressed.data()),
  Confidence: band=very_high; score=0.99
- Line 4977: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
  Confidence: band=very_high; score=0.99
- Line 4988: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.insert(out.end(), iv.begin(), iv.end());
  Confidence: band=very_high; score=0.99
- Line 4989: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.insert(out.end(), tag.begin(), tag.end());
  Confidence: band=very_high; score=0.99
- Line 4990: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: out.insert(out.end(), ciphertext.begin(), ciphertext.begin() + ct_len);
  Confidence: band=very_high; score=0.99
- Line 5014: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: EVP_DecryptUpdate(ctx, plain.data(), &len, ct, ct_len);
  Confidence: band=very_high; score=0.99
- Line 5463: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: MultiRegionActiveActiveManager::write(
  Confidence: band=very_high; score=0.99
- Line 5757: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: uint64_t BidirectionalReplicationManager::submitWrite(
  Confidence: band=very_high; score=0.99
- Line 5843: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (detectConflict(entry, it->second)) {
  Confidence: band=very_high; score=0.99
- Line 5844: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: handleConflict(it->second, entry, entry.is_ddl);
  Confidence: band=very_high; score=0.99
- Line 5846: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // the conflict history.
  Confidence: band=very_high; score=0.99
- Line 5897: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::count_if(conflict_timestamps_.begin(), conflict_timestamps_.end(),
  Confidence: band=very_high; score=0.99
- Line 5922: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<BidiConflictRecord> result;
  Confidence: band=very_high; score=0.99
- Line 5923: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& rec : conflict_history_) {
  Confidence: band=very_high; score=0.99
- Line 5924: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (rec.strategy_used == ConflictResolution::CUSTOM
  Confidence: band=very_high; score=0.99
- Line 5932: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // ── Conflict resolution ───────────────────────────────────────────────────────
  Confidence: band=very_high; score=0.99
- Line 5934: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool BidirectionalReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.99
- Line 5943: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::lock_guard<std::mutex> lk(conflicts_mutex_);
  Confidence: band=very_high; score=0.99
- Line 5944: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Walk in reverse to find the most-recent conflict for this document.
  Confidence: band=very_high; score=0.99
- Line 5945: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto it = conflict_history_.rbegin(); it != conflict_history_.rend(); ++it) {
  Confidence: band=very_high; score=0.99
- Line 5950: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_.fetch_add(1);
  Confidence: band=very_high; score=0.99
- Line 5961: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: ConflictResolution strategy)
  Confidence: band=very_high; score=0.99
- Line 5966: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: ConflictResolution BidirectionalReplicationManager::getEffectiveStrategy(
  Confidence: band=very_high; score=0.99
- Line 6010: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: return applyRemoteWrite(entry);
  Confidence: band=very_high; score=0.99
- Line 6072: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: bool BidirectionalReplicationManager::detectConflict(
  Confidence: band=very_high; score=0.99
- Line 6077: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // are always considered concurrent — a true conflict.
  Confidence: band=very_high; score=0.99
- Line 6085: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void BidirectionalReplicationManager::handleConflict(
  Confidence: band=very_high; score=0.99
- Line 6117: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflict_timestamps_.pop_front();
  Confidence: band=very_high; score=0.99
- Line 6122: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (strategy != ConflictResolution::CUSTOM) {
  Confidence: band=very_high; score=0.99
- Line 6123: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicts_resolved_.fetch_add(1);
  Confidence: band=very_high; score=0.99
- Line 6287: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool GeoReplicationManager::write(
  Confidence: band=very_high; score=0.99
- Line 188: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: oss << "# HELP themisdb_conflicts_resolved_total Total conflicts resolved\n"
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: << "themisdb_conflicts_resolved_total " << conflicts_resolved.load() << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(data.data()), len);
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: THEMIS_ERROR("WAL segment {}: incomplete read (expected {} bytes, got {})",
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ifs.read(reinterpret_cast<char*>(data.data()), len);
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: election_thread_ = std::thread(&LeaderElection::electionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 703: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: stream_thread_ = std::thread(&ReplicationStream::streamLoop, this);
  Confidence: band=very_high; score=0.9
- Line 824: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_ = std::thread(&ReplicationManager::heartbeatLoop, this);
  Confidence: band=very_high; score=0.9
- Line 825: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: compaction_thread_ = std::thread(&ReplicationManager::compactionLoop, this);
  Confidence: band=very_high; score=0.9
- Line 826: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_monitor_thread_ = std::thread(&ReplicationManager::healthMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 856: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function replicate without trace point
  Context: bool ReplicationManager::replicate(const WALEntry& entry) {
  Confidence: band=very_high; score=0.9
- Line 1025: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void ReplicationManager::setConflictResolver(std::shared_ptr<IConflictResolver> resolver) {
  Confidence: band=very_high; score=0.9
- Line 1444: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ReplicationManager::LeaseReadResult ReplicationManager::leaseRead(
  Confidence: band=very_high; score=0.9
- Line 1753: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Build merged document: walk the remote document and for numeric fields
  Confidence: band=very_high; score=0.9
- Line 1758: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Scan both documents for numeric fields and merge with max semantics.
  Confidence: band=very_high; score=0.9
- Line 1760: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string merged = base;
  Confidence: band=very_high; score=0.9
- Line 1803: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // For each field present in both documents, patch the merged document with max value
  Confidence: band=very_high; score=0.9
- Line 1812: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Replace "key": cur_val with "key": max_val in merged
  Confidence: band=very_high; score=0.9
- Line 1814: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: auto pos = merged.find(search);
  Confidence: band=very_high; score=0.9
- Line 1818: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: while (vp < merged.size() && (merged[vp] == ' ' || merged[vp] == ':')) ++vp;
  Confidence: band=very_high; score=0.9
- Line 1821: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (vend < merged.size() && merged[vend] == '-') ++vend;
  Confidence: band=very_high; score=0.9
- Line 1822: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: while (vend < merged.size() &&
  Confidence: band=very_high; score=0.9
- Line 1823: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::isdigit(static_cast<unsigned char>(merged[vend]))) {
  Confidence: band=very_high; score=0.9
- Line 1826: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = merged.substr(0, vp) + std::to_string(max_val) + merged.substr(vend);
  Confidence: band=very_high; score=0.9
- Line 1831: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 1941: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=very_high; score=0.9
- Line 1945: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // two threads concurrently call A.merge(B) and B.merge(A).
  Confidence: band=very_high; score=0.9
- Line 1957: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=very_high; score=0.9
- Line 2072: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: VectorClock merged_clock = winner.vector_clock;
  Confidence: band=very_high; score=0.9
- Line 2073: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::set<std::string> merged_dependencies(
  Confidence: band=very_high; score=0.9
- Line 2078: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged_clock.merge(write.vector_clock);
  Confidence: band=very_high; score=0.9
- Line 2079: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
  Confidence: band=very_high; score=0.9
- Line 2081: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged_dependencies.insert(write.write_id);
  Confidence: band=very_high; score=0.9
- Line 2088: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: enriched.vector_clock = std::move(merged_clock);
  Confidence: band=very_high; score=0.9
- Line 2089: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: enriched.dependencies.assign(merged_dependencies.begin(), merged_dependencies.end());
  Confidence: band=very_high; score=0.9
- Line 2105: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: CRDTMergeResolver::CRDTMergeResolver(CRDTType type)
  Confidence: band=very_high; score=0.9
- Line 2109: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry CRDTMergeResolver::resolve(
  Confidence: band=very_high; score=0.9
- Line 2115: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string merged_data;
  Confidence: band=very_high; score=0.9
- Line 2117: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::LWW_REGISTER: merged_data = mergeLWWRegister(conflicting_writes); break;
  Confidence: band=very_high; score=0.9
- Line 2118: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::MV_REGISTER:  merged_data = mergeMVRegister(conflicting_writes);  break;
  Confidence: band=very_high; score=0.9
- Line 2119: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::G_COUNTER:    merged_data = mergeGCounter(conflicting_writes);    break;
  Confidence: band=very_high; score=0.9
- Line 2120: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case CRDTType::PN_COUNTER:   merged_data = mergePNCounter(conflicting_writes);   break;
  Confidence: band=very_high; score=0.9
- Line 2132: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry result = lwr.resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 2133: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.data = merged_data;
  Confidence: band=very_high; score=0.9
- Line 2135: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Keep checksum aligned with merged payload and metadata-carrying fields.
  Confidence: band=very_high; score=0.9
- Line 2147: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::strategyName() const {
  Confidence: band=very_high; score=0.9
- Line 2164: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeLWWRegister(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2173: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeMVRegister(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2264: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeGCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2266: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, int64_t> merged;
  Confidence: band=very_high; score=0.9
- Line 2270: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[k] = std::max(merged[k], v);
  Confidence: band=very_high; score=0.9
- Line 2276: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& [k, v] : merged) {
  Confidence: band=very_high; score=0.9
- Line 2285: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergePNCounter(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2288: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: take max per key for both P and N sub-counters.
  Confidence: band=very_high; score=0.9
- Line 2289: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, int64_t> mergedP, mergedN;
  Confidence: band=very_high; score=0.9
- Line 2294: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergedP[k] = std::max(mergedP[k], v);
  Confidence: band=very_high; score=0.9
- Line 2299: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: mergedN[k] = std::max(mergedN[k], v);
  Confidence: band=very_high; score=0.9
- Line 2316: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: oss << "{\"P\":" << serializeMap(mergedP) << ",\"N\":" << serializeMap(mergedN) << "}";
  Confidence: band=very_high; score=0.9
- Line 2320: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeGSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2346: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeORSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2354: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: union of all add pairs, union of all tombstones.
  Confidence: band=very_high; score=0.9
- Line 2371: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto lb = addArr.find('[', p);
  Confidence: band=very_high; score=0.9
- Line 2373: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto rb = addArr.find(']', lb + 1);
  Confidence: band=very_high; score=0.9
- Line 2395: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (tombstones.find(t) == tombstones.end()) { liveElements.insert(elem); break; }
  Confidence: band=very_high; score=0.9
- Line 2416: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = best.find(k);
  Confidence: band=very_high; score=0.9
- Line 2434: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeTwoPSet(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2439: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: union(add) across writes, union(remove) across writes.
  Confidence: band=very_high; score=0.9
- Line 2440: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Result: elements that appear in the merged add-set but NOT in the merged remove-set.
  Confidence: band=very_high; score=0.9
- Line 2462: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeRGA(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2470: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: union of all elements by unique id; if an id appears in multiple writes, prefer
  Confidence: band=very_high; score=0.9
- Line 2489: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto ob = src.find('{', p);
  Confidence: band=very_high; score=0.9
- Line 2538: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: deletion is irrevocable
  Confidence: band=very_high; score=0.9
- Line 2565: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeFlagEW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2572: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.9
- Line 2586: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (disableTags.find(t) == disableTags.end()) { enabled = true; break; }
  Confidence: band=very_high; score=0.9
- Line 2593: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string CRDTMergeResolver::mergeFlagDW(const std::vector<MMWriteEntry>& writes) {
  Confidence: band=very_high; score=0.9
- Line 2599: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge:  union of all "e" arrays; union of all "d" arrays.
  Confidence: band=very_high; score=0.9
- Line 2712: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return lwr.resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 2757: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: replication_thread_ = std::thread(&MultiMasterReplicationManager::replicationLoop, this);
  Confidence: band=very_high; score=0.9
- Line 2758: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: heartbeat_thread_   = std::thread(&MultiMasterReplicationManager::heartbeatLoop,   this);
  Confidence: band=very_high; score=0.9
- Line 2759: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sync_thread_        = std::thread(&MultiMasterReplicationManager::syncLoop,        this);
  Confidence: band=very_high; score=0.9
- Line 2857: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return future.wait_for(timeout) == std::future_status::ready && future.get();
  Confidence: band=very_high; score=0.9
- Line 2864: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: MultiMasterReplicationManager::ReadResult MultiMasterReplicationManager::read(
  Confidence: band=very_high; score=0.9
- Line 2992: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<ConflictRecord> MultiMasterReplicationManager::getUnresolvedConflicts() const {
  Confidence: band=very_high; score=0.9
- Line 3003: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: bool MultiMasterReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.9
- Line 3046: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: s.conflicts_resolved    = stats_conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 3137: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: << "# HELP themisdb_mm_conflicts_resolved Conflicts resolved\n"
  Confidence: band=very_high; score=0.9
- Line 3139: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: << "themisdb_mm_conflicts_resolved{node=\"" << config_.node_id << "\"} " << s.conflicts_resolved << "\n"
  Confidence: band=very_high; score=0.9
- Line 3179: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [entry, cb] : batch) {
  Confidence: band=very_high; score=0.9
- Line 3318: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: vector_clock_->merge(incoming.vector_clock);
  Confidence: band=very_high; score=0.9
- Line 3343: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // For each unresolved conflict on this document, add the incoming entry
  Confidence: band=very_high; score=0.9
- Line 3386: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry winner = resolver->resolve(document_id, conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 3622: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: QuorumReadManager::QuorumReadResult QuorumReadManager::read(
  Confidence: band=very_high; score=0.9
- Line 3712: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto resp = fut.get();
  Confidence: band=very_high; score=0.9
- Line 3747: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<ReplicaResponse> reconcile_set;
  Confidence: band=very_high; score=0.9
- Line 3749: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: reconcile_set = responses;
  Confidence: band=very_high; score=0.9
- Line 3751: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: reconcile_set.reserve(qualifying.size());
  Confidence: band=very_high; score=0.9
- Line 3752: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto* p : qualifying) reconcile_set.push_back(*p);
  Confidence: band=very_high; score=0.9
- Line 3755: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Reconcile: pick the response with the highest version
  Confidence: band=very_high; score=0.9
- Line 3756: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const ReplicaResponse* best = &reconcile_set[0];
  Confidence: band=very_high; score=0.9
- Line 3758: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& r : reconcile_set) {
  Confidence: band=very_high; score=0.9
- Line 3766: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& r : reconcile_set) {
  Confidence: band=very_high; score=0.9
- Line 3781: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& r : reconcile_set) {
  Confidence: band=very_high; score=0.9
- Line 4101: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // compressed inputs whose expansion is dominated by header overhead.
  Confidence: band=very_high; score=0.9
- Line 4205: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: flush_thread_ = std::thread(&BatchedAckTracker::flushLoop, this);
  Confidence: band=very_high; score=0.9
- Line 4411: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // A high value indicates large relative spread (typical of network jitter).
  Confidence: band=very_high; score=0.9
- Line 4426: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: b.details         = "High lag spread (normalized_range=" +
  Confidence: band=very_high; score=0.9
- Line 5498: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: MultiRegionActiveActiveManager::read(
  Confidence: band=very_high; score=0.9
- Line 5888: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: s.conflicts_resolved = conflicts_resolved_.load();
  Confidence: band=very_high; score=0.9
- Line 5934: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: bool BidirectionalReplicationManager::resolveConflict(
  Confidence: band=very_high; score=0.9
- Line 6064: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Return an empty placeholder; the application must call resolveConflict().
  Confidence: band=very_high; score=0.9
- Line 6325: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<std::string> GeoReplicationManager::read(
  Confidence: band=very_high; score=0.9
- Line 48: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALEntry::serialize()
  Context: std::vector<uint8_t> WALEntry::serialize() const {
  Confidence: band=medium; score=0.56
- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: WALManager::append(const WALEntry& entry)
  Context: uint64_t WALManager::append(const WALEntry& entry) {
  Confidence: band=medium; score=0.56
- Line 348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(*entry);
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // If term is stale, reject
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: !commit_index_.compare_exchange_weak(expected, new_commit)) {}
  Confidence: band=high; score=0.74
- Line 810: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replicas_.push_back(replica);
  Confidence: band=high; score=0.74
- Line 1111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: replicas_.push_back(replica);
  Confidence: band=high; score=0.74
- Line 1343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(replica.node_id, replica.health_status);
  Confidence: band=high; score=0.74
- Line 1401: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes.push_back({replica.node_id, old_status, replica.health_status});
  Confidence: band=high; score=0.74
- Line 1517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unreachable_nodes.push_back(replica.node_id);
  Confidence: band=high; score=0.74
- Line 1765: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: -> std::map<std::string, int64_t>
  Confidence: band=high; score=0.74
- Line 1767: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int64_t> fields;
  Confidence: band=high; score=0.74
- Line 1844: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Context: HybridLogicalClock::Timestamp HybridLogicalClock::now() {
  Confidence: band=medium; score=0.56
- Line 1936: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::increment(const std::string& node_id)
  Context: void VectorClock::increment(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 1941: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::merge(const VectorClock& other)
  Context: void VectorClock::merge(const VectorClock& other) {
  Confidence: band=medium; score=0.56
- Line 1957: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::get(const std::string& node_id)
  Context: uint64_t VectorClock::get(const std::string& node_id) const {
  Confidence: band=medium; score=0.56
- Line 1963: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::compare(const VectorClock& other)
  Context: int VectorClock::compare(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 1996: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::happensBefore(const VectorClock& other)
  Context: bool VectorClock::happensBefore(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 2000: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: VectorClock::isConcurrent(const VectorClock& other)
  Context: bool VectorClock::isConcurrent(const VectorClock& other) const {
  Confidence: band=medium; score=0.56
- Line 2188: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static std::map<std::string, int64_t> extractJsonInts(const std::string& doc) {
  Confidence: band=high; score=0.74
- Line 2189: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int64_t> fields;
  Confidence: band=high; score=0.74
- Line 2289: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int64_t> mergedP, mergedN;
  Confidence: band=high; score=0.74
- Line 2303: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: auto serializeMap = [](const std::map<std::string, int64_t>& m) {
  Confidence: band=high; score=0.74
- Line 2380: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allAdds.emplace_back(elem, tag);
  Confidence: band=high; score=0.74
- Line 2388: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> elemTags;
  Confidence: band=high; score=0.74
- Line 2389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elemTags[elem].push_back(tag);
  Confidence: band=high; score=0.74
- Line 2412: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::pair<HybridLogicalClock::Timestamp, std::string>> best;
  Confidence: band=high; score=0.74
- Line 2480: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, RGAElem> byId;
  Confidence: band=high; score=0.74
- Line 2625: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2625: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2629: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 2888: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peers_snapshot.push_back(info);
  Confidence: band=high; score=0.74
- Line 2896: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // not yet received — that peer signals a potential stale read.
  Confidence: band=high; score=0.74
- Line 2898: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: bool stale_read_detected = false;
  Confidence: band=high; score=0.74
- Line 2918: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: stale_read_detected = true;
  Confidence: band=high; score=0.74
- Line 2928: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (stale_read_detected) {
  Confidence: band=high; score=0.74
- Line 2929: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: THEMIS_WARN("MultiMasterRead: potential stale read detected for node={} "
  Confidence: band=high; score=0.74
- Line 2957: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 2996: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 3100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 3173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.emplace_back(std::move(entry), std::move(cb));
  Confidence: band=high; score=0.74
- Line 3187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: committed_writes_log_.push_back(entry);
  Confidence: band=high; score=0.74
- Line 3227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 3227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 3227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 3348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec.conflicting_writes.push_back(incoming);
  Confidence: band=high; score=0.74
- Line 3348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec.conflicting_writes.push_back(incoming);
  Confidence: band=high; score=0.74
- Line 3478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&ParallelReplicationWorker::workerLoop, this);
  Confidence: band=high; score=0.74
- Line 3505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: item.deps.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 3566: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(work_queue_.front()));
  Confidence: band=high; score=0.74
- Line 3692: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 3702: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // total responses would discard fresh replicas that come after a stale one
  Confidence: band=high; score=0.74
- Line 3712: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (resp.ok) responses.push_back(std::move(resp));
  Confidence: band=high; score=0.74
- Line 3746: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // session token was supplied so that stale replicas are not considered).
  Confidence: band=high; score=0.74
- Line 3764: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // identify stale replicas and schedule repair.
  Confidence: band=high; score=0.74
- Line 3781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.sources.push_back(r.endpoint);
  Confidence: band=high; score=0.74
- Line 3781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.sources.push_back(r.endpoint);
  Confidence: band=high; score=0.74
- Line 4021: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += std::to_string(e.sequence_number) + "|"
  Confidence: band=high; score=0.74
- Line 4327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data_points.push_back(dp);
  Confidence: band=high; score=0.74
- Line 4382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: insights.push_back(std::move(ins));
  Confidence: band=high; score=0.74
- Line 4640: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies_us.push_back(
  Confidence: band=high; score=0.74
- Line 4824: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: m += "themisdb_cross_cluster_publication_published_total" + label + " " +
  Confidence: band=high; score=0.74
- Line 4860: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (last_applied_seq_.compare_exchange_weak(expected, e.sequence_number))
  Confidence: band=high; score=0.74
- Line 4948: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bytes.push_back(static_cast<uint8_t>(val));
  Confidence: band=high; score=0.74
- Line 5379: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(entry.path().filename().string());
  Confidence: band=high; score=0.74
- Line 5410: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Initialise staleness entries for peer regions (unknown at start)
  Confidence: band=high; score=0.74
- Line 5412: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: RegionStalenessInfo info;
  Confidence: band=high; score=0.74
- Line 5414: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms          = std::numeric_limits<int64_t>::max();
  Confidence: band=high; score=0.74
- Line 5418: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: region_staleness_[peer]    = info;
  Confidence: band=high; score=0.74
- Line 5477: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5510: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5512: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it != region_staleness_.end()) {
  Confidence: band=high; score=0.74
- Line 5530: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++staleness_rejections_;
  Confidence: band=high; score=0.74
- Line 5536: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::BOUNDED_STALENESS:
  Confidence: band=high; score=0.74
- Line 5537: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++bounded_staleness_reads_;
  Confidence: band=high; score=0.74
- Line 5539: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: THEMIS_WARN("MultiRegionActiveActive: BOUNDED_STALENESS read rejected – "
  Confidence: band=high; score=0.74
- Line 5542: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++staleness_rejections_;
  Confidence: band=high; score=0.74
- Line 5565: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Always succeeds regardless of staleness
  Confidence: band=high; score=0.74
- Line 5570: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: THEMIS_INFO("MultiRegionActiveActive: read served region={} staleness={}ms",
  Confidence: band=high; score=0.74
- Line 5605: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::chrono::milliseconds MultiRegionActiveActiveManager::getStaleness(
  Confidence: band=high; score=0.74
- Line 5608: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5609: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(region_id);
  Confidence: band=high; score=0.74
- Line 5610: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it == region_staleness_.end()) {
  Confidence: band=high; score=0.74
- Line 5613: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return std::chrono::milliseconds(it->second.staleness_ms);
  Confidence: band=high; score=0.74
- Line 5616: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: bool MultiRegionActiveActiveManager::isWithinStalenessBound(
  Confidence: band=high; score=0.74
- Line 5619: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return getStaleness(region_id) <=
  Confidence: band=high; score=0.74
- Line 5620: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::chrono::milliseconds(config_.max_staleness_ms);
  Confidence: band=high; score=0.74
- Line 5623: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::vector<RegionStalenessInfo>
  Confidence: band=high; score=0.74
- Line 5624: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: MultiRegionActiveActiveManager::getAllRegionStaleness() const
  Confidence: band=high; score=0.74
- Line 5626: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5627: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::vector<RegionStalenessInfo> result;
  Confidence: band=high; score=0.74
- Line 5628: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: result.reserve(region_staleness_.size());
  Confidence: band=high; score=0.74
- Line 5629: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& kv : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 5629: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 5635: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void MultiRegionActiveActiveManager::updateRegionStaleness(
  Confidence: band=high; score=0.74
- Line 5637: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t            staleness_ms,
  Confidence: band=high; score=0.74
- Line 5640: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5641: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto& info              = region_staleness_[region_id];
  Confidence: band=high; score=0.74
- Line 5643: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms       = staleness_ms;
  Confidence: band=high; score=0.74
- Line 5646: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.is_healthy         = (staleness_ms >= 0 &&
  Confidence: band=high; score=0.74
- Line 5647: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: staleness_ms <= static_cast<int64_t>(config_.max_staleness_ms) * 2);
  Confidence: band=high; score=0.74
- Line 5664: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_mraaa_staleness_rejections_total Reads rejected due to excessive staleness\n"
  Confidence: band=high; score=0.74
- Line 5665: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_mraaa_staleness_rejections_total counter\n"
  Confidence: band=high; score=0.74
- Line 5666: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "themisdb_mraaa_staleness_rejections_total{region=\"" << r << "\"} "
  Confidence: band=high; score=0.74
- Line 5667: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << staleness_rejections_.load() << "\n\n";
  Confidence: band=high; score=0.74
- Line 5674: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_mraaa_bounded_staleness_reads_total Reads served at BOUNDED_STALENESS\n"
  Confidence: band=high; score=0.74
- Line 5675: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_mraaa_bounded_staleness_reads_total counter\n"
  Confidence: band=high; score=0.74
- Line 5676: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "themisdb_mraaa_bounded_staleness_reads_total{region=\"" << r << "\"} "
  Confidence: band=high; score=0.74
- Line 5677: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << bounded_staleness_reads_.load() << "\n\n";
  Confidence: band=high; score=0.74
- Line 5689: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Per-region staleness gauges
  Confidence: band=high; score=0.74
- Line 5691: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 5692: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_mraaa_region_staleness_ms Current replication staleness per region\n"
  Confidence: band=high; score=0.74
- Line 5693: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_mraaa_region_staleness_ms gauge\n";
  Confidence: band=high; score=0.74
- Line 5694: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& kv : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 5695: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "themisdb_mraaa_region_staleness_ms{region=\"" << kv.first << "\"} "
  Confidence: band=high; score=0.74
- Line 5696: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << kv.second.staleness_ms << "\n";
  Confidence: band=high; score=0.74
- Line 5860: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (remote_sequence_.compare_exchange_weak(cur, entry.origin_seq)) {
  Confidence: band=high; score=0.74
- Line 5925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 5982: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (remote_sequence_.compare_exchange_weak(cur, remote_seq)) {
  Confidence: band=high; score=0.74
- Line 6146: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: RegionStalenessInfo info;
  Confidence: band=high; score=0.74
- Line 6148: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms           = std::numeric_limits<int64_t>::max();
  Confidence: band=high; score=0.74
- Line 6152: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: region_staleness_[r]       = info;
  Confidence: band=high; score=0.74
- Line 6197: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // ── Staleness management ──────────────────────────────────────────────────────
  Confidence: band=high; score=0.74
- Line 6199: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: void GeoReplicationManager::updateRegionStaleness(const std::string& region,
  Confidence: band=high; score=0.74
- Line 6200: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t            staleness_ms,
  Confidence: band=high; score=0.74
- Line 6203: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6204: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto& info                  = region_staleness_[region];
  Confidence: band=high; score=0.74
- Line 6206: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.staleness_ms            = staleness_ms;
  Confidence: band=high; score=0.74
- Line 6209: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: info.is_healthy              = (staleness_ms >= 0);
  Confidence: band=high; score=0.74
- Line 6212: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::chrono::milliseconds GeoReplicationManager::getStaleness(
  Confidence: band=high; score=0.74
- Line 6215: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6216: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: auto it = region_staleness_.find(region);
  Confidence: band=high; score=0.74
- Line 6217: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it == region_staleness_.end()) {
  Confidence: band=high; score=0.74
- Line 6220: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return std::chrono::milliseconds(it->second.staleness_ms);
  Confidence: band=high; score=0.74
- Line 6229: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6235: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it != region_staleness_.end() && it->second.staleness_ms == 0) {
  Confidence: band=high; score=0.74
- Line 6239: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& [rid, info] : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 6240: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (info.staleness_ms == 0 && info.is_healthy) return rid;
  Confidence: band=high; score=0.74
- Line 6253: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Pick the region with smallest staleness that is within bound.
  Confidence: band=high; score=0.74
- Line 6256: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& [rid, info] : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 6257: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (info.is_healthy && info.staleness_ms <= bound &&
  Confidence: band=high; score=0.74
- Line 6269: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // so it is safe to call while holding staleness_mutex_ as a shared lock.
  Confidence: band=high; score=0.74
- Line 6272: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: if (it != region_staleness_.end() &&
  Confidence: band=high; score=0.74
- Line 6279: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::EVENTUAL:
  Confidence: band=high; score=0.74
- Line 6309: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::unique_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6337: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::BOUNDED_STALENESS:
  Confidence: band=high; score=0.74
- Line 6338: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++bounded_staleness_reads_;
  Confidence: band=high; score=0.74
- Line 6343: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: case ConsistencyLevel::EVENTUAL:
  Confidence: band=high; score=0.74
- Line 6344: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ++eventual_reads_;
  Confidence: band=high; score=0.74
- Line 6391: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_geo_repl_bounded_staleness_reads_total BOUNDED_STALENESS reads\n"
  Confidence: band=high; score=0.74
- Line 6392: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_geo_repl_bounded_staleness_reads_total counter\n"
  Confidence: band=high; score=0.74
- Line 6393: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "themisdb_geo_repl_bounded_staleness_reads_total" << lbl << " "
  Confidence: band=high; score=0.74
- Line 6394: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << bounded_staleness_reads_.load() << "\n";
  Confidence: band=high; score=0.74
- Line 6406: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Per-region staleness gauge
  Confidence: band=high; score=0.74
- Line 6407: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "# HELP themisdb_geo_repl_region_staleness_ms Replication lag per region (ms)\n"
  Confidence: band=high; score=0.74
- Line 6408: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: << "# TYPE themisdb_geo_repl_region_staleness_ms gauge\n";
  Confidence: band=high; score=0.74
- Line 6410: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: std::shared_lock<std::shared_mutex> lock(staleness_mutex_);
  Confidence: band=high; score=0.74
- Line 6411: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: for (const auto& [rid, info] : region_staleness_) {
  Confidence: band=high; score=0.74
- Line 6412: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: int64_t lag = (info.staleness_ms == std::numeric_limits<int64_t>::max())
  Confidence: band=high; score=0.74
- Line 6413: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: ? -1 : info.staleness_ms;
  Confidence: band=high; score=0.74
- Line 6414: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: oss << "themisdb_geo_repl_region_staleness_ms{region=\"" << rid << "\"} "
  Confidence: band=high; score=0.74

### src/replication/conflict_resolution.cpp
Total findings: 76

- Line 2: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * ThemisDB | File: conflict_resolution.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 20:41:24
  Confidence: band=very_high; score=0.99
- Line 11: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * ThemisDB Advanced Conflict Resolution Implementation
  Confidence: band=very_high; score=0.99
- Line 17: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: #include "replication/conflict_resolution.h"
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
  Confidence: band=very_high; score=0.99
- Line 189: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: merged_dependencies.insert(write.write_id);
  Confidence: band=very_high; score=0.99
- Line 237: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string ThreeWayMergeResolver::mergeJson(
  Confidence: band=very_high; score=0.99
- Line 246: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::map<std::string, std::string> merged;
  Confidence: band=very_high; score=0.99
- Line 249: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& kv : base_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.99
- Line 250: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& kv : left_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.99
- Line 251: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (const auto& kv : right_f) merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.99
- Line 253: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: for (auto& kv : merged) {
  Confidence: band=very_high; score=0.99
- Line 282: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const std::vector<MMWriteEntry>&  conflicting_writes,
  Confidence: band=very_high; score=0.99
- Line 301: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: conflicting_writes[j].vector_clock))
  Confidence: band=very_high; score=0.99
- Line 325: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return enrichWinnerWithCausality(winner, conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 329: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // FieldLevelMergeResolver
  Confidence: band=very_high; score=0.99
- Line 332: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
  Confidence: band=very_high; score=0.99
- Line 376: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (strategy_ == MergeStrategy::INTERSECT) {
  Confidence: band=very_high; score=0.99
- Line 384: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::LEFT_BIAS:
  Confidence: band=very_high; score=0.99
- Line 385: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged[key] = field_maps[present_indices.front()][key];
  Confidence: band=very_high; score=0.99
- Line 387: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::RIGHT_BIAS:
  Confidence: band=very_high; score=0.99
- Line 388: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged[key] = field_maps[present_indices.back()][key];
  Confidence: band=very_high; score=0.99
- Line 390: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::UNION:
  Confidence: band=very_high; score=0.99
- Line 391: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::INTERSECT:
  Confidence: band=very_high; score=0.99
- Line 393: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Latest HLC wins for conflicting fields
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return enrichWinnerWithCausality(winner, conflicting_writes);
  Confidence: band=very_high; score=0.99
- Line 180: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: VectorClock merged_clock = winner.vector_clock;
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::set<std::string> merged_dependencies(
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged_clock.merge(write.vector_clock);
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged_dependencies.insert(write.write_id);
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: enriched.vector_clock = std::move(merged_clock);
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: enriched.dependencies.assign(merged_dependencies.begin(), merged_dependencies.end());
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // ThreeWayMergeResolver
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry ThreeWayMergeResolver::selectBase(
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string ThreeWayMergeResolver::mergeJson(
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, std::string> merged;
  Confidence: band=very_high; score=0.9
- Line 249: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& kv : base_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& kv : left_f)  merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (const auto& kv : right_f) merged[kv.first] = kv.second;
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: for (auto& kv : merged) {
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto base_it  = base_f.find(key);
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto left_it  = left_f.find(key);
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto right_it = right_f.find(key);
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // else: neither changed — keep base value (already in merged)
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return buildJson(merged);
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry ThreeWayMergeResolver::resolve(
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: throw std::invalid_argument("ThreeWayMergeResolver::resolve requires at least one conflicting write");
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: winner.data = mergeJson(base.data,
  Confidence: band=very_high; score=0.9
- Line 329: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // FieldLevelMergeResolver
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string FieldLevelMergeResolver::strategyName() const
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::UNION:      return "FIELD_MERGE_UNION";
  Confidence: band=very_high; score=0.9
- Line 340: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::INTERSECT:  return "FIELD_MERGE_INTERSECT";
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::LEFT_BIAS:  return "FIELD_MERGE_LEFT_BIAS";
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::RIGHT_BIAS: return "FIELD_MERGE_RIGHT_BIAS";
  Confidence: band=very_high; score=0.9
- Line 344: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return "FIELD_MERGE_UNKNOWN";
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string FieldLevelMergeResolver::mergeFields(
  Confidence: band=very_high; score=0.9
- Line 365: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::map<std::string, std::string> merged;
  Confidence: band=very_high; score=0.9
- Line 376: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (strategy_ == MergeStrategy::INTERSECT) {
  Confidence: band=very_high; score=0.9
- Line 384: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::LEFT_BIAS:
  Confidence: band=very_high; score=0.9
- Line 385: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[key] = field_maps[present_indices.front()][key];
  Confidence: band=very_high; score=0.9
- Line 387: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::RIGHT_BIAS:
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[key] = field_maps[present_indices.back()][key];
  Confidence: band=very_high; score=0.9
- Line 390: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::UNION:
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::INTERSECT:
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged[key] = field_maps[best][key];
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return buildJson(merged);
  Confidence: band=very_high; score=0.9
- Line 407: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MMWriteEntry FieldLevelMergeResolver::resolve(
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: throw std::invalid_argument("FieldLevelMergeResolver::resolve requires at least one conflicting write");
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: winner.data = mergeFields(conflicting_writes);
  Confidence: band=very_high; score=0.9
- Line 246: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> merged;
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_maps.push_back(parseTopLevelFields(w.data));
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (field_maps[i].count(key)) present_indices.push_back(i);
  Confidence: band=high; score=0.74

### src/replication/logical_replication.cpp
Total findings: 16

- Line 119: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: runtime->snapshot_keys.insert(key);
  Confidence: band=very_high; score=0.99
- Line 269: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return {0, 0};  // conflict-free initial sync: skip duplicates from snapshot
  Confidence: band=very_high; score=0.99
- Line 640: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: ::_write(fd, payload.data(), static_cast<unsigned int>(payload.size()));
  Confidence: band=very_high; score=0.99
- Line 166: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : slots_) {
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& slot : slots_copy) {
  Confidence: band=very_high; score=0.9
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime->buffer.push_back(std::move(snap));
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second->meta);
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(runtime->buffer.front()));
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slots_copy.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slot->buffer.push_back(ddl);
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slots_copy.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers.emplace_back([&next_index, &slots_copy, &process_slot, &worker_error, &worker_err_mutex, this] {
  Confidence: band=high; score=0.74
- Line 224: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6
- Line 290: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6
- Line 326: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6
- Line 580: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::lock_guard<std::mutex> slog(stats_mutex_);
  Confidence: band=medium; score=0.6

### src/replication/observability.cpp
Total findings: 7

- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(snap);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.downstream_replicas.push_back(r.node_id);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.downstream_replicas.push_back(r.node_id);
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bottlenecks.push_back(std::move(b));
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score.issues.push_back(std::to_string(failed) + " replica(s) FAILED");
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score.issues.push_back("Max replication lag " + std::to_string(max_lag_ms) +
  Confidence: band=high; score=0.74

### src/replication/multi_tier_replication.cpp
Total findings: 3

- Line 174: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // to avoid data races with concurrent readers.
  Confidence: band=very_high; score=0.99
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(stats);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(col);
  Confidence: band=high; score=0.74

### src/replication/event_stream.cpp
Total findings: 2

- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!nodes.empty()) nodes += ',';
  Confidence: band=high; score=0.74

### src/replication/policy.cpp
Total findings: 2

- Line 48: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (!r.datacenter.empty()) dcs.insert(r.datacenter);
  Confidence: band=very_high; score=0.99
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(
  Confidence: band=high; score=0.74

### src/replication/raft_v2.cpp
Total findings: 2

- Line 185: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return;  // Stale callback – ignore
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return;  // Stale callback – ignore
  Confidence: band=high; score=0.74

### src/replication/replication_slot.cpp
Total findings: 2

- Line 305: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : std::filesystem::directory_iterator(slots_dir)) {
  Confidence: band=very_high; score=0.9
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: states.push_back(kv.second->state());
  Confidence: band=high; score=0.74

### src/replication/schema_cdc.cpp
Total findings: 1

- Line 151: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: themis::cdc::CdcSchemaEncoder encoder(registry_.get());
  Confidence: band=very_high; score=0.9

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
