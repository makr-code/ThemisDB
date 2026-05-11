// Copyright 2026 ThemisDB — Licensed under MIT License

/**
 * @file bench_distributed_knowledge.cpp
 * @brief DK-8: Performance benchmarks for federated learning components.
 *
 * Performance targets (binding):
 *  - BM_TriggerAggregation_N64  : p99 ≤ 500 ms (N=64 shards, 100 gradient keys)
 *  - BM_FederatedRAGMerge_N16x50: p99 ≤  20 ms (N=16 shards, 50 docs each)
 *  - BM_FeedbackDedup_Throughput : ≥ 10k msg/s  (384-dim embeddings, 100k msgs)
 *  - BM_PublishFeedback_Latency  : p99 ≤   1 ms (end-to-end mock-gossip)
 */

#include <benchmark/benchmark.h>

#include "distributed_knowledge/cross_shard_feedback_sync.h"
#include "distributed_knowledge/federated_rag_merger.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

using namespace themis::distributed_knowledge;

// ─────────────────────────────────────────────────────────────────────────────
// Shared helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

constexpr uint64_t kDefaultFixtureRound = 1;
constexpr size_t kDefaultFixtureSampleCount = 100;

/// Build a gradient with `num_keys` numeric fields.
EncryptedGradient makeGradN(const std::string& shard_id, uint64_t round,
                             size_t num_keys) {
    EncryptedGradient g;
    g.shard_id    = shard_id;
    g.round       = round;
    g.sample_count = 100;
    g.data        = nlohmann::json::object();
    for (size_t k = 0; k < num_keys; ++k) {
        g.data["key_" + std::to_string(k)] = 0.001 * static_cast<double>(k);
    }
    return g;
}

/// Load benchmark gradients from JSON fixture.
///
/// Supported fixture formats:
///  1. Top-level array of gradient objects.
///  2. Object containing `gradients` array.
///
/// Each gradient object can provide:
///  - `shard_id` (string, optional; defaults to `fixture-shard-<index>`)
///  - `round` (number, optional; defaults to 1)
///  - `sample_count` (number, optional; defaults to 100)
///  - `data` (object, required for a usable entry)
///
/// Returns `std::nullopt` when the file cannot be opened, parsed, or no valid
/// gradient entries are present.
[[nodiscard]] std::optional<std::vector<EncryptedGradient>>
loadGradientFixture(const std::string& fixture_path) {
    std::ifstream input(fixture_path);
    if (!input.is_open()) {
        std::cerr << "Failed to open gradient fixture: " << fixture_path << '\n';
        return std::nullopt;
    }

    nlohmann::json root;
    try {
        input >> root;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse gradient fixture JSON (" << fixture_path
                  << "): " << e.what() << '\n';
        return std::nullopt;
    }

    const auto* entries = &root;
    if (root.is_object() && root.contains("gradients")) {
        entries = &root["gradients"];
    }
    if (!entries->is_array()) {
        std::cerr << "Invalid gradient fixture schema (" << fixture_path
                  << "): expected top-level array or object with `gradients` array.\n";
        return std::nullopt;
    }

    std::vector<EncryptedGradient> gradients;
    gradients.reserve(entries->size());
    size_t skipped_entries = 0;

    for (size_t i = 0; i < entries->size(); ++i) {
        const auto& entry = (*entries)[i];
        if (!entry.is_object()) {
            ++skipped_entries;
            continue;
        }

        std::string shard_id = "fixture-shard-" + std::to_string(i);
        if (entry.contains("shard_id") && entry["shard_id"].is_string()) {
            shard_id = entry["shard_id"].get<std::string>();
        }
        nlohmann::json gradient_data = entry.value("data", nlohmann::json::object());
        if (!gradient_data.is_object()) {
            ++skipped_entries;
            continue;
        }

        EncryptedGradient g;
        g.shard_id = shard_id;
        g.round = entry.value("round", kDefaultFixtureRound);
        g.sample_count = entry.value("sample_count", kDefaultFixtureSampleCount);
        g.data = std::move(gradient_data);
        gradients.push_back(std::move(g));
    }

    if (gradients.empty()) {
        std::cerr << "Gradient fixture contains no usable entries (" << fixture_path
                  << "); skipped " << skipped_entries << " invalid entries.\n";
        return std::nullopt;
    }
    return gradients;
}

/// Build a shard result with `num_docs` documents.
ShardRetrievalResult makeShardResult(const std::string& shard_id, size_t num_docs) {
    ShardRetrievalResult sr;
    sr.shard_id = shard_id;
    sr.ok       = true;
    sr.adapter_accuracy_delta = 0.0;
    sr.documents.reserve(num_docs);
    for (size_t i = 0; i < num_docs; ++i) {
        RetrievedDocument doc;
        doc.doc_id          = shard_id + "-" + std::to_string(i);
        doc.content         = "doc content " + std::to_string(i);
        doc.shard_id        = shard_id;
        doc.relevance_score = 1.0 - (0.01 * static_cast<double>(i));
        doc.rank_in_shard   = i + 1;
        sr.documents.push_back(std::move(doc));
    }
    return sr;
}

/// Build a 384-dim FeedbackSummary with a unique summary_id.
FeedbackSummary makeSummary(size_t index) {
    FeedbackSummary s;
    s.summary_id          = "bench-sum-" + std::to_string(index);
    s.feedback_type_label = "USER_NEGATIVE";
    s.shard_origin        = "ANON";
    s.reason_embedding    = std::vector<float>(384, static_cast<float>(index) * 0.001f);
    return s;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM_TriggerAggregation_N64: target ≤ 500 ms (p99)
// ─────────────────────────────────────────────────────────────────────────────

// SIMULATION NOTE:
// Purpose: Benchmark can replay real LoRA-style gradients from fixture JSON.
// Activation: Set THEMIS_BENCH_LORA_GRADIENT_FIXTURE=/abs/path/to/gradients.json.
// Production Delta: Aggregation still runs in-process; transfer/network latency
//                   is excluded by design and benchmark measures CPU-side cost.
// Removal Plan: N/A — benchmark file is permanent.

static void BM_TriggerAggregation_N64(benchmark::State& state) {
    const size_t num_shards = static_cast<size_t>(state.range(0));
    const size_t num_keys   = static_cast<size_t>(state.range(1));
    const char* fixture_env = std::getenv("THEMIS_BENCH_LORA_GRADIENT_FIXTURE");

    FederationConfig cfg;
    cfg.min_participants = num_shards;
    cfg.dp_epsilon       = 0.1;
    cfg.dp_delta         = 1e-5;
    cfg.dp_sensitivity   = 1.0;

    std::vector<EncryptedGradient> prepared_gradients;
    bool using_fixture = false;
    if (fixture_env && fixture_env[0] != '\0') {
        auto loaded = loadGradientFixture(fixture_env);
        if (!loaded) {
            state.SkipWithError("Invalid THEMIS_BENCH_LORA_GRADIENT_FIXTURE payload");
            return;
        }
        prepared_gradients = std::move(*loaded);
        using_fixture = true;
    } else {
        prepared_gradients.reserve(num_shards);
        for (size_t i = 0; i < num_shards; ++i) {
            prepared_gradients.push_back(
                makeGradN("shard-" + std::to_string(i), 1, num_keys));
        }
    }

    for (auto _ : state) {
        state.PauseTiming();
        LoRAFederationCoordinator coord(cfg);
        for (const auto& gradient : prepared_gradients) {
            coord.submitGradient(gradient);
        }
        state.ResumeTiming();

        auto delta = coord.triggerAggregation();
        benchmark::DoNotOptimize(delta.round);
    }

    state.SetLabel("shards=" + std::to_string(num_shards) +
                   " keys=" + std::to_string(num_keys) +
                   " source=" + std::string(using_fixture ? "fixture" : "synthetic"));
}

BENCHMARK(BM_TriggerAggregation_N64)
    ->Args({64, 100})   // DK-8 target: 64 shards × 100 keys
    ->Args({16, 100})   // Comparative: 16 shards
    ->Args({4,  100})   // Comparative: 4 shards
    ->Iterations(10)
    ->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_FederatedRAGMerge_N16x50: target ≤ 20 ms (p99)
// ─────────────────────────────────────────────────────────────────────────────

static void BM_FederatedRAGMerge_N16x50(benchmark::State& state) {
    const size_t num_shards = static_cast<size_t>(state.range(0));
    const size_t docs_per_shard = static_cast<size_t>(state.range(1));

    FederatedRAGMergerConfig cfg;
    cfg.top_k = 20;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> inputs;
    inputs.reserve(num_shards);
    for (size_t i = 0; i < num_shards; ++i) {
        inputs.push_back(makeShardResult("shard-" + std::to_string(i), docs_per_shard));
    }

    for (auto _ : state) {
        auto ctx = merger.merge(inputs);
        benchmark::DoNotOptimize(ctx.documents.size());
    }

    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(num_shards * docs_per_shard));
    state.SetLabel("shards=" + std::to_string(num_shards) +
                   " docs_per_shard=" + std::to_string(docs_per_shard));
}

BENCHMARK(BM_FederatedRAGMerge_N16x50)
    ->Args({16, 50})    // DK-8 target: 16 shards × 50 docs
    ->Args({8,  50})    // Comparative
    ->Args({4,  50})    // Comparative
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_FeedbackDedup_Throughput: target ≥ 10k msg/s
// ─────────────────────────────────────────────────────────────────────────────

static void BM_FeedbackDedup_Throughput(benchmark::State& state) {
    const size_t batch_size = static_cast<size_t>(state.range(0));

    FeedbackSyncConfig sync_cfg;
    sync_cfg.max_embedding_dim    = 384;
    sync_cfg.validate_embedding_dim = true;
    sync_cfg.dedup_cache_size     = 200'000; // large enough for full batch

    CrossShardFeedbackSync sync(
        sync_cfg, "bench-shard",
        [](nlohmann::json) {}); // no-op gossip dispatch

    // Pre-build summaries to exclude setup from timing
    std::vector<nlohmann::json> payloads;
    payloads.reserve(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
        payloads.push_back(makeSummary(i).toJson());
    }

    for (auto _ : state) {
        // Re-populate dedup cache each iteration so duplicates don't skew throughput
        state.PauseTiming();
        CrossShardFeedbackSync fresh_sync(sync_cfg, "bench-shard", [](nlohmann::json) {});
        state.ResumeTiming();

        for (const auto& payload : payloads) {
            fresh_sync.handleInboundSummary(payload);
        }
        benchmark::DoNotOptimize(fresh_sync.receivedCount());
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch_size));
    state.SetLabel("batch_size=" + std::to_string(batch_size));
}

BENCHMARK(BM_FeedbackDedup_Throughput)
    ->Arg(10'000)       // DK-8 target validation: 10k msg/s baseline
    ->Arg(50'000)       // Extended validation
    ->Iterations(3)
    ->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_PublishFeedback_Latency: target ≤ 1 ms (p99)
// ─────────────────────────────────────────────────────────────────────────────

static void BM_PublishFeedback_Latency(benchmark::State& state) {
    FeedbackSyncConfig sync_cfg;
    sync_cfg.max_embedding_dim    = 384;
    sync_cfg.validate_embedding_dim = true;

    size_t gossip_calls = 0;
    CrossShardFeedbackSync sync(
        sync_cfg, "bench-pub",
        [&gossip_calls](nlohmann::json) { ++gossip_calls; });

    FeedbackSummary summary;
    summary.feedback_type_label = "USER_NEGATIVE";
    summary.shard_origin        = "ANON";
    summary.reason_embedding    = std::vector<float>(384, 0.5f);

    size_t call_index = 0;
    for (auto _ : state) {
        // Use unique IDs to avoid dedup skipping
        summary.summary_id = "pub-bench-" + std::to_string(call_index++);
        sync.publishFeedback(summary);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("embed_dim=384");
    benchmark::DoNotOptimize(gossip_calls);
}

BENCHMARK(BM_PublishFeedback_Latency)
    ->Iterations(10'000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
