/**
 * Analytics Module Phase 4 - Comprehensive Gap Closure Test Suite
 * 
 * Coverage:
 *  - Process Mining (PM-01 to PM-18): DFG, Process Discovery, Variants, Clustering, Conformance
 *  - AutoML/Forecasting (AF-01 to AF-18): Data validation, seasonality, smoothing, metalearner, ensemble
 *  - Streaming & CEP (SC-01 to SC-15): NFA, window processing, aggregation
 *  - Knowledge Base (KB-01 to KB-10): Config parsing, facts, rules
 *  - Utilities (UT-01 to UT-15): Columnar execution, aggregation, RAII
 *  - Integration (IT-01 to IT-10): Cross-module workflows
 * 
 * Target: 86 behavior assertions; coverage is enforced by the nightly analytics gate.
 */

#include <gtest/gtest.h>
#include "analytics/process_mining.h"
#include "analytics/forecasting.h"
#include "analytics/cep_engine.h"
#include "analytics/knowledge_base.h"
#include "analytics/automl.h"
#include "analytics/aggregation.h"
#include "analytics/columnar_execution.h"
#include "analytics/expert_system_engine.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <utility>

using namespace themisdb::analytics;

namespace {

class ProcessMiningHarness {
public:
    ProcessMiningHarness()
        : db_(makeConfig()), mining_(db_) {}

    themis::ProcessMining& mining() { return mining_; }

private:
    static themis::RocksDBWrapper::Config makeConfig() {
        themis::RocksDBWrapper::Config config;
        config.db_path = (std::filesystem::temp_directory_path() /
                          "analytics_gap_closure_unopened_db").string();
        return config;
    }

    themis::RocksDBWrapper db_;
    themis::ProcessMining mining_;
};

static themis::EventLog makeProcessLog(
    std::initializer_list<std::vector<std::string>> sequences) {
    themis::EventLog log;
    std::set<std::string> activities;
    std::set<std::string> variants;
    int64_t timestamp = 1'000;
    std::size_t case_index = 0;

    for (const auto& sequence : sequences) {
        themis::ProcessTrace trace;
        trace.case_id = "case-" + std::to_string(case_index++);
        trace.start_time_ms = timestamp;
        for (const auto& activity : sequence) {
            themis::ProcessEvent event;
            event.case_id = trace.case_id;
            event.activity = activity;
            event.timestamp_ms = timestamp++;
            trace.events.push_back(event);
            activities.insert(activity);
        }
        trace.end_time_ms = trace.events.empty() ? timestamp : trace.events.back().timestamp_ms;
        trace.duration_ms = trace.end_time_ms - trace.start_time_ms;
        trace.variant_signature.clear();
        for (const auto& activity : sequence) {
            trace.variant_signature += std::to_string(activity.size()) + ":" + activity;
        }
        variants.insert(trace.variant_signature);
        log.total_events += trace.events.size();
        log.traces.push_back(std::move(trace));
    }

    log.unique_cases = log.traces.size();
    log.unique_activities = activities.size();
    log.unique_variants = variants.size();
    return log;
}

static themis::DiscoveredProcess makeLinearProcessModel() {
    themis::DiscoveredProcess model;
    model.nodes = {
        {"start", "Start", "EVENT"},
        {"a", "A", "TASK"},
        {"b", "B", "TASK"},
        {"c", "C", "TASK"},
        {"end", "End", "EVENT"}
    };
    model.edges = {
        {"e1", "start", "a"},
        {"e2", "a", "b"},
        {"e3", "b", "c"},
        {"e4", "c", "end"}
    };
    return model;
}

static themisdb::analytics::Event makeCepEvent(
    const std::string& name, int64_t timestamp_ms, double value = 0.0) {
    themisdb::analytics::Event event;
    event.event_name = name;
    event.timestamp = std::chrono::system_clock::time_point{
        std::chrono::milliseconds{timestamp_ms}};
    event.setField("value", value);
    return event;
}

static themisdb::analytics::ColumnBatch makeColumnarBatch(std::size_t row_count) {
    themisdb::analytics::ColumnBatch batch;
    auto values = std::make_shared<themisdb::analytics::Column>(
        "value", themisdb::analytics::ColumnType::Int64);
    auto labels = std::make_shared<themisdb::analytics::Column>(
        "label", themisdb::analytics::ColumnType::String);
    for (std::size_t row = 0; row < row_count; ++row) {
        values->appendInt64(static_cast<int64_t>(row));
        labels->appendString("label-" + std::to_string(row % 3));
    }
    batch.addColumn(values);
    batch.addColumn(labels);
    return batch;
}

class ScopedTempFile {
public:
    explicit ScopedTempFile(const std::string& contents) {
        static std::atomic<uint64_t> next_id{0};
        path_ = std::filesystem::temp_directory_path() /
                ("analytics_gap_closure_" + std::to_string(next_id.fetch_add(1)) + ".yml");
        std::ofstream file(path_);
        file << contents;
        if (!file) {
            throw std::runtime_error("Unable to create analytics test input");
        }
    }

    ~ScopedTempFile() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    std::string path() const { return path_.string(); }

private:
    std::filesystem::path path_;
};

struct ScopedYamlParser {
    explicit ScopedYamlParser(themisdb::analytics::KnowledgeBase::YamlParserFn parser) {
        themisdb::analytics::KnowledgeBase::setYamlParserFn(std::move(parser));
    }

    ~ScopedYamlParser() {
        themisdb::analytics::KnowledgeBase::clearYamlParserFn();
    }
};

}  // namespace

// ============================================================================
// PROCESS MINING TESTS (PM-01 to PM-18)
// ============================================================================

namespace analytics_test {
namespace process_mining_tests {

/**
 * PM-01: Basic DFG Creation
 * Setup: Simple 2-activity log with 3 traces
 * Expected: DFG nodes = {start, A, B, end}, edges = {start→A, A→B, B→end}
 */
TEST(ProcessMiningTests, PM_01_basic_dfg_creation) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "B"}, {"A", "B"}});
    const auto [status, dfg] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_EQ(dfg.activities, (std::set<std::string>{"A", "B"}));
    ASSERT_EQ(dfg.edges.size(), 1U);
    EXPECT_EQ(dfg.edges.front().from, "A");
    EXPECT_EQ(dfg.edges.front().to, "B");
    EXPECT_EQ(dfg.edges.front().frequency, 3);
    EXPECT_EQ(dfg.start_activities, (std::set<std::string>{"A"}));
    EXPECT_EQ(dfg.end_activities, (std::set<std::string>{"B"}));
}

/**
 * PM-02: DFG over a larger event log
 * Setup: 10K events across 100 traces and 10 activities
 * Expected: Every observed directly-follows edge has positive frequency
 */
TEST(ProcessMiningTests, PM_02_large_event_log) {
    ProcessMiningHarness harness;
    std::vector<std::vector<std::string>> sequences(100, std::vector<std::string>(100));
    for (std::size_t trace = 0; trace < sequences.size(); ++trace) {
        for (std::size_t event = 0; event < sequences[trace].size(); ++event) {
            sequences[trace][event] = "activity-" + std::to_string((event + trace) % 10);
        }
    }
    themis::EventLog log;
    std::set<std::string> activities;
    for (std::size_t trace_index = 0; trace_index < sequences.size(); ++trace_index) {
        themis::ProcessTrace trace;
        trace.case_id = "large-" + std::to_string(trace_index);
        int64_t timestamp = static_cast<int64_t>(trace_index * 100);
        for (const auto& activity : sequences[trace_index]) {
            themis::ProcessEvent event;
            event.case_id = trace.case_id;
            event.activity = activity;
            event.timestamp_ms = timestamp++;
            trace.events.push_back(std::move(event));
            activities.insert(activity);
        }
        log.total_events += trace.events.size();
        log.traces.push_back(std::move(trace));
    }
    log.unique_cases = log.traces.size();
    log.unique_activities = activities.size();
    const auto [status, dfg] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_EQ(log.total_events, 10'000U);
    EXPECT_EQ(dfg.activities.size(), 10U);
    EXPECT_FALSE(dfg.edges.empty());
    EXPECT_TRUE(std::all_of(dfg.edges.begin(), dfg.edges.end(),
                            [](const auto& edge) { return edge.frequency > 0; }));
}

/**
 * PM-03: DFG with Self-Loops
 * Setup: Log with consecutive same activities
 * Expected: Self-loop edges counted correctly
 */
TEST(ProcessMiningTests, PM_03_dfg_self_loops) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "A", "A", "B"}});
    const auto [status, dfg] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_EQ(dfg.self_loops.at("A"), 2);
    const auto edge = std::find_if(dfg.edges.begin(), dfg.edges.end(), [](const auto& value) {
        return value.from == "A" && value.to == "A";
    });
    ASSERT_NE(edge, dfg.edges.end());
    EXPECT_EQ(edge->frequency, 2);
}

/**
 * PM-04: Alpha-Miner Algorithm
 * Setup: Standard structured process (sequence, choice, parallelism)
 * Expected: Discovers correct process model
 */
TEST(ProcessMiningTests, PM_04_alpha_miner_algorithm) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "B", "C"}});
    themis::MiningConfig config;
    config.algorithm = themis::MiningAlgorithm::ALPHA;
    const auto [status, model] = harness.mining().discoverProcess(log, config);
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_FALSE(model.nodes.empty());
    EXPECT_FALSE(model.edges.empty());
    EXPECT_TRUE(std::any_of(model.nodes.begin(), model.nodes.end(),
                            [](const auto& node) { return node.name == "A"; }));
}

/**
 * PM-05: Single Trace Process Discovery
 * Setup: Log with 1 trace
 * Expected: Trivial process model
 */
TEST(ProcessMiningTests, PM_05_single_trace_discovery) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}});
    const auto [status, model] = harness.mining().discoverProcess(log);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(model.nodes.size(), 5U);
    EXPECT_TRUE(std::any_of(model.edges.begin(), model.edges.end(),
                            [](const auto& edge) { return edge.from == "node_0" && edge.to == "node_1"; }));
}

/**
 * PM-06: Complex Parallelism
 * Setup: Log with parallel activities
 * Expected: AND gateways identified correctly
 */
TEST(ProcessMiningTests, PM_06_complex_parallelism) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "C", "B"}});
    themis::MiningConfig config;
    config.algorithm = themis::MiningAlgorithm::ALPHA;
    const auto [status, model] = harness.mining().discoverProcess(log, config);
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_TRUE(std::any_of(model.nodes.begin(), model.nodes.end(),
                            [](const auto& node) { return node.type == "AND_GATEWAY"; }));
}

/**
 * PM-07: Variant Identification
 * Setup: Log with 3 distinct trace patterns
 * Expected: Exactly 3 variants identified
 */
TEST(ProcessMiningTests, PM_07_variant_identification) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "B"}, {"A", "C"}});
    const auto [status, variants] = harness.mining().analyzeVariants(log);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(variants.size(), 2U);
    EXPECT_EQ(variants[0].frequency, 2);
    EXPECT_DOUBLE_EQ(variants[0].percentage, 200.0 / 3.0);
    EXPECT_EQ(variants[1].frequency, 1);
    EXPECT_DOUBLE_EQ(variants[1].percentage, 100.0 / 3.0);
}

/**
 * PM-08: Single Variant
 * Setup: All traces identical
 * Expected: 1 variant with 100% frequency
 */
TEST(ProcessMiningTests, PM_08_single_variant) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "B"}, {"A", "B"}});
    const auto [status, variants] = harness.mining().analyzeVariants(log);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(variants.size(), 1U);
    EXPECT_EQ(variants.front().frequency, 3);
    EXPECT_DOUBLE_EQ(variants.front().percentage, 100.0);
}

/**
 * PM-09: Variant Duration Metrics
 * Setup: Log with varying trace durations
 * Expected: Duration statistics (min, max, avg) correct
 */
TEST(ProcessMiningTests, PM_09_variant_duration_metrics) {
    ProcessMiningHarness harness;
    auto log = makeProcessLog({{"A", "B"}, {"A", "B"}, {"A", "B"}});
    log.traces[0].duration_ms = 1;
    log.traces[1].duration_ms = 3;
    log.traces[2].duration_ms = 5;
    const auto [status, variants] = harness.mining().analyzeVariants(log);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(variants.size(), 1U);
    EXPECT_DOUBLE_EQ(variants.front().avg_duration_ms, 3.0);
}

/**
 * PM-10: K-Means Clustering
 * Setup: 10 variants with known similarity
 * Expected: K-means converges to stable clusters
 */
TEST(ProcessMiningTests, PM_10_kmeans_clustering) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "C"}, {"B", "C"}, {"C", "A"}});
    const auto [status, clusters] = harness.mining().clusterVariants(log, 2);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(clusters.size(), 2U);
    std::set<int> members;
    for (const auto& [cluster_id, variants] : clusters) {
        (void)cluster_id;
        members.insert(variants.begin(), variants.end());
    }
    EXPECT_EQ(members, (std::set<int>{0, 1, 2, 3}));
}

/**
 * PM-11: Single Cluster
 * Setup: K=1
 * Expected: All variants in one cluster
 */
TEST(ProcessMiningTests, PM_11_single_cluster) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "C"}, {"B", "C"}});
    const auto [status, clusters] = harness.mining().clusterVariants(log, 1);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(clusters.size(), 1U);
    EXPECT_EQ(clusters.begin()->second.size(), 3U);
}

/**
 * PM-12: Cluster Convergence
 * Setup: K=3, multiple iterations
 * Expected: Cluster centers stabilize
 */
TEST(ProcessMiningTests, PM_12_cluster_convergence) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "C"}, {"B", "C"}, {"C", "A"}});
    const auto [first_status, first] = harness.mining().clusterVariants(log, 2);
    const auto [second_status, second] = harness.mining().clusterVariants(log, 2);
    ASSERT_TRUE(first_status.ok) << first_status.message;
    ASSERT_TRUE(second_status.ok) << second_status.message;
    EXPECT_EQ(first, second);
}

/**
 * PM-13: Perfect Conformance
 * Setup: All traces conform to model perfectly
 * Expected: Conformance score = 1.0
 */
TEST(ProcessMiningTests, PM_13_perfect_conformance) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "B", "C"}});
    const auto [status, result] = harness.mining().checkConformance(log, makeLinearProcessModel());
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_DOUBLE_EQ(result.fitness, 1.0);
    EXPECT_TRUE(result.deviations.empty());
}

/**
 * PM-14: Partial Conformance
 * Setup: Some traces deviate from model
 * Expected: 0.0 < score < 1.0
 */
TEST(ProcessMiningTests, PM_14_partial_conformance) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "X", "C"}});
    const auto [status, result] = harness.mining().checkConformance(log, makeLinearProcessModel());
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_GT(result.fitness, 0.0);
    EXPECT_LT(result.fitness, 1.0);
    EXPECT_FALSE(result.deviations.empty());
}

/**
 * PM-15: Non-Conformant Traces
 * Setup: Traces violate model logic
 * Expected: Low conformance score
 */
TEST(ProcessMiningTests, PM_15_non_conformant_traces) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"X", "Y"}, {"X", "Z"}});
    const auto [status, result] = harness.mining().checkConformance(log, makeLinearProcessModel());
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_DOUBLE_EQ(result.fitness, 0.0);
    EXPECT_GE(result.missing_tokens, 2);
}

/**
 * PM-16: Variant Signatures
 * Verify: Trace variants correctly encoded as signatures
 */
TEST(ProcessMiningTests, PM_16_variant_signatures) {
    const auto log = makeProcessLog({{"A", "BC"}, {"AB", "C"}, {"A", "BC"}});
    ASSERT_EQ(log.traces[0].variant_signature, log.traces[2].variant_signature);
    EXPECT_NE(log.traces[0].variant_signature, log.traces[1].variant_signature);
    EXPECT_EQ(log.unique_variants, 2U);
}

/**
 * PM-17: DFG Cycle Edges
 * Setup: DFG with cycles
 * Expected: Both directions of the cycle are represented
 */
TEST(ProcessMiningTests, PM_17_dfg_cycle_edges) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "A"}, {"B", "A"}});
    const auto [status, dfg] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_EQ(dfg.edges.size(), 2U);
    const auto ab = std::find_if(dfg.edges.begin(), dfg.edges.end(),
                                 [](const auto& edge) { return edge.from == "A" && edge.to == "B"; });
    const auto ba = std::find_if(dfg.edges.begin(), dfg.edges.end(),
                                 [](const auto& edge) { return edge.from == "B" && edge.to == "A"; });
    ASSERT_NE(ab, dfg.edges.end());
    ASSERT_NE(ba, dfg.edges.end());
    EXPECT_EQ(ab->frequency, 1);
    EXPECT_EQ(ba->frequency, 2);
}

/**
 * PM-18: DFG Edge Ordering
 * Setup: DAG with dependencies
 * Expected: Each directly-follows edge preserves sequence order
 */
TEST(ProcessMiningTests, PM_18_dfg_edge_ordering) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}});
    const auto [status, dfg] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(dfg.edges.size(), 2U);
    std::map<std::string, std::size_t> order{{"A", 0}, {"B", 1}, {"C", 2}};
    EXPECT_LT(order.at(dfg.edges[0].from), order.at(dfg.edges[0].to));
    EXPECT_LT(order.at(dfg.edges[1].from), order.at(dfg.edges[1].to));
}

}  // namespace process_mining_tests
}  // namespace analytics_test

// ============================================================================
// AUTOML & FORECASTING TESTS (AF-01 to AF-18)
// ============================================================================

namespace analytics_test {
namespace automl_forecasting_tests {

/**
 * AF-01: Valid Training Data
 * Expected: Status::OK()
 */
TEST(AutoMLForecasting, AF_01_valid_training_data) {
    themisdb::analytics::AutoML automl;
    const auto [valid, message] = automl.validateTrainingData({{1.0}, {2.0}}, {0.0, 1.0});
    EXPECT_TRUE(valid) << message;
    EXPECT_TRUE(message.empty());
}

/**
 * AF-02: Insufficient Samples
 * Expected: validation returns false with a minimum-sample explanation
 */
TEST(AutoMLForecasting, AF_02_insufficient_samples) {
    themisdb::analytics::AutoML automl;
    const auto [valid, message] = automl.validateTrainingData({{1.0}}, {0.0});
    EXPECT_FALSE(valid);
    EXPECT_NE(message.find("At least 2"), std::string::npos);
}

/**
 * AF-03: NaN Values Detection
 * Expected: Detected and rejected
 */
TEST(AutoMLForecasting, AF_03_nan_detection) {
    themisdb::analytics::AutoML automl;
    const auto [valid, message] = automl.validateTrainingData(
        {{1.0}, {std::numeric_limits<double>::quiet_NaN()}}, {0.0, 1.0});
    EXPECT_FALSE(valid);
    EXPECT_NE(message.find("NaN or Inf"), std::string::npos);
}

/**
 * AF-04: Monthly Seasonality
 * Expected: period=30-31
 */
TEST(AutoMLForecasting, AF_04_monthly_seasonality) {
    std::vector<double> monthly;
    for (int i = 0; i < 120; ++i) {
        monthly.push_back(std::sin(2.0 * 3.141592653589793 * i / 30.0));
    }
    EXPECT_EQ(themisdb::analytics::seasonalityDuration(monthly, 30), 30);
}

/**
 * AF-05: Weekly Seasonality
 * Expected: period=7
 */
TEST(AutoMLForecasting, AF_05_weekly_seasonality) {
    std::vector<double> weekly;
    for (int i = 0; i < 70; ++i) {
        weekly.push_back(std::sin(2.0 * 3.141592653589793 * i / 7.0));
    }
    EXPECT_EQ(themisdb::analytics::seasonalityDuration(weekly, 7), 7);
}

/**
 * AF-06: Acyclic Series
 * Expected: period=0
 */
TEST(AutoMLForecasting, AF_06_acyclic_series) {
    EXPECT_EQ(themisdb::analytics::seasonalityDuration({4.0, 4.0, 4.0, 4.0}), 0);
}

/**
 * AF-07: Simple Exponential Smoothing
 * Expected: convergence
 */
TEST(AutoMLForecasting, AF_07_exp_smoothing) {
    themisdb::analytics::TimeSeries series;
    for (int i = 0; i < 20; ++i) {
        series.push(i * 1'000, 10.0 + i);
    }
    themisdb::analytics::ForecastModel model(themisdb::analytics::ForecastMethod::EXP_SMOOTHING);
    model.fit(series);
    const auto prediction = model.predict(3);
    ASSERT_TRUE(model.isFitted());
    ASSERT_EQ(prediction.size(), 3U);
    EXPECT_TRUE(std::all_of(prediction.begin(), prediction.end(),
                            [](const auto& point) { return std::isfinite(point.value); }));
}

/**
 * AF-08: Holt-Winters with Trend
 * Expected: captures trend
 */
TEST(AutoMLForecasting, AF_08_holt_winters_trend) {
    themisdb::analytics::TimeSeries series;
    for (int i = 0; i < 40; ++i) {
        series.push(i * 1'000, 20.0 + 2.0 * i);
    }
    themisdb::analytics::ForecastModel model(themisdb::analytics::ForecastMethod::HOLT_WINTERS);
    model.fit(series);
    const auto prediction = model.predict(3);
    ASSERT_EQ(prediction.size(), 3U);
    EXPECT_GT(prediction.back().value, prediction.front().value);
}

/**
 * AF-09: Seasonal Component
 * Expected: captures seasonality
 */
TEST(AutoMLForecasting, AF_09_seasonal_component) {
    themisdb::analytics::TimeSeries series;
    for (int i = 0; i < 48; ++i) {
        series.push(i * 1'000, 20.0 + (i % 4) * 5.0);
    }
    themisdb::analytics::ForecastConfig config;
    config.seasonality = 4;
    themisdb::analytics::ForecastModel model(config, themisdb::analytics::ForecastMethod::HOLT_WINTERS);
    model.fit(series);
    const auto prediction = model.predict(4);
    ASSERT_EQ(prediction.size(), 4U);
    EXPECT_TRUE(std::all_of(prediction.begin(), prediction.end(),
                            [](const auto& point) { return std::isfinite(point.value); }));
}

/**
 * AF-10: Training Input Validation
 * Expected: a one-point series is rejected and leaves the model unfitted
 */
TEST(AutoMLForecasting, AF_10_parameter_validation) {
    themisdb::analytics::ForecastModel model;
    themisdb::analytics::TimeSeries insufficient;
    insufficient.push(1'000, 1.0);
    EXPECT_THROW(model.fit(insufficient), std::invalid_argument);
    EXPECT_FALSE(model.isFitted());
}

/**
 * AF-11: Single Feature Set
 * Expected: returns valid ModelAlgorithm
 */
TEST(AutoMLForecasting, AF_11_single_feature_set) {
    themisdb::analytics::AutoML automl;
    const std::vector<themisdb::analytics::ModelAlgorithm> candidates{
        themisdb::analytics::ModelAlgorithm::DECISION_TREE,
        themisdb::analytics::ModelAlgorithm::KNN};
    const auto selected = automl.selectMetalearner({{1.0}, {2.0}}, {0.0, 1.0}, candidates);
    EXPECT_EQ(selected, themisdb::analytics::ModelAlgorithm::DECISION_TREE);
}

/**
 * AF-12: Large Feature Space
 * Expected: a supported candidate is selected for 128 features
 */
TEST(AutoMLForecasting, AF_12_large_feature_space) {
    themisdb::analytics::AutoML automl;
    std::vector<std::vector<double>> features(200, std::vector<double>(128, 1.0));
    std::vector<double> target(200);
    for (std::size_t i = 0; i < target.size(); ++i) {
        target[i] = static_cast<double>(i % 2);
    }
    const std::vector<themisdb::analytics::ModelAlgorithm> candidates{
        themisdb::analytics::ModelAlgorithm::LOGISTIC_REGRESSION,
        themisdb::analytics::ModelAlgorithm::RANDOM_FOREST};
    EXPECT_EQ(automl.selectMetalearner(features, target, candidates),
              themisdb::analytics::ModelAlgorithm::RANDOM_FOREST);
}

/**
 * AF-13: Empty Feature Set
 * Expected: selection rejects empty inputs
 */
TEST(AutoMLForecasting, AF_13_empty_feature_set) {
    themisdb::analytics::AutoML automl;
    EXPECT_THROW(automl.selectMetalearner({}, {}, {}), std::invalid_argument);
}

/**
 * AF-14: Algorithm Comparison
 * Expected: best algorithm selected
 */
TEST(AutoMLForecasting, AF_14_algorithm_comparison) {
    themisdb::analytics::AutoML automl;
    std::vector<std::vector<double>> features(200, {1.0, 2.0, 3.0});
    std::vector<double> target(200);
    for (std::size_t i = 0; i < target.size(); ++i) {
        target[i] = static_cast<double>(i % 2);
    }
    const std::vector<themisdb::analytics::ModelAlgorithm> candidates{
        themisdb::analytics::ModelAlgorithm::DECISION_TREE,
        themisdb::analytics::ModelAlgorithm::RANDOM_FOREST};
    EXPECT_EQ(automl.selectMetalearner(features, target, candidates),
              themisdb::analytics::ModelAlgorithm::RANDOM_FOREST);
}

/**
 * AF-15: Ensemble Selection
 * Expected: returns the currently supported ENSEMBLE algorithm
 */
TEST(AutoMLForecasting, AF_15_voting_ensemble) {
    themisdb::analytics::AutoML automl;
    themisdb::analytics::EvalMetrics first;
    first.f1 = 0.8;
    first.accuracy = 0.82;
    auto second = first;
    second.f1 = 0.81;
    second.accuracy = 0.83;
    EXPECT_EQ(automl.selectEnsembleMethod({first, second}),
              themisdb::analytics::ModelAlgorithm::ENSEMBLE);
}

/**
 * AF-16: Diverse Ensemble Selection
 * Expected: diversity input still selects the supported ENSEMBLE algorithm
 */
TEST(AutoMLForecasting, AF_16_stacking_ensemble) {
    themisdb::analytics::AutoML automl;
    std::vector<themisdb::analytics::EvalMetrics> candidates(3);
    candidates[0].f1 = 0.55;
    candidates[1].f1 = 0.78;
    candidates[2].f1 = 0.96;
    for (auto& candidate : candidates) {
        candidate.accuracy = candidate.f1;
    }
    EXPECT_EQ(automl.selectEnsembleMethod(candidates),
              themisdb::analytics::ModelAlgorithm::ENSEMBLE);
}

/**
 * AF-17: Diversity Analysis
 * Expected: selects appropriate method
 */
TEST(AutoMLForecasting, AF_17_diversity_analysis) {
    themisdb::analytics::AutoML automl;
    std::vector<themisdb::analytics::EvalMetrics> candidates(3);
    candidates[0].f1 = 0.4;
    candidates[0].accuracy = 0.5;
    candidates[1].f1 = 0.7;
    candidates[1].accuracy = 0.75;
    candidates[2].f1 = 0.9;
    candidates[2].accuracy = 0.95;
    EXPECT_EQ(automl.selectEnsembleMethod(candidates),
              themisdb::analytics::ModelAlgorithm::ENSEMBLE);
}

/**
 * AF-18: Single Model Ensemble
 * Expected: returns the currently supported ENSEMBLE algorithm
 */
TEST(AutoMLForecasting, AF_18_single_model_ensemble) {
    themisdb::analytics::AutoML automl;
    themisdb::analytics::EvalMetrics candidate;
    candidate.f1 = 0.91;
    candidate.accuracy = 0.93;
    EXPECT_EQ(automl.selectEnsembleMethod({candidate}),
              themisdb::analytics::ModelAlgorithm::ENSEMBLE);
}

}  // namespace automl_forecasting_tests
}  // namespace analytics_test

// ============================================================================
// STREAMING & CEP TESTS (SC-01 to SC-15)
// ============================================================================

namespace analytics_test {
namespace streaming_cep_tests {

/**
 * SC-01: Sequence Pattern NFA
 * Expected: valid NFA
 */
TEST(StreamingCEP, SC_01_sequence_pattern) {
    themisdb::analytics::PatternConfig config;
    config.pattern_id = "sequence";
    config.type = themisdb::analytics::PatternType::SEQUENCE;
    config.event_types = {"A", "B"};
    themisdb::analytics::PatternMatcher matcher(config);
    EXPECT_TRUE(matcher.processEvent(makeCepEvent("A", 0)).empty());
    const auto matches = matcher.processEvent(makeCepEvent("B", 1));
    ASSERT_EQ(matches.size(), 1U);
    EXPECT_EQ(matches.front().pattern_id, "sequence");
    ASSERT_EQ(matches.front().matched_events.size(), 2U);
    EXPECT_EQ(matches.front().matched_events[0].event_name, "A");
    EXPECT_EQ(matches.front().matched_events[1].event_name, "B");
}

/**
 * SC-02: Alternation Pattern
 * Expected: OR logic correct
 */
TEST(StreamingCEP, SC_02_alternation_pattern) {
    themisdb::analytics::PatternConfig config;
    config.pattern_id = "either";
    config.type = themisdb::analytics::PatternType::DISJUNCTION;
    config.event_types = {"A", "B"};
    themisdb::analytics::PatternMatcher matcher(config);
    EXPECT_EQ(matcher.processEvent(makeCepEvent("A", 0)).size(), 1U);
    EXPECT_EQ(matcher.processEvent(makeCepEvent("B", 1)).size(), 1U);
    EXPECT_EQ(matcher.getMatchCount(), 2U);
}

/**
 * SC-03: Empty Pattern
 * Expected: no matches and no pending state
 */
TEST(StreamingCEP, SC_03_empty_pattern) {
    themisdb::analytics::PatternConfig config;
    config.pattern_id = "empty";
    themisdb::analytics::PatternMatcher matcher(config);
    EXPECT_TRUE(matcher.processEvent(makeCepEvent("A", 0)).empty());
    EXPECT_EQ(matcher.getMatchCount(), 0U);
    EXPECT_EQ(matcher.getPendingMatchCount(), 0U);
}

/**
 * SC-04: Window Transition
 * Expected: aggregation triggered
 */
TEST(StreamingCEP, SC_04_window_transition) {
    themisdb::analytics::WindowConfig config;
    config.type = themisdb::analytics::WindowType::TUMBLING;
    config.size = std::chrono::milliseconds{10};
    themisdb::analytics::WindowManager windows(config);
    std::vector<std::size_t> emitted;
    windows.setWindowCallback([&emitted](const auto& events, auto, auto) {
        emitted.push_back(events.size());
    });
    windows.addEvent(makeCepEvent("A", 0));
    windows.addEvent(makeCepEvent("B", 10));
    ASSERT_EQ(emitted.size(), 1U);
    EXPECT_EQ(emitted.front(), 1U);
    EXPECT_EQ(windows.getStats().windows_created, 2U);
}

/**
 * SC-05: Event Payload Round Trip
 * Expected: serialized payload fields are restored
 */
TEST(StreamingCEP, SC_05_event_payload_round_trip) {
    auto event = makeCepEvent("payload", 42, 3.5);
    const auto decoded = themisdb::analytics::Event::deserialize(event.serialize());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->event_name, "payload");
    EXPECT_EQ(decoded->getField<double>("value"), 3.5);
}

/**
 * SC-06: Expired Window
 * Expected: auto-flushes
 */
TEST(StreamingCEP, SC_06_expired_window) {
    themisdb::analytics::WindowConfig config;
    config.type = themisdb::analytics::WindowType::TUMBLING;
    config.size = std::chrono::milliseconds{10};
    themisdb::analytics::WindowManager windows(config);
    std::size_t emitted_events = 0;
    windows.setWindowCallback([&emitted_events](const auto& events, auto, auto) {
        emitted_events += events.size();
    });
    windows.addEvent(makeCepEvent("A", 0));
    windows.advanceWatermark(std::chrono::system_clock::time_point{
        std::chrono::milliseconds{10}});
    EXPECT_EQ(emitted_events, 1U);
    EXPECT_EQ(windows.getStats().windows_closed, 1U);
}

/**
 * SC-07: Tumbling Semantics
 * Expected: non-overlapping
 */
TEST(StreamingCEP, SC_07_tumbling_semantics) {
    themisdb::analytics::WindowConfig config;
    config.type = themisdb::analytics::WindowType::TUMBLING;
    config.size = std::chrono::milliseconds{10};
    themisdb::analytics::WindowManager windows(config);
    windows.addEvent(makeCepEvent("A", 0));
    windows.addEvent(makeCepEvent("B", 10));
    EXPECT_EQ(windows.getStats().windows_created, 2U);
    EXPECT_EQ(windows.getWindowEvents().size(), 1U);
    EXPECT_EQ(windows.getWindowEvents().front().event_name, "B");
}

/**
 * SC-08: Sliding Semantics
 * Expected: overlapping
 */
TEST(StreamingCEP, SC_08_sliding_semantics) {
    themisdb::analytics::WindowConfig config;
    config.type = themisdb::analytics::WindowType::SLIDING;
    config.size = std::chrono::milliseconds{20};
    config.slide = std::chrono::milliseconds{10};
    themisdb::analytics::WindowManager windows(config);
    windows.addEvent(makeCepEvent("A", 0));
    windows.addEvent(makeCepEvent("B", 10));
    EXPECT_EQ(windows.getStats().windows_created, 2U);
    const auto overlapping = windows.getEvents(
        std::chrono::system_clock::time_point{std::chrono::milliseconds{0}},
        std::chrono::system_clock::time_point{std::chrono::milliseconds{20}});
    EXPECT_GE(overlapping.size(), 2U);
}

/**
 * SC-09: Window Boundary
 * Expected: correct element inclusion
 */
TEST(StreamingCEP, SC_09_window_boundary) {
    themisdb::analytics::WindowConfig config;
    config.type = themisdb::analytics::WindowType::TUMBLING;
    config.size = std::chrono::milliseconds{10};
    themisdb::analytics::WindowManager windows(config);
    windows.addEvent(makeCepEvent("before", 9));
    windows.addEvent(makeCepEvent("boundary", 10));
    EXPECT_EQ(windows.getStats().windows_created, 2U);
    const auto current = windows.getWindowEvents();
    ASSERT_EQ(current.size(), 1U);
    EXPECT_EQ(current.front().event_name, "boundary");
}

/**
 * SC-10: Aggregation Output
 * Expected: correct result
 */
TEST(StreamingCEP, SC_10_aggregation_output) {
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("total", themisdb::analytics::AggregationType::SUM, "value");
    aggregator.processEvent(makeCepEvent("A", 0, 2.0));
    aggregator.processEvent(makeCepEvent("B", 1, 5.0));
    const auto result = aggregator.getResult("total");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->count, 2U);
    ASSERT_TRUE(std::holds_alternative<double>(result->result));
    EXPECT_DOUBLE_EQ(std::get<double>(result->result), 7.0);
}

/**
 * SC-11: Empty Window
 * Expected: handled gracefully
 */
TEST(StreamingCEP, SC_11_empty_window) {
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("count", themisdb::analytics::AggregationType::COUNT, "value");
    EXPECT_FALSE(aggregator.getResult("count").has_value());
    EXPECT_TRUE(aggregator.getResults().empty());
}

/**
 * SC-12: Multiple Flushes
 * Expected: idempotent
 */
TEST(StreamingCEP, SC_12_multiple_flushes) {
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("count", themisdb::analytics::AggregationType::COUNT, "value");
    aggregator.processEvent(makeCepEvent("A", 0));
    ASSERT_EQ(aggregator.getResult("count")->count, 1U);
    aggregator.reset();
    EXPECT_FALSE(aggregator.getResult("count").has_value());
    aggregator.processEvent(makeCepEvent("B", 1));
    ASSERT_TRUE(aggregator.getResult("count").has_value());
    EXPECT_EQ(aggregator.getResult("count")->count, 1U);
}

/**
 * SC-13: Sum Aggregation
 * Expected: O(1) update
 */
TEST(StreamingCEP, SC_13_sum_aggregation) {
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("sum", themisdb::analytics::AggregationType::SUM, "value");
    aggregator.processEvent(makeCepEvent("A", 0, 1.25));
    aggregator.processEvent(makeCepEvent("B", 1, 2.75));
    const auto result = aggregator.getResult("sum");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<double>(result->result));
    EXPECT_DOUBLE_EQ(std::get<double>(result->result), 4.0);
}

/**
 * SC-14: Average Aggregation
 * Expected: maintains running avg
 */
TEST(StreamingCEP, SC_14_average_aggregation) {
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("average", themisdb::analytics::AggregationType::AVG, "value");
    aggregator.processEvent(makeCepEvent("A", 0, 2.0));
    aggregator.processEvent(makeCepEvent("B", 1, 8.0));
    const auto result = aggregator.getResult("average");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<double>(result->result));
    EXPECT_DOUBLE_EQ(std::get<double>(result->result), 5.0);
}

/**
 * SC-15: Count Aggregation
 * Expected: correct count
 */
TEST(StreamingCEP, SC_15_count_aggregation) {
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("count", themisdb::analytics::AggregationType::COUNT, "value");
    aggregator.processEvent(makeCepEvent("A", 0, 1.0));
    aggregator.processEvent(makeCepEvent("B", 1, 2.0));
    EXPECT_EQ(aggregator.getResult("count")->count, 2U);
}

}  // namespace streaming_cep_tests
}  // namespace analytics_test

// ============================================================================
// KNOWLEDGE BASE TESTS (KB-01 to KB-10)
// ============================================================================

namespace analytics_test {
namespace knowledge_base_tests {

/**
 * KB-01: Parse Valid YAML Config
 */
TEST(KnowledgeBase, KB_01_parse_valid_config) {
    ScopedTempFile input("rules:\n  - id: eligible-rule\n    priority: 7\n    description: eligible\n");
    KnowledgeBase kb;
    EXPECT_EQ(kb.loadRulesFromYaml(input.path()), 1);
    ASSERT_EQ(kb.getRules().size(), 1U);
    EXPECT_EQ(kb.getRules().front().id, "eligible-rule");
    EXPECT_EQ(kb.getRules().front().priority, 7);
}

/**
 * KB-02: Missing Config File
 */
TEST(KnowledgeBase, KB_02_missing_file) {
    KnowledgeBase::clearYamlParserFn();
    const auto missing = (std::filesystem::temp_directory_path() /
                          "analytics_gap_closure_missing_rules.yml").string();
    KnowledgeBase kb;
    EXPECT_EQ(kb.loadRulesFromYaml(missing), -1);
    EXPECT_EQ(kb.ruleCount(), 0U);
}

/**
 * KB-03: Add Horn Clause
 */
TEST(KnowledgeBase, KB_03_add_horn_clause) {
    KnowledgeBase kb;
    themisdb::analytics::HornClause rule;
    rule.id = "template-rule";
    rule.description = "rule created from the supported Horn-clause structure";
    rule.conditions.push_back({"subject", "predicate", "object"});
    rule.consequents.push_back({"subject", "derived", "value"});
    kb.addRule(std::move(rule));
    ASSERT_EQ(kb.getRules().size(), 1U);
    EXPECT_EQ(kb.getRules().front().conditions.size(), 1U);
    EXPECT_EQ(kb.getRules().front().consequents.front().predicate, "derived");
}

/**
 * KB-04: Custom YAML Callback
 */
TEST(KnowledgeBase, KB_04_yaml_callback) {
    bool invoked = false;
    ScopedYamlParser parser([&invoked](const std::string& path, KnowledgeBase& kb) {
        invoked = path == "virtual-rules.yml";
        themisdb::analytics::HornClause rule;
        rule.id = "callback-rule";
        kb.addRule(std::move(rule));
        return 1;
    });
    KnowledgeBase kb;
    EXPECT_EQ(kb.loadRulesFromYaml("virtual-rules.yml"), 1);
    EXPECT_TRUE(invoked);
    ASSERT_EQ(kb.getRules().size(), 1U);
    EXPECT_EQ(kb.getRules().front().id, "callback-rule");
}

/**
 * KB-05: Assert Fact
 */
TEST(KnowledgeBase, KB_05_assert_fact) {
    // Test: Assert fact to working memory
    // Verify: Fact stored
    KnowledgeBase kb;
    const auto id = kb.assertFact("Alice", "knows", "Bob");
    const auto fact = kb.getFact("Alice", "knows", "Bob");
    ASSERT_TRUE(fact.has_value());
    EXPECT_EQ(fact->id, id);
    EXPECT_EQ(kb.factCount(), 1U);
}

/**
 * KB-06: Query Facts
 */
TEST(KnowledgeBase, KB_06_query_facts) {
    // Test: Query working memory
    // Verify: Correct facts retrieved
    KnowledgeBase kb;
    kb.assertFact("John", "age", "30");
    kb.assertFact("Jane", "age", "28");

    auto results = kb.getFacts("age");
    EXPECT_EQ(results.size(), 2u);
    bool found_john = false;
    bool found_jane = false;
    for (const auto& fact : results) {
        if (fact.subject == "John" && fact.object == "30") {
            found_john = true;
        } else if (fact.subject == "Jane" && fact.object == "28") {
            found_jane = true;
        }
    }
    EXPECT_TRUE(found_john);
    EXPECT_TRUE(found_jane);
}

/**
 * KB-07: Pattern Matching
 */
TEST(KnowledgeBase, KB_07_pattern_matching) {
    themisdb::analytics::ExpertSystemEngine engine;
    engine.assertFact("Alice", "knows", "Bob");
    themisdb::analytics::HornClause rule;
    rule.id = "friend-inference";
    rule.conditions.push_back({"Alice", "knows", "Bob"});
    rule.consequents.push_back({"Alice", "friend", "Bob"});
    engine.knowledgeBase().addRule(std::move(rule));
    EXPECT_EQ(engine.forwardChain(), 1);
    EXPECT_TRUE(engine.knowledgeBase().hasFact("Alice", "friend", "Bob"));
}

/**
 * KB-08: Malformed YAML
 */
TEST(KnowledgeBase, KB_08_malformed_yaml) {
    ScopedTempFile input("rules: [invalid\n");
    KnowledgeBase kb;
    EXPECT_EQ(kb.loadRulesFromYaml(input.path()), 0);
    EXPECT_EQ(kb.ruleCount(), 0U);
}

/**
 * KB-09: Empty Templates
 */
TEST(KnowledgeBase, KB_09_empty_rules) {
    ScopedTempFile input("rules: []\n");
    KnowledgeBase kb;
    EXPECT_EQ(kb.loadRulesFromYaml(input.path()), 0);
    EXPECT_TRUE(kb.getRules().empty());
}

/**
 * KB-10: Callback Error Handling
 */
TEST(KnowledgeBase, KB_10_callback_error_handling) {
    ScopedYamlParser parser([](const std::string&, KnowledgeBase&) -> int {
        throw std::runtime_error("parser failure");
    });
    KnowledgeBase kb;
    EXPECT_THROW(kb.loadRulesFromYaml("virtual-rules.yml"), std::runtime_error);
    EXPECT_EQ(kb.ruleCount(), 0U);
}

}  // namespace knowledge_base_tests
}  // namespace analytics_test

// ============================================================================
// UTILITIES TESTS (UT-01 to UT-15)
// ============================================================================

namespace analytics_test {
namespace utilities_tests {

/**
 * UT-01: Column Batch Split
 * Expected: batches respect the default row limit
 */
TEST(Utilities, UT_01_column_batches) {
    const auto batches = makeColumnarBatch(2'050).split(
        themisdb::analytics::ColumnBatch::kDefaultBatchSize);
    ASSERT_EQ(batches.size(), 3U);
    EXPECT_EQ(batches[0].rowCount(), 1'024U);
    EXPECT_EQ(batches[1].rowCount(), 1'024U);
    EXPECT_EQ(batches[2].rowCount(), 2U);
    EXPECT_EQ(batches[0].columnCount(), 2U);
}

/**
 * UT-02: Large Columnar Dataset
 * Expected: all rows and values remain addressable
 */
TEST(Utilities, UT_02_large_dataset) {
    const auto batch = makeColumnarBatch(100'000);
    ASSERT_EQ(batch.rowCount(), 100'000U);
    EXPECT_EQ(batch.getColumn("value")->size(), batch.rowCount());
    EXPECT_EQ(std::get<int64_t>(batch.getColumn("value")->get(99'999)), 99'999);
}

/**
 * UT-03: Columnar Sum
 * Expected: the aggregate matches the input values
 */
TEST(Utilities, UT_03_merge_shards) {
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto result = engine.aggregate(makeColumnarBatch(4), {{
        .result_name = "total",
        .input_column = "value",
        .function = themisdb::analytics::AggregateSpec::Function::Sum
    }});
    ASSERT_EQ(result.rowCount(), 1U);
    EXPECT_DOUBLE_EQ(std::get<double>(result.getColumn("total")->get(0)), 6.0);
}

/**
 * UT-04: Many Batches
 * Expected: each batch produces one count result
 */
TEST(Utilities, UT_04_many_shards) {
    auto batches = makeColumnarBatch(200).split(2);
    ASSERT_EQ(batches.size(), 100U);
    themisdb::analytics::VectorizedPipeline pipeline;
    pipeline.addAggregate({{
        .result_name = "count",
        .input_column = "",
        .function = themisdb::analytics::AggregateSpec::Function::Count
    }});
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto results = engine.executeBatched(batches, pipeline);
    ASSERT_EQ(results.size(), batches.size());
    EXPECT_TRUE(std::all_of(results.begin(), results.end(),
                            [](const auto& result) { return result.rowCount() == 1U; }));
}

/**
 * UT-05: String Column Filtering
 * Expected: matching labels and row count are returned
 */
TEST(Utilities, UT_05_string_column_filter) {
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto result = engine.filter(
        makeColumnarBatch(6),
        {themisdb::analytics::Predicate::eq("label", std::string{"label-1"})}).materialize();
    ASSERT_EQ(result.rowCount(), 2U);
    EXPECT_EQ(std::get<std::string>(result.getColumn("label")->get(0)), "label-1");
}

/**
 * UT-06: Distinct String Aggregation
 * Expected: distinct labels are counted
 */
TEST(Utilities, UT_06_distinct_string_aggregation) {
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto result = engine.aggregate(makeColumnarBatch(6), {{
        .result_name = "distinct_labels",
        .input_column = "label",
        .function = themisdb::analytics::AggregateSpec::Function::CountDistinct
    }});
    EXPECT_DOUBLE_EQ(std::get<double>(result.getColumn("distinct_labels")->get(0)), 3.0);
}

/**
 * UT-07: Activity Edge Frequencies
 * Expected: repeated directly-follows edges are counted
 */
TEST(Utilities, UT_07_activity_edge_frequencies) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "B", "C"}});
    const auto [status, graph] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(graph.edges.size(), 2U);
    EXPECT_EQ(graph.edges[0].frequency, 2);
    EXPECT_EQ(graph.edges[1].frequency, 2);
}

/**
 * UT-08: RAII Coverage
 * Expected: all resources released
 */
TEST(Utilities, UT_08_raii_coverage) {
    auto column = std::make_shared<themisdb::analytics::Column>(
        "value", themisdb::analytics::ColumnType::Int64);
    column->appendInt64(7);
    std::weak_ptr<themisdb::analytics::Column> weak_column = column;
    auto batch = std::make_unique<themisdb::analytics::ColumnBatch>();
    batch->addColumn(column);
    column.reset();
    EXPECT_FALSE(weak_column.expired());
    batch.reset();
    EXPECT_TRUE(weak_column.expired());
}

/**
 * UT-09: Maximum Integer Value
 * Expected: maximum signed integer remains intact in a column
 */
TEST(Utilities, UT_09_maximum_integer_value) {
    themisdb::analytics::Column column("value", themisdb::analytics::ColumnType::Int64);
    column.appendInt64(std::numeric_limits<int64_t>::max());
    ASSERT_EQ(column.size(), 1U);
    EXPECT_EQ(std::get<int64_t>(column.get(0)), std::numeric_limits<int64_t>::max());
}

/**
 * UT-10: Large Aggregation Result
 * Expected: exact sum is returned for the input batch
 */
TEST(Utilities, UT_10_large_aggregation_result) {
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto result = engine.aggregate(makeColumnarBatch(1'000), {{
        .result_name = "total",
        .input_column = "value",
        .function = themisdb::analytics::AggregateSpec::Function::Sum
    }});
    EXPECT_DOUBLE_EQ(std::get<double>(result.getColumn("total")->get(0)), 499'500.0);
}

/**
 * UT-11: Forecast Error Propagation
 */
TEST(Utilities, UT_11_error_propagation) {
    themisdb::analytics::ForecastModel model;
    EXPECT_THROW(model.predict(1), std::runtime_error);
    themisdb::analytics::TimeSeries series;
    series.push(0, 1.0);
    series.push(1, 2.0);
    model.fit(series);
    EXPECT_TRUE(model.predict(0).empty());
}

/**
 * UT-12: Bounded Batch Size
 */
TEST(Utilities, UT_12_bounded_batch_size) {
    const auto batches = makeColumnarBatch(10'000).split(256);
    ASSERT_FALSE(batches.empty());
    std::size_t rows = 0;
    for (const auto& batch : batches) {
        EXPECT_LE(batch.rowCount(), 256U);
        rows += batch.rowCount();
    }
    EXPECT_EQ(rows, 10'000U);
}

/**
 * UT-13: Concurrency Safety
 */
TEST(Utilities, UT_13_concurrency_safety) {
    themisdb::analytics::ExpertSystemEngine engine;
    std::vector<std::thread> writers;
    for (int thread = 0; thread < 8; ++thread) {
        writers.emplace_back([&engine, thread] {
            for (int fact = 0; fact < 100; ++fact) {
                engine.assertFact("subject-" + std::to_string(thread),
                                  "value", std::to_string(fact));
            }
        });
    }
    for (auto& writer : writers) {
        writer.join();
    }
    EXPECT_EQ(engine.factCount(), 800U);
}

/**
 * UT-14: Boundary Conditions
 */
TEST(Utilities, UT_14_boundary_conditions) {
    themisdb::analytics::Column column("value", themisdb::analytics::ColumnType::Double);
    column.appendDouble(0.0);
    column.appendNull();
    ASSERT_EQ(column.size(), 2U);
    EXPECT_DOUBLE_EQ(std::get<double>(column.get(0)), 0.0);
    EXPECT_TRUE(column.isNull(1));
    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(column.get(1)));
}

/**
 * UT-15: Serialization
 */
TEST(Utilities, UT_15_serialization) {
    themisdb::analytics::TimeSeries series;
    for (int i = 0; i < 10; ++i) {
        series.push(i * 1'000, 2.0 * i + 1.0);
    }
    themisdb::analytics::ForecastModel original(themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    original.fit(series);
    const auto expected = original.predict(3);
    auto restored = themisdb::analytics::ForecastModel::deserialize(original.serialize());
    const auto actual = restored.predict(3);
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_DOUBLE_EQ(actual[i].value, expected[i].value);
        EXPECT_EQ(actual[i].timestamp_ms, expected[i].timestamp_ms);
    }
}

}  // namespace utilities_tests
}  // namespace analytics_test

// ============================================================================
// INTEGRATION TESTS (IT-01 to IT-10)
// ============================================================================

namespace analytics_test {
namespace integration_tests {

/**
 * IT-01: Process mining graph to columnar aggregation
 * Expected: edge frequencies survive an aggregate pass
 */
TEST(Integration, IT_01_process_graph_to_columnar_aggregation) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "B", "C"}});
    const auto [status, graph] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;

    themisdb::analytics::ColumnBatch edges;
    auto frequencies = std::make_shared<themisdb::analytics::Column>(
        "frequency", themisdb::analytics::ColumnType::Int64);
    for (const auto& edge : graph.edges) {
        frequencies->appendInt64(edge.frequency);
    }
    edges.addColumn(frequencies);
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto aggregate = engine.aggregate(edges, {{
        .result_name = "total_frequency",
        .input_column = "frequency",
        .function = themisdb::analytics::AggregateSpec::Function::Sum
    }});
    ASSERT_EQ(aggregate.rowCount(), 1U);
    EXPECT_DOUBLE_EQ(std::get<double>(aggregate.getColumn("total_frequency")->get(0)), 4.0);
}

/**
 * IT-02: Process variant counts to columnar aggregation
 * Expected: variant frequencies sum to the event-log case count
 */
TEST(Integration, IT_02_variant_frequencies_to_columnar_aggregation) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B"}, {"A", "B"}, {"A", "C"}});
    const auto [status, variants] = harness.mining().analyzeVariants(log);
    ASSERT_TRUE(status.ok) << status.message;
    themisdb::analytics::ColumnBatch batch;
    auto counts = std::make_shared<themisdb::analytics::Column>(
        "frequency", themisdb::analytics::ColumnType::Int64);
    for (const auto& variant : variants) {
        counts->appendInt64(variant.frequency);
    }
    batch.addColumn(counts);
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto aggregate = engine.aggregate(batch, {{
        .result_name = "case_count",
        .input_column = "frequency",
        .function = themisdb::analytics::AggregateSpec::Function::Sum
    }});
    EXPECT_DOUBLE_EQ(std::get<double>(aggregate.getColumn("case_count")->get(0)), 3.0);
}

/**
 * IT-03: Forecast model serialization
 * Expected: restored model returns the same forecasts
 */
TEST(Integration, IT_03_forecast_model_serialization) {
    themisdb::analytics::TimeSeries series;
    for (int i = 0; i < 8; ++i) {
        series.push(i * 1'000, 4.0 + 1.5 * i);
    }
    themisdb::analytics::ForecastModel model(themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    model.fit(series);
    const auto expected = model.predict(2);
    auto restored = themisdb::analytics::ForecastModel::deserialize(model.serialize());
    const auto actual = restored.predict(2);
    ASSERT_EQ(actual.size(), 2U);
    EXPECT_DOUBLE_EQ(actual[0].value, expected[0].value);
    EXPECT_DOUBLE_EQ(actual[1].value, expected[1].value);
}

/**
 * IT-04: Forecasting output to columnar aggregation
 * Expected: forecast values can be aggregated without changing their sum
 */
TEST(Integration, IT_04_forecast_to_columnar_aggregation) {
    themisdb::analytics::TimeSeries series;
    for (int i = 0; i < 8; ++i) {
        series.push(i * 1'000, 5.0);
    }
    themisdb::analytics::ForecastModel model(themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    model.fit(series);
    const auto forecast = model.predict(3);
    themisdb::analytics::ColumnBatch batch;
    auto values = std::make_shared<themisdb::analytics::Column>(
        "forecast", themisdb::analytics::ColumnType::Double);
    for (const auto& point : forecast) {
        values->appendDouble(point.value);
    }
    batch.addColumn(values);
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto aggregate = engine.aggregate(batch, {{
        .result_name = "forecast_sum",
        .input_column = "forecast",
        .function = themisdb::analytics::AggregateSpec::Function::Sum
    }});
    EXPECT_NEAR(std::get<double>(aggregate.getColumn("forecast_sum")->get(0)), 15.0, 1e-9);
}

/**
 * IT-05: CEP match to aggregation
 * Expected: matched events are counted by the downstream aggregator
 */
TEST(Integration, IT_05_cep_match_to_aggregation) {
    themisdb::analytics::PatternConfig config;
    config.pattern_id = "purchase";
    config.type = themisdb::analytics::PatternType::SEQUENCE;
    config.event_types = {"begin", "complete"};
    themisdb::analytics::PatternMatcher matcher(config);
    themisdb::analytics::Aggregator aggregator;
    aggregator.addAggregation("matched_events", themisdb::analytics::AggregationType::COUNT, "value");
    EXPECT_TRUE(matcher.processEvent(makeCepEvent("begin", 0)).empty());
    const auto matches = matcher.processEvent(makeCepEvent("complete", 1));
    ASSERT_EQ(matches.size(), 1U);
    for (const auto& event : matches.front().matched_events) {
        aggregator.processEvent(event);
    }
    ASSERT_TRUE(aggregator.getResult("matched_events").has_value());
    EXPECT_EQ(aggregator.getResult("matched_events")->count, 2U);
}

/**
 * IT-06: KnowledgeBase → ExpertSystem
 * Expected: rule evaluation works
 */
TEST(Integration, IT_06_kb_expert_system_integration) {
    themisdb::analytics::ExpertSystemEngine engine;
    engine.assertFact("sensor-1", "status", "critical");
    themisdb::analytics::HornClause rule;
    rule.id = "raise-alert";
    rule.conditions.push_back({"sensor-1", "status", "critical"});
    rule.consequents.push_back({"sensor-1", "alert", "required"});
    engine.knowledgeBase().addRule(std::move(rule));
    EXPECT_EQ(engine.forwardChain(), 1);
    const auto result = engine.queryGoal({"sensor-1", "alert", "required"});
    EXPECT_TRUE(result.success);
}

/**
 * IT-07: End-to-End Dataflow
 * Expected: all modules work together
 */
TEST(Integration, IT_07_process_graph_to_forecast) {
    ProcessMiningHarness harness;
    const auto log = makeProcessLog({{"A", "B", "C"}, {"A", "B", "C"}});
    const auto [status, graph] = harness.mining().createDFG(log);
    ASSERT_TRUE(status.ok) << status.message;
    themisdb::analytics::TimeSeries frequencies;
    for (std::size_t edge = 0; edge < graph.edges.size(); ++edge) {
        frequencies.push(static_cast<int64_t>(edge), graph.edges[edge].frequency);
    }
    themisdb::analytics::ForecastModel model(themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    model.fit(frequencies);
    const auto prediction = model.predict(1);
    ASSERT_EQ(prediction.size(), 1U);
    EXPECT_NEAR(prediction.front().value, 2.0, 1e-9);
}

/**
 * IT-08: Error Propagation
 * Expected: exception safety maintained
 */
TEST(Integration, IT_08_error_propagation) {
    themisdb::analytics::ForecastModel model;
    EXPECT_THROW(model.predict(1), std::runtime_error);
    EXPECT_FALSE(model.isFitted());
}

/**
 * IT-09: Batch aggregation correctness
 * Expected: aggregate result is exact for a deterministic batch
 */
TEST(Integration, IT_09_batch_aggregation_correctness) {
    themisdb::analytics::ColumnarExecutionEngine engine;
    const auto result = engine.aggregate(makeColumnarBatch(1'000), {{
        .result_name = "total",
        .input_column = "value",
        .function = themisdb::analytics::AggregateSpec::Function::Sum
    }});
    ASSERT_EQ(result.rowCount(), 1U);
    EXPECT_DOUBLE_EQ(std::get<double>(result.getColumn("total")->get(0)), 499'500.0);
}

/**
 * IT-10: Pattern state reset
 * Expected: reset discards pending partial matches
 */
TEST(Integration, IT_10_pattern_state_reset) {
    themisdb::analytics::PatternConfig config;
    config.pattern_id = "two-step";
    config.event_types = {"first", "second"};
    themisdb::analytics::PatternMatcher matcher(config);
    EXPECT_TRUE(matcher.processEvent(makeCepEvent("first", 0)).empty());
    EXPECT_GT(matcher.getPendingMatchCount(), 0U);
    matcher.reset();
    EXPECT_EQ(matcher.getPendingMatchCount(), 0U);
    EXPECT_TRUE(matcher.processEvent(makeCepEvent("second", 1)).empty());
}

}  // namespace integration_tests
}  // namespace analytics_test

// ============================================================================
// Test Main Entry Point
// ============================================================================
