// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "training/incremental_lora_trainer.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace {

std::string makeTempDbPath() {
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("themis_incremental_lora_db_" + std::to_string(timestamp))).string();
}

class IncrementalLoRADbTrainingTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeTempDbPath();
        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.create_if_missing = true;
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        indexes_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        query_engine_ = std::make_unique<themis::query::QueryEngine>(*db_, *indexes_);

        for (int i = 0; i < 3; ++i) {
            themis::BaseEntity sample("sample_" + std::to_string(i));
            sample.setField("input", std::string("instruction number ") + std::to_string(i));
            sample.setField("output", std::string("expected answer ") + std::to_string(i));
            ASSERT_TRUE(indexes_->put("training_samples", sample).ok);
        }
    }

    void TearDown() override {
        query_engine_.reset();
        indexes_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    themis::training::IncrementalTrainingConfig makeConfig() const {
        themis::training::IncrementalTrainingConfig config;
        config.training_data_collection = "training_samples";
        config.max_training_samples = 16;
        config.rank = 2;
        config.alpha = 4.0f;
        config.learning_rate = 0.001f;
        config.batch_size = 2;
        config.num_epochs = 1;
        config.max_seq_length = 8;
        config.device = "cpu";
        return config;
    }

    std::string db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> indexes_;
    std::unique_ptr<themis::query::QueryEngine> query_engine_;
};

TEST_F(IncrementalLoRADbTrainingTest, TrainsOnExactlyTheSamplesReadFromCollection) {
    auto config = makeConfig();
    themis::training::IncrementalLoRATrainer trainer(config, "", query_engine_.get());

    const auto result = trainer.train(themis::training::TrainingMode::INITIAL);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.samples_trained, 3u);
    EXPECT_GE(result.data_load_time_seconds, 0.0);
    EXPECT_GT(trainer.getMetrics().total_steps, 0u);
}

TEST_F(IncrementalLoRADbTrainingTest, EmptyCollectionFailsWithoutSyntheticFallback) {
    auto config = makeConfig();
    config.training_data_collection = "missing_samples";
    themis::training::IncrementalLoRATrainer trainer(config, "", query_engine_.get());

    const auto result = trainer.train(themis::training::TrainingMode::INITIAL);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.samples_trained, 0u);
    EXPECT_NE(result.error_message.find("no valid input/output samples"), std::string::npos);
}

TEST_F(IncrementalLoRADbTrainingTest, InvalidCollectionIdentifierFailsClosed) {
    auto config = makeConfig();
    config.training_data_collection = "training_samples; REMOVE sample IN training_samples";
    themis::training::IncrementalLoRATrainer trainer(config, "", query_engine_.get());

    const auto result = trainer.train(themis::training::TrainingMode::INITIAL);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.samples_trained, 0u);
    EXPECT_NE(result.error_message.find("invalid collection identifier"), std::string::npos);
}

}  // namespace
