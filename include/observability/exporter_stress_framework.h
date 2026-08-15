/**
 * @file exporter_stress_framework.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 2 Observability Expansion)
 * @note Score: 0/100 (implementation in progress)
 * @note Status: OpenTelemetry exporter stress testing framework
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cstdint>
#include <functional>

namespace themis {
namespace observability {

/**
 * @brief Result status for a stress test run.
 */
enum class StressTestStatus {
    PASSED,               ///< All checks passed within target thresholds
    FAILED,               ///< One or more checks failed
    DEGRADED,             ///< Performance degraded but system recovered
    TIMEOUT,              ///< Test exceeded maximum duration
    INCOMPLETE,           ///< Test interrupted or not fully executed
};

/**
 * @brief Failure mode to simulate during exporter stress testing.
 */
enum class FailureMode {
    NONE,                 ///< No failures (baseline test)
    BACKEND_TIMEOUT,      ///< Backend response timeout
    BACKEND_UNAVAILABLE,  ///< Backend service completely unavailable
    PARTIAL_PACKET_LOSS,  ///< Simulate network packet loss (1-10%)
    HIGH_LATENCY,         ///< Simulate high network latency (50-500ms)
    BACKEND_SLOW,         ///< Backend responds slowly (>1s)
    QUEUE_EXHAUSTION,     ///< Telemetry queue overflow/backpressure
    MEMORY_PRESSURE,      ///< System under memory pressure
};

/**
 * @brief Stress test configuration parameters.
 */
struct ExporterStressTestConfig {
    /// Target number of metrics per second to generate.
    std::uint32_t metrics_per_second{5000};

    /// Target number of spans per second to generate.
    std::uint32_t spans_per_second{2000};

    /// Number of distinct label sets per metric (cardinality).
    std::uint32_t metric_cardinality{100};

    /// Test duration in seconds.
    std::uint32_t duration_seconds{60};

    /// Failure mode to simulate.
    FailureMode failure_mode{FailureMode::NONE};

    /// P95 latency threshold for metrics export (milliseconds).
    std::uint32_t p95_latency_ms_threshold{10};

    /// P99 latency threshold for metrics export (milliseconds).
    std::uint32_t p99_latency_ms_threshold{50};

    /// Maximum allowed metric loss percentage (0-100).
    double acceptable_metric_loss_percent{0.1};

    /// Maximum allowed span loss percentage (0-100).
    double acceptable_span_loss_percent{1.0};

    /// Whether to validate exporter recovery after backend failure.
    bool validate_recovery{true};

    /// Recovery validation timeout (seconds).
    std::uint32_t recovery_timeout_seconds{30};

    /// Number of concurrent producer threads.
    std::uint32_t concurrent_producers{4};

    /// Whether to measure memory usage.
    bool measure_memory{true};

    /// Memory usage threshold (MB).
    std::uint32_t memory_threshold_mb{512};

    /// Random seed for reproducible test runs.
    std::uint32_t random_seed{42};

    /// Whether to emit debug/trace logging.
    bool verbose{false};
};

/**
 * @brief Single metric or span observation during stress testing.
 */
struct Observation {
    /// Unique observation ID.
    std::uint64_t id;

    /// Timestamp when observation was created.
    std::int64_t created_ns;

    /// Type: "metric" or "span".
    std::string type;

    /// Metric name or span name.
    std::string name;

    /// Metric value (if type == "metric").
    double value{0.0};

    /// Labels/attributes.
    std::map<std::string, std::string> labels;

    /// Whether this observation was successfully exported.
    bool exported{false};

    /// Timestamp when observation was exported (0 if not exported).
    std::int64_t exported_ns{0};

    /// Latency from creation to export in milliseconds (0 if not exported).
    std::int64_t latency_ms{0};
};

/**
 * @brief Metrics collected during a stress test run.
 */
struct ExporterStressTestMetrics {
    /// Total observations generated.
    std::uint64_t total_observations{0};

    /// Total observations successfully exported.
    std::uint64_t successful_observations{0};

    /// Total observations dropped or lost.
    std::uint64_t lost_observations{0};

    /// Percentage of observations lost (0-100).
    double loss_percent{0.0};

    /// Minimum latency observed (milliseconds).
    std::int64_t min_latency_ms{0};

    /// Maximum latency observed (milliseconds).
    std::int64_t max_latency_ms{0};

    /// P50 (median) latency (milliseconds).
    std::int64_t p50_latency_ms{0};

    /// P95 latency (milliseconds).
    std::int64_t p95_latency_ms{0};

    /// P99 latency (milliseconds).
    std::int64_t p99_latency_ms{0};

    /// Average latency (milliseconds).
    double avg_latency_ms{0.0};

    /// Peak memory usage (MB).
    std::uint64_t peak_memory_mb{0};

    /// Number of exporter reconnections during test.
    std::uint32_t exporter_reconnects{0};

    /// Time to recover from failure (seconds).
    std::uint32_t recovery_time_seconds{0};

    /// Whether recovery succeeded within timeout.
    bool recovery_successful{false};

    /// Total test duration (seconds).
    std::uint32_t test_duration_seconds{0};

    /// Timestamp when metrics were collected.
    std::int64_t collected_at_ns{0};
};

/**
 * @brief Result of a single stress test run.
 */
struct ExporterStressTestResult {
    /// Test configuration that was run.
    ExporterStressTestConfig config;

    /// Collected metrics from the test run.
    ExporterStressTestMetrics metrics;

    /// Overall test status.
    StressTestStatus status{StressTestStatus::INCOMPLETE};

    /// Human-readable summary of the result.
    std::string summary;

    /// Detailed failure reason if status != PASSED.
    std::string failure_reason;

    /// List of failed checks with explanations.
    std::vector<std::string> failed_checks;

    /// Recommendations for improvement if degraded/failed.
    std::vector<std::string> recommendations;

    /// Raw observations (may be empty for large test runs).
    std::vector<Observation> sample_observations;
};

/**
 * @brief Mock exporter backend for testing.
 *
 * Simulates a remote exporter backend that can experience various
 * failure modes for testing observability resilience.
 */
class MockExporterBackend {
public:
    /**
     * @brief Construct a mock backend.
     * @param failure_mode Failure mode to simulate.
     */
    explicit MockExporterBackend(FailureMode failure_mode = FailureMode::NONE);

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~MockExporterBackend() = default;

    /**
     * @brief Process an export request.
     *
     * Simulates sending telemetry to the backend. May fail, timeout, or
     * introduce latency based on the configured failure mode.
     *
     * @param observations Observations to export.
     * @return Number of observations successfully accepted (may be partial).
     */
    virtual std::size_t export_observations(
        const std::vector<Observation>& observations) = 0;

    /**
     * @brief Check backend health status.
     * @return true if backend is reachable and responsive.
     */
    virtual bool isHealthy() = 0;

    /**
     * @brief Reset backend to healthy state.
     * Used for recovery testing after failure simulation.
     */
    virtual void reset() = 0;

    /**
     * @brief Get statistics about backend operations.
     * @return Map with keys: "export_calls", "successful_exports", "failed_exports", etc.
     */
    virtual std::map<std::string, double> getStatistics() = 0;
};

/**
 * @brief OpenTelemetry exporter stress testing framework.
 *
 * The ExporterStressFramework provides utilities for:
 * - Generating high-volume metric and span workloads
 * - Simulating various failure modes (timeouts, latency, packet loss)
 * - Validating exporter recovery and resilience
 * - Measuring latency distribution and loss rates
 * - Comparing performance across different configurations
 *
 * ## Typical Usage
 *
 * ```cpp
 * // Create stress test framework
 * auto framework = std::make_unique<ExporterStressFramework>();
 *
 * // Configure baseline test
 * ExporterStressTestConfig baseline_config;
 * baseline_config.metrics_per_second = 10000;
 * baseline_config.spans_per_second = 5000;
 * baseline_config.duration_seconds = 60;
 * baseline_config.failure_mode = FailureMode::NONE;
 *
 * // Run baseline
 * auto baseline_result = framework->runStressTest(baseline_config);
 * logger.info("Baseline: {} metrics/sec, p95={}ms",
 *     baseline_result.metrics.total_observations / 60,
 *     baseline_result.metrics.p95_latency_ms);
 *
 * // Configure failure scenario
 * ExporterStressTestConfig failure_config = baseline_config;
 * failure_config.failure_mode = FailureMode::BACKEND_TIMEOUT;
 *
 * // Run failure test
 * auto failure_result = framework->runStressTest(failure_config);
 * if (failure_result.status != StressTestStatus::PASSED) {
 *     logger.warn("Failure scenario: {}", failure_result.failure_reason);
 *     for (const auto& rec : failure_result.recommendations) {
 *         logger.info("  -> {}", rec);
 *     }
 * }
 * ```
 *
 * ## Registered Test Gates (Phase 5 Performance & Hardening)
 *
 * - OEX-01: Baseline throughput (≥5M metrics/sec, ≥2k spans/sec)
 * - OEX-02: P95 latency under normal load (≤10ms)
 * - OEX-03: P99 latency under normal load (≤50ms)
 * - OEX-04: Graceful recovery after backend timeout
 * - OEX-05: Bounded memory usage under sustained load (≤512MB)
 * - OEX-06: Acceptable loss during degraded mode (<0.1% metrics, <1% spans)
 *
 * ## Error Codes (Observability Phase 2 Extension)
 *
 * - No new error codes for stress framework (uses existing Observability codes)
 */
class ExporterStressFramework {
public:
    /**
     * @brief Construct a stress testing framework.
     */
    ExporterStressFramework() = default;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~ExporterStressFramework() = default;

    /**
     * @brief Run a single stress test with the given configuration.
     *
     * @param config Test configuration parameters.
     * @return Result with metrics and status.
     *
     * @note This is a blocking call. Test duration depends on config.duration_seconds.
     */
    virtual ExporterStressTestResult runStressTest(
        const ExporterStressTestConfig& config) = 0;

    /**
     * @brief Run a series of stress tests with different configurations.
     *
     * @param configs Vector of test configurations to run sequentially.
     * @return Vector of test results (same order as configs).
     *
     * @note This is a blocking call. Total duration is sum of all test durations.
     */
    virtual std::vector<ExporterStressTestResult> runMultipleTests(
        const std::vector<ExporterStressTestConfig>& configs) = 0;

    /**
     * @brief Run a comprehensive benchmark suite covering registered gates.
     *
     * Runs tests for gates OEX-01 through OEX-06 and returns results.
     *
     * @return Vector of 6 test results (one per gate).
     */
    virtual std::vector<ExporterStressTestResult> runGateBenchmarks() = 0;

    /**
     * @brief Compare two test results and generate a comparison report.
     *
     * @param baseline Baseline test result.
     * @param candidate Candidate test result.
     * @return Comparison report with performance deltas and recommendations.
     */
    virtual std::string generateComparisonReport(
        const ExporterStressTestResult& baseline,
        const ExporterStressTestResult& candidate) = 0;

    /**
     * @brief Register a callback for test progress updates.
     *
     * @param callback Function called with progress percentage (0-100) during test execution.
     */
    virtual void setProgressCallback(
        std::function<void(std::uint32_t progress_percent)> callback) = 0;

    /**
     * @brief Cancel any currently-running test.
     *
     * @return true if a test was cancelled, false if no test was running.
     */
    virtual bool cancelTest() = 0;

    /**
     * @brief Get the number of registered release gate benchmarks.
     * @return Number of gates (typically 6 for OEX-01..06).
     */
    virtual std::size_t getRegisteredGateCount() const = 0;

    /**
     * @brief Get the name and description of a registered gate benchmark.
     *
     * @param gate_index Zero-based index (0-5 for gates OEX-01..06).
     * @return Gate name (e.g., "OEX-01") or empty string if index invalid.
     */
    virtual std::string getGateName(std::size_t gate_index) const = 0;
};

} // namespace observability
} // namespace themis
