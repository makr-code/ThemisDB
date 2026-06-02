# distributed_knowledge Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: distributed_knowledge
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 112
- Actionable Findings (Critical + High): 91
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 28 |
| High | 63 |
| Medium | 19 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| distributed_consistency | 85 |
| performance_patterns | 7 |
| exception_safety | 6 |
| determinism | 5 |
| container | 3 |
| memory | 2 |
| observability | 2 |
| reliability | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/distributed_knowledge/federated_rag_merger.cpp | 90 | 25 | 54 | 11 | 0 |
| src/distributed_knowledge/lora_federation_coordinator.cpp | 14 | 1 | 6 | 6 | 1 |
| src/distributed_knowledge/federated_distillation_coordinator.cpp | 6 | 1 | 3 | 1 | 1 |
| src/distributed_knowledge/cross_shard_feedback_sync.cpp | 2 | 1 | 0 | 1 | 0 |

## Full Scanner Findings

### src/distributed_knowledge/federated_rag_merger.cpp
Total findings: 90

- Line 2: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * ThemisDB | File: federated_rag_merger.cpp | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.99
- Line 13: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * @file federated_rag_merger.cpp
  Confidence: band=very_high; score=0.99
- Line 14: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * @brief Ebene C — Cross-Shard RAG result merge implementation.
  Confidence: band=very_high; score=0.99
- Line 16: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * Implements Reciprocal Rank Fusion (RRF), score-weighted merge, and
  Confidence: band=very_high; score=0.99
- Line 24: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: #include "distributed_knowledge/federated_rag_merger.h"
  Confidence: band=very_high; score=0.99
- Line 35: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // MergedRAGContext::buildPromptContext
  Confidence: band=very_high; score=0.99
- Line 38: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string MergedRAGContext::buildPromptContext(size_t max_docs, size_t max_chars) const {
  Confidence: band=very_high; score=0.99
- Line 88: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MergedRAGContext FederatedRAGMerger::merge(const std::vector<ShardRetrievalResult> &shard_results) const {
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 120: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::RECIPROCAL_RANK_FUSION:
  Confidence: band=very_high; score=0.99
- Line 121: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged = mergeRRF(shard_results);
  Confidence: band=very_high; score=0.99
- Line 123: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::SCORE_WEIGHTED:
  Confidence: band=very_high; score=0.99
- Line 124: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged = mergeScoreWeighted(shard_results);
  Confidence: band=very_high; score=0.99
- Line 126: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::ROUND_ROBIN:
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged = mergeRoundRobin(shard_results);
  Confidence: band=very_high; score=0.99
- Line 155: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> FederatedRAGMerger::mergeRRF(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 200: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: FederatedRAGMerger::mergeScoreWeighted(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.99
- Line 219: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 244: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 248: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: while (any_remaining && merged.size() < config_.top_k * 2) {
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // DK-OR: GDPR erase (clears cached merge context)
  Confidence: band=very_high; score=0.99
- Line 281: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: themis::governance::StoreErasureResult FederatedRAGMerger::erase(const std::string & /*subject_id*/,
  Confidence: band=very_high; score=0.99
- Line 284: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // FederatedRAGMerger is stateless (no cached merge contexts); erase is a
  Confidence: band=very_high; score=0.99
- Line 287: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.store_id       = "FederatedRAGMerger";
  Confidence: band=very_high; score=0.99
- Line 2: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * ThemisDB | File: federated_rag_merger.cpp | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 13: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * @file federated_rag_merger.cpp
  Confidence: band=very_high; score=0.9
- Line 14: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * @brief Ebene C — Cross-Shard RAG result merge implementation.
  Confidence: band=very_high; score=0.9
- Line 16: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * Implements Reciprocal Rank Fusion (RRF), score-weighted merge, and
  Confidence: band=very_high; score=0.9
- Line 24: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: #include "distributed_knowledge/federated_rag_merger.h"
  Confidence: band=very_high; score=0.9
- Line 35: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // MergedRAGContext::buildPromptContext
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string MergedRAGContext::buildPromptContext(size_t max_docs, size_t max_chars) const {
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // FederatedRAGMerger
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FederatedRAGMerger::FederatedRAGMerger(FederatedRAGMergerConfig config) : config_(std::move(config)) {
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: throw std::invalid_argument("FederatedRAGMerger: invalid config");
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // merge
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedRAGContext FederatedRAGMerger::merge(const std::vector<ShardRetrievalResult> &shard_results) const {
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedRAGContext ctx;
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Count candidates before merge (skip timed-out shards)
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Run merge strategy
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::RECIPROCAL_RANK_FUSION:
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = mergeRRF(shard_results);
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::SCORE_WEIGHTED:
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = mergeScoreWeighted(shard_results);
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::ROUND_ROBIN:
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = mergeRoundRobin(shard_results);
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = deduplicate(std::move(merged));
  Confidence: band=very_high; score=0.9
- Line 135: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ctx.unique_doc_count = merged.size();
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (merged.size() > config_.top_k) {
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.resize(config_.top_k);
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ctx.documents = std::move(merged);
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string FederatedRAGMerger::mergeAndBuildContext(const std::vector<ShardRetrievalResult> &shard_results,
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merge(shard_results).buildPromptContext(max_docs, max_chars);
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeRRF (internal)
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> FederatedRAGMerger::mergeRRF(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.reserve(rrf_scores.size());
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(std::move(doc));
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(), [](const RetrievedDocument &a, const RetrievedDocument &b) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeScoreWeighted (internal)
  Confidence: band=very_high; score=0.9
- Line 200: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FederatedRAGMerger::mergeScoreWeighted(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.reserve(sum_scores.size());
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(std::move(doc));
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(), [](const RetrievedDocument &a, const RetrievedDocument &b) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeRoundRobin (internal)
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FederatedRAGMerger::mergeRoundRobin(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: while (any_remaining && merged.size() < config_.top_k * 2) {
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back((*list)[pos]);
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> FederatedRAGMerger::deduplicate(std::vector<RetrievedDocument> docs) const {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // DK-OR: GDPR erase (clears cached merge context)
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: themis::governance::StoreErasureResult FederatedRAGMerger::erase(const std::string & /*subject_id*/,
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // FederatedRAGMerger is stateless (no cached merge contexts); erase is a
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.store_id       = "FederatedRAGMerger";
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: snippet += "\n  Entities: " + it->second;
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> rrf_scores;
  Confidence: band=medium; score=0.66
- Line 158: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RetrievedDocument> best_doc;
  Confidence: band=medium; score=0.66
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> sum_scores;
  Confidence: band=medium; score=0.66
- Line 202: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RetrievedDocument> best_doc;
  Confidence: band=medium; score=0.66
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lists.push_back(&sr.documents);
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back((*list)[pos]);
- Line 266: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(doc));
  Confidence: band=high; score=0.74

### src/distributed_knowledge/lora_federation_coordinator.cpp
Total findings: 14

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 434: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool LoRAFederationCoordinator::verifyPrivacyBudget() const {
  Confidence: band=very_high; score=0.9
- Line 475: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (timeout_ms == 0 || future.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status:
- Line 480: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return future.get();
  Confidence: band=very_high; score=0.9
- Line 518: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [z_threshold](const EncryptedGradient &candidate,
- Line 521: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto l2norm = [](const nlohmann::json &data) -> double {
- Line 523: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto &[key, val] : data.items()) {
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 107: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return; // silently ignore stale or future rounds
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(v);
  Confidence: band=high; score=0.74
- Line 536: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norms.push_back(l2norm(grad.data));
  Confidence: band=high; score=0.74
- Line 324: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: = config_.dp_sensitivity * std::sqrt(2.0 * std::log(1.25 / config_.dp_delta)) / config_.dp_epsilon;
  Confidence: band=medium; score=0.6

### src/distributed_knowledge/federated_distillation_coordinator.cpp
Total findings: 6

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 89: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!verifyPrivacyBudget()) {
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!verifyPrivacyBudget()) {
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool FederatedDistillationCoordinator::verifyPrivacyBudget() const {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 41: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;
  Confidence: band=medium; score=0.6

### src/distributed_knowledge/cross_shard_feedback_sync.cpp
Total findings: 2

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 82: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
