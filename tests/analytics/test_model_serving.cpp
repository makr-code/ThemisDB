/**
 * ModelServingEngine unit tests.
 *
 * Covers:
 *  - registerModel / unregisterModel
 *  - predict (single-record online inference)
 *  - predictBatch (batch inference)
 *  - predictProba (class probabilities – classification and regression)
 *  - listModels / modelInfo
 *  - healthMetrics (prediction counters, latency tracking)
 *  - isRegistered
 *  - serializeModel / loadModel (round-trip persistence)
 *  - makeModelKey helper
 *  - Error paths: empty name/version, missing model, full registry,
 *    duplicate registration, oversized batch
 *  - Thread-safety: concurrent readers
 */

#include <gtest/gtest.h>
#include "analytics/model_serving.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

/** Build a binary classification dataset: label "pos" when x1 > 0. */
static std::vector<DataPoint> makeBinaryData(int n, int seed = 0) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    double step = 2.0 / static_cast<double>(n > 1 ? n - 1 : 1);
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id           = "p" + std::to_string(i);
        p.timestamp_ms = static_cast<int64_t>(i);
        double x1 = -1.0 + step * i;
        double x2 = std::sin(static_cast<double>(i + seed) * 0.7);
        p.set("x1", x1);
        p.set("x2", x2);
        p.fields["label"] = std::string((x1 + x2 > 0.0) ? "pos" : "neg");
        data.push_back(std::move(p));
    }
    return data;
}

/** Build a regression dataset: y = 2*x1 - x2. */
static std::vector<DataPoint> makeRegressionData(int n) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id = "r" + std::to_string(i);
        double x1 = -1.0 + 2.0 * static_cast<double>(i) / (n - 1);
        double x2 = std::cos(static_cast<double>(i) * 0.5);
        p.set("x1", x1);
        p.set("x2", x2);
        p.fields["y"] = 2.0 * x1 - x2;
        data.push_back(std::move(p));
    }
    return data;
}

/** Train a minimal classification model. */
static AutoMLModel trainClassifier(int n = 60) {
    auto data = makeBinaryData(n);
    AutoML automl;
    AutoMLConfig cfg;
    cfg.target              = "label";
    cfg.task                = AutoMLTask::CLASSIFICATION;
    cfg.metric              = AutoMLMetric::ACCURACY;
    cfg.max_time_minutes    = 1;
    cfg.max_trials          = 5;
    cfg.cv_folds            = 2;
    cfg.feature_engineering = false;
    cfg.ensemble            = false;
    cfg.random_seed         = 42;
    return automl.trainClassifier(data, cfg);
}

/** Train a minimal regression model. */
static AutoMLModel trainRegressor(int n = 60) {
    auto data = makeRegressionData(n);
    AutoML automl;
    AutoMLConfig cfg;
    cfg.target              = "y";
    cfg.task                = AutoMLTask::REGRESSION;
    cfg.metric              = AutoMLMetric::R2;
    cfg.max_time_minutes    = 1;
    cfg.max_trials          = 5;
    cfg.cv_folds            = 2;
    cfg.feature_engineering = false;
    cfg.ensemble            = false;
    cfg.random_seed         = 42;
    return automl.trainRegressor(data, cfg);
}

// ============================================================================
// makeModelKey helper
// ============================================================================

TEST(ModelServingHelpers, MakeModelKey) {
    EXPECT_EQ(makeModelKey("m", "v1"), "m:v1");
    EXPECT_EQ(makeModelKey("churn", "2024-01"), "churn:2024-01");
    EXPECT_EQ(makeModelKey("", ""), ":");
}

// ============================================================================
// Registration basics
// ============================================================================

TEST(ModelServingEngine, RegisterAndIsRegistered) {
    ModelServingEngine engine;
    EXPECT_FALSE(engine.isRegistered("m", "v1"));

    engine.registerModel("m", "v1", trainClassifier());
    EXPECT_TRUE(engine.isRegistered("m", "v1"));
    EXPECT_FALSE(engine.isRegistered("m", "v2"));
    EXPECT_FALSE(engine.isRegistered("other", "v1"));
}

TEST(ModelServingEngine, UnregisterReturnsTrueOnSuccess) {
    ModelServingEngine engine;
    engine.registerModel("m", "v1", trainClassifier());
    EXPECT_TRUE(engine.unregisterModel("m", "v1"));
    EXPECT_FALSE(engine.isRegistered("m", "v1"));
}

TEST(ModelServingEngine, UnregisterReturnsFalseIfNotFound) {
    ModelServingEngine engine;
    EXPECT_FALSE(engine.unregisterModel("no-such", "v1"));
}

TEST(ModelServingEngine, RegisterEmptyNameThrows) {
    ModelServingEngine engine;
    EXPECT_THROW(engine.registerModel("", "v1", trainClassifier()),
                 std::invalid_argument);
}

TEST(ModelServingEngine, RegisterEmptyVersionThrows) {
    ModelServingEngine engine;
    EXPECT_THROW(engine.registerModel("m", "", trainClassifier()),
                 std::invalid_argument);
}

TEST(ModelServingEngine, RegisterDuplicateThrows) {
    ModelServingEngine engine;
    engine.registerModel("m", "v1", trainClassifier());
    EXPECT_THROW(engine.registerModel("m", "v1", trainClassifier()),
                 std::runtime_error);
}

TEST(ModelServingEngine, RegisterFullRegistryThrows) {
    ModelServingConfig cfg;
    cfg.max_models = 2;
    ModelServingEngine engine(cfg);

    engine.registerModel("m", "v1", trainClassifier());
    engine.registerModel("m", "v2", trainClassifier());
    EXPECT_THROW(engine.registerModel("m", "v3", trainClassifier()),
                 std::runtime_error);
}

TEST(ModelServingEngine, MultipleVersionsSameModel) {
    ModelServingEngine engine;
    engine.registerModel("m", "v1", trainClassifier());
    engine.registerModel("m", "v2", trainClassifier());
    EXPECT_TRUE(engine.isRegistered("m", "v1"));
    EXPECT_TRUE(engine.isRegistered("m", "v2"));
}

// ============================================================================
// listModels / modelInfo
// ============================================================================

TEST(ModelServingEngine, ListModelsEmpty) {
    ModelServingEngine engine;
    EXPECT_TRUE(engine.listModels().empty());
}

TEST(ModelServingEngine, ListModelsReturnsMeta) {
    ModelServingEngine engine;
    engine.registerModel("churn", "v1", trainClassifier());
    engine.registerModel("revenue", "v1", trainRegressor());

    auto list = engine.listModels();
    ASSERT_EQ(list.size(), 2u);

    bool found_churn   = false;
    bool found_revenue = false;
    for (const auto& mi : list) {
        if (mi.name == "churn"   && mi.version == "v1") found_churn   = true;
        if (mi.name == "revenue" && mi.version == "v1") found_revenue = true;
    }
    EXPECT_TRUE(found_churn);
    EXPECT_TRUE(found_revenue);
}

TEST(ModelServingEngine, ModelInfoFound) {
    ModelServingEngine engine;
    engine.registerModel("m", "v1", trainClassifier());

    auto mi = engine.modelInfo("m", "v1");
    ASSERT_TRUE(mi.has_value());
    EXPECT_EQ(mi->name,    "m");
    EXPECT_EQ(mi->version, "v1");
    EXPECT_EQ(mi->task,    AutoMLTask::CLASSIFICATION);
    EXPECT_TRUE(mi->is_active);
    EXPECT_GT(mi->registered_at_ms, 0);
}

TEST(ModelServingEngine, ModelInfoNotFound) {
    ModelServingEngine engine;
    EXPECT_FALSE(engine.modelInfo("no", "v1").has_value());
}

// ============================================================================
// predict (single record)
// ============================================================================

TEST(ModelServingEngine, PredictClassificationReturnsLabel) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    DataPoint dp;
    dp.id = "test";
    dp.set("x1", 0.8);
    dp.set("x2", 0.0);

    auto label = engine.predict("cls", "v1", dp);
    EXPECT_TRUE(label == "pos" || label == "neg");
}

TEST(ModelServingEngine, PredictRegressionReturnsNumericString) {
    ModelServingEngine engine;
    engine.registerModel("reg", "v1", trainRegressor());

    DataPoint dp;
    dp.id = "test";
    dp.set("x1", 0.5);
    dp.set("x2", 0.3);

    auto val_str = engine.predict("reg", "v1", dp);
    EXPECT_NO_THROW((void)std::stod(val_str));
}

TEST(ModelServingEngine, PredictMissingModelThrows) {
    ModelServingEngine engine;
    DataPoint dp;
    EXPECT_THROW(engine.predict("no-such", "v1", dp), std::out_of_range);
}

// ============================================================================
// predictBatch
// ============================================================================

TEST(ModelServingEngine, PredictBatchReturnsSameSizeAsInput) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    auto batch = makeBinaryData(20);
    auto preds = engine.predictBatch("cls", "v1", batch);
    ASSERT_EQ(preds.size(), batch.size());
    for (const auto& p : preds)
        EXPECT_TRUE(p == "pos" || p == "neg");
}

TEST(ModelServingEngine, PredictBatchEmptyInput) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());
    auto preds = engine.predictBatch("cls", "v1", {});
    EXPECT_TRUE(preds.empty());
}

TEST(ModelServingEngine, PredictBatchOversizedThrows) {
    ModelServingConfig cfg;
    cfg.max_batch_size = 5;
    ModelServingEngine engine(cfg);
    engine.registerModel("cls", "v1", trainClassifier());

    auto batch = makeBinaryData(10);
    EXPECT_THROW(engine.predictBatch("cls", "v1", batch), std::invalid_argument);
}

TEST(ModelServingEngine, PredictBatchMissingModelThrows) {
    ModelServingEngine engine;
    auto batch = makeBinaryData(5);
    EXPECT_THROW(engine.predictBatch("no", "v1", batch), std::out_of_range);
}

// ============================================================================
// predictProba
// ============================================================================

TEST(ModelServingEngine, PredictProbaClassificationShape) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    auto batch = makeBinaryData(10);
    auto proba = engine.predictProba("cls", "v1", batch);
    ASSERT_EQ(proba.size(), batch.size());
    for (const auto& pm : proba) {
        EXPECT_FALSE(pm.empty());
        double total = 0.0;
        for (const auto& [lbl, p] : pm) total += p;
        EXPECT_NEAR(total, 1.0, 0.01);
    }
}

TEST(ModelServingEngine, PredictProbaRegressionReturnsValueKey) {
    ModelServingEngine engine;
    engine.registerModel("reg", "v1", trainRegressor());

    auto batch = makeRegressionData(5);
    auto proba = engine.predictProba("reg", "v1", batch);
    ASSERT_EQ(proba.size(), batch.size());
    for (const auto& pm : proba) {
        EXPECT_EQ(pm.size(), 1u);
        EXPECT_TRUE(pm.count("value") > 0);
    }
}

TEST(ModelServingEngine, PredictProbaOversizedThrows) {
    ModelServingConfig cfg;
    cfg.max_batch_size = 3;
    ModelServingEngine engine(cfg);
    engine.registerModel("cls", "v1", trainClassifier());

    auto batch = makeBinaryData(10);
    EXPECT_THROW(engine.predictProba("cls", "v1", batch), std::invalid_argument);
}

// ============================================================================
// healthMetrics
// ============================================================================

TEST(ModelServingEngine, HealthMetricsNotFoundReturnsNullopt) {
    ModelServingEngine engine;
    EXPECT_FALSE(engine.healthMetrics("no", "v1").has_value());
}

TEST(ModelServingEngine, HealthMetricsTotalPredictions) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    DataPoint dp;
    dp.id = "x";
    dp.set("x1", 0.5);
    dp.set("x2", 0.1);

    engine.predict("cls", "v1", dp);
    engine.predict("cls", "v1", dp);
    engine.predict("cls", "v1", dp);

    auto h = engine.healthMetrics("cls", "v1");
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->total_predictions, 3u);
    EXPECT_EQ(h->total_batch_calls, 0u);
}

TEST(ModelServingEngine, HealthMetricsBatchCounters) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    auto batch = makeBinaryData(10);
    engine.predictBatch("cls", "v1", batch);
    engine.predictBatch("cls", "v1", batch);

    auto h = engine.healthMetrics("cls", "v1");
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->total_batch_calls, 2u);
    EXPECT_EQ(h->total_batch_records, 20u);
    EXPECT_EQ(h->total_predictions, 0u);
}

TEST(ModelServingEngine, HealthMetricsLatencyTracked) {
    ModelServingConfig cfg;
    cfg.track_latency  = true;
    cfg.latency_window = 50;
    ModelServingEngine engine(cfg);
    engine.registerModel("cls", "v1", trainClassifier());

    DataPoint dp;
    dp.set("x1", 0.1);
    dp.set("x2", 0.2);
    for (int i = 0; i < 5; ++i) engine.predict("cls", "v1", dp);

    auto h = engine.healthMetrics("cls", "v1");
    ASSERT_TRUE(h.has_value());
    EXPECT_GE(h->avg_latency_ms, 0.0);
    EXPECT_GE(h->p99_latency_ms, 0.0);
    EXPECT_GE(h->last_latency_ms, 0.0);
    EXPECT_GT(h->last_used_ms, 0);
}

TEST(ModelServingEngine, HealthMetricsLatencyDisabled) {
    ModelServingConfig cfg;
    cfg.track_latency = false;
    ModelServingEngine engine(cfg);
    engine.registerModel("cls", "v1", trainClassifier());

    DataPoint dp;
    dp.set("x1", 0.5);
    engine.predict("cls", "v1", dp);

    auto h = engine.healthMetrics("cls", "v1");
    ASSERT_TRUE(h.has_value());
    // When tracking disabled, latency should remain 0
    EXPECT_EQ(h->avg_latency_ms, 0.0);
    EXPECT_EQ(h->p99_latency_ms, 0.0);
}

// ============================================================================
// serializeModel / loadModel (round-trip)
// ============================================================================

TEST(ModelServingEngine, SerializeDeserializeRoundTrip) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    std::string blob = engine.serializeModel("cls", "v1");
    EXPECT_FALSE(blob.empty());

    ModelServingEngine engine2;
    engine2.loadModel("cls", "v1", blob);
    EXPECT_TRUE(engine2.isRegistered("cls", "v1"));

    auto mi = engine2.modelInfo("cls", "v1");
    ASSERT_TRUE(mi.has_value());
    EXPECT_EQ(mi->task, AutoMLTask::CLASSIFICATION);
}

TEST(ModelServingEngine, SerializeMissingModelThrows) {
    ModelServingEngine engine;
    EXPECT_THROW(engine.serializeModel("no", "v1"), std::out_of_range);
}

TEST(ModelServingEngine, LoadModelThrowsOnDuplicate) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());
    std::string blob = engine.serializeModel("cls", "v1");

    // Registering again with the same key must fail
    EXPECT_THROW(engine.loadModel("cls", "v1", blob), std::runtime_error);
}

// ============================================================================
// Predict after unregister
// ============================================================================

TEST(ModelServingEngine, PredictAfterUnregisterThrows) {
    ModelServingEngine engine;
    engine.registerModel("m", "v1", trainClassifier());
    engine.unregisterModel("m", "v1");

    DataPoint dp;
    dp.set("x1", 0.5);
    EXPECT_THROW(engine.predict("m", "v1", dp), std::out_of_range);
}

// ============================================================================
// Concurrent readers (thread-safety smoke test)
// ============================================================================

TEST(ModelServingEngine, ConcurrentReadsAreThreadSafe) {
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier());

    auto batch = makeBinaryData(20);

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 10; ++i) {
                auto preds = engine.predictBatch("cls", "v1", batch);
                ASSERT_EQ(preds.size(), batch.size());
            }
        });
    }
    for (auto& th : threads) th.join();

    auto h = engine.healthMetrics("cls", "v1");
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->total_batch_calls, static_cast<uint64_t>(kThreads * 10));
}

// ============================================================================
// predict() does not hold registry lock during inference
// (concurrent unregisterModel must not deadlock or crash)
// ============================================================================

TEST(ModelServingEngine, PredictReleasesRegistryLockBeforeInference) {
    // Register a model, start many concurrent predict() callers, then
    // concurrently call unregisterModel().  With the old implementation this
    // would have held the shared lock across inference, preventing the
    // exclusive lock in unregisterModel() from making progress.  With the new
    // shared_ptr-capture pattern both sides proceed independently.
    ModelServingEngine engine;
    engine.registerModel("cls", "v1", trainClassifier(100));

    DataPoint dp;
    dp.set("x1", 0.5);
    dp.set("x2", 0.3);

    constexpr int kReaders = 8;
    constexpr int kIter    = 20;

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int t = 0; t < kReaders; ++t) {
        readers.emplace_back([&] {
            for (int i = 0; i < kIter; ++i) {
                // After unregister the call may throw out_of_range — that is
                // the expected behaviour; what must NOT happen is a crash or
                // deadlock.
                try { engine.predict("cls", "v1", dp); } catch (...) {}
            }
        });
    }

    // Give readers a head-start then unregister concurrently.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    engine.unregisterModel("cls", "v1");

    for (auto& th : readers) th.join();
    // Engine must still be usable after the concurrent unregister.
    EXPECT_FALSE(engine.isRegistered("cls", "v1"));
}

// ============================================================================
// Throughput benchmark: 16 concurrent predict() callers (opt-in)
// ============================================================================

TEST(ModelServingEngine, ConcurrentPredictThroughputBenchmark) {
    const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!env || std::string(env) != "1") {
        GTEST_SKIP() << "Skipping performance benchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to run)";
    }

    // Disable per-call latency tracking: recordLatency() holds health_mu
    // while sorting a 1000-element window — that O(N log N) operation under
    // a shared mutex would dominate the measurement and hide the registry-lock
    // improvement this benchmark is designed to verify.
    ModelServingConfig cfg;
    cfg.track_latency = false;
    ModelServingEngine engine(cfg);
    engine.registerModel("bench", "v1", trainClassifier(200));

    DataPoint dp;
    dp.set("x1", 0.5);
    dp.set("x2", 0.3);

    constexpr int    kThreads    = 16;
    constexpr double kDurationS  = 1.0;

    std::atomic<uint64_t> total_preds{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    auto wall_start = std::chrono::steady_clock::now();

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            auto t_end = wall_start +
                         std::chrono::duration<double>(kDurationS);
            uint64_t local = 0;
            while (std::chrono::steady_clock::now() < t_end) {
                engine.predict("bench", "v1", dp);
                ++local;
            }
            total_preds.fetch_add(local, std::memory_order_relaxed);
        });
    }
    for (auto& th : threads) th.join();

    double elapsed_s = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();
    double preds_per_sec_per_core =
        static_cast<double>(total_preds.load()) / elapsed_s /
        static_cast<double>(kThreads);

    // Requirement: ≥ 10 000 predictions/s per core (easily met by any
    // in-process decision-tree model once the registry lock is released
    // before inference; only fails if the registry lock is still held
    // during inference, causing 16-way serialisation).
    EXPECT_GE(preds_per_sec_per_core, 10'000.0)
        << "Throughput: " << preds_per_sec_per_core
        << " predictions/s/core (required ≥ 10 000)";

    // Report for informational purposes even on pass.
    std::cout << "[Benchmark] ConcurrentPredictThroughput: "
              << static_cast<uint64_t>(preds_per_sec_per_core)
              << " predictions/s/core across " << kThreads << " threads\n";
}
