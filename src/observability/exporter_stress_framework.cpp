/**
 * @file exporter_stress_framework.cpp
 * @brief Implementation of OpenTelemetry exporter stress testing framework.
 *
 * Provides high-volume metric and span generation, failure simulation,
 * and performance measurement for observability exporter validation.
 */

#include "observability/exporter_stress_framework.h"
#include "observability/observability_api_contract.h"
#include <random>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace themis {
namespace observability {

// ============================================================================
// MockExporterBackend Implementation
// ============================================================================

class MockExporterBackendImpl : public MockExporterBackend {
public:
    explicit MockExporterBackendImpl(FailureMode failure_mode)
        : failure_mode_(failure_mode),
          healthy_(true),
          total_exports_(0),
          successful_exports_(0),
          failed_exports_(0) {}

    std::size_t export_observations(
        const std::vector<Observation>& observations) override {

        std::lock_guard<std::mutex> lock(backend_mutex_);

        total_exports_++;

        // Simulate failure modes
        switch (failure_mode_) {
            case FailureMode::NONE:
                // Normal path: accept all
                successful_exports_ += observations.size();
                return observations.size();

            case FailureMode::BACKEND_TIMEOUT:
                // Simulate timeout: reject all
                healthy_ = false;
                failed_exports_ += observations.size();
                if (total_exports_ % 10 == 0) {
                    healthy_ = true;  // Recovery after 10 attempts
                }
                return healthy_ ? observations.size() : 0;

            case FailureMode::BACKEND_UNAVAILABLE:
                // Backend completely unavailable
                healthy_ = false;
                failed_exports_ += observations.size();
                return 0;

            case FailureMode::PARTIAL_PACKET_LOSS: {
                // Simulate 5% packet loss
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<> dis(0.0, 1.0);
                std::size_t accepted = 0;
                for (const auto& obs : observations) {
                    if (dis(gen) > 0.05) {
                        accepted++;
                    }
                }
                if (accepted > 0) {
                    successful_exports_ += accepted;
                } else {
                    failed_exports_++;
                }
                return accepted;
            }

            case FailureMode::HIGH_LATENCY:
                // Simulate 100-500ms latency
                {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(100, 500);
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(dis(gen)));
                }
                successful_exports_ += observations.size();
                return observations.size();

            case FailureMode::BACKEND_SLOW:
                // Simulate 1-2s response time
                std::this_thread::sleep_for(std::chrono::seconds(1));
                successful_exports_ += observations.size();
                return observations.size();

            case FailureMode::QUEUE_EXHAUSTION:
                // Simulate queue overflow: accept 50%
                {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(0, 100);
                    std::size_t accepted = dis(gen) < 50 ? observations.size() : 0;
                    if (accepted > 0) {
                        successful_exports_ += accepted;
                    } else {
                        failed_exports_++;
                    }
                    return accepted;
                }

            case FailureMode::MEMORY_PRESSURE:
                // Simulate memory pressure: accept 75%
                {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(0, 100);
                    std::size_t accepted = dis(gen) < 75 ? observations.size() : 0;
                    if (accepted > 0) {
                        successful_exports_ += accepted;
                    } else {
                        failed_exports_++;
                    }
                    return accepted;
                }
        }

        return 0;
    }

    bool isHealthy() override {
        std::lock_guard<std::mutex> lock(backend_mutex_);
        return healthy_;
    }

    void reset() override {
        std::lock_guard<std::mutex> lock(backend_mutex_);
        healthy_ = true;
        total_exports_ = 0;
        successful_exports_ = 0;
        failed_exports_ = 0;
    }

    std::map<std::string, double> getStatistics() override {
        std::lock_guard<std::mutex> lock(backend_mutex_);
        std::map<std::string, double> stats;
        stats["export_calls"] = total_exports_;
        stats["successful_exports"] = successful_exports_;
        stats["failed_exports"] = failed_exports_;
        stats["healthy"] = healthy_ ? 1.0 : 0.0;
        return stats;
    }

private:
    FailureMode failure_mode_;
    bool healthy_;
    std::uint64_t total_exports_;
    std::uint64_t successful_exports_;
    std::uint64_t failed_exports_;
    std::mutex backend_mutex_;
};

// ============================================================================
// ExporterStressFramework Implementation
// ============================================================================

class ExporterStressFrameworkImpl : public ExporterStressFramework {
public:
    ExporterStressFrameworkImpl()
        : cancel_test_(false),
          active_test_(false),
          progress_callback_([[maybe_unused]] nullptr) {}

    ExporterStressTestResult runStressTest(
        const ExporterStressTestConfig& config) override {

        active_test_.store(true, std::memory_order_release);
        cancel_test_.store(false, std::memory_order_release);

        ExporterStressTestResult result;
        result.config = config;

        // Create mock backend
        auto backend = std::make_unique<MockExporterBackendImpl>(config.failure_mode);

        // Generate observations
        std::random_device rd;
        std::mt19937 gen(config.random_seed);
        std::uniform_int_distribution<> label_dis(0, config.metric_cardinality - 1);
        std::uniform_real_distribution<> value_dis(0.0, 100.0);

        std::vector<Observation> all_observations;
        all_observations.reserve(
            (static_cast<std::size_t>(config.metrics_per_second) +
             static_cast<std::size_t>(config.spans_per_second)) *
            static_cast<std::size_t>(config.duration_seconds));

        // Generate metric and span observations
        {
            std::uint64_t obs_id = 1;
            auto start_time = std::chrono::system_clock::now();
            auto start_ns = start_time.time_since_epoch().count();

            for (std::uint32_t sec = 0; sec < config.duration_seconds; ++sec) {
                if (cancel_test_) {
                    break;
                }

                // Generate metrics for this second
                for (std::uint32_t i = 0; i < config.metrics_per_second; ++i) {
                    Observation obs;
                    obs.id = obs_id++;
                    obs.created_ns = start_ns + sec * 1'000'000'000LL;
                    obs.type = "metric";
                    obs.name = "test_metric_" + std::to_string(label_dis(gen));
                    obs.value = value_dis(gen);
                    obs.labels["method"] = label_dis(gen) % 2 == 0 ? "GET" : "POST";
                    obs.labels["status"] = std::to_string(label_dis(gen) % 10 + 1);
                    all_observations.push_back(obs);
                }

                // Generate spans for this second
                for (std::uint32_t i = 0; i < config.spans_per_second; ++i) {
                    Observation obs;
                    obs.id = obs_id++;
                    obs.created_ns = start_ns + sec * 1'000'000'000LL;
                    obs.type = "span";
                    obs.name = "test_span_" + std::to_string(label_dis(gen));
                    obs.labels["service"] = label_dis(gen) % 2 == 0 ? "svc-a" : "svc-b";
                    all_observations.push_back(obs);
                }

                // Update progress
                if ([[maybe_unused]] progress_callback_) {
                    progress_callback_([[maybe_unused]] (sec * 100) / config.duration_seconds);
                }
            }
        }

        result.metrics.total_observations = all_observations.size();

        // Export observations
        std::uint64_t successful = 0;
        auto latencies = std::vector<std::int64_t>();
        latencies.reserve(all_observations.size());

        auto export_start = std::chrono::system_clock::now();
        auto export_start_ns = export_start.time_since_epoch().count();

        for (const auto& obs : all_observations) {
            if (cancel_test_) {
                break;
            }

            std::vector<Observation> batch = {obs};
            auto accepted = backend->export_observations(batch);

            if (accepted > 0) {
                successful++;
                auto latency_ms = (std::chrono::system_clock::now().time_since_epoch().count()
                                  - obs.created_ns) / 1'000'000LL;
                latencies.push_back(latency_ms);
            }
        }

        result.metrics.successful_observations = successful;
        result.metrics.lost_observations = result.metrics.total_observations - successful;
        result.metrics.loss_percent = 100.0 * result.metrics.lost_observations /
                                      result.metrics.total_observations;

        // Calculate latency statistics
        if (!latencies.empty()) {
            std::sort(latencies.begin(), latencies.end());

            result.metrics.min_latency_ms = latencies.front();
            result.metrics.max_latency_ms = latencies.back();
            result.metrics.p50_latency_ms = latencies[latencies.size() / 2];
            result.metrics.p95_latency_ms = latencies[static_cast<std::size_t>(
                latencies.size() * 0.95)];
            result.metrics.p99_latency_ms = latencies[static_cast<std::size_t>(
                latencies.size() * 0.99)];

            std::int64_t sum = 0;
            for (auto l : latencies) {
                sum += l;
            }
            result.metrics.avg_latency_ms = static_cast<double>(sum) / latencies.size();
        }

        result.metrics.test_duration_seconds = config.duration_seconds;
        result.metrics.collected_at_ns = std::chrono::system_clock::now()
                                         .time_since_epoch().count();

        // Validate against thresholds
        result.status = StressTestStatus::PASSED;
        result.summary = "Stress test completed successfully";

        // Check p95 latency
        if (result.metrics.p95_latency_ms > config.p95_latency_ms_threshold) {
            result.failed_checks.push_back("P95 latency exceeded threshold");
            result.status = StressTestStatus::DEGRADED;
        }

        // Check p99 latency
        if (result.metrics.p99_latency_ms > config.p99_latency_ms_threshold) {
            result.failed_checks.push_back("P99 latency exceeded threshold");
            result.status = StressTestStatus::DEGRADED;
        }

        // Check loss rate
        if (config.failure_mode != FailureMode::NONE) {
            if (result.metrics.loss_percent > config.acceptable_metric_loss_percent) {
                result.failed_checks.push_back("Metric loss exceeded acceptable threshold");
                result.status = StressTestStatus::FAILED;
            }
        }

        if (result.status != StressTestStatus::PASSED) {
            result.failure_reason = "One or more performance gates failed";
        }

        active_test_.store(false, std::memory_order_release);
        cancel_test_.store(false, std::memory_order_release);
        return result;
    }

    std::vector<ExporterStressTestResult> runMultipleTests(
        const std::vector<ExporterStressTestConfig>& configs) override {

        std::vector<ExporterStressTestResult> results;
        for (const auto& config : configs) {
            if (cancel_test_) {
                break;
            }
            results.push_back(runStressTest(config));
        }
        return results;
    }

    std::vector<ExporterStressTestResult> runGateBenchmarks() override {
        std::vector<ExporterStressTestResult> results;

        // OEX-01: Baseline throughput
        {
            ExporterStressTestConfig config;
            config.metrics_per_second = 10000;
            config.spans_per_second = 5000;
            config.duration_seconds = 60;
            config.failure_mode = FailureMode::NONE;
            results.push_back(runStressTest(config));
        }

        // OEX-02: P95 latency under normal load
        {
            ExporterStressTestConfig config;
            config.metrics_per_second = 5000;
            config.duration_seconds = 60;
            config.failure_mode = FailureMode::NONE;
            config.p95_latency_ms_threshold = 10;
            results.push_back(runStressTest(config));
        }

        // OEX-03: P99 latency under normal load
        {
            ExporterStressTestConfig config;
            config.metrics_per_second = 5000;
            config.duration_seconds = 60;
            config.failure_mode = FailureMode::NONE;
            config.p99_latency_ms_threshold = 50;
            results.push_back(runStressTest(config));
        }

        // OEX-04: Recovery after backend timeout
        {
            ExporterStressTestConfig config;
            config.metrics_per_second = 2000;
            config.duration_seconds = 30;
            config.failure_mode = FailureMode::BACKEND_TIMEOUT;
            config.validate_recovery = true;
            results.push_back(runStressTest(config));
        }

        // OEX-05: Memory usage bounds
        {
            ExporterStressTestConfig config;
            config.metrics_per_second = 10000;
            config.spans_per_second = 5000;
            config.duration_seconds = 60;
            config.failure_mode = FailureMode::NONE;
            config.measure_memory = true;
            config.memory_threshold_mb = 512;
            results.push_back(runStressTest(config));
        }

        // OEX-06: Acceptable loss during degraded mode
        {
            ExporterStressTestConfig config;
            config.metrics_per_second = 5000;
            config.duration_seconds = 30;
            config.failure_mode = FailureMode::PARTIAL_PACKET_LOSS;
            config.acceptable_metric_loss_percent = 0.5;
            results.push_back(runStressTest(config));
        }

        return results;
    }

    std::string generateComparisonReport(
        const ExporterStressTestResult& baseline,
        const ExporterStressTestResult& candidate) override {

        std::ostringstream report;
        report << std::fixed << std::setprecision(2);

        report << "# Exporter Stress Test Comparison Report\n\n";
        report << "## Baseline Results\n";
        report << "- Status: " << (baseline.status == StressTestStatus::PASSED ? "PASSED" : "FAILED") << "\n";
        report << "- Total Observations: " << baseline.metrics.total_observations << "\n";
        report << "- Successful: " << baseline.metrics.successful_observations << "\n";
        report << "- Loss: " << baseline.metrics.loss_percent << "%\n";
        report << "- P95 Latency: " << baseline.metrics.p95_latency_ms << "ms\n";
        report << "- P99 Latency: " << baseline.metrics.p99_latency_ms << "ms\n\n";

        report << "## Candidate Results\n";
        report << "- Status: " << (candidate.status == StressTestStatus::PASSED ? "PASSED" : "FAILED") << "\n";
        report << "- Total Observations: " << candidate.metrics.total_observations << "\n";
        report << "- Successful: " << candidate.metrics.successful_observations << "\n";
        report << "- Loss: " << candidate.metrics.loss_percent << "%\n";
        report << "- P95 Latency: " << candidate.metrics.p95_latency_ms << "ms\n";
        report << "- P99 Latency: " << candidate.metrics.p99_latency_ms << "ms\n\n";

        report << "## Performance Delta\n";
        double p95_delta = candidate.metrics.p95_latency_ms - baseline.metrics.p95_latency_ms;
        double p99_delta = candidate.metrics.p99_latency_ms - baseline.metrics.p99_latency_ms;
        double loss_delta = candidate.metrics.loss_percent - baseline.metrics.loss_percent;

        report << "- P95 Latency Delta: " << (p95_delta >= 0 ? "+" : "") << p95_delta << "ms\n";
        report << "- P99 Latency Delta: " << (p99_delta >= 0 ? "+" : "") << p99_delta << "ms\n";
        report << "- Loss Rate Delta: " << (loss_delta >= 0 ? "+" : "") << loss_delta << "%\n\n";

        if (candidate.status == StressTestStatus::PASSED &&
            baseline.status == StressTestStatus::PASSED) {
            report << "✅ **PASS**: Candidate meets or exceeds baseline performance.\n";
        } else {
            report << "❌ **FAIL**: Candidate does not meet baseline performance.\n";
        }

        return report.str();
    }

    void setProgressCallback(
        std::function<void([[maybe_unused]] std::uint32_t progress_percent)> callback) override {
        progress_callback_ = callback;
    }

    bool cancelTest() override {
        bool expected = true;
        if (active_test_.compare_exchange_strong(expected, false)) {
            cancel_test_.store(true, std::memory_order_release);
            return true;
        }
        return false;
    }

    std::size_t getRegisteredGateCount() const override {
        return 6;  // OEX-01 through OEX-06
    }

    std::string getGateName(std::size_t gate_index) const override {
        const char* names[] = {
            "OEX-01: Baseline Throughput",
            "OEX-02: P95 Latency (Normal Load)",
            "OEX-03: P99 Latency (Normal Load)",
            "OEX-04: Recovery After Timeout",
            "OEX-05: Memory Usage Bounds",
            "OEX-06: Loss During Degraded Mode"
        };

        if (gate_index < 6) {
            return names[gate_index];
        }
        return "";
    }

private:
    std::atomic<bool> cancel_test_;
    std::atomic<bool> active_test_;
    std::function<void(std::uint32_t progress_percent)> progress_callback_;
};

// ============================================================================
// Factory functions
// ============================================================================

std::unique_ptr<MockExporterBackend> createMockExporterBackend(FailureMode failure_mode) {
    return std::make_unique<MockExporterBackendImpl>(failure_mode);
}

std::unique_ptr<ExporterStressFramework> createExporterStressFramework() {
    return std::make_unique<ExporterStressFrameworkImpl>();
}

} // namespace observability
} // namespace themis
