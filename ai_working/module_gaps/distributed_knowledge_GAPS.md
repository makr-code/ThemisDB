# distributed_knowledge Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: distributed_knowledge
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 157
- Actionable Findings (Critical + High): 130
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 30 |
| High | 100 |
| Medium | 27 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| distributed_consistency | 85 |
| reliability | 24 |
| concurrency | 13 |
| container | 10 |
| performance_patterns | 10 |
| exception_safety | 6 |
| determinism | 5 |
| memory | 2 |
| observability | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/distributed_knowledge/federated_rag_merger.cpp | 97 | 25 | 57 | 15 | 0 |
| src/distributed_knowledge/lora_federation_coordinator.cpp | 30 | 2 | 19 | 8 | 1 |
| src/distributed_knowledge/federated_distillation_coordinator.cpp | 22 | 2 | 18 | 1 | 1 |
| src/distributed_knowledge/cross_shard_feedback_sync.cpp | 8 | 1 | 6 | 1 | 0 |

## Full Scanner Findings

### src/distributed_knowledge/federated_rag_merger.cpp
Total findings: 97

- Line 2: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * ThemisDB | File: federated_rag_merger.cpp | Version: 0.0.1
  Confidence: band=very_high; score=0.99
- Line 12: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * @file federated_rag_merger.cpp
  Confidence: band=very_high; score=0.99
- Line 13: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * @brief Ebene C — Cross-Shard RAG result merge implementation.
  Confidence: band=very_high; score=0.99
- Line 15: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * Implements Reciprocal Rank Fusion (RRF), score-weighted merge, and
  Confidence: band=very_high; score=0.99
- Line 23: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: #include "distributed_knowledge/federated_rag_merger.h"
  Confidence: band=very_high; score=0.99
- Line 34: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // MergedRAGContext::buildPromptContext
  Confidence: band=very_high; score=0.99
- Line 37: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string MergedRAGContext::buildPromptContext(size_t max_docs, size_t max_chars) const {
  Confidence: band=very_high; score=0.99
- Line 87: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MergedRAGContext FederatedRAGMerger::merge(const std::vector<ShardRetrievalResult> &shard_results) const {
  Confidence: band=very_high; score=0.99
- Line 117: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::RECIPROCAL_RANK_FUSION:
  Confidence: band=very_high; score=0.99
- Line 120: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged = mergeRRF(shard_results);
  Confidence: band=very_high; score=0.99
- Line 122: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::SCORE_WEIGHTED:
  Confidence: band=very_high; score=0.99
- Line 123: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged = mergeScoreWeighted(shard_results);
  Confidence: band=very_high; score=0.99
- Line 125: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: case MergeStrategy::ROUND_ROBIN:
  Confidence: band=very_high; score=0.99
- Line 126: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged = mergeRoundRobin(shard_results);
  Confidence: band=very_high; score=0.99
- Line 154: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> FederatedRAGMerger::mergeRRF(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.99
- Line 182: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 199: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: FederatedRAGMerger::mergeScoreWeighted(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.99
- Line 218: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.99
- Line 247: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: while (any_remaining && merged.size() < config_.top_k * 2) {
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // DK-OR: GDPR erase (clears cached merge context)
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: themis::governance::StoreErasureResult FederatedRAGMerger::erase(const std::string & /*subject_id*/,
  Confidence: band=very_high; score=0.99
- Line 283: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // FederatedRAGMerger is stateless (no cached merge contexts); erase is a
  Confidence: band=very_high; score=0.99
- Line 286: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.store_id       = "FederatedRAGMerger";
  Confidence: band=very_high; score=0.99
- Line 2: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * ThemisDB | File: federated_rag_merger.cpp | Version: 0.0.1
  Confidence: band=very_high; score=0.9
- Line 12: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * @file federated_rag_merger.cpp
  Confidence: band=very_high; score=0.9
- Line 13: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * @brief Ebene C — Cross-Shard RAG result merge implementation.
  Confidence: band=very_high; score=0.9
- Line 15: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * Implements Reciprocal Rank Fusion (RRF), score-weighted merge, and
  Confidence: band=very_high; score=0.9
- Line 23: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: #include "distributed_knowledge/federated_rag_merger.h"
  Confidence: band=very_high; score=0.9
- Line 34: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // MergedRAGContext::buildPromptContext
  Confidence: band=very_high; score=0.9
- Line 37: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string MergedRAGContext::buildPromptContext(size_t max_docs, size_t max_chars) const {
  Confidence: band=very_high; score=0.9
- Line 74: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // FederatedRAGMerger
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FederatedRAGMerger::FederatedRAGMerger(FederatedRAGMergerConfig config) : config_(std::move(config)) {
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: throw std::invalid_argument("FederatedRAGMerger: invalid config");
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederatedRAGMerger: invalid config");
- Line 84: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // merge
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedRAGContext FederatedRAGMerger::merge(const std::vector<ShardRetrievalResult> &shard_results) const {
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("all shards timed out");
- Line 99: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("all shards timed out");
- Line 104: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedRAGContext ctx;
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Count candidates before merge (skip timed-out shards)
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Run merge strategy
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::RECIPROCAL_RANK_FUSION:
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = mergeRRF(shard_results);
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::SCORE_WEIGHTED:
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = mergeScoreWeighted(shard_results);
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: case MergeStrategy::ROUND_ROBIN:
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = mergeRoundRobin(shard_results);
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged = deduplicate(std::move(merged));
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ctx.unique_doc_count = merged.size();
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (merged.size() > config_.top_k) {
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.resize(config_.top_k);
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ctx.documents = std::move(merged);
  Confidence: band=very_high; score=0.9
- Line 145: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string FederatedRAGMerger::mergeAndBuildContext(const std::vector<ShardRetrievalResult> &shard_results,
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merge(shard_results).buildPromptContext(max_docs, max_chars);
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeRRF (internal)
  Confidence: band=very_high; score=0.9
- Line 154: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> FederatedRAGMerger::mergeRRF(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.reserve(rrf_scores.size());
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(std::move(doc));
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(), [](const RetrievedDocument &a, const RetrievedDocument &b) {
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeScoreWeighted (internal)
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FederatedRAGMerger::mergeScoreWeighted(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.reserve(sum_scores.size());
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(std::move(doc));
  Confidence: band=very_high; score=0.9
- Line 224: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(), [](const RetrievedDocument &a, const RetrievedDocument &b) {
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 231: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeRoundRobin (internal)
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: FederatedRAGMerger::mergeRoundRobin(const std::vector<ShardRetrievalResult> &results) const {
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> merged;
  Confidence: band=very_high; score=0.9
- Line 247: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: while (any_remaining && merged.size() < config_.top_k * 2) {
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back((*list)[pos]);
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<RetrievedDocument> FederatedRAGMerger::deduplicate(std::vector<RetrievedDocument> docs) const {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // DK-OR: GDPR erase (clears cached merge context)
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: themis::governance::StoreErasureResult FederatedRAGMerger::erase(const std::string & /*subject_id*/,
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // FederatedRAGMerger is stateless (no cached merge contexts); erase is a
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.store_id       = "FederatedRAGMerger";
  Confidence: band=very_high; score=0.9
- Line 51: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: snippet += "\n  Entities: " + it->second;
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> rrf_scores;
  Confidence: band=medium; score=0.66
- Line 157: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RetrievedDocument> best_doc;
  Confidence: band=medium; score=0.66
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(doc));
- Line 200: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> sum_scores;
  Confidence: band=medium; score=0.66
- Line 201: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RetrievedDocument> best_doc;
  Confidence: band=medium; score=0.66
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(doc));
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lists.push_back(&sr.documents);
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lists.push_back(&sr.documents);
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back((*list)[pos]);
- Line 265: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(doc));

### src/distributed_knowledge/lora_federation_coordinator.cpp
Total findings: 30

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 226: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = pending_gradients_.begin(); it != pending_gradients_.end();) {
- Line 52: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRAFederationCoordinator: invalid FederationConfig");
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LoRAFederationCoordinator::triggerAggregation: DP budget exhausted "
- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LoRAFederationCoordinator::triggerAggregation: Cross-border "
- Line 179: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LoRAFederationCoordinator::triggerAggregation: insufficient "
- Line 236: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LoRAFederationCoordinator::doAggregation: insufficient "
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("NaN detected in gradient data for round " + std::to_string(current_round_)
- Line 350: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 355: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 360: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 433: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool LoRAFederationCoordinator::verifyPrivacyBudget() const {
  Confidence: band=very_high; score=0.9
- Line 447: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 452: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 474: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: if (timeout_ms == 0 || future.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status::ready) {
  Confidence: band=very_high; score=0.9
- Line 474: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (timeout_ms == 0 || future.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status:
- Line 475: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LoRAFederationCoordinator::triggerAggregation: federation round "
- Line 479: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return future.get();
  Confidence: band=very_high; score=0.9
- Line 517: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [z_threshold](const EncryptedGradient &candidate,
- Line 520: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto l2norm = [](const nlohmann::json &data) -> double {
- Line 522: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto &[key, val] : data.items()) {
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 106: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: return; // silently ignore stale or future rounds
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(v);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vals.push_back(v);
- Line 535: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: norms.push_back(l2norm(grad.data));
  Confidence: band=high; score=0.74
- Line 536: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: norms.push_back(l2norm(grad.data));
- Line 323: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: = config_.dp_sensitivity * std::sqrt(2.0 * std::log(1.25 / config_.dp_delta)) / config_.dp_epsilon;
  Confidence: band=medium; score=0.6

### src/distributed_knowledge/federated_distillation_coordinator.cpp
Total findings: 22

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 350: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: card.teacher_id           = last_round_.has_value() ? last_round_->teacher_id : pending_teacher_id_;
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederatedDistillationCoordinator: invalid DistillationConfig");
- Line 83: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("submitSoftLabels: labels must not be empty");
- Line 86: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("submitSoftLabels: teacher_id must not be empty");
- Line 88: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!verifyPrivacyBudget()) {
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FederatedDistillationCoordinator: DP privacy budget exhausted");
- Line 110: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("broadcastToStudents: no soft labels submitted for this round");
- Line 112: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!verifyPrivacyBudget()) {
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FederatedDistillationCoordinator: DP privacy budget exhausted");
- Line 119: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Policy gate rejected distillation broadcast");
- Line 170: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 175: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 209: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("registerStudent: student_id must not be empty");
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("registerStudent: callback must not be null");
- Line 223: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 228: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 278: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool FederatedDistillationCoordinator::verifyPrivacyBudget() const {
  Confidence: band=very_high; score=0.9
- Line 333: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &label : labels) {
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &p : label.probabilities) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 40: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;
  Confidence: band=medium; score=0.6

### src/distributed_knowledge/cross_shard_feedback_sync.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 107: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 153: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 158: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 163: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 81: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
