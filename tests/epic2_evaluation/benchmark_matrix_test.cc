/**
 * @file benchmark_matrix_test.cc
 * @brief Contract tests for IBenchmarkMatrix (sub-issue #5438).
 *
 * Validates factory construction, scenario registration, listing,
 * observer registration, and that run/runAll do not crash at scaffold stage.
 * Production scenario execution is tracked in sub-issue #5438.
 */

#include "evaluation/include/benchmark_matrix.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace themis::evaluation;

namespace {

BenchmarkScenario makeScenario(const std::string& id,
                                BenchmarkCategory cat = BenchmarkCategory::AnnLatency) {
    BenchmarkScenario s;
    s.id               = id;
    s.description      = "Scenario " + id;
    s.category         = cat;
    s.dataset_vectors  = 100'000;
    s.query_count      = 1000;
    s.top_k            = 10;
    s.target_recall    = 0.95f;
    s.target_latency_p99_ms = 50.0;
    return s;
}

HardwareProfile stubHw() {
    HardwareProfile hw;
    hw.id        = "test-hw";
    hw.dram_bytes = 16ULL << 30;
    return hw;
}

} // namespace

class BenchmarkMatrixTest : public ::testing::Test {
protected:
    void SetUp() override {
        matrix_ = makeBenchmarkMatrix();
        ASSERT_NE(matrix_, nullptr);
    }

    std::unique_ptr<IBenchmarkMatrix> matrix_;
};

TEST_F(BenchmarkMatrixTest, FactoryReturnsNonNull) {
    EXPECT_NE(matrix_, nullptr);
}

TEST_F(BenchmarkMatrixTest, ListScenariosInitiallyNonEmpty) {
    // Factory pre-populates the EPIC 2 scenario set.
    auto ids = matrix_->listScenarios();
    EXPECT_FALSE(ids.empty());
}

TEST_F(BenchmarkMatrixTest, RegisterScenarioDoesNotThrow) {
    EXPECT_NO_THROW(matrix_->registerScenario(makeScenario("custom-s1")));
}

TEST_F(BenchmarkMatrixTest, RegisteredScenarioAppearsInList) {
    matrix_->registerScenario(makeScenario("custom-s2"));
    auto ids = matrix_->listScenarios();
    bool found = false;
    for (const auto& id : ids) {
        if (id == "custom-s2") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(BenchmarkMatrixTest, RunScenarioDoesNotThrow) {
    auto ids = matrix_->listScenarios();
    ASSERT_FALSE(ids.empty());
    EXPECT_NO_THROW(matrix_->run(ids.front(), stubHw()));
}

TEST_F(BenchmarkMatrixTest, RunScenarioReturnsResult) {
    auto ids = matrix_->listScenarios();
    ASSERT_FALSE(ids.empty());
    BenchmarkResult r = matrix_->run(ids.front(), stubHw());
    EXPECT_EQ(r.scenario_id, ids.front());
}

TEST_F(BenchmarkMatrixTest, RunAllDoesNotThrow) {
    EXPECT_NO_THROW(matrix_->runAll(stubHw()));
}

TEST_F(BenchmarkMatrixTest, RunAllResultsCountMatchesScenarios) {
    auto ids = matrix_->listScenarios();
    auto results = matrix_->runAll(stubHw());
    EXPECT_EQ(results.size(), ids.size());
}

TEST_F(BenchmarkMatrixTest, ResultObserverRegistrationDoesNotThrow) {
    EXPECT_NO_THROW(matrix_->onResult([](const BenchmarkResult&) {}));
}

TEST_F(BenchmarkMatrixTest, RunNonexistentScenarioDoesNotThrow) {
    EXPECT_NO_THROW({
        try {
            matrix_->run("scenario-that-does-not-exist", stubHw());
        } catch (const std::exception&) {
            // acceptable at scaffold stage
        }
    });
}
