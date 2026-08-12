// Copyright 2026 ThemisDB — Licensed under MIT License

/**
 * @file bench_distributed_knowledge_or.cpp
 * @brief DK-OR (S-15): Operational Resilience benchmarks for federated
 *        knowledge layer.
 *
 * Performance targets (binding):
 *  - BM_TriggerAggregation_TimeoutOverhead : overhead ≤ 2 ms vs. no-timeout path
 *  - BM_FeedbackPublish_Backpressure       : skipped-publish path ≤ 0.5 µs/call
 *  - BM_MergeWithTimedOutShards_50pct      : ≤ 110% of baseline (no timeout)
 *  - BM_Erase_FederationCoordinator        : p99 ≤ 5 µs
 *  - BM_ZeroTrust_LowRiskPath              : p99 ≤ 0.2 µs/msg overhead
 */

#include <benchmark/benchmark.h>

#include "distributed_knowledge/cross_shard_feedback_sync.h"
#include "distributed_knowledge/federated_rag_merger.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

#include <string>
#include <vector>

using namespace themis::distributed_knowledge;

// ─────────────────────────────────────────────────────────────────────────────
// Shared helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

FederationConfig makeFedConfig(size_t n = 8) {
    FederationConfig cfg;
    cfg.min_participants = n;
    cfg.dp_epsilon       = 0.5;
    cfg.dp_delta         = 1e-5;
    cfg.dp_sensitivity   = 1.0;
    return cfg;
}

EncryptedGradient makeGradN(const std::string& shard_id, uint64_t round,
                             size_t num_keys) {
    EncryptedGradient g;
    g.shard_id     = shard_id;
    g.round        = round;
    g.sample_count = 100;
    for (size_t k = 0; k < num_keys; ++k) {
        g.data["key_" + std::to_string(k)] = 0.001 * static_cast<double>(k);
    }
    return g;
}

ShardRetrievalResult makeShardResultOR(const std::string& shard_id,
                                        size_t num_docs,
                                        bool timed_out = false) {
    ShardRetrievalResult sr;
    sr.shard_id  = shard_id;
    sr.ok        = true;
    sr.timed_out = timed_out;
    sr.documents.reserve(num_docs);
    for (size_t i = 0; i < num_docs; ++i) {
        RetrievedDocument doc;
        doc.doc_id          = shard_id + "-" + std::to_string(i);
        doc.content         = "doc";
        doc.shard_id        = shard_id;
        doc.relevance_score = 1.0 - 0.01 * static_cast<double>(i);
        doc.rank_in_shard   = i + 1;
        sr.documents.push_back(std::move(doc));
    }
    return sr;
}

FeedbackSummary makeFeedback(const std::string& summary_id, size_t embed_dim = 64) {
    FeedbackSummary fs;
    fs.summary_id          = summary_id;
    fs.feedback_type_label = "USER_NEGATIVE";
    fs.shard_origin        = "ANON";
    fs.reason_embedding.assign(embed_dim, 0.1f);
    return fs;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM_TriggerAggregation_NoTimeout
// Baseline: standard triggerAggregation() with N=8 shards, 50 keys.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TriggerAggregation_NoTimeout(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const size_t num_keys = 50;

    for (auto _ : state) {
        state.PauseTiming();
        auto cfg = makeFedConfig(n);
        LoRAFederationCoordinator coord(cfg);
        for (size_t s = 0; s < n; ++s) {
            coord.submitGradient(makeGradN("shard-" + std::to_string(s), 1, num_keys));
        }
        state.ResumeTiming();

        benchmark::DoNotOptimize(coord.triggerAggregation());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TriggerAggregation_NoTimeout)->Arg(4)->Arg(8)->Arg(16);

// ─────────────────────────────────────────────────────────────────────────────
// BM_FeedbackPublish_BackpressureSkip
// Measures the overhead of the non-blocking skipped-publish path (gossip throws).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_FeedbackPublish_BackpressureSkip(benchmark::State& state) {
    FeedbackSyncConfig cfg;
    cfg.max_embedding_dim      = 64;
    cfg.validate_embedding_dim = true;

    // Gossip sink always throws → tests the skipped-publish path
    CrossShardFeedbackSync sync(cfg, "bench-shard",
        [](nlohmann::json) { throw std::runtime_error("queue full"); });

    size_t id = 0;
    for (auto _ : state) {
        sync.publishFeedback(makeFeedback("fs-" + std::to_string(id++)));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FeedbackPublish_BackpressureSkip);

// ─────────────────────────────────────────────────────────────────────────────
// BM_MergeWithTimedOutShards
// N shards, M% timed out. Measures merge performance with partial timeouts.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_MergeWithTimedOutShards(benchmark::State& state) {
    const size_t total_shards   = static_cast<size_t>(state.range(0));
    const size_t timed_out_pct  = static_cast<size_t>(state.range(1));
    const size_t timed_out_cnt  = (total_shards * timed_out_pct) / 100;
    const size_t docs_per_shard = 50;

    FederatedRAGMergerConfig cfg;
    cfg.top_k            = 20;
    cfg.shard_timeout_ms = 1000;

    std::vector<ShardRetrievalResult> shard_results;
    shard_results.reserve(total_shards);
    for (size_t s = 0; s < total_shards; ++s) {
        shard_results.push_back(
            makeShardResultOR("shard-" + std::to_string(s), docs_per_shard,
                              s < timed_out_cnt));
    }

    FederatedRAGMerger merger(cfg);

    for (auto _ : state) {
        benchmark::DoNotOptimize(merger.merge(shard_results));
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("shards=" + std::to_string(total_shards) +
                   " timed_out=" + std::to_string(timed_out_pct) + "%");
}
BENCHMARK(BM_MergeWithTimedOutShards)
    ->Args({8,  0})    // 0% timeout — baseline
    ->Args({8,  50})   // 50% timeout
    ->Args({16, 25});  // 25% timeout, 16 shards

// ─────────────────────────────────────────────────────────────────────────────
// BM_Erase_FederationCoordinator
// Measures GDPR erase latency (clear pending_gradients_, reset round).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_Erase_FederationCoordinator(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        auto cfg = makeFedConfig(n);
        LoRAFederationCoordinator coord(cfg);
        for (size_t s = 0; s < n; ++s) {
            coord.submitGradient(makeGradN("shard-" + std::to_string(s), 1, 20));
        }
        state.ResumeTiming();

        coord.erase("subject-bench");
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Erase_FederationCoordinator)->Arg(4)->Arg(16)->Arg(64);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ZeroTrust_LowRiskPath
// Overhead of the ZeroTrust enforcer (trusted path, returns true).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ZeroTrust_LowRiskPath(benchmark::State& state) {
    FeedbackSyncConfig cfg;
    cfg.max_embedding_dim      = 64;
    cfg.validate_embedding_dim = true;

    std::vector<nlohmann::json> sent;
    CrossShardFeedbackSync sync(cfg, "bench-shard",
        [&sent](nlohmann::json j) { sent.push_back(std::move(j)); });

    // Low-risk enforcer (always trusted)
    sync.setZeroTrustEnforcer([](const FeedbackSummary&) -> bool { return true; });

    size_t id = 0;
    for (auto _ : state) {
        FeedbackSummary fs = makeFeedback("zt-" + std::to_string(id++));
        sync.handleInboundSummary(fs.toJson());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ZeroTrust_LowRiskPath);

BENCHMARK_MAIN();
