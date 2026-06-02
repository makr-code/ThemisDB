# cache Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cache
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 43
- Actionable Findings (Critical + High): 13
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 12 |
| Medium | 29 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| security | 57 |
| concurrency | 23 |
| reliability | 20 |
| performance_patterns | 18 |
| legacy_duplication | 16 |
| container | 13 |
| exception_safety | 8 |
| performance | 8 |
| raii | 7 |
| memory | 6 |
| distributed_consistency | 4 |
| observability | 4 |
| determinism | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/cache/distributed_cache_coordinator.cpp | 10 | 0 | 2 | 8 | 0 |
| src/cache/adaptive_query_cache.cpp | 8 | 0 | 4 | 3 | 1 |
| src/cache/redis_cache_coordinator.cpp | 7 | 0 | 0 | 7 | 0 |
| src/cache/cache_replication_coordinator.cpp | 5 | 0 | 2 | 3 | 0 |
| src/cache/predictive_prefetcher.cpp | 5 | 0 | 2 | 3 | 0 |
| src/cache/warmup.cpp | 3 | 0 | 0 | 3 | 0 |
| src/cache/cache_replication.cpp | 2 | 1 | 0 | 1 | 0 |
| src/cache/embedding_cache.cpp | 2 | 0 | 1 | 1 | 0 |
| src/cache/semantic_cache.cpp | 1 | 0 | 1 | 0 | 0 |
| src/cache/bounded_lru_cache.cpp | 0 | 0 | 0 | 0 | 0 |
| src/cache/cache_hit_rate_slo_monitor.cpp | 0 | 0 | 0 | 0 | 0 |
| src/cache/grpc_remote_cache_peer.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/cache/distributed_cache_coordinator.cpp
Total findings: 10

- Line 84: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //   Windows SDK builds without POSIX compatibility shims).  The coordinator
  Confidence: band=high; score=0.8
- Line 283: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sub_thread_ = std::thread(&RedisCacheCoordinator::subscriberLoop, this);
  Confidence: band=very_high; score=0.9
- Line 93: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: //   one node is never propagated to other nodes via Redis pub/sub; stale reads
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
  Confidence: band=medium; score=0.56
- Line 178: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
  Confidence: band=medium; score=0.56
- Line 183: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
  Confidence: band=medium; score=0.56
- Line 188: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Context: bool RedisCacheCoordinator::isConnected() const {
  Confidence: band=medium; score=0.56
- Line 192: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
  Confidence: band=medium; score=0.56
- Line 844: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
  Confidence: band=medium; score=0.56
- Line 871: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
  Confidence: band=medium; score=0.56

### src/cache/adaptive_query_cache.cpp
Total findings: 8

- Line 1230: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1803: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1950: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &k : keys_to_purge) {
  Confidence: band=very_high; score=0.9
- Line 2306: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1696: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back("L1:" + key.substr(0, 16) + "...");
  Confidence: band=high; score=0.74
- Line 1707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back("L2:" + key.substr(0, 16) + "...");
  Confidence: band=high; score=0.74
- Line 1935: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> keys_to_purge;
  Confidence: band=medium; score=0.66
- Line 1207: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_factor = std::log(static_cast<double>(access_count + 1)) / config_.adaptive_ttl_scaling_factor;
  Confidence: band=medium; score=0.6

### src/cache/redis_cache_coordinator.cpp
Total findings: 7

- Line 193: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
  Confidence: band=medium; score=0.56
- Line 266: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
  Confidence: band=medium; score=0.56
- Line 271: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
  Confidence: band=medium; score=0.56
- Line 280: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Context: bool RedisCacheCoordinator::isConnected() const {
  Confidence: band=medium; score=0.56
- Line 288: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
  Confidence: band=medium; score=0.56
- Line 638: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
  Confidence: band=medium; score=0.56
- Line 665: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
  Confidence: band=medium; score=0.56

### src/cache/cache_replication_coordinator.cpp
Total findings: 5

- Line 163: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: fanout_thread_ = std::thread(&CacheReplicationCoordinator::fanoutWorker, this);
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& peer : peers_to_contact) {
  Confidence: band=very_high; score=0.9
- Line 190: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<IRemoteCachePeer>> existing;
  Confidence: band=medium; score=0.66
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_peers.emplace_back(it->second);
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_peers.push_back(peer);
  Confidence: band=high; score=0.74

### src/cache/predictive_prefetcher.cpp
Total findings: 5

- Line 354: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto tod_from_it = tod_buckets_.find(from);
  Confidence: band=very_high; score=0.9
- Line 356: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto tod_to_it = tod_from_it->second.find(to);
  Confidence: band=very_high; score=0.9
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(score, to);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(candidates[i].second));
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: range_end.push_back(static_cast<char>(0xFF));
  Confidence: band=high; score=0.74

### src/cache/warmup.cpp
Total findings: 3

- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>((buf >> bits) & 0xFF));
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kB64Chars[(buf >> bits) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, processChunk, start, end));
  Confidence: band=high; score=0.74

### src/cache/cache_replication.cpp
Total findings: 2

- Line 176: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void CacheReplicationManager::notifyWrite(const std::string &key, const std::string &payload,
  Confidence: band=very_high; score=0.99
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/cache/embedding_cache.cpp
Total findings: 2

- Line 96: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
  Confidence: band=very_high; score=0.9
- Line 96: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
  Confidence: band=high; score=0.74

### src/cache/semantic_cache.cpp
Total findings: 1

- Line 140: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohmann::json &params) {
  Confidence: band=very_high; score=0.9

### src/cache/bounded_lru_cache.cpp
Total findings: 0


### src/cache/cache_hit_rate_slo_monitor.cpp
Total findings: 0


### src/cache/grpc_remote_cache_peer.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
