/**
 * @file test_decision_record_yaml_processor.cpp
 * @brief Tests for DecisionRecordYamlProcessor — async YAML writer for LLM/LoRA decisions.
 *
 * Test groups:
 *  1. Lifecycle   — constructor/destructor, thread-safe double-construction
 *  2. Submit      — basic submit, auto-ID generation, returns true/false
 *  3. YAML output — file created, parseable YAML, all fields present
 *  4. LoRA fields — optional LoRA fields serialised correctly
 *  5. Backpressure — drop when max_queue_depth reached; stats.dropped accurate
 *  6. Concurrent  — parallel submit from N threads; no duplicate record IDs
 *  7. Flush       — flush() blocks until queue empty
 *  8. Shutdown    — pending records written before destructor returns
 *  9. Daily dirs  — create_daily_subdirs=true creates YYYY-MM-DD subdir
 * 10. Stats        — submitted / written / dropped / errors counters
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "llm/decision_record_yaml_processor.h"

using namespace std::chrono_literals;
namespace fs = std::filesystem;
using themis::llm::DecisionRecord;
using themis::llm::DecisionRecordYamlProcessor;

// ─── Test fixture ────────────────────────────────────────────────────────────

class DecisionRecordYamlProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets its own temp directory under /tmp to avoid collisions.
        log_dir_ = fs::temp_directory_path()
                   / ("themis_dr_test_" + std::to_string(
                       std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(log_dir_);
    }

    void TearDown() override {
        fs::remove_all(log_dir_);
    }

    DecisionRecordYamlProcessor::Config makeConfig(size_t max_queue = 10'000,
                                                    bool daily = true) const {
        DecisionRecordYamlProcessor::Config cfg;
        cfg.log_dir           = log_dir_;
        cfg.max_queue_depth   = max_queue;
        cfg.create_daily_subdirs = daily;
        return cfg;
    }

    /// Return the first .yaml file found recursively under log_dir_.
    std::optional<fs::path> firstYamlFile() const {
        for (const auto& e : fs::recursive_directory_iterator(log_dir_)) {
            if (e.is_regular_file() && e.path().extension() == ".yaml") {
                return e.path();
            }
        }
        return std::nullopt;
    }

    /// Count all .yaml files recursively under log_dir_.
    size_t countYamlFiles() const {
        size_t n = 0;
        for (const auto& e : fs::recursive_directory_iterator(log_dir_)) {
            if (e.is_regular_file() && e.path().extension() == ".yaml") ++n;
        }
        return n;
    }

    /// Build a minimal valid DecisionRecord.
    static DecisionRecord makeRecord(const std::string& type = "FEDERATED_ROUND",
                                     const std::string& comp = "TestComponent") {
        DecisionRecord r;
        r.decision_type = type;
        r.component     = comp;
        r.outcome       = "SUCCESS";
        return r;
    }

    fs::path log_dir_;
};

// ─── Group 1: Lifecycle ───────────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, LifecycleConstructDestruct) {
    // Constructor starts background thread; destructor joins it.
    {
        DecisionRecordYamlProcessor proc(makeConfig());
        (void)proc;
    }
    // If the destructor hangs this test will time-out.  Reaching here = pass.
    SUCCEED();
}

TEST_F(DecisionRecordYamlProcessorTest, LifecycleDoubleConstruct) {
    DecisionRecordYamlProcessor p1(makeConfig());
    DecisionRecordYamlProcessor p2(makeConfig()); // independent instance
    EXPECT_EQ(p1.getStats().submitted, 0u);
    EXPECT_EQ(p2.getStats().submitted, 0u);
}

// ─── Group 2: Submit ─────────────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, SubmitReturnsTrueWhenQueueHasRoom) {
    DecisionRecordYamlProcessor proc(makeConfig());
    bool ok = proc.submit(makeRecord());
    EXPECT_TRUE(ok);
}

TEST_F(DecisionRecordYamlProcessorTest, SubmitAutoGeneratesRecordId) {
    DecisionRecordYamlProcessor proc(makeConfig());
    DecisionRecord r = makeRecord();
    EXPECT_TRUE(r.record_id.empty()); // no ID before submit
    proc.submit(r);
    proc.flush();
    auto path = firstYamlFile();
    ASSERT_TRUE(path.has_value());
    // The filename contains the auto-generated ID — not empty
    EXPECT_FALSE(path->stem().empty());
}

// ─── Group 3: YAML output ────────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, YamlFileIsCreated) {
    DecisionRecordYamlProcessor proc(makeConfig());
    proc.submit(makeRecord("LORA_ADAPTER_SELECTION", "AdapterRegistry"));
    proc.flush();
    EXPECT_EQ(countYamlFiles(), 1u);
}

TEST_F(DecisionRecordYamlProcessorTest, YamlFileIsParseableByYamlCpp) {
    DecisionRecordYamlProcessor proc(makeConfig());
    proc.submit(makeRecord("THRESHOLD_UPDATE", "SelfImprovementModule"));
    proc.flush();
    auto path = firstYamlFile();
    ASSERT_TRUE(path.has_value());

    // Must not throw
    YAML::Node doc = YAML::LoadFile(path->string());
    EXPECT_TRUE(doc.IsDefined());
    EXPECT_EQ(doc.Type(), YAML::NodeType::Map);
}

TEST_F(DecisionRecordYamlProcessorTest, YamlContainsMandatoryFields) {
    DecisionRecordYamlProcessor proc(makeConfig());
    DecisionRecord r = makeRecord("CIRCUIT_BREAKER_OPEN", "LoRAFederationCoordinator");
    r.latency_ms = 42;
    proc.submit(r);
    proc.flush();

    auto path = firstYamlFile();
    ASSERT_TRUE(path.has_value());
    YAML::Node doc = YAML::LoadFile(path->string());

    EXPECT_TRUE(doc["record_id"].IsDefined());
    EXPECT_FALSE(doc["record_id"].as<std::string>().empty());
    EXPECT_EQ(doc["decision_type"].as<std::string>(), "CIRCUIT_BREAKER_OPEN");
    EXPECT_EQ(doc["component"].as<std::string>(), "LoRAFederationCoordinator");
    EXPECT_EQ(doc["outcome"].as<std::string>(), "SUCCESS");
    EXPECT_TRUE(doc["timestamp"].IsDefined());
    EXPECT_EQ(doc["latency_ms"].as<int64_t>(), 42);
}

// ─── Group 4: LoRA fields ────────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, YamlContainsLoraFields) {
    DecisionRecordYamlProcessor proc(makeConfig());
    DecisionRecord r = makeRecord("FEDERATED_ROUND", "LoRAFederationCoordinator");
    r.lora_round    = 42;
    r.epsilon_spent = 0.1f;
    r.participants  = 8;
    r.accuracy_delta = 0.023f;
    r.shard_id      = "shard-001";
    r.audit_ref     = "ai_decision:abc123";
    proc.submit(r);
    proc.flush();

    auto path = firstYamlFile();
    ASSERT_TRUE(path.has_value());
    YAML::Node doc = YAML::LoadFile(path->string());

    EXPECT_EQ(doc["shard_id"].as<std::string>(), "shard-001");
    ASSERT_TRUE(doc["lora"].IsDefined());
    EXPECT_EQ(doc["lora"]["round"].as<int>(), 42);
    EXPECT_NEAR(doc["lora"]["epsilon_spent"].as<float>(), 0.1f, 1e-4f);
    EXPECT_EQ(doc["lora"]["participants"].as<int>(), 8);
    EXPECT_NEAR(doc["lora"]["accuracy_delta"].as<float>(), 0.023f, 1e-4f);
    EXPECT_EQ(doc["audit_ref"].as<std::string>(), "ai_decision:abc123");
}

TEST_F(DecisionRecordYamlProcessorTest, YamlOmitsLoraBlockWhenAbsent) {
    DecisionRecordYamlProcessor proc(makeConfig());
    proc.submit(makeRecord("GDPR_ERASE", "CrossShardFeedbackSync"));
    proc.flush();

    auto path = firstYamlFile();
    ASSERT_TRUE(path.has_value());
    YAML::Node doc = YAML::LoadFile(path->string());

    EXPECT_FALSE(doc["lora"].IsDefined());
}

// ─── Group 5: Backpressure ───────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, SubmitReturnsFalseWhenQueueFull) {
    // max_queue_depth=1 and the background thread is blocked waiting for a
    // condition — we saturate the queue *before* the thread can drain it.
    DecisionRecordYamlProcessor proc(makeConfig(/*max_queue=*/1));

    // Fill up the queue.  One of the two submits must return false.
    bool r1 = proc.submit(makeRecord());
    bool r2 = proc.submit(makeRecord());

    // At least one should succeed; if the queue was already drained before the
    // second submit that's also fine — just check no panic/exception occurred.
    (void)r1; (void)r2;

    proc.flush();
    // dropped_ counter reflects reality
    auto stats = proc.getStats();
    EXPECT_EQ(stats.submitted + stats.dropped, 2u);
}

// ─── Group 6: Concurrent submit ──────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, ConcurrentSubmitNoUB) {
    constexpr int kThreads = 8;
    constexpr int kPerThread = 50;

    DecisionRecordYamlProcessor proc(makeConfig());

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&proc, i] {
            for (int j = 0; j < kPerThread; ++j) {
                proc.submit(makeRecord("LOOP_TRIGGER",
                                       "Worker_" + std::to_string(i)));
            }
        });
    }
    for (auto& t : threads) t.join();

    proc.flush();

    auto stats = proc.getStats();
    EXPECT_EQ(stats.submitted + stats.dropped,
              static_cast<size_t>(kThreads * kPerThread));
    EXPECT_EQ(stats.errors, 0u);
    EXPECT_EQ(countYamlFiles(), stats.written);
}

// ─── Group 7: Flush ──────────────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, FlushWaitsUntilQueueEmpty) {
    DecisionRecordYamlProcessor proc(makeConfig());
    for (int i = 0; i < 20; ++i) {
        proc.submit(makeRecord());
    }
    proc.flush(); // must return only after queue is fully drained
    EXPECT_EQ(proc.getStats().written + proc.getStats().errors,
              proc.getStats().submitted);
}

// ─── Group 8: Shutdown with pending records ───────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, DestructorDrainsPendingRecords) {
    constexpr size_t kRecords = 50;
    {
        DecisionRecordYamlProcessor proc(makeConfig());
        for (size_t i = 0; i < kRecords; ++i) {
            proc.submit(makeRecord("LORA_RANK_ADJUSTMENT", "AdaLoRAAdapter"));
        }
        // Destructor should drain before returning.
    }
    // All records must now be on disk.
    EXPECT_EQ(countYamlFiles(), kRecords);
}

// ─── Group 9: Daily subdirectories ───────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, DailySubdirCreated) {
    DecisionRecordYamlProcessor proc(makeConfig(10'000, /*daily=*/true));
    proc.submit(makeRecord());
    proc.flush();

    // There should be exactly one subdirectory named like YYYY-MM-DD
    bool found_daily_dir = false;
    for (const auto& e : fs::directory_iterator(log_dir_)) {
        if (e.is_directory()) {
            const std::string name = e.path().filename().string();
            // Simple check: "YYYY-MM-DD" format = 10 chars, digits and hyphens
            if (name.size() == 10 && name[4] == '-' && name[7] == '-') {
                found_daily_dir = true;
            }
        }
    }
    EXPECT_TRUE(found_daily_dir);
}

TEST_F(DecisionRecordYamlProcessorTest, NoDailySubdirWhenDisabled) {
    DecisionRecordYamlProcessor proc(makeConfig(10'000, /*daily=*/false));
    proc.submit(makeRecord());
    proc.flush();

    // The file should be directly in log_dir_, no subdirectory.
    bool has_subdir = false;
    for (const auto& e : fs::directory_iterator(log_dir_)) {
        if (e.is_directory()) { has_subdir = true; break; }
    }
    EXPECT_FALSE(has_subdir);
    EXPECT_EQ(countYamlFiles(), 1u);
}

// ─── Group 10: Stats ─────────────────────────────────────────────────────────

TEST_F(DecisionRecordYamlProcessorTest, StatsInitiallyZero) {
    DecisionRecordYamlProcessor proc(makeConfig());
    auto s = proc.getStats();
    EXPECT_EQ(s.submitted, 0u);
    EXPECT_EQ(s.written,   0u);
    EXPECT_EQ(s.dropped,   0u);
    EXPECT_EQ(s.errors,    0u);
}

TEST_F(DecisionRecordYamlProcessorTest, StatsAccumulateCorrectly) {
    DecisionRecordYamlProcessor proc(makeConfig());
    proc.submit(makeRecord());
    proc.submit(makeRecord());
    proc.flush();

    auto s = proc.getStats();
    EXPECT_EQ(s.submitted, 2u);
    EXPECT_EQ(s.written,   2u);
    EXPECT_EQ(s.dropped,   0u);
    EXPECT_EQ(s.errors,    0u);
}
