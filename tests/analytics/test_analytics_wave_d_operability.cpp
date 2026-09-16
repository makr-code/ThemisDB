/**
 * @file test_analytics_wave_d_operability.cpp
 * @brief Focused Wave-D analytics operability tests for fail-closed diagnostics,
 *        correlation identifiers, stress coverage, and soak-style stability.
 */

#include <gtest/gtest.h>

#include "analytics/analytics_export.h"
#include "analytics/distributed_analytics.h"
#include "analytics/ml_serving.h"
#include "analytics/streaming_window.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::analytics;
using namespace themisdb::analytics;

namespace {

ArrowRecordBatch makeWaveDBatch() {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({int64_t(1), std::string("alpha")});
    batch.appendRow({int64_t(2), std::string("beta")});
    return batch;
}

bool hasHintContaining(const std::vector<std::string>& hints, const std::string& token) {
    return std::any_of(hints.begin(), hints.end(), [&](const std::string& hint) {
        return hint.find(token) != std::string::npos;
    });
}

class SlowExporter final : public IAnalyticsExporter {
public:
    using IAnalyticsExporter::exportToFile;

    explicit SlowExporter(std::chrono::milliseconds delay) : delay_(delay) {}

    ExportResult exportToFile(const ArrowRecordBatch& batch,
                              const std::string&,
                              const ExportOptions& options) override {
        entered_.store(true, std::memory_order_release);
        std::this_thread::sleep_for(delay_);
        ExportResult result;
        result.status = ExportStatus::SUCCESS;
        result.rows_exported = batch.rowCount();
        result.bytes_written = options.batch_size;
        return result;
    }

    std::string exportToString(const ArrowRecordBatch&, const ExportOptions&) override {
        return {};
    }

    ExportResult exportWithCallback(const ArrowRecordBatch& batch,
                                    std::function<void(const std::vector<uint8_t>&)>,
                                    const ExportOptions& options) override {
        return exportToFile(batch, {}, options);
    }

    bool supportsFormat(ExportFormat) const override { return true; }
    std::string getExporterInfo() const override { return "SlowExporter"; }

    bool entered() const { return entered_.load(std::memory_order_acquire); }

private:
    std::chrono::milliseconds delay_;
    std::atomic<bool> entered_{false};
};

class AlwaysFailingExecutor final : public ShardQueryExecutor {
public:
    themis::analytics::OLAPResult execute(const std::string&,
                                          const themis::analytics::OLAPQuery&) override {
        throw std::runtime_error("forced shard failure");
    }
    bool isHealthy() const override { return true; }
};

class AlternatingExecutor final : public ShardQueryExecutor {
public:
    explicit AlternatingExecutor(bool fail) : fail_(fail) {}

    themis::analytics::OLAPResult execute(const std::string&,
                                          const themis::analytics::OLAPQuery&) override {
        if (fail_) {
            throw std::runtime_error("transient timeout");
        }
        themis::analytics::OLAPResult result;
        result.rows.push_back({});
        return result;
    }

    bool isHealthy() const override { return true; }

private:
    bool fail_;
};

StreamRecord makeHighCardinalityRecord(std::size_t index) {
    StreamRecord record;
    record.record_id = "record_" + std::to_string(index);
    record.event_time = std::chrono::system_clock::time_point{} + std::chrono::milliseconds(1);
    record.ingest_time = record.event_time;
    record.partition_key = "tenant_" + std::to_string(index);
    record.set("value", static_cast<double>(index % 128));
    return record;
}

} // namespace

TEST(AnalyticsWaveDOperability, WDO01_ExportFailureContainsCorrelationMetadata) {
    auto exporter = ExporterFactory::createDefaultExporter();
    ASSERT_NE(exporter, nullptr);

    const auto batch = makeWaveDBatch();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    auto result = exporter->exportToFile(batch, "/dev/null/not-writable/export.csv", options);

    EXPECT_EQ(result.status, ExportStatus::FAILED);
    EXPECT_EQ(result.failure_class, "io_failure");
    EXPECT_FALSE(result.operation_id.empty());
    EXPECT_EQ(result.correlation_id, result.operation_id);
    EXPECT_TRUE(hasHintContaining(result.operator_hints, "Correlation ID"));
    EXPECT_TRUE(hasHintContaining(result.operator_hints, "writable"));
}

TEST(AnalyticsWaveDOperability, WDO02_ExportPolicyRejectedContainsOperatorHints) {
    SlowExporter exporter(std::chrono::milliseconds(75));
    const auto batch = makeWaveDBatch();
    ExportOptions options;
    BoundedExecutionPolicy policy;
    policy.max_concurrent_requests = 1;

    auto first = std::async(std::launch::async, [&] {
        return exporter.exportToFile(batch, "/tmp/wdo2-a.csv", options, policy);
    });

    for (int i = 0; i < 50 && !exporter.entered(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_TRUE(exporter.entered());

    auto rejected = exporter.exportToFile(batch, "/tmp/wdo2-b.csv", options, policy);
    auto first_result = first.get();

    EXPECT_EQ(rejected.status, ExportStatus::POLICY_REJECTED);
    EXPECT_EQ(rejected.failure_class, "policy_rejected");
    EXPECT_FALSE(rejected.operation_id.empty());
    EXPECT_TRUE(hasHintContaining(rejected.operator_hints, "bounded execution"));
    EXPECT_TRUE(hasHintContaining(rejected.operator_hints, "Correlation ID"));
    EXPECT_EQ(first_result.status, ExportStatus::SUCCESS);
}

TEST(AnalyticsWaveDOperability, WDO03_ServingInvalidInputContainsCorrelationMetadata) {
    MLServingClient client;
    DataPoint point;

    auto response = client.inferFromDataPoint("wave-d-model", point);

    EXPECT_EQ(response.status, MLServingStatus::INVALID_INPUT);
    EXPECT_EQ(response.failure_class, "input_validation");
    EXPECT_FALSE(response.operation_id.empty());
    EXPECT_EQ(response.correlation_id, response.operation_id);
    EXPECT_TRUE(hasHintContaining(response.operator_hints, "Validate tensor shapes"));
    EXPECT_TRUE(hasHintContaining(response.operator_hints, "Correlation ID"));
}

TEST(AnalyticsWaveDOperability, WDO04_DistributedNoHealthyShardsProducesFailClosedHints) {
    DistributedAnalyticsSharding coordinator;
    themis::analytics::OLAPQuery query;
    query.collection = "orders";
    query.tenant_id = "tenant-a";

    auto result = coordinator.executeDistributed(query);

    EXPECT_EQ(result.total_shards, 0u);
    EXPECT_EQ(result.failure_class, "dependency_unavailable");
    EXPECT_FALSE(result.operation_id.empty());
    EXPECT_EQ(result.correlation_id, result.operation_id);
    EXPECT_TRUE(hasHintContaining(result.operator_hints, "No healthy shards"));
    EXPECT_TRUE(hasHintContaining(result.operator_hints, "Correlation ID"));
}

TEST(AnalyticsWaveDOperability, WDO05_DistributedFailClosedAbortProducesOperatorHints) {
    DistributedAnalyticsSharding::Config config;
    config.allow_partial_results = false;
    config.enable_circuit_breaker = false;

    DistributedAnalyticsSharding coordinator(config);
    coordinator.addShard("broken", std::make_shared<AlwaysFailingExecutor>());

    themis::analytics::OLAPQuery query;
    query.collection = "risk";
    query.tenant_id = "tenant-b";

    auto result = coordinator.executeDistributed(query);

    EXPECT_EQ(result.total_shards, 1u);
    EXPECT_EQ(result.successful_shards, 0u);
    EXPECT_EQ(result.failure_class, "partial_failure");
    EXPECT_TRUE(hasHintContaining(result.operator_hints, "Fail-closed"));
    EXPECT_TRUE(hasHintContaining(result.operator_hints, "allow_partial_results=false"));
}

TEST(AnalyticsWaveDOperability, WDO06_HighCardinalityStressRemainsBounded) {
    TumblingWindowConfig config;
    config.size = std::chrono::seconds(30);
    config.max_distinct_partition_keys = 16;

    auto window = createTumblingWindow(config);
    for (std::size_t i = 0; i < 2048; ++i) {
        window->ingest(makeHighCardinalityRecord(i));
    }

    const auto stats = window->getStats();
    // makeHighCardinalityRecord() emits one unique partition key per record, so
    // after the first 16 admitted keys every subsequent record introduces a new
    // key and is rejected by the distinct-key cap.
    EXPECT_EQ(stats.partition_keys_rejected, 2048u - 16u);
    EXPECT_GE(stats.records_dropped, stats.partition_keys_rejected);
}

TEST(AnalyticsWaveDOperability, WDO07_DistributedSoakStyleLoopPreservesDiagnostics) {
    DistributedAnalyticsSharding::Config config;
    config.allow_partial_results = true;
    config.enable_circuit_breaker = false;
    config.max_failure_rate = 1.0;

    DistributedAnalyticsSharding coordinator(config);
    coordinator.addShard("healthy", std::make_shared<AlternatingExecutor>(false));
    coordinator.addShard("failing", std::make_shared<AlternatingExecutor>(true));

    themis::analytics::OLAPQuery query;
    query.collection = "telemetry";
    query.tenant_id = "tenant-c";

    for (int i = 0; i < 200; ++i) {
        auto result = coordinator.executeDistributed(query);
        ASSERT_EQ(result.total_shards, 2u);
        ASSERT_EQ(result.successful_shards, 1u);
        ASSERT_EQ(result.failure_class, "partial_failure");
        ASSERT_FALSE(result.operation_id.empty());
        // finalizeDistributedResult() adds the generic degraded-result hint on
        // the partial-success path when no fail-closed-specific hint is present.
        ASSERT_TRUE(hasHintContaining(result.operator_hints, "Partial result"));
    }
}
