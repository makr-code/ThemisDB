// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "llm/training_data_iterator.h"
#include "query/query_engine.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string makeTempDbPath() {
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("themis_training_data_iterator_" + std::to_string(timestamp))).string();
}

class TrainingDataIteratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeTempDbPath();
        themis::RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_;
        db_config.create_if_missing = true;
        db_ = std::make_shared<themis::RocksDBWrapper>(db_config);
        ASSERT_TRUE(db_->open());
        indexes_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        query_engine_ = std::make_unique<themis::query::QueryEngine>(*db_, *indexes_);

        themis::exporters::JSONLLLMConfig export_config;
        export_config.style = themis::exporters::JSONLFormat::Style::INSTRUCTION_TUNING;
        export_config.field_mapping.instruction_field = "instruction";
        export_config.field_mapping.input_field = "context";
        export_config.field_mapping.output_field = "output";
        export_config.weighting.enable_weights = true;
        export_config.weighting.weight_field = "weight";
        export_config.quality.min_text_length = 1;
        export_config.quality.max_text_length = 512;
        export_config.quality.skip_duplicates = true;
        exporter_ = std::make_shared<themis::exporters::JSONLLLMExporter>(export_config);

        insertSample("sample_1", "Explain the first record", "first answer", "ctx-1", 0.75);
        insertSample("sample_2", "Explain the second record", "second answer", "ctx-2", 0.50);
        insertSample("sample_3", "Explain the third record", "third answer", "ctx-3", 1.00);
        insertSample("sample_4", "Explain the first record", "first answer", "ctx-1", 0.75);
        insertSample("sample_5", "Explain the empty", "", "ctx-5", 1.00);
    }

    void TearDown() override {
        query_engine_.reset();
        indexes_.reset();
        exporter_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    void insertSample(const std::string& key,
                      const std::string& instruction,
                      const std::string& output,
                      const std::string& context,
                      double weight) {
        themis::BaseEntity entity(key);
        entity.setField("instruction", instruction);
        entity.setField("output", output);
        entity.setField("context", context);
        entity.setField("weight", weight);
        entity.setField("kind", std::string("training"));
        ASSERT_TRUE(indexes_->put("training_samples", entity).ok);
    }

    themis::llm::TrainingDataIterator makeIterator(
        themis::llm::TrainingDataIterator::Config config = {}) {
        return themis::llm::TrainingDataIterator(db_, exporter_, config, query_engine_.get());
    }

    std::string db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> indexes_;
    std::unique_ptr<themis::query::QueryEngine> query_engine_;
    std::shared_ptr<themis::exporters::JSONLLLMExporter> exporter_;
};

TEST_F(TrainingDataIteratorTest, ReadsCollectionAndReturnsBoundedBatches) {
    themis::llm::TrainingDataIterator::Config config;
    config.batch_size = 2;
    config.shuffle_data = false;
    auto iterator = makeIterator(config);

    themis::llm::AdapterMetadata metadata;
    ASSERT_TRUE(iterator.initialize("FOR sample IN training_samples RETURN sample", metadata))
        << iterator.lastError();
    EXPECT_EQ(iterator.getTotalSamples(), 3u);
    EXPECT_EQ(iterator.getTotalBatches(), 2u);
    EXPECT_EQ(iterator.getStats().skipped_samples, 2u);

    auto first = iterator.getNextBatch();
    ASSERT_TRUE(first.has_value());
    ASSERT_EQ(first->samples.size(), 2u);
    EXPECT_EQ(first->samples[0].instruction, "Explain the first record");
    EXPECT_EQ(first->samples[0].output, "first answer");
    EXPECT_EQ(first->samples[0].input, "ctx-1");
    EXPECT_DOUBLE_EQ(first->samples[0].weight, 0.75);
    EXPECT_EQ(first->samples[1].metadata.at("sample_id"), "sample_2");

    auto second = iterator.getNextBatch();
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(second->samples.size(), 1u);
    EXPECT_EQ(second->batch_id, 1u);
    EXPECT_FALSE(iterator.getNextBatch().has_value());

    const auto stats = iterator.getStats();
    EXPECT_EQ(stats.processed_samples, 3u);
    EXPECT_EQ(stats.processed_batches, 2u);
    EXPECT_EQ(stats.total_tokens, 21u);
}

TEST_F(TrainingDataIteratorTest, RejectsMutationAqlWithoutExecutingIt) {
    auto iterator = makeIterator();
    themis::llm::AdapterMetadata metadata;

    EXPECT_FALSE(iterator.initialize(
        "FOR sample IN training_samples REMOVE sample IN training_samples", metadata));
    EXPECT_FALSE(iterator.lastError().empty());
    EXPECT_EQ(iterator.getTotalSamples(), 0u);
}

TEST_F(TrainingDataIteratorTest, EmptyReadQueryInitializesWithNoBatches) {
    auto iterator = makeIterator();
    themis::llm::AdapterMetadata metadata;

    ASSERT_TRUE(iterator.initialize(
        "FOR sample IN training_samples FILTER sample.kind == 'missing' RETURN sample", metadata))
        << iterator.lastError();
    EXPECT_EQ(iterator.getTotalSamples(), 0u);
    EXPECT_EQ(iterator.getTotalBatches(), 0u);
    EXPECT_FALSE(iterator.hasNext());
}

TEST_F(TrainingDataIteratorTest, ResetRestartsDeterministicShuffledIteration) {
    themis::llm::TrainingDataIterator::Config config;
    config.batch_size = 1;
    config.shuffle_data = true;
    config.random_seed = 19;
    auto iterator = makeIterator(config);
    themis::llm::AdapterMetadata metadata;
    ASSERT_TRUE(iterator.initialize("FOR sample IN training_samples RETURN sample", metadata))
        << iterator.lastError();

    std::vector<std::string> first_pass;
    while (auto batch = iterator.getNextBatch()) {
        first_pass.push_back(batch->samples.front().metadata.at("sample_id"));
    }
    iterator.reset();

    std::vector<std::string> second_pass;
    while (auto batch = iterator.getNextBatch()) {
        second_pass.push_back(batch->samples.front().metadata.at("sample_id"));
    }
    EXPECT_EQ(second_pass, first_pass);
}

TEST_F(TrainingDataIteratorTest, RequiresEngineAndValidBatchSize) {
    themis::llm::TrainingDataIterator::Config config;
    config.batch_size = 0;
    EXPECT_THROW((themis::llm::TrainingDataIterator(db_, exporter_, config, query_engine_.get())),
                 std::invalid_argument);

    themis::llm::TrainingDataIterator offline(db_, exporter_);
    themis::llm::AdapterMetadata metadata;
    EXPECT_FALSE(offline.initialize("FOR sample IN training_samples RETURN sample", metadata));
    EXPECT_NE(offline.lastError().find("QueryEngine"), std::string::npos);
}

}  // namespace
