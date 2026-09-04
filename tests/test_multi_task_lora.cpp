/**
 * @file tests/test_multi_task_lora.cpp
 * @brief Unit tests for MultiTaskLoRATrainer — MTL-01..13
 *
 * Coverage:
 *   MTL-01  Default-constructed trainer has zero tasks
 *   MTL-02  addTask registers unique tasks; duplicates are ignored
 *   MTL-03  train() throws with no tasks registered
 *   MTL-04  train() throws with empty sample list
 *   MTL-05  train() throws when sample references unknown task_id
 *   MTL-06  train() succeeds with two tasks and mixed samples
 *   MTL-07  train() result has correct epoch count and per-task entries
 *   MTL-08  exportSharedWeights() is non-empty after training
 *   MTL-09  exportTaskWeights() returns correct-sized vector per registered task
 *   MTL-10  inferTask() returns registered task id and confidence in [0,1]
 *   MTL-11  train() throws when shared_rank is zero
 *   MTL-12  addTask() rejects zero task_rank
 *   MTL-13  task_rank limits active projection rows
 */

#include <gtest/gtest.h>
#include "training/multi_task_lora.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace themis::training;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static MultiTaskLoRAConfig smallCfg() {
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank  = 4;
    cfg.epochs       = 3;
    cfg.batch_size   = 8;
    cfg.learning_rate = 1e-3f;
    cfg.input_dim    = 8;
    return cfg;
}

static TaskConfig makeTask(const std::string& id, float w = 1.0f, size_t rank = 2) {
    TaskConfig t;
    t.id          = id;
    t.loss_weight = w;
    t.task_rank   = rank;
    t.learning_rate = 1e-3f;
    return t;
}

// Generate n random-ish samples for a given task (deterministic).
static std::vector<MTLSample> makeSamples(const std::string& task_id,
                                           size_t             n,
                                           size_t             dim,
                                           float              offset = 0.0f)
{
    std::vector<MTLSample> out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        MTLSample s;
        s.task_id = task_id;
        s.input.resize(dim);
        s.target.resize(dim);
        for (size_t k = 0; k < dim; ++k) {
            s.input[k]  = offset + static_cast<float>(i % 5) * 0.1f + static_cast<float>(k) * 0.01f;
            s.target[k] = s.input[k] * 0.5f;
        }
        out.push_back(std::move(s));
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-01  Default-constructed trainer has zero tasks
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_01_ZeroTasksDefault) {
    MultiTaskLoRATrainer trainer;
    EXPECT_EQ(trainer.taskCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-02  addTask registers unique tasks; duplicates ignored
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_02_DuplicateTaskIgnored) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa"));
    trainer.addTask(makeTask("summary"));
    trainer.addTask(makeTask("qa")); // duplicate — should be ignored

    EXPECT_EQ(trainer.taskCount(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-03  train() throws with no tasks
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_03_ThrowsNoTasks) {
    MultiTaskLoRATrainer trainer(smallCfg());
    std::vector<MTLSample> samples = makeSamples("qa", 5, 8);
    EXPECT_THROW(trainer.train(samples), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-04  train() throws with empty sample list
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_04_ThrowsEmptySamples) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa"));
    EXPECT_THROW(trainer.train({}), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-05  train() throws when sample has unknown task_id
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_05_ThrowsUnknownTaskId) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa"));

    std::vector<MTLSample> samples = makeSamples("unknown_task", 3, 8);
    EXPECT_THROW(trainer.train(samples), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-06  train() succeeds with two tasks and mixed samples
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_06_TrainSucceedsTwoTasks) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa",      0.5f));
    trainer.addTask(makeTask("summary", 0.5f));

    auto samples = makeSamples("qa",      10, 8, 0.0f);
    auto s2      = makeSamples("summary", 10, 8, 1.0f);
    samples.insert(samples.end(), s2.begin(), s2.end());

    auto result = trainer.train(samples);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.epochs_run, 1u);
    EXPECT_TRUE(std::isfinite(result.joint_loss));
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-07  train() result has correct epoch count and per-task entries
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_07_TrainResultCounts) {
    auto cfg = smallCfg();
    MultiTaskLoRATrainer trainer(cfg);
    trainer.addTask(makeTask("qa"));
    trainer.addTask(makeTask("summary"));
    trainer.addTask(makeTask("classify"));

    auto samples = makeSamples("qa",       5, 8);
    auto s2      = makeSamples("summary",  5, 8, 1.0f);
    auto s3      = makeSamples("classify", 5, 8, 2.0f);
    samples.insert(samples.end(), s2.begin(), s2.end());
    samples.insert(samples.end(), s3.begin(), s3.end());

    auto result = trainer.train(samples);
    EXPECT_EQ(result.epochs_run, cfg.epochs);
    EXPECT_EQ(result.per_task.size(), 3u);

    // Each registered task should appear in per_task.
    std::vector<std::string> seen_ids;
    for (const auto& m : result.per_task) {
      seen_ids.push_back(m.task_id);
    }
    EXPECT_NE(std::find(seen_ids.begin(), seen_ids.end(), "qa"), seen_ids.end());
    EXPECT_NE(std::find(seen_ids.begin(), seen_ids.end(), "summary"), seen_ids.end());
    EXPECT_NE(std::find(seen_ids.begin(), seen_ids.end(), "classify"), seen_ids.end());
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-08  exportSharedWeights() is non-empty after training
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_08_ExportSharedWeightsNonEmpty) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa"));
    trainer.train(makeSamples("qa", 10, 8));

    auto weights = trainer.exportSharedWeights();
    EXPECT_FALSE(weights.empty());
    // Size = input_dim * shared_rank = 8 * 4 = 32
    EXPECT_EQ(weights.size(), 8u * 4u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-09  exportTaskWeights() returns non-empty for known task, empty for unknown
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_09_ExportTaskWeights) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa"));
    trainer.addTask(makeTask("summary"));

    auto samples = makeSamples("qa",      10, 8);
    auto s2      = makeSamples("summary", 10, 8, 1.0f);
    samples.insert(samples.end(), s2.begin(), s2.end());
    trainer.train(samples);

    auto w_qa = trainer.exportTaskWeights("qa");
    EXPECT_FALSE(w_qa.empty());

    auto w_unknown = trainer.exportTaskWeights("nonexistent");
    EXPECT_TRUE(w_unknown.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-10  inferTask() returns registered task id and confidence in [0,1]
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_10_InferTaskReturnsValidId) {
    MultiTaskLoRATrainer trainer(smallCfg());
    trainer.addTask(makeTask("qa",      0.6f));
    trainer.addTask(makeTask("summary", 0.4f));

    auto samples = makeSamples("qa",      10, 8, 0.0f);
    auto s2      = makeSamples("summary", 10, 8, 5.0f); // different region
    samples.insert(samples.end(), s2.begin(), s2.end());
    trainer.train(samples);

    std::vector<float> test_input(8, 0.1f); // similar to qa samples
    auto gate = trainer.inferTask(test_input);

    // task_id must be one of the registered tasks
    bool valid_id = (gate.task_id == "qa" || gate.task_id == "summary");
    EXPECT_TRUE(valid_id);

    // Confidence must be in [0, 1]
    EXPECT_GE(gate.confidence, 0.0f);
    EXPECT_LE(gate.confidence, 1.0f);

    // Scores must be provided for all tasks
    EXPECT_EQ(gate.scores.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-11  train() throws when shared_rank is zero
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_11_ThrowsZeroSharedRank) {
    auto cfg = smallCfg();
    cfg.shared_rank = 0;

    MultiTaskLoRATrainer trainer(cfg);
    trainer.addTask(makeTask("qa"));
    EXPECT_THROW(trainer.train(makeSamples("qa", 5, 8)), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-12  addTask() rejects zero task_rank
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_12_AddTaskRejectsZeroRank) {
    MultiTaskLoRATrainer trainer(smallCfg());
    TaskConfig bad = makeTask("qa");
    bad.task_rank = 0;
    EXPECT_THROW(trainer.addTask(bad), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// MTL-13  task_rank limits active projection rows in task head
// ─────────────────────────────────────────────────────────────────────────────
TEST(MultiTaskLoRATrainerTest, MTL_13_TaskRankLimitsActiveRows) {
    auto cfg = smallCfg();
    cfg.shared_rank = 4;
    cfg.input_dim   = 8;

    MultiTaskLoRATrainer trainer(cfg);
    trainer.addTask(makeTask("qa", 1.0f, 1)); // only rank-0 row should be active
    trainer.train(makeSamples("qa", 12, cfg.input_dim));

    auto weights = trainer.exportTaskWeights("qa");
    ASSERT_EQ(weights.size(), cfg.shared_rank * cfg.input_dim);

    for (size_t k = 1; k < cfg.shared_rank; ++k) {
        for (size_t j = 0; j < cfg.input_dim; ++j) {
            EXPECT_FLOAT_EQ(weights[k * cfg.input_dim + j], 0.0f);
        }
    }
}
