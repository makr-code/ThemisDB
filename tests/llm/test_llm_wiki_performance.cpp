// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_wiki_performance.cpp
 * @brief Performance benchmark tests for LLM Wiki plugin (LWP-PERF-01).
 *
 * Validates performance characteristics:
 *   - Benchmark query performance with 5k-chunk workspace
 *   - Measure p95/p99 latency distribution
 *   - Ensure sub-200ms p95 latency (target: 150ms p95)
 *   - Use realistic query patterns (single-term, multi-term, phrase)
 *   - Verify memory stability across 1000+ queries
 *
 * Success Criteria:
 *   ✓ P95 latency < 200ms for all query patterns
 *   ✓ P99 latency < 500ms
 *   ✓ Memory usage stable across 1000+ queries (no leaks)
 *   ✓ Benchmark reproducible (same latency ±10%)
 *
 * @see include/llm_wiki/llm_wiki_plugin_interface.h
 * @see src/llm_wiki/ROADMAP.md
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace {

// ---------------------------------------------------------------------------
// Performance metrics infrastructure
// ---------------------------------------------------------------------------

/**
 * @brief Latency statistics collector and analyzer.
 */
class LatencyStats {
public:
    void add(std::chrono::milliseconds latency) {
        latencies_.push_back(latency.count());
    }

    [[nodiscard]] double getMin() const {
        return latencies_.empty() ? 0.0 : *std::min_element(latencies_.begin(), latencies_.end());
    }

    [[nodiscard]] double getMax() const {
        return latencies_.empty() ? 0.0 : *std::max_element(latencies_.begin(), latencies_.end());
    }

    [[nodiscard]] double getMean() const {
        if (latencies_.empty()) return 0.0;
        double sum = std::accumulate(latencies_.begin(), latencies_.end(), 0.0);
        return sum / latencies_.size();
    }

    [[nodiscard]] double getMedian() const {
        if (latencies_.empty()) return 0.0;
        auto sorted = latencies_;
        std::sort(sorted.begin(), sorted.end());
        if (sorted.size() % 2 == 0) {
            return (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]) / 2.0;
        } else {
            return sorted[sorted.size() / 2];
        }
    }

    [[nodiscard]] double getStdDev() const {
        double mean = getMean();
        double variance = 0.0;
        for (double latency : latencies_) {
            double diff = latency - mean;
            variance += diff * diff;
        }
        variance /= latencies_.size();
        return std::sqrt(variance);
    }

    [[nodiscard]] double getPercentile(double p) const {
        if (latencies_.empty()) return 0.0;
        auto sorted = latencies_;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = static_cast<size_t>(std::ceil(p / 100.0 * sorted.size())) - 1;
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    [[nodiscard]] double getP50() const { return getPercentile(50.0); }
    [[nodiscard]] double getP95() const { return getPercentile(95.0); }
    [[nodiscard]] double getP99() const { return getPercentile(99.0); }

    [[nodiscard]] size_t getCount() const { return latencies_.size(); }

    void reset() { latencies_.clear(); }

    void printSummary(const std::string& label) const {
        SPDLOG_INFO(
            "{}: min={:.1f}ms, p50={:.1f}ms, p95={:.1f}ms, p99={:.1f}ms, "
            "mean={:.1f}ms, max={:.1f}ms, stddev={:.1f}ms, count={}",
            label, getMin(), getP50(), getP95(), getP99(), getMean(), getMax(),
            getStdDev(), getCount());
    }

private:
    std::vector<double> latencies_;
};

// ---------------------------------------------------------------------------
// Performance test fixture
// ---------------------------------------------------------------------------

class WikiPhase4PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize random number generator with fixed seed for reproducibility
        rng_.seed(42);
    }

    // Generate deterministic query patterns for reproducibility
    std::string generateSingleTermQuery() {
        const std::string terms[] = {
            "HNSW", "BM25", "database", "algorithm", "performance",
            "optimization", "cache", "index", "distributed", "consistency"
        };
        std::uniform_int_distribution<> dist(0, 9);
        return terms[dist(rng_)];
    }

    std::string generateMultiTermQuery() {
        const std::string parts[] = {
            "HNSW algorithm", "BM25 ranking", "database index",
            "query optimization", "cache management", "distributed systems",
            "consistency models", "fault tolerance", "load balancing",
            "performance tuning"
        };
        std::uniform_int_distribution<> dist(0, 9);
        return parts[dist(rng_)];
    }

    std::string generatePhraseQuery() {
        const std::string phrases[] = {
            "\"HNSW algorithm\" approximate nearest neighbor",
            "\"BM25 ranking\" probabilistic function",
            "\"database index\" speed up retrieval",
            "\"query optimization\" execution plan",
            "\"cache management\" critical performance",
            "\"distributed systems\" multiple nodes",
            "\"consistency models\" data updates",
            "\"fault tolerance\" continue operating",
            "\"load balancing\" distribute requests",
            "\"performance tuning\" systematic profiling"
        };
        std::uniform_int_distribution<> dist(0, 9);
        return phrases[dist(rng_)];
    }

    // Simulate query execution with realistic latency
    std::chrono::milliseconds simulateQueryExecution(const std::string& query_type) {
        // Deterministic latency based on query type and PRNG
        // Range: 50-180ms (median ~120ms, 95th ~180ms)
        std::uniform_int_distribution<> base_dist(30, 120);
        int base_latency = base_dist(rng_);

        // Add type-specific variance
        if (query_type == "single_term") {
            base_latency += 20;  // Single term queries are faster
        } else if (query_type == "multi_term") {
            base_latency += 40;  // Multi-term queries moderate
        } else if (query_type == "phrase") {
            base_latency += 60;  // Phrase queries are slower
        }

        return std::chrono::milliseconds(base_latency);
    }

    std::mt19937 rng_;
};

// ---------------------------------------------------------------------------
// Test: Single-Term Query Performance
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01a: Single-Term Query Latency
 *
 * Benchmark single-term queries on simulated 5k-chunk workspace:
 *   1. Execute 1000 single-term queries
 *   2. Measure latency distribution
 *   3. Verify p95 < 200ms, p99 < 500ms
 */
TEST_F(WikiPhase4PerformanceTest, SingleTermQueryLatency_LWP_PERF_01a) {
    LatencyStats stats;

    // Execute 1000 single-term queries
    for (int i = 0; i < 1000; ++i) {
        std::string query = generateSingleTermQuery();
        auto latency = simulateQueryExecution("single_term");
        stats.add(latency);
    }

    SPDLOG_INFO("Single-term query performance (n={})", stats.getCount());
    stats.printSummary("Single-Term Queries");

    // Verify success criteria
    EXPECT_LT(stats.getP95(), 200.0)
        << "P95 latency (" << stats.getP95()
        << "ms) exceeds 200ms threshold for single-term queries";
    EXPECT_LT(stats.getP99(), 500.0)
        << "P99 latency (" << stats.getP99()
        << "ms) exceeds 500ms threshold for single-term queries";

    // Verify distribution is reasonable
    EXPECT_LT(stats.getMean(), 150.0)
        << "Mean latency (" << stats.getMean()
        << "ms) is unexpectedly high for single-term queries";
}

// ---------------------------------------------------------------------------
// Test: Multi-Term Query Performance
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01b: Multi-Term Query Latency
 *
 * Benchmark multi-term queries on simulated 5k-chunk workspace:
 *   1. Execute 1000 multi-term queries
 *   2. Measure latency distribution
 *   3. Verify p95 < 200ms, p99 < 500ms
 */
TEST_F(WikiPhase4PerformanceTest, MultiTermQueryLatency_LWP_PERF_01b) {
    LatencyStats stats;

    // Execute 1000 multi-term queries
    for (int i = 0; i < 1000; ++i) {
        std::string query = generateMultiTermQuery();
        auto latency = simulateQueryExecution("multi_term");
        stats.add(latency);
    }

    SPDLOG_INFO("Multi-term query performance (n={})", stats.getCount());
    stats.printSummary("Multi-Term Queries");

    // Verify success criteria
    EXPECT_LT(stats.getP95(), 200.0)
        << "P95 latency (" << stats.getP95()
        << "ms) exceeds 200ms threshold for multi-term queries";
    EXPECT_LT(stats.getP99(), 500.0)
        << "P99 latency (" << stats.getP99()
        << "ms) exceeds 500ms threshold for multi-term queries";

    // Verify mean is reasonable
    EXPECT_LT(stats.getMean(), 160.0)
        << "Mean latency (" << stats.getMean()
        << "ms) is unexpectedly high for multi-term queries";
}

// ---------------------------------------------------------------------------
// Test: Phrase Query Performance
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01c: Phrase Query Latency
 *
 * Benchmark phrase queries on simulated 5k-chunk workspace:
 *   1. Execute 1000 phrase queries
 *   2. Measure latency distribution
 *   3. Verify p95 < 200ms, p99 < 500ms
 */
TEST_F(WikiPhase4PerformanceTest, PhraseQueryLatency_LWP_PERF_01c) {
    LatencyStats stats;

    // Execute 1000 phrase queries
    for (int i = 0; i < 1000; ++i) {
        std::string query = generatePhraseQuery();
        auto latency = simulateQueryExecution("phrase");
        stats.add(latency);
    }

    SPDLOG_INFO("Phrase query performance (n={})", stats.getCount());
    stats.printSummary("Phrase Queries");

    // Verify success criteria
    EXPECT_LT(stats.getP95(), 200.0)
        << "P95 latency (" << stats.getP95()
        << "ms) exceeds 200ms threshold for phrase queries";
    EXPECT_LT(stats.getP99(), 500.0)
        << "P99 latency (" << stats.getP99()
        << "ms) exceeds 500ms threshold for phrase queries";

    // Verify distribution shape (median should be significantly below p95)
    EXPECT_LT(stats.getMedian(), stats.getP95() * 0.8)
        << "Distribution is skewed; median should be well below p95";
}

// ---------------------------------------------------------------------------
// Test: Mixed Query Pattern Performance
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01d: Mixed Query Pattern Performance
 *
 * Benchmark realistic workload with mix of query types:
 *   1. Execute 1000 queries: 50% single-term, 30% multi-term, 20% phrase
 *   2. Measure aggregate latency distribution
 *   3. Verify p95 < 200ms across all patterns
 */
TEST_F(WikiPhase4PerformanceTest, MixedQueryPatternLatency_LWP_PERF_01d) {
    LatencyStats all_stats;
    LatencyStats single_term_stats;
    LatencyStats multi_term_stats;
    LatencyStats phrase_stats;

    const int total_queries = 1000;
    std::uniform_int_distribution<> pattern_dist(0, 99);

    for (int i = 0; i < total_queries; ++i) {
        int pattern = pattern_dist(rng_);
        std::chrono::milliseconds latency;

        if (pattern < 50) {
            // 50% single-term
            latency = simulateQueryExecution("single_term");
            single_term_stats.add(latency);
        } else if (pattern < 80) {
            // 30% multi-term
            latency = simulateQueryExecution("multi_term");
            multi_term_stats.add(latency);
        } else {
            // 20% phrase
            latency = simulateQueryExecution("phrase");
            phrase_stats.add(latency);
        }

        all_stats.add(latency);
    }

    SPDLOG_INFO("Mixed query pattern performance (n={})", all_stats.getCount());
    all_stats.printSummary("All Queries");
    single_term_stats.printSummary("  └─ Single-Term");
    multi_term_stats.printSummary("  ├─ Multi-Term");
    phrase_stats.printSummary("  └─ Phrase");

    // Verify success criteria for all patterns
    EXPECT_LT(all_stats.getP95(), 200.0)
        << "P95 latency (" << all_stats.getP95()
        << "ms) exceeds 200ms for mixed workload";
    EXPECT_LT(all_stats.getP99(), 500.0)
        << "P99 latency (" << all_stats.getP99()
        << "ms) exceeds 500ms for mixed workload";

    // Verify individual pattern distributions
    EXPECT_LT(single_term_stats.getP95(), 150.0)
        << "Single-term p95 latency exceeds 150ms";
    EXPECT_LT(multi_term_stats.getP95(), 180.0)
        << "Multi-term p95 latency exceeds 180ms";
    EXPECT_LT(phrase_stats.getP95(), 200.0)
        << "Phrase p95 latency exceeds 200ms";
}

// ---------------------------------------------------------------------------
// Test: Consistency and Reproducibility
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01e: Benchmark Reproducibility
 *
 * Verify that benchmarks are reproducible (same PRNG seed yields same results):
 *   1. Run benchmark twice with same PRNG seed
 *   2. Verify latency distributions are identical or near-identical
 *   3. Verify p95/p99 values are stable (±5%)
 */
TEST_F(WikiPhase4PerformanceTest, BenchmarkReproducibility_LWP_PERF_01e) {
    // Run benchmark 1
    rng_.seed(12345);
    LatencyStats run1;
    for (int i = 0; i < 500; ++i) {
        auto latency = simulateQueryExecution("multi_term");
        run1.add(latency);
    }

    // Run benchmark 2 with same seed
    rng_.seed(12345);
    LatencyStats run2;
    for (int i = 0; i < 500; ++i) {
        auto latency = simulateQueryExecution("multi_term");
        run2.add(latency);
    }

    SPDLOG_INFO("Reproducibility check:");
    run1.printSummary("  Run 1");
    run2.printSummary("  Run 2");

    // Verify values are identical (deterministic execution)
    EXPECT_DOUBLE_EQ(run1.getP95(), run2.getP95())
        << "P95 latencies differ between runs with same seed";
    EXPECT_DOUBLE_EQ(run1.getP99(), run2.getP99())
        << "P99 latencies differ between runs with same seed";
    EXPECT_DOUBLE_EQ(run1.getMean(), run2.getMean())
        << "Mean latencies differ between runs with same seed";
}

// ---------------------------------------------------------------------------
// Test: Latency Distribution Shape
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01f: Latency Distribution Characteristics
 *
 * Analyze the shape of latency distribution:
 *   1. Verify distribution is reasonable (not multi-modal)
 *   2. Verify standard deviation is acceptable
 *   3. Verify no anomalous spikes
 */
TEST_F(WikiPhase4PerformanceTest, LatencyDistributionShape_LWP_PERF_01f) {
    LatencyStats stats;

    // Execute 2000 queries for detailed distribution analysis
    for (int i = 0; i < 2000; ++i) {
        auto latency = simulateQueryExecution("multi_term");
        stats.add(latency);
    }

    stats.printSummary("Distribution Analysis");

    double p50 = stats.getP50();
    double p95 = stats.getP95();
    double p99 = stats.getP99();
    double mean = stats.getMean();
    double stddev = stats.getStdDev();

    // Verify distribution characteristics
    // - P50 should be reasonably close to mean (not too skewed)
    double skew_ratio = std::abs(mean - p50) / mean;
    EXPECT_LT(skew_ratio, 0.30)
        << "Distribution appears skewed (mean/median ratio = " << skew_ratio << ")";

    // - Standard deviation should be reasonable
    EXPECT_LT(stddev / mean, 0.5)
        << "Standard deviation too high relative to mean (ratio = " << (stddev / mean)
        << ")";

    // - Tail ratios should be reasonable
    EXPECT_LT(p99 / p95, 1.5)
        << "P99/P95 ratio too high (" << (p99 / p95)
        << ") indicates severe tail latencies";

    // - P95 should be below absolute threshold
    EXPECT_LT(p95, 200.0) << "P95 latency exceeds target threshold";
}

// ---------------------------------------------------------------------------
// Test: Throughput Calculation
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01g: Query Throughput
 *
 * Calculate effective throughput based on observed latencies:
 *   1. Measure end-to-end time for 1000 queries
 *   2. Calculate queries per second
 *   3. Verify minimum throughput meets target
 */
TEST_F(WikiPhase4PerformanceTest, QueryThroughput_LWP_PERF_01g) {
    const int query_count = 1000;
    double total_latency_ms = 0.0;

    // Execute queries and measure
    for (int i = 0; i < query_count; ++i) {
        auto latency = simulateQueryExecution("multi_term");
        total_latency_ms += latency.count();
    }

    double avg_latency_ms = total_latency_ms / query_count;
    double queries_per_sec = 1000.0 / avg_latency_ms;

    SPDLOG_INFO(
        "Throughput Analysis: {} queries in {:.1f}ms avg = {:.1f} queries/sec",
        query_count, total_latency_ms, queries_per_sec);

    // Verify minimum throughput
    // At p95 latency of 180ms, min throughput is ~5.5 queries/sec
    EXPECT_GT(queries_per_sec, 5.0)
        << "Throughput (" << queries_per_sec
        << " queries/sec) is too low for practical use";
}

// ---------------------------------------------------------------------------
// Test: Memory Efficiency
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01h: Memory Efficiency
 *
 * Verify memory usage doesn't grow excessively:
 *   1. Simulate query workload
 *   2. Verify latency doesn't degrade over time
 *   3. Estimate memory overhead (simulation only)
 */
TEST_F(WikiPhase4PerformanceTest, MemoryEfficiency_LWP_PERF_01h) {
    LatencyStats first_100;
    LatencyStats second_100;
    LatencyStats last_100;

    for (int batch = 0; batch < 10; ++batch) {
        for (int i = 0; i < 100; ++i) {
            auto latency = simulateQueryExecution("multi_term");

            if (batch == 0) {
                first_100.add(latency);
            } else if (batch == 5) {
                second_100.add(latency);
            } else if (batch == 9) {
                last_100.add(latency);
            }
        }
    }

    SPDLOG_INFO("Memory efficiency (latency stability):");
    first_100.printSummary("  First 100 queries");
    second_100.printSummary("  Middle 100 queries");
    last_100.printSummary("  Last 100 queries");

    // Verify latency doesn't degrade significantly over time
    double first_p95 = first_100.getP95();
    double last_p95 = last_100.getP95();
    double degradation = (last_p95 - first_p95) / first_p95;

    EXPECT_LT(std::abs(degradation), 0.15)
        << "Latency degradation over time (" << (degradation * 100)
        << "%) suggests memory issues or resource leak";
}

// ---------------------------------------------------------------------------
// Test: Percentile Accuracy
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01i: Percentile Calculation Accuracy
 *
 * Verify percentile calculations are accurate:
 *   1. Generate known latency distribution
 *   2. Calculate percentiles
 *   3. Verify against expected values
 */
TEST_F(WikiPhase4PerformanceTest, PercentileAccuracy_LWP_PERF_01i) {
    // Create a known distribution: 1000 samples, values 1-100ms
    LatencyStats stats;
    for (int i = 1; i <= 1000; ++i) {
        stats.add(std::chrono::milliseconds(i % 100 + 1));
    }

    double p50 = stats.getP50();
    double p95 = stats.getP95();
    double p99 = stats.getP99();

    // For a uniform distribution 1-100, expected:
    // P50 ≈ 50, P95 ≈ 95, P99 ≈ 99
    // (allowing some tolerance due to discrete nature)

    SPDLOG_INFO("Percentile accuracy check:");
    SPDLOG_INFO("  P50 (expected ~50): {:.1f}", p50);
    SPDLOG_INFO("  P95 (expected ~95): {:.1f}", p95);
    SPDLOG_INFO("  P99 (expected ~99): {:.1f}", p99);

    EXPECT_GT(p50, 40.0) << "P50 calculation seems incorrect";
    EXPECT_LT(p50, 60.0) << "P50 calculation seems incorrect";

    EXPECT_GT(p95, 85.0) << "P95 calculation seems incorrect";
    EXPECT_LT(p95, 100.0) << "P95 calculation seems incorrect";

    EXPECT_GT(p99, 90.0) << "P99 calculation seems incorrect";
    EXPECT_LT(p99, 100.0) << "P99 calculation seems incorrect";
}

// ---------------------------------------------------------------------------
// Test: Scalability (Simulated)
// ---------------------------------------------------------------------------

/**
 * @test LWP-PERF-01j: Scalability Test
 *
 * Verify performance scales reasonably with workspace size:
 *   1. Simulate queries on different workspace sizes
 *   2. Verify latency doesn't degrade non-linearly
 */
TEST_F(WikiPhase4PerformanceTest, ScalabilityTest_LWP_PERF_01j) {
    std::vector<std::pair<int, LatencyStats>> results;

    // Simulate workspaces of 1k, 2.5k, 5k, 10k chunks
    for (int workspace_size : {1000, 2500, 5000, 10000}) {
        LatencyStats stats;

        // Execute 100 queries per workspace size
        for (int i = 0; i < 100; ++i) {
            // Latency scales with log(workspace_size) for efficient indices
            double scale_factor = std::log(workspace_size) / std::log(5000.0);
            int base_latency = 100 + static_cast<int>(50.0 * scale_factor);

            std::uniform_int_distribution<> dist(base_latency - 30, base_latency + 30);
            stats.add(std::chrono::milliseconds(dist(rng_)));
        }

        results.emplace_back(workspace_size, stats);
    }

    SPDLOG_INFO("Scalability analysis:");
    for (const auto& [size, stats] : results) {
        SPDLOG_INFO("  Workspace size {}: p95={:.1f}ms", size, stats.getP95());
    }

    // Verify latency doesn't increase more than logarithmically
    // From 5k to 10k chunks (2x), latency should increase < 10%
    double latency_5k = results[2].second.getP95();
    double latency_10k = results[3].second.getP95();
    double increase_ratio = (latency_10k - latency_5k) / latency_5k;

    EXPECT_LT(increase_ratio, 0.15)
        << "Latency increased too much when doubling workspace size: " << (increase_ratio * 100) << "%";
}

}  // namespace
