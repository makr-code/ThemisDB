/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_continuous_batch_scheduler.cpp               ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:22:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     318                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b3d8aa4a55  2026-03-15  refactor: streamline performance statistics retrieval and... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_continuous_batch_scheduler.cpp
 * @brief Throughput and latency benchmarks for ContinuousBatchScheduler.
 *
 * Measures:
 *  - Request submission throughput (requests/second) at various batch sizes.
 *  - Queue-depth enforcement latency (time to reject when full).
 *  - scheduleNextBatch() latency under increasing queue load.
 *  - Quota-rejection throughput (TokenQuotaManager integration).
 *
 * These benchmarks are designed to run in a unit-test environment without a
 * real GPU or loaded model.  All benchmarks use GTest and std::chrono.
 *
 * SLA targets (conservative, for CI gate):
 *  - submitRequest() p99 < 100 µs under 1 000-request load.
 *  - scheduleNextBatch() with 64 waiting requests < 5 ms.
 *  - Rejection (queue full) p99 < 50 µs.
 *
 * @see docs/llm_roadmap.md — Q3 Testing checklist
 */

#include <gtest/gtest.h>
#include "llm/continuous_batch_scheduler.h"
#include "llm/paged_kv_cache.h"
#include "llm/paged_block_manager.h"
#include "llm/token_quota_manager.h"
#include <chrono>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>

using namespace themis::llm;
using Clock = std::chrono::steady_clock;
using Micros = std::chrono::microseconds;
using Millis = std::chrono::milliseconds;

// ---------------------------------------------------------------------------
// Benchmark fixture
// ---------------------------------------------------------------------------

class SchedulerBenchmark : public ::testing::Test {
protected:
    static constexpr size_t BLOCK_SIZE   = 16;
    static constexpr size_t NUM_BLOCKS   = 4096;
    static constexpr size_t BATCH_SIZE   = 64;
    static constexpr size_t MAX_TOKENS   = 16384;

    void SetUp() override {
        PagedBlockManager::Config bm_cfg;
        bm_cfg.block_size_tokens = BLOCK_SIZE;
        bm_cfg.max_blocks        = static_cast<int>(NUM_BLOCKS);
        block_manager = std::make_shared<PagedBlockManager>(bm_cfg);

        PagedKVCache::Config kv_cfg;
        kv_cfg.block_size = BLOCK_SIZE;
        kv_cache = std::make_unique<PagedKVCache>(kv_cfg, block_manager);

        ContinuousBatchScheduler::SchedulerConfig sched_cfg;
        sched_cfg.max_batch_size       = BATCH_SIZE;
        sched_cfg.max_tokens_per_batch = MAX_TOKENS;
        sched_cfg.block_size_tokens    = BLOCK_SIZE;
        scheduler = std::make_unique<ContinuousBatchScheduler>(sched_cfg, kv_cache.get());
        scheduler->start();
    }

    void TearDown() override {
        scheduler->stop();
    }

    InferenceRequest makeRequest(size_t prompt_len = 20, size_t max_tok = 10) {
        InferenceRequest req;
        req.prompt    = std::string(prompt_len, 'x');
        req.max_tokens = static_cast<int>(max_tok);
        req.model_id  = "bench-model";
        return req;
    }

    /// Helper: submit N requests, record per-call durations (µs).
    std::vector<long> submitN(size_t n, size_t prompt_len = 20) {
        std::vector<long> durations_us;
        durations_us.reserve(n);
        std::vector<std::string> ids;
        ids.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            auto t0 = Clock::now();
            auto id = scheduler->submitRequest(makeRequest(prompt_len));
            auto t1 = Clock::now();
            durations_us.push_back(
                std::chrono::duration_cast<Micros>(t1 - t0).count());
            if (!id.empty()) {
                ids.push_back(std::move(id));
            }
        }
        // Cancel accepted requests to keep state clean
        for (const auto& id : ids) {
            scheduler->cancelRequest(id);
        }
        return durations_us;
    }

    static long percentile(std::vector<long> v, int pct) {
        if (v.empty()) return 0;
        std::sort(v.begin(), v.end());
        size_t idx = static_cast<size_t>(
            std::ceil(pct / 100.0 * static_cast<double>(v.size()))) - 1;
        idx = std::min(idx, v.size() - 1);
        return v[idx];
    }

    std::shared_ptr<PagedBlockManager> block_manager;
    std::unique_ptr<PagedKVCache>      kv_cache;
    std::unique_ptr<ContinuousBatchScheduler> scheduler;
};

// ---------------------------------------------------------------------------
// Benchmark 1 — submitRequest() throughput (1 000 requests)
// ---------------------------------------------------------------------------

TEST_F(SchedulerBenchmark, SubmitThroughput_1000Requests) {
    constexpr size_t N = 1000;
    auto durations = submitN(N);

    long p50 = percentile(durations, 50);
    long p99 = percentile(durations, 99);
    long mean_us = 0;
    for (auto d : durations) mean_us += d;
    mean_us /= static_cast<long>(durations.size());

    // Print for CI visibility
    printf("[Bench] submitRequest 1000x: mean=%ldµs  p50=%ldµs  p99=%ldµs\n",
           mean_us, p50, p99);

    // SLA gate: p99 < 100 µs in a no-GPU unit-test environment
    EXPECT_LT(p99, 100L) << "submitRequest p99 exceeded 100µs SLA";
}

// ---------------------------------------------------------------------------
// Benchmark 2 — scheduleNextBatch() latency with 64 waiting requests
// ---------------------------------------------------------------------------

TEST_F(SchedulerBenchmark, ScheduleBatch_LatencyWith64WaitingRequests) {
    constexpr size_t N = 64;
    std::vector<std::string> ids;
    ids.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto id = scheduler->submitRequest(makeRequest(16, 8));
        if (!id.empty()) ids.push_back(std::move(id));
    }

    auto t0    = Clock::now();
    auto batch = scheduler->scheduleNextBatch();
    auto t1    = Clock::now();

    long elapsed_us = std::chrono::duration_cast<Micros>(t1 - t0).count();
    printf("[Bench] scheduleNextBatch(%zu waiting): %ldµs\n",
           ids.size(), elapsed_us);

    // SLA gate: < 5 ms = 5 000 µs
    EXPECT_LT(elapsed_us, 5000L)
        << "scheduleNextBatch exceeded 5ms SLA with " << ids.size() << " waiting requests";

    for (const auto& id : ids) {
        scheduler->cancelRequest(id);
    }
}

// ---------------------------------------------------------------------------
// Benchmark 3 — Queue-full rejection latency
// ---------------------------------------------------------------------------

TEST_F(SchedulerBenchmark, RejectionLatency_QueueFull) {
    // Create a scheduler with a very small queue to force rejection quickly
    PagedBlockManager::Config bm_cfg;
    bm_cfg.block_size_tokens = BLOCK_SIZE;
    bm_cfg.max_blocks        = static_cast<int>(NUM_BLOCKS);
    auto bm = std::make_shared<PagedBlockManager>(bm_cfg);

    PagedKVCache::Config kv_cfg;
    kv_cfg.block_size = BLOCK_SIZE;
    auto kv = std::make_unique<PagedKVCache>(kv_cfg, bm);

    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_batch_size       = BATCH_SIZE;
    cfg.max_tokens_per_batch = MAX_TOKENS;
    cfg.block_size_tokens    = BLOCK_SIZE;
    cfg.max_queue_depth      = 4;

    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, kv.get());
    sched->start();

    // Fill the queue
    std::vector<std::string> ids;
    for (size_t i = 0; i < 4; ++i) {
        auto id = sched->submitRequest(makeRequest(16, 8));
        if (!id.empty()) ids.push_back(std::move(id));
    }

    // Measure rejection latency over 1 000 overflow attempts
    constexpr size_t REJECT_ITERS = 1000;
    std::vector<long> durations_us;
    durations_us.reserve(REJECT_ITERS);

    for (size_t i = 0; i < REJECT_ITERS; ++i) {
        auto t0 = Clock::now();
        auto id = sched->submitRequest(makeRequest());
        auto t1 = Clock::now();
        EXPECT_TRUE(id.empty());  // Must be rejected
        durations_us.push_back(
            std::chrono::duration_cast<Micros>(t1 - t0).count());
    }

    long p99 = percentile(durations_us, 99);
    printf("[Bench] rejection p99: %ldµs\n", p99);

    // SLA gate: rejection must be < 50 µs p99
    EXPECT_LT(p99, 50L) << "Rejection p99 exceeded 50µs SLA";

    for (const auto& id : ids) sched->cancelRequest(id);
    sched->stop();
}

// ---------------------------------------------------------------------------
// Benchmark 4 — Quota-rejection throughput (TokenQuotaManager)
// ---------------------------------------------------------------------------

TEST_F(SchedulerBenchmark, QuotaRejectionThroughput) {
    // Attach a quota manager with a tiny limit so every request is rejected
    TokenQuotaManager quota;
    quota.setQuota("bench-user", "bench-model", /*limit=*/1);  // 1 token per 60-second window

    scheduler->setQuotaManager(&quota);

    constexpr size_t N = 500;
    std::vector<long> durations_us;
    durations_us.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        InferenceRequest req = makeRequest(20, 10);
        req.request_id = "bench-user";  // used as quota key in scheduler
        auto t0 = Clock::now();
        auto id = scheduler->submitRequest(req);
        auto t1 = Clock::now();
        durations_us.push_back(
            std::chrono::duration_cast<Micros>(t1 - t0).count());
        if (!id.empty()) scheduler->cancelRequest(id);
    }

    long p99 = percentile(durations_us, 99);
    long mean_us = 0;
    for (auto d : durations_us) mean_us += d;
    mean_us /= static_cast<long>(durations_us.size());

    printf("[Bench] quota rejection %zux: mean=%ldµs  p99=%ldµs\n",
           N, mean_us, p99);

    // SLA gate: quota check + rejection < 200 µs p99
    EXPECT_LT(p99, 200L) << "Quota rejection p99 exceeded 200µs SLA";

    scheduler->setQuotaManager(nullptr);
}

// ---------------------------------------------------------------------------
// Benchmark 5 — Stats under load (getStats() call cost)
// ---------------------------------------------------------------------------

TEST_F(SchedulerBenchmark, GetStats_CallCostUnderLoad) {
    constexpr size_t N_REQUESTS = 32;
    std::vector<std::string> ids;
    for (size_t i = 0; i < N_REQUESTS; ++i) {
        auto id = scheduler->submitRequest(makeRequest(16, 8));
        if (!id.empty()) ids.push_back(std::move(id));
    }

    constexpr size_t N_CALLS = 10000;
    auto t0 = Clock::now();
    for (size_t i = 0; i < N_CALLS; ++i) {
        volatile auto stats = scheduler->getStats();
        (void)stats;
    }
    auto t1 = Clock::now();

    long total_us    = std::chrono::duration_cast<Micros>(t1 - t0).count();
    long per_call_ns = (total_us * 1000L) / static_cast<long>(N_CALLS);

    printf("[Bench] getStats() per-call: %ldns (%ld calls)\n",
           per_call_ns, static_cast<long>(N_CALLS));

    // SLA gate: getStats() should be < 10 µs per call even under mutex
    EXPECT_LT(per_call_ns, 10000L) << "getStats() per-call cost > 10µs";

    for (const auto& id : ids) scheduler->cancelRequest(id);
}
